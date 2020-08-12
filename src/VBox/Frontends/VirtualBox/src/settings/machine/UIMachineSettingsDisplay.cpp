/* $Id$ */
/** @file
 * VBox Qt GUI - UIMachineSettingsDisplay class implementation.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QStackedLayout>

/* GUI includes: */
#include "QIAdvancedSlider.h"
#include "QITabWidget.h"
#include "QIWidgetValidator.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIErrorString.h"
#include "UIExtraDataManager.h"
#include "UIFilePathSelector.h"
#include "UIFilmContainer.h"
#include "UIGraphicsControllerEditor.h"
#include "UIMachineSettingsDisplay.h"
#include "UIScaleFactorEditor.h"
#include "UIVideoMemoryEditor.h"

/* COM includes: */
#include "CGraphicsAdapter.h"
#include "CRecordingSettings.h"
#include "CRecordingScreenSettings.h"
#include "CExtPack.h"
#include "CExtPackManager.h"
#include "CVRDEServer.h"


#define VIDEO_CAPTURE_BIT_RATE_MIN 32
#define VIDEO_CAPTURE_BIT_RATE_MAX 2048


/** Machine settings: Display page data structure. */
struct UIDataSettingsMachineDisplay
{
    /** Constructs data. */
    UIDataSettingsMachineDisplay()
        : m_iCurrentVRAM(0)
        , m_cGuestScreenCount(0)
        , m_graphicsControllerType(KGraphicsControllerType_Null)
#ifdef VBOX_WITH_3D_ACCELERATION
        , m_f3dAccelerationEnabled(false)
#endif
        , m_fRemoteDisplayServerSupported(false)
        , m_fRemoteDisplayServerEnabled(false)
        , m_strRemoteDisplayPort(QString())
        , m_remoteDisplayAuthType(KAuthType_Null)
        , m_uRemoteDisplayTimeout(0)
        , m_fRemoteDisplayMultiConnAllowed(false)
        , m_fRecordingEnabled(false)
        , m_strRecordingFolder(QString())
        , m_strRecordingFilePath(QString())
        , m_iRecordingVideoFrameWidth(0)
        , m_iRecordingVideoFrameHeight(0)
        , m_iRecordingVideoFrameRate(0)
        , m_iRecordingVideoBitRate(0)
        , m_strRecordingVideoOptions(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsMachineDisplay &other) const
    {
        return true
               && (m_iCurrentVRAM == other.m_iCurrentVRAM)
               && (m_cGuestScreenCount == other.m_cGuestScreenCount)
               && (m_scaleFactors == other.m_scaleFactors)
               && (m_graphicsControllerType == other.m_graphicsControllerType)
#ifdef VBOX_WITH_3D_ACCELERATION
               && (m_f3dAccelerationEnabled == other.m_f3dAccelerationEnabled)
#endif
               && (m_fRemoteDisplayServerSupported == other.m_fRemoteDisplayServerSupported)
               && (m_fRemoteDisplayServerEnabled == other.m_fRemoteDisplayServerEnabled)
               && (m_strRemoteDisplayPort == other.m_strRemoteDisplayPort)
               && (m_remoteDisplayAuthType == other.m_remoteDisplayAuthType)
               && (m_uRemoteDisplayTimeout == other.m_uRemoteDisplayTimeout)
               && (m_fRemoteDisplayMultiConnAllowed == other.m_fRemoteDisplayMultiConnAllowed)
               && (m_fRecordingEnabled == other.m_fRecordingEnabled)
               && (m_strRecordingFilePath == other.m_strRecordingFilePath)
               && (m_iRecordingVideoFrameWidth == other.m_iRecordingVideoFrameWidth)
               && (m_iRecordingVideoFrameHeight == other.m_iRecordingVideoFrameHeight)
               && (m_iRecordingVideoFrameRate == other.m_iRecordingVideoFrameRate)
               && (m_iRecordingVideoBitRate == other.m_iRecordingVideoBitRate)
               && (m_vecRecordingScreens == other.m_vecRecordingScreens)
               && (m_strRecordingVideoOptions == other.m_strRecordingVideoOptions)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsMachineDisplay &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsMachineDisplay &other) const { return !equal(other); }

    /** Recording options. */
    enum RecordingOption
    {
        RecordingOption_Unknown,
        RecordingOption_AC,
        RecordingOption_VC,
        RecordingOption_AC_Profile
    };

    /** Returns enum value corresponding to passed @a strKey. */
    static RecordingOption toRecordingOptionKey(const QString &strKey)
    {
        /* Compare case-sensitive: */
        QMap<QString, RecordingOption> keys;
        keys["ac_enabled"] = RecordingOption_AC;
        keys["vc_enabled"] = RecordingOption_VC;
        keys["ac_profile"] = RecordingOption_AC_Profile;
        /* Return known value or RecordingOption_Unknown otherwise: */
        return keys.value(strKey, RecordingOption_Unknown);
    }

    /** Returns string representation for passed enum @a enmKey. */
    static QString fromRecordingOptionKey(RecordingOption enmKey)
    {
        /* Compare case-sensitive: */
        QMap<RecordingOption, QString> values;
        values[RecordingOption_AC] = "ac_enabled";
        values[RecordingOption_VC] = "vc_enabled";
        values[RecordingOption_AC_Profile] = "ac_profile";
        /* Return known value or QString() otherwise: */
        return values.value(enmKey);
    }

    /** Parses recording options. */
    static void parseRecordingOptions(const QString &strOptions,
                                      QList<RecordingOption> &outKeys,
                                      QStringList &outValues)
    {
        outKeys.clear();
        outValues.clear();
        const QStringList aPairs = strOptions.split(',');
        foreach (const QString &strPair, aPairs)
        {
            const QStringList aPair = strPair.split('=');
            if (aPair.size() != 2)
                continue;
            const RecordingOption enmKey = toRecordingOptionKey(aPair.value(0));
            if (enmKey == RecordingOption_Unknown)
                continue;
            outKeys << enmKey;
            outValues << aPair.value(1);
        }
    }

    /** Serializes recording options. */
    static void serializeRecordingOptions(const QList<RecordingOption> &inKeys,
                                          const QStringList &inValues,
                                          QString &strOptions)
    {
        QStringList aPairs;
        for (int i = 0; i < inKeys.size(); ++i)
        {
            QStringList aPair;
            aPair << fromRecordingOptionKey(inKeys.value(i));
            aPair << inValues.value(i);
            aPairs << aPair.join('=');
        }
        strOptions = aPairs.join(',');
    }

    /** Returns whether passed Recording @a enmOption is enabled. */
    static bool isRecordingOptionEnabled(const QString &strOptions,
                                         RecordingOption enmOption)
    {
        QList<RecordingOption> aKeys;
        QStringList aValues;
        parseRecordingOptions(strOptions, aKeys, aValues);
        int iIndex = aKeys.indexOf(enmOption);
        if (iIndex == -1)
            return false; /* If option is missing, assume disabled (false). */
        if (aValues.value(iIndex).compare("true", Qt::CaseInsensitive) == 0)
            return true;
        return false;
    }

    /** Searches for ac_profile and return 1 for "low", 2 for "med", and 3 for "high". Returns 2
        if ac_profile is missing */
    static int getAudioQualityFromOptions(const QString &strOptions)
    {
        QList<RecordingOption> aKeys;
        QStringList aValues;
        parseRecordingOptions(strOptions, aKeys, aValues);
        int iIndex = aKeys.indexOf(RecordingOption_AC_Profile);
        if (iIndex == -1)
            return 2;
        if (aValues.value(iIndex).compare("low", Qt::CaseInsensitive) == 0)
            return 1;
        if (aValues.value(iIndex).compare("high", Qt::CaseInsensitive) == 0)
            return 3;
        return 2;
    }

    /** Sets the video recording options for @a enmOptions to @a values. */
    static QString setRecordingOptions(const QString &strOptions,
                                       const QVector<RecordingOption> &enmOptions,
                                       const QStringList &values)
    {
        if (enmOptions.size() != values.size())
            return QString();
        QList<RecordingOption> aKeys;
        QStringList aValues;
        parseRecordingOptions(strOptions, aKeys, aValues);
        for(int i = 0; i < values.size(); ++i)
        {
            QString strValue = values[i];
            int iIndex = aKeys.indexOf(enmOptions[i]);
            if (iIndex == -1)
            {
                aKeys << enmOptions[i];
                aValues << strValue;
            }
            else
            {
                aValues[iIndex] = strValue;
            }
        }
        QString strResult;
        serializeRecordingOptions(aKeys, aValues, strResult);
        return strResult;
    }

    /** Holds the video RAM amount. */
    int                      m_iCurrentVRAM;
    /** Holds the guest screen count. */
    int                      m_cGuestScreenCount;
    /** Holds the guest screen scale-factor. */
    QList<double>            m_scaleFactors;
    /** Holds the graphics controller type. */
    KGraphicsControllerType  m_graphicsControllerType;
#ifdef VBOX_WITH_3D_ACCELERATION
    /** Holds whether the 3D acceleration is enabled. */
    bool                     m_f3dAccelerationEnabled;
#endif
    /** Holds whether the remote display server is supported. */
    bool                     m_fRemoteDisplayServerSupported;
    /** Holds whether the remote display server is enabled. */
    bool                     m_fRemoteDisplayServerEnabled;
    /** Holds the remote display server port. */
    QString                  m_strRemoteDisplayPort;
    /** Holds the remote display server auth type. */
    KAuthType                m_remoteDisplayAuthType;
    /** Holds the remote display server timeout. */
    ulong                    m_uRemoteDisplayTimeout;
    /** Holds whether the remote display server allows multiple connections. */
    bool                     m_fRemoteDisplayMultiConnAllowed;

    /** Holds whether recording is enabled. */
    bool m_fRecordingEnabled;
    /** Holds the recording folder. */
    QString m_strRecordingFolder;
    /** Holds the recording file path. */
    QString m_strRecordingFilePath;
    /** Holds the recording frame width. */
    int m_iRecordingVideoFrameWidth;
    /** Holds the recording frame height. */
    int m_iRecordingVideoFrameHeight;
    /** Holds the recording frame rate. */
    int m_iRecordingVideoFrameRate;
    /** Holds the recording bit rate. */
    int m_iRecordingVideoBitRate;
    /** Holds which of the guest screens should be recorded. */
    QVector<BOOL> m_vecRecordingScreens;
    /** Holds the video recording options. */
    QString m_strRecordingVideoOptions;
};


UIMachineSettingsDisplay::UIMachineSettingsDisplay()
    : m_comGuestOSType(CGuestOSType())
#ifdef VBOX_WITH_3D_ACCELERATION
    , m_fWddmModeSupported(false)
#endif
    , m_enmGraphicsControllerTypeRecommended(KGraphicsControllerType_Null)
    , m_pCache(0)
    , m_pCheckbox3D(0)
    , m_pCheckboxRemoteDisplay(0)
    , m_pCheckboxMultipleConn(0)
    , m_pCheckboxVideoCapture(0)
    , m_pComboRemoteDisplayAuthMethod(0)
    , m_pComboBoxCaptureMode(0)
    , m_pComboVideoCaptureSize(0)
    , m_pLabelVideoScreenCountMin(0)
    , m_pLabelVideoScreenCountMax(0)
    , m_pLabelVideoCaptureFrameRateMin(0)
    , m_pLabelVideoCaptureFrameRateMax(0)
    , m_pLabelVideoCaptureQualityMin(0)
    , m_pLabelVideoCaptureQualityMed(0)
    , m_pLabelVideoCaptureQualityMax(0)
    , m_pLabelAudioCaptureQualityMin(0)
    , m_pLabelAudioCaptureQualityMed(0)
    , m_pLabelAudioCaptureQualityMax(0)
    , m_pVideoMemoryLabel(0)
    , m_pLabelVideoScreenCount(0)
    , m_pGraphicsControllerLabel(0)
    , m_pLabelVideoOptions(0)
    , m_pLabelRemoteDisplayOptions(0)
    , m_pLabelCaptureMode(0)
    , m_pLabelVideoCapturePath(0)
    , m_pLabelVideoCaptureSizeHint(0)
    , m_pLabelVideoCaptureSize(0)
    , m_pLabelVideoCaptureFrameRate(0)
    , m_pLabelVideoCaptureRate(0)
    , m_pAudioCaptureQualityLabel(0)
    , m_pLabelVideoCaptureScreens(0)
    , m_pLabelGuestScreenScaleFactorEditor(0)
    , m_pLabelRemoteDisplayPort(0)
    , m_pLabelRemoteDisplayAuthMethod(0)
    , m_pLabelRemoteDisplayTimeout(0)
    , m_pEditorRemoteDisplayPort(0)
    , m_pEditorRemoteDisplayTimeout(0)
    , m_pEditorVideoScreenCount(0)
    , m_pEditorVideoCaptureWidth(0)
    , m_pEditorVideoCaptureFrameRate(0)
    , m_pEditorVideoCaptureHeight(0)
    , m_pEditorVideoCaptureBitRate(0)
    , m_pGraphicsControllerEditor(0)
    , m_pScaleFactorEditor(0)
    , m_pVideoMemoryEditor(0)
    , m_pEditorVideoCapturePath(0)
    , m_pScrollerVideoCaptureScreens(0)
    , m_pSliderAudioCaptureQuality(0)
    , m_pSliderVideoScreenCount(0)
    , m_pSliderVideoCaptureFrameRate(0)
    , m_pSliderVideoCaptureQuality(0)
    , m_pTabWidget(0)
    , m_pContainerRemoteDisplay(0)
    , m_pContainerRemoteDisplayOptions(0)
    , m_pContainerVideoCapture(0)
    , m_pContainerSliderVideoCaptureFrameRate(0)
    , m_pContainerSliderVideoCaptureQuality(0)
    , m_pContainerSliderAudioCaptureQuality(0)
    , m_pTabVideo(0)
    , m_pTabRemoteDisplay(0)
    , m_pTabVideoCapture(0)
    , m_pContainerLayoutSliderVideoCaptureQuality(0)
    , m_pLayout3D(0)
{
    /* Prepare: */
    prepare();
}

UIMachineSettingsDisplay::~UIMachineSettingsDisplay()
{
    /* Cleanup: */
    cleanup();
}

void UIMachineSettingsDisplay::setGuestOSType(CGuestOSType comGuestOSType)
{
    /* Check if guest OS type changed: */
    if (m_comGuestOSType == comGuestOSType)
        return;

    /* Remember new guest OS type: */
    m_comGuestOSType = comGuestOSType;
    m_pVideoMemoryEditor->setGuestOSType(m_comGuestOSType);

#ifdef VBOX_WITH_3D_ACCELERATION
    /* Check if WDDM mode supported by the guest OS type: */
    const QString strGuestOSTypeId = m_comGuestOSType.isNotNull() ? m_comGuestOSType.GetId() : QString();
    m_fWddmModeSupported = UICommon::isWddmCompatibleOsType(strGuestOSTypeId);
    m_pVideoMemoryEditor->set3DAccelerationSupported(m_fWddmModeSupported);
#endif
    /* Acquire recommended graphics controller type: */
    m_enmGraphicsControllerTypeRecommended = m_comGuestOSType.GetRecommendedGraphicsController();

    /* Revalidate: */
    revalidate();
}

#ifdef VBOX_WITH_3D_ACCELERATION
bool UIMachineSettingsDisplay::isAcceleration3DSelected() const
{
    return m_pCheckbox3D->isChecked();
}
#endif /* VBOX_WITH_3D_ACCELERATION */

KGraphicsControllerType UIMachineSettingsDisplay::graphicsControllerTypeRecommended() const
{
    return   m_pGraphicsControllerEditor->supportedValues().contains(m_enmGraphicsControllerTypeRecommended)
           ? m_enmGraphicsControllerTypeRecommended
           : graphicsControllerTypeCurrent();
}

KGraphicsControllerType UIMachineSettingsDisplay::graphicsControllerTypeCurrent() const
{
    return m_pGraphicsControllerEditor->value();
}

bool UIMachineSettingsDisplay::changed() const
{
    return m_pCache->wasChanged();
}

void UIMachineSettingsDisplay::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old display data: */
    UIDataSettingsMachineDisplay oldDisplayData;

    /* Check whether graphics adapter is valid: */
    const CGraphicsAdapter &comGraphics = m_machine.GetGraphicsAdapter();
    if (!comGraphics.isNull())
    {
        /* Gather old 'Screen' data: */
        oldDisplayData.m_iCurrentVRAM = comGraphics.GetVRAMSize();
        oldDisplayData.m_cGuestScreenCount = comGraphics.GetMonitorCount();
        oldDisplayData.m_scaleFactors = gEDataManager->scaleFactors(m_machine.GetId());
        oldDisplayData.m_graphicsControllerType = comGraphics.GetGraphicsControllerType();
#ifdef VBOX_WITH_3D_ACCELERATION
        oldDisplayData.m_f3dAccelerationEnabled = comGraphics.GetAccelerate3DEnabled();
#endif
    }

    /* Check whether remote display server is valid: */
    const CVRDEServer &vrdeServer = m_machine.GetVRDEServer();
    oldDisplayData.m_fRemoteDisplayServerSupported = !vrdeServer.isNull();
    if (!vrdeServer.isNull())
    {
        /* Gather old 'Remote Display' data: */
        oldDisplayData.m_fRemoteDisplayServerEnabled = vrdeServer.GetEnabled();
        oldDisplayData.m_strRemoteDisplayPort = vrdeServer.GetVRDEProperty("TCP/Ports");
        oldDisplayData.m_remoteDisplayAuthType = vrdeServer.GetAuthType();
        oldDisplayData.m_uRemoteDisplayTimeout = vrdeServer.GetAuthTimeout();
        oldDisplayData.m_fRemoteDisplayMultiConnAllowed = vrdeServer.GetAllowMultiConnection();
    }

    /* Gather old 'Recording' data: */
    CRecordingSettings recordingSettings = m_machine.GetRecordingSettings();
    Assert(recordingSettings.isNotNull());
    oldDisplayData.m_fRecordingEnabled = recordingSettings.GetEnabled();

    /* For now we're using the same settings for all screens; so get settings from screen 0 and work with that. */
    CRecordingScreenSettings recordingScreen0Settings = recordingSettings.GetScreenSettings(0);
    if (!recordingScreen0Settings.isNull())
    {
        oldDisplayData.m_strRecordingFolder = QFileInfo(m_machine.GetSettingsFilePath()).absolutePath();
        oldDisplayData.m_strRecordingFilePath = recordingScreen0Settings.GetFilename();
        oldDisplayData.m_iRecordingVideoFrameWidth = recordingScreen0Settings.GetVideoWidth();
        oldDisplayData.m_iRecordingVideoFrameHeight = recordingScreen0Settings.GetVideoHeight();
        oldDisplayData.m_iRecordingVideoFrameRate = recordingScreen0Settings.GetVideoFPS();
        oldDisplayData.m_iRecordingVideoBitRate = recordingScreen0Settings.GetVideoRate();
        oldDisplayData.m_strRecordingVideoOptions = recordingScreen0Settings.GetOptions();
    }

    CRecordingScreenSettingsVector recordingScreenSettingsVector = recordingSettings.GetScreens();
    oldDisplayData.m_vecRecordingScreens.resize(recordingScreenSettingsVector.size());
    for (int iScreenIndex = 0; iScreenIndex < recordingScreenSettingsVector.size(); ++iScreenIndex)
    {
        CRecordingScreenSettings recordingScreenSettings = recordingScreenSettingsVector.at(iScreenIndex);
        if (!recordingScreenSettings.isNull())
            oldDisplayData.m_vecRecordingScreens[iScreenIndex] = recordingScreenSettings.GetEnabled();
    }

    /* Cache old display data: */
    m_pCache->cacheInitialData(oldDisplayData);

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

void UIMachineSettingsDisplay::getFromCache()
{
    /* Get old display data from the cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();

    /* Load old 'Screen' data from the cache: */
    m_pEditorVideoScreenCount->setValue(oldDisplayData.m_cGuestScreenCount);
    m_pScaleFactorEditor->setScaleFactors(oldDisplayData.m_scaleFactors);
    m_pScaleFactorEditor->setMonitorCount(oldDisplayData.m_cGuestScreenCount);
    m_pGraphicsControllerEditor->setValue(oldDisplayData.m_graphicsControllerType);
#ifdef VBOX_WITH_3D_ACCELERATION
    m_pCheckbox3D->setChecked(oldDisplayData.m_f3dAccelerationEnabled);
#endif
    /* Push required value to m_pVideoMemoryEditor: */
    sltHandleGuestScreenCountEditorChange();
    sltHandleGraphicsControllerComboChange();
#ifdef VBOX_WITH_3D_ACCELERATION
    sltHandle3DAccelerationCheckboxChange();
#endif
    // Should be the last one for this tab, since it depends on some of others:
    m_pVideoMemoryEditor->setValue(oldDisplayData.m_iCurrentVRAM);

    /* If remote display server is supported: */
    if (oldDisplayData.m_fRemoteDisplayServerSupported)
    {
        /* Load old 'Remote Display' data from the cache: */
        m_pCheckboxRemoteDisplay->setChecked(oldDisplayData.m_fRemoteDisplayServerEnabled);
        m_pEditorRemoteDisplayPort->setText(oldDisplayData.m_strRemoteDisplayPort);
        m_pComboRemoteDisplayAuthMethod->setCurrentIndex(m_pComboRemoteDisplayAuthMethod->findText(gpConverter->toString(oldDisplayData.m_remoteDisplayAuthType)));
        m_pEditorRemoteDisplayTimeout->setText(QString::number(oldDisplayData.m_uRemoteDisplayTimeout));
        m_pCheckboxMultipleConn->setChecked(oldDisplayData.m_fRemoteDisplayMultiConnAllowed);
    }

    /* Load old 'Recording' data from the cache: */
    m_pCheckboxVideoCapture->setChecked(oldDisplayData.m_fRecordingEnabled);
    m_pEditorVideoCapturePath->setHomeDir(oldDisplayData.m_strRecordingFolder);
    m_pEditorVideoCapturePath->setPath(oldDisplayData.m_strRecordingFilePath);
    m_pEditorVideoCaptureWidth->setValue(oldDisplayData.m_iRecordingVideoFrameWidth);
    m_pEditorVideoCaptureHeight->setValue(oldDisplayData.m_iRecordingVideoFrameHeight);
    m_pEditorVideoCaptureFrameRate->setValue(oldDisplayData.m_iRecordingVideoFrameRate);
    m_pEditorVideoCaptureBitRate->setValue(oldDisplayData.m_iRecordingVideoBitRate);
    m_pScrollerVideoCaptureScreens->setValue(oldDisplayData.m_vecRecordingScreens);

    /* Load data from old 'Recording option': */
    bool fRecordAudio = UIDataSettingsMachineDisplay::isRecordingOptionEnabled(oldDisplayData.m_strRecordingVideoOptions,
                                                                                UIDataSettingsMachineDisplay::RecordingOption_AC);
    bool fRecordVideo = UIDataSettingsMachineDisplay::isRecordingOptionEnabled(oldDisplayData.m_strRecordingVideoOptions,
                                                                                UIDataSettingsMachineDisplay::RecordingOption_VC);
    if (fRecordAudio && fRecordVideo)
        m_pComboBoxCaptureMode->setCurrentIndex(m_pComboBoxCaptureMode->findText(gpConverter->toString(UISettingsDefs::RecordingMode_VideoAudio)));
    else if (fRecordAudio && !fRecordVideo)
        m_pComboBoxCaptureMode->setCurrentIndex(m_pComboBoxCaptureMode->findText(gpConverter->toString(UISettingsDefs::RecordingMode_AudioOnly)));
    else
        m_pComboBoxCaptureMode->setCurrentIndex(m_pComboBoxCaptureMode->findText(gpConverter->toString(UISettingsDefs::RecordingMode_VideoOnly)));

    m_pSliderAudioCaptureQuality->setValue(UIDataSettingsMachineDisplay::getAudioQualityFromOptions(oldDisplayData.m_strRecordingVideoOptions));

    /* Polish page finally: */
    polishPage();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::putToCache()
{
    /* Prepare new display data: */
    UIDataSettingsMachineDisplay newDisplayData;

    /* Gather new 'Screen' data: */
    newDisplayData.m_iCurrentVRAM = m_pVideoMemoryEditor->value();
    newDisplayData.m_cGuestScreenCount = m_pEditorVideoScreenCount->value();
    newDisplayData.m_scaleFactors = m_pScaleFactorEditor->scaleFactors();
    newDisplayData.m_graphicsControllerType = m_pGraphicsControllerEditor->value();
#ifdef VBOX_WITH_3D_ACCELERATION
    newDisplayData.m_f3dAccelerationEnabled = m_pCheckbox3D->isChecked();
#endif
    /* If remote display server is supported: */
    newDisplayData.m_fRemoteDisplayServerSupported = m_pCache->base().m_fRemoteDisplayServerSupported;
    if (newDisplayData.m_fRemoteDisplayServerSupported)
    {
        /* Gather new 'Remote Display' data: */
        newDisplayData.m_fRemoteDisplayServerEnabled = m_pCheckboxRemoteDisplay->isChecked();
        newDisplayData.m_strRemoteDisplayPort = m_pEditorRemoteDisplayPort->text();
        newDisplayData.m_remoteDisplayAuthType = gpConverter->fromString<KAuthType>(m_pComboRemoteDisplayAuthMethod->currentText());
        newDisplayData.m_uRemoteDisplayTimeout = m_pEditorRemoteDisplayTimeout->text().toULong();
        newDisplayData.m_fRemoteDisplayMultiConnAllowed = m_pCheckboxMultipleConn->isChecked();
    }

    /* Gather new 'Recording' data: */
    newDisplayData.m_fRecordingEnabled = m_pCheckboxVideoCapture->isChecked();
    newDisplayData.m_strRecordingFolder = m_pCache->base().m_strRecordingFolder;
    newDisplayData.m_strRecordingFilePath = m_pEditorVideoCapturePath->path();
    newDisplayData.m_iRecordingVideoFrameWidth = m_pEditorVideoCaptureWidth->value();
    newDisplayData.m_iRecordingVideoFrameHeight = m_pEditorVideoCaptureHeight->value();
    newDisplayData.m_iRecordingVideoFrameRate = m_pEditorVideoCaptureFrameRate->value();
    newDisplayData.m_iRecordingVideoBitRate = m_pEditorVideoCaptureBitRate->value();
    newDisplayData.m_vecRecordingScreens = m_pScrollerVideoCaptureScreens->value();

    /* Update recording options */
    const UISettingsDefs::RecordingMode enmRecordingMode =
        gpConverter->fromString<UISettingsDefs::RecordingMode>(m_pComboBoxCaptureMode->currentText());
    QStringList optionValues;
    /* Option value for video recording: */
    optionValues.push_back(     (enmRecordingMode == UISettingsDefs::RecordingMode_VideoAudio)
                             || (enmRecordingMode == UISettingsDefs::RecordingMode_VideoOnly)
                           ? "true" : "false");
    /* Option value for audio recording: */
    optionValues.push_back(     (enmRecordingMode == UISettingsDefs::RecordingMode_VideoAudio)
                             || (enmRecordingMode == UISettingsDefs::RecordingMode_AudioOnly)
                           ? "true" : "false");

    if (m_pSliderAudioCaptureQuality->value() == 1)
        optionValues.push_back("low");
    else if (m_pSliderAudioCaptureQuality->value() == 2)
        optionValues.push_back("med");
    else
        optionValues.push_back("high");

    QVector<UIDataSettingsMachineDisplay::RecordingOption> recordingOptionsVector;
    recordingOptionsVector.push_back(UIDataSettingsMachineDisplay::RecordingOption_VC);
    recordingOptionsVector.push_back(UIDataSettingsMachineDisplay::RecordingOption_AC);
    recordingOptionsVector.push_back(UIDataSettingsMachineDisplay::RecordingOption_AC_Profile);

    newDisplayData.m_strRecordingVideoOptions = UIDataSettingsMachineDisplay::setRecordingOptions(m_pCache->base().m_strRecordingVideoOptions,
                                                                                                   recordingOptionsVector,
                                                                                                   optionValues);

    /* Cache new display data: */
    m_pCache->cacheCurrentData(newDisplayData);
}

void UIMachineSettingsDisplay::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to machine: */
    UISettingsPageMachine::fetchData(data);

    /* Update display data and failing state: */
    setFailed(!saveDisplayData());

    /* Upload machine to data: */
    UISettingsPageMachine::uploadData(data);
}

bool UIMachineSettingsDisplay::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Screen tab: */
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.first = UICommon::removeAccelMark(m_pTabWidget->tabText(0));

        /* Video RAM amount test: */
        if (shouldWeWarnAboutLowVRAM() && !m_comGuestOSType.isNull())
        {
            quint64 uNeedBytes = UICommon::requiredVideoMemory(m_comGuestOSType.GetId(), m_pEditorVideoScreenCount->value());

            /* Basic video RAM amount test: */
            if ((quint64)m_pVideoMemoryEditor->value() * _1M < uNeedBytes)
            {
                message.second << tr("The virtual machine is currently assigned less than <b>%1</b> of video memory "
                                     "which is the minimum amount required to switch to full-screen or seamless mode.")
                                     .arg(uiCommon().formatSize(uNeedBytes, 0, FormatSize_RoundUp));
            }
#ifdef VBOX_WITH_3D_ACCELERATION
            /* 3D acceleration video RAM amount test: */
            else if (m_pCheckbox3D->isChecked() && m_fWddmModeSupported)
            {
                uNeedBytes = qMax(uNeedBytes, (quint64) 128 * _1M);
                if ((quint64)m_pVideoMemoryEditor->value() * _1M < uNeedBytes)
                {
                    message.second << tr("The virtual machine is set up to use hardware graphics acceleration "
                                         "and the operating system hint is set to Windows Vista or later. "
                                         "For best performance you should set the machine's video memory to at least <b>%1</b>.")
                                         .arg(uiCommon().formatSize(uNeedBytes, 0, FormatSize_RoundUp));
                }
            }
#endif /* VBOX_WITH_3D_ACCELERATION */
        }

        /* Graphics controller type test: */
        if (!m_comGuestOSType.isNull())
        {
            if (graphicsControllerTypeCurrent() != graphicsControllerTypeRecommended())
            {
#ifdef VBOX_WITH_3D_ACCELERATION
                if (m_pCheckbox3D->isChecked())
                    message.second << tr("The virtual machine is configured to use 3D acceleration. This will work only if you "
                                         "pick a different graphics controller (%1). Either disable 3D acceleration or switch "
                                         "to required graphics controller type. The latter will be done automatically if you "
                                         "confirm your changes.")
                                         .arg(gpConverter->toString(m_enmGraphicsControllerTypeRecommended));
                else
#endif /* VBOX_WITH_3D_ACCELERATION */
                    message.second << tr("The virtual machine is configured to use a graphics controller other than the "
                                         "recommended one (%1). Please consider switching unless you have a reason to keep the "
                                         "currently selected graphics controller.")
                                         .arg(gpConverter->toString(m_enmGraphicsControllerTypeRecommended));
            }
        }

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Remote Display tab: */
    {
        /* Prepare message: */
        UIValidationMessage message;
        message.first = UICommon::removeAccelMark(m_pTabWidget->tabText(1));

#ifdef VBOX_WITH_EXTPACK
        /* VRDE Extension Pack presence test: */
        CExtPack extPack = uiCommon().virtualBox().GetExtensionPackManager().Find(GUI_ExtPackName);
        if (m_pCheckboxRemoteDisplay->isChecked() && (extPack.isNull() || !extPack.GetUsable()))
        {
            message.second << tr("Remote Display is currently enabled for this virtual machine. "
                                 "However, this requires the <i>%1</i> to be installed. "
                                 "Please install the Extension Pack from the VirtualBox download site as "
                                 "otherwise your VM will be started with Remote Display disabled.")
                                 .arg(GUI_ExtPackName);
        }
#endif /* VBOX_WITH_EXTPACK */

        /* Check VRDE server port: */
        if (m_pEditorRemoteDisplayPort->text().trimmed().isEmpty())
        {
            message.second << tr("The VRDE server port value is not currently specified.");
            fPass = false;
        }

        /* Check VRDE server timeout: */
        if (m_pEditorRemoteDisplayTimeout->text().trimmed().isEmpty())
        {
            message.second << tr("The VRDE authentication timeout value is not currently specified.");
            fPass = false;
        }

        /* Serialize message: */
        if (!message.second.isEmpty())
            messages << message;
    }

    /* Return result: */
    return fPass;
}

void UIMachineSettingsDisplay::setOrderAfter(QWidget *pWidget)
{
    /* Screen tab-order: */
    setTabOrder(pWidget, m_pTabWidget->focusProxy());
    setTabOrder(m_pTabWidget->focusProxy(), m_pVideoMemoryEditor);
    setTabOrder(m_pVideoMemoryEditor, m_pSliderVideoScreenCount);
    setTabOrder(m_pSliderVideoScreenCount, m_pEditorVideoScreenCount);
    setTabOrder(m_pEditorVideoScreenCount, m_pScaleFactorEditor);
    setTabOrder(m_pScaleFactorEditor, m_pGraphicsControllerEditor);

    /* Remote Display tab-order: */
    setTabOrder(m_pCheckboxRemoteDisplay, m_pEditorRemoteDisplayPort);
    setTabOrder(m_pEditorRemoteDisplayPort, m_pComboRemoteDisplayAuthMethod);
    setTabOrder(m_pComboRemoteDisplayAuthMethod, m_pEditorRemoteDisplayTimeout);
    setTabOrder(m_pEditorRemoteDisplayTimeout, m_pCheckboxMultipleConn);

    /* Recording tab-order: */
    setTabOrder(m_pCheckboxMultipleConn, m_pCheckboxVideoCapture);
    setTabOrder(m_pCheckboxVideoCapture, m_pEditorVideoCapturePath);
    setTabOrder(m_pEditorVideoCapturePath, m_pComboVideoCaptureSize);
    setTabOrder(m_pComboVideoCaptureSize, m_pEditorVideoCaptureWidth);
    setTabOrder(m_pEditorVideoCaptureWidth, m_pEditorVideoCaptureHeight);
    setTabOrder(m_pEditorVideoCaptureHeight, m_pSliderVideoCaptureFrameRate);
    setTabOrder(m_pSliderVideoCaptureFrameRate, m_pEditorVideoCaptureFrameRate);
    setTabOrder(m_pEditorVideoCaptureFrameRate, m_pSliderVideoCaptureQuality);
    setTabOrder(m_pSliderVideoCaptureQuality, m_pEditorVideoCaptureBitRate);
}

void UIMachineSettingsDisplay::retranslateUi()
{
    m_pVideoMemoryLabel->setText(tr("Video &Memory:"));
    m_pVideoMemoryEditor->setWhatsThis(tr("Controls the amount of video memory provided to the virtual machine."));
    m_pLabelVideoScreenCount->setText(tr("Mo&nitor Count:"));
    m_pSliderVideoScreenCount->setWhatsThis(tr("Controls the amount of virtual monitors provided to the virtual machine."));
    m_pEditorVideoScreenCount->setWhatsThis(tr("Controls the amount of virtual monitors provided to the virtual machine."));
    m_pLabelGuestScreenScaleFactorEditor->setText(tr("Scale Factor:"));
    m_pScaleFactorEditor->setWhatsThis(tr("Controls the guest screen scale factor."));
    m_pGraphicsControllerLabel->setText(tr("&Graphics Controller:"));
    m_pGraphicsControllerEditor->setWhatsThis(tr("Selects the graphics adapter type the virtual machine will use."));
    m_pLabelVideoOptions->setText(tr("Acceleration:"));
    m_pCheckbox3D->setWhatsThis(tr("When checked, the virtual machine will be given access to the 3D graphics capabilities available on the host."));
    m_pCheckbox3D->setText(tr("Enable &3D Acceleration"));
    m_pTabWidget->setTabText(m_pTabWidget->indexOf(m_pTabVideo), tr("&Screen"));
    m_pCheckboxRemoteDisplay->setWhatsThis(tr("When checked, the VM will act as a Remote Desktop Protocol (RDP) server, allowing remote clients to connect and operate the VM (when it is running) using a standard RDP client."));
    m_pCheckboxRemoteDisplay->setText(tr("&Enable Server"));
    m_pLabelRemoteDisplayPort->setText(tr("Server &Port:"));
    m_pEditorRemoteDisplayPort->setWhatsThis(tr("Holds the VRDP Server port number. You may specify <tt>0</tt> (zero), to select port 3389, the standard port for RDP."));
    m_pLabelRemoteDisplayAuthMethod->setText(tr("Authentication &Method:"));
    m_pComboRemoteDisplayAuthMethod->setWhatsThis(tr("Selects the VRDP authentication method."));
    m_pLabelRemoteDisplayTimeout->setText(tr("Authentication &Timeout:"));
    m_pEditorRemoteDisplayTimeout->setWhatsThis(tr("Holds the timeout for guest authentication, in milliseconds."));
    m_pLabelRemoteDisplayOptions->setText(tr("Extended Features:"));
    m_pCheckboxMultipleConn->setWhatsThis(tr("When checked, multiple simultaneous connections to the VM are permitted."));
    m_pCheckboxMultipleConn->setText(tr("&Allow Multiple Connections"));
    m_pTabWidget->setTabText(m_pTabWidget->indexOf(m_pTabRemoteDisplay), tr("&Remote Display"));
    m_pCheckboxVideoCapture->setWhatsThis(tr("When checked, VirtualBox will record the virtual machine session as a video file."));
    m_pCheckboxVideoCapture->setText(tr("&Enable Recording"));
    m_pLabelCaptureMode->setText(tr("Recording &Mode:"));
    m_pComboBoxCaptureMode->setWhatsThis(tr("Selects the recording mode."));
    m_pLabelVideoCapturePath->setText(tr("File &Path:"));
    m_pEditorVideoCapturePath->setWhatsThis(tr("Holds the filename VirtualBox uses to save the recorded content."));
    m_pLabelVideoCaptureSize->setText(tr("Frame &Size:"));
    m_pComboVideoCaptureSize->setWhatsThis(tr("Selects the resolution (frame size) of the recorded video."));
    m_pEditorVideoCaptureWidth->setWhatsThis(tr("Holds the <b>horizontal</b> resolution (frame width) of the recorded video."));
    m_pEditorVideoCaptureHeight->setWhatsThis(tr("Holds the <b>vertical</b> resolution (frame height) of the recorded video."));
    m_pLabelVideoCaptureFrameRate->setText(tr("&Frame Rate:"));
    m_pSliderVideoCaptureFrameRate->setWhatsThis(tr("Controls the maximum number of <b>frames per second</b>. Additional frames will be skipped. Reducing this value will increase the number of skipped frames and reduce the file size."));
    m_pEditorVideoCaptureFrameRate->setWhatsThis(tr("Controls the maximum number of <b>frames per second</b>. Additional frames will be skipped. Reducing this value will increase the number of skipped frames and reduce the file size."));
    m_pLabelVideoCaptureRate->setText(tr("&Video Quality:"));
    m_pSliderVideoCaptureQuality->setWhatsThis(tr("Controls the <b>quality</b>. Increasing this value will make the video look better at the cost of an increased file size."));
    m_pEditorVideoCaptureBitRate->setWhatsThis(tr("Holds the bitrate in <b>kilobits per second</b>. Increasing this value will make the video look better at the cost of an increased file size."));
    m_pAudioCaptureQualityLabel->setText(tr("&Audio Quality:"));
    m_pSliderAudioCaptureQuality->setWhatsThis(tr("Controls the <b>quality</b>. Increasing this value will make the audio sound better at the cost of an increased file size."));
    m_pLabelVideoCaptureScreens->setText(tr("&Screens:"));
    m_pScrollerVideoCaptureScreens->setWhatsThis(QString());
    m_pTabWidget->setTabText(m_pTabWidget->indexOf(m_pTabVideoCapture), tr("Re&cording"));


    /* Screen stuff: */
    CSystemProperties sys = uiCommon().virtualBox().GetSystemProperties();
    m_pLabelVideoScreenCountMin->setText(QString::number(1));
    m_pLabelVideoScreenCountMax->setText(QString::number(qMin(sys.GetMaxGuestMonitors(), (ULONG)8)));

    /* Remote Display stuff: */
    m_pComboRemoteDisplayAuthMethod->setItemText(0, gpConverter->toString(KAuthType_Null));
    m_pComboRemoteDisplayAuthMethod->setItemText(1, gpConverter->toString(KAuthType_External));
    m_pComboRemoteDisplayAuthMethod->setItemText(2, gpConverter->toString(KAuthType_Guest));

    /* Recording stuff: */
    m_pEditorVideoCaptureFrameRate->setSuffix(QString(" %1").arg(tr("fps")));
    m_pEditorVideoCaptureBitRate->setSuffix(QString(" %1").arg(tr("kbps")));
    m_pComboVideoCaptureSize->setItemText(0, tr("User Defined"));
    m_pLabelVideoCaptureFrameRateMin->setText(tr("%1 fps").arg(m_pSliderVideoCaptureFrameRate->minimum()));
    m_pLabelVideoCaptureFrameRateMax->setText(tr("%1 fps").arg(m_pSliderVideoCaptureFrameRate->maximum()));
    m_pLabelVideoCaptureQualityMin->setText(tr("low", "quality"));
    m_pLabelVideoCaptureQualityMed->setText(tr("medium", "quality"));
    m_pLabelVideoCaptureQualityMax->setText(tr("high", "quality"));
    m_pLabelAudioCaptureQualityMin->setText(tr("low", "quality"));
    m_pLabelAudioCaptureQualityMed->setText(tr("medium", "quality"));
    m_pLabelAudioCaptureQualityMax->setText(tr("high", "quality"));

    m_pComboBoxCaptureMode->setItemText(0, gpConverter->toString(UISettingsDefs::RecordingMode_VideoAudio));
    m_pComboBoxCaptureMode->setItemText(1, gpConverter->toString(UISettingsDefs::RecordingMode_VideoOnly));
    m_pComboBoxCaptureMode->setItemText(2, gpConverter->toString(UISettingsDefs::RecordingMode_AudioOnly));

    updateRecordingFileSizeHint();
}

void UIMachineSettingsDisplay::polishPage()
{
    /* Get old display data from the cache: */
    const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();

    /* Polish 'Screen' availability: */
    m_pVideoMemoryLabel->setEnabled(isMachineOffline());
    m_pVideoMemoryEditor->setEnabled(isMachineOffline());
    m_pLabelVideoScreenCount->setEnabled(isMachineOffline());
    m_pSliderVideoScreenCount->setEnabled(isMachineOffline());
    m_pLabelVideoScreenCountMin->setEnabled(isMachineOffline());
    m_pLabelVideoScreenCountMax->setEnabled(isMachineOffline());
    m_pEditorVideoScreenCount->setEnabled(isMachineOffline());
    m_pScaleFactorEditor->setEnabled(isMachineInValidMode());
    m_pGraphicsControllerLabel->setEnabled(isMachineOffline());
    m_pGraphicsControllerEditor->setEnabled(isMachineOffline());
    m_pLabelVideoOptions->setEnabled(isMachineOffline());
#ifdef VBOX_WITH_3D_ACCELERATION
    m_pCheckbox3D->setEnabled(isMachineOffline());
#else
    m_pCheckbox3D->hide();
#endif

    /* Polish 'Remote Display' availability: */
    m_pTabWidget->setTabEnabled(1, oldDisplayData.m_fRemoteDisplayServerSupported);
    m_pContainerRemoteDisplay->setEnabled(isMachineInValidMode());
    m_pContainerRemoteDisplayOptions->setEnabled(m_pCheckboxRemoteDisplay->isChecked());
    m_pLabelRemoteDisplayOptions->setEnabled(isMachineOffline() || isMachineSaved());
    m_pCheckboxMultipleConn->setEnabled(isMachineOffline() || isMachineSaved());

    /* Polish 'Recording' availability: */
    m_pContainerVideoCapture->setEnabled(isMachineInValidMode());
    sltHandleRecordingCheckboxToggle();
}

void UIMachineSettingsDisplay::sltHandleGuestScreenCountSliderChange()
{
    /* Apply proposed screen-count: */
    m_pEditorVideoScreenCount->blockSignals(true);
    m_pEditorVideoScreenCount->setValue(m_pSliderVideoScreenCount->value());
    m_pEditorVideoScreenCount->blockSignals(false);

    /* Update Video RAM requirements: */
    m_pVideoMemoryEditor->setGuestScreenCount(m_pSliderVideoScreenCount->value());

    /* Update recording tab screen count: */
    updateGuestScreenCount();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleGuestScreenCountEditorChange()
{
    /* Apply proposed screen-count: */
    m_pSliderVideoScreenCount->blockSignals(true);
    m_pSliderVideoScreenCount->setValue(m_pEditorVideoScreenCount->value());
    m_pSliderVideoScreenCount->blockSignals(false);

    /* Update Video RAM requirements: */
    m_pVideoMemoryEditor->setGuestScreenCount(m_pEditorVideoScreenCount->value());

    /* Update recording tab screen count: */
    updateGuestScreenCount();

    /* Revalidate: */
    revalidate();
}

void UIMachineSettingsDisplay::sltHandleGraphicsControllerComboChange()
{
    /* Update Video RAM requirements: */
    m_pVideoMemoryEditor->setGraphicsControllerType(m_pGraphicsControllerEditor->value());

    /* Revalidate: */
    revalidate();
}

#ifdef VBOX_WITH_3D_ACCELERATION
void UIMachineSettingsDisplay::sltHandle3DAccelerationCheckboxChange()
{
    /* Update Video RAM requirements: */
    m_pVideoMemoryEditor->set3DAccelerationEnabled(m_pCheckbox3D->isChecked());

    /* Revalidate: */
    revalidate();
}
#endif /* VBOX_WITH_3D_ACCELERATION */

void UIMachineSettingsDisplay::sltHandleRecordingCheckboxToggle()
{
    /* Recording options should be enabled only if:
     * 1. Machine is in 'offline' or 'saved' state and check-box is checked,
     * 2. Machine is in 'online' state, check-box is checked, and video recording is *disabled* currently. */
    const bool fIsRecordingOptionsEnabled = ((isMachineOffline() || isMachineSaved()) && m_pCheckboxVideoCapture->isChecked()) ||
                                               (isMachineOnline() && !m_pCache->base().m_fRecordingEnabled && m_pCheckboxVideoCapture->isChecked());

    m_pLabelCaptureMode->setEnabled(fIsRecordingOptionsEnabled);
    m_pComboBoxCaptureMode->setEnabled(fIsRecordingOptionsEnabled);

    m_pLabelVideoCapturePath->setEnabled(fIsRecordingOptionsEnabled);
    m_pEditorVideoCapturePath->setEnabled(fIsRecordingOptionsEnabled);

    enableDisableRecordingWidgets();
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoFrameSizeComboboxChange()
{
    /* Get the proposed size: */
    const int iCurrentIndex = m_pComboVideoCaptureSize->currentIndex();
    const QSize videoCaptureSize = m_pComboVideoCaptureSize->itemData(iCurrentIndex).toSize();

    /* Make sure its valid: */
    if (!videoCaptureSize.isValid())
        return;

    /* Apply proposed size: */
    m_pEditorVideoCaptureWidth->setValue(videoCaptureSize.width());
    m_pEditorVideoCaptureHeight->setValue(videoCaptureSize.height());
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoFrameWidthEditorChange()
{
    /* Look for preset: */
    lookForCorrespondingFrameSizePreset();
    /* Update quality and bit-rate: */
    sltHandleRecordingVideoQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoFrameHeightEditorChange()
{
    /* Look for preset: */
    lookForCorrespondingFrameSizePreset();
    /* Update quality and bit-rate: */
    sltHandleRecordingVideoQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoFrameRateSliderChange()
{
    /* Apply proposed frame-rate: */
    m_pEditorVideoCaptureFrameRate->blockSignals(true);
    m_pEditorVideoCaptureFrameRate->setValue(m_pSliderVideoCaptureFrameRate->value());
    m_pEditorVideoCaptureFrameRate->blockSignals(false);
    /* Update quality and bit-rate: */
    sltHandleRecordingVideoQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoFrameRateEditorChange()
{
    /* Apply proposed frame-rate: */
    m_pSliderVideoCaptureFrameRate->blockSignals(true);
    m_pSliderVideoCaptureFrameRate->setValue(m_pEditorVideoCaptureFrameRate->value());
    m_pSliderVideoCaptureFrameRate->blockSignals(false);
    /* Update quality and bit-rate: */
    sltHandleRecordingVideoQualitySliderChange();
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoQualitySliderChange()
{
    /* Calculate/apply proposed bit-rate: */
    m_pEditorVideoCaptureBitRate->blockSignals(true);
    m_pEditorVideoCaptureBitRate->setValue(calculateBitRate(m_pEditorVideoCaptureWidth->value(),
                                                            m_pEditorVideoCaptureHeight->value(),
                                                            m_pEditorVideoCaptureFrameRate->value(),
                                                            m_pSliderVideoCaptureQuality->value()));
    m_pEditorVideoCaptureBitRate->blockSignals(false);
    updateRecordingFileSizeHint();
}

void UIMachineSettingsDisplay::sltHandleRecordingVideoBitRateEditorChange()
{
    /* Calculate/apply proposed quality: */
    m_pSliderVideoCaptureQuality->blockSignals(true);
    m_pSliderVideoCaptureQuality->setValue(calculateQuality(m_pEditorVideoCaptureWidth->value(),
                                                            m_pEditorVideoCaptureHeight->value(),
                                                            m_pEditorVideoCaptureFrameRate->value(),
                                                            m_pEditorVideoCaptureBitRate->value()));
    m_pSliderVideoCaptureQuality->blockSignals(false);
    updateRecordingFileSizeHint();
}

void UIMachineSettingsDisplay::sltHandleRecordingComboBoxChange()
{
    enableDisableRecordingWidgets();
}

void UIMachineSettingsDisplay::prepare()
{
    prepareWidgets();

    /* Prepare cache: */
    m_pCache = new UISettingsCacheMachineDisplay;
    AssertPtrReturnVoid(m_pCache);

    /* Tree-widget created in the .ui file. */
    {
        /* Prepare 'Screen' tab: */
        prepareTabScreen();
        /* Prepare 'Remote Display' tab: */
        prepareTabRemoteDisplay();
        /* Prepare 'Recording' tab: */
        prepareTabRecording();
        /* Prepare connections: */
        prepareConnections();
    }

    /* Apply language settings: */
    retranslateUi();
}

void UIMachineSettingsDisplay::prepareTabScreen()
{
    /* Prepare common variables: */
    const CSystemProperties sys = uiCommon().virtualBox().GetSystemProperties();

    /* Tab and it's layout created in the .ui file. */
    {
        /* Video-memory label and editor created in the .ui file. */
        AssertPtrReturnVoid(m_pVideoMemoryLabel);
        AssertPtrReturnVoid(m_pVideoMemoryEditor);
        {
            /* Configure label & editor: */
            m_pVideoMemoryLabel->setBuddy(m_pVideoMemoryEditor->focusProxy());
        }

        /* Screen-count slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderVideoScreenCount);
        {
            /* Configure slider: */
            const uint cHostScreens = gpDesktop->screenCount();
            const uint cMinGuestScreens = 1;
            const uint cMaxGuestScreens = sys.GetMaxGuestMonitors();
            const uint cMaxGuestScreensForSlider = qMin(cMaxGuestScreens, (uint)8);
            m_pSliderVideoScreenCount->setMinimum(cMinGuestScreens);
            m_pSliderVideoScreenCount->setMaximum(cMaxGuestScreensForSlider);
            m_pSliderVideoScreenCount->setPageStep(1);
            m_pSliderVideoScreenCount->setSingleStep(1);
            m_pSliderVideoScreenCount->setTickInterval(1);
            m_pSliderVideoScreenCount->setOptimalHint(cMinGuestScreens, cHostScreens);
            m_pSliderVideoScreenCount->setWarningHint(cHostScreens, cMaxGuestScreensForSlider);
        }

        /* Screen-count editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoScreenCount);
        {
            /* Configure editor: */
            const uint cMaxGuestScreens = sys.GetMaxGuestMonitors();
            m_pEditorVideoScreenCount->setMinimum(1);
            m_pEditorVideoScreenCount->setMaximum(cMaxGuestScreens);
        }

        /* Graphics controller label & editor created in the .ui file. */
        AssertPtrReturnVoid(m_pGraphicsControllerEditor);
        {
            /* Configure label & editor: */
            m_pGraphicsControllerLabel->setBuddy(m_pGraphicsControllerEditor->focusProxy());
        }
    }
}

void UIMachineSettingsDisplay::prepareTabRemoteDisplay()
{
    /* Tab and it's layout created in the .ui file. */
    {
        /* Port editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorRemoteDisplayPort);
        {
            /* Configure editor: */
            m_pEditorRemoteDisplayPort->setValidator(new QRegExpValidator(
                QRegExp("(([0-9]{1,5}(\\-[0-9]{1,5}){0,1}),)*([0-9]{1,5}(\\-[0-9]{1,5}){0,1})"), this));
        }

        /* Timeout editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorRemoteDisplayTimeout);
        {
            /* Configure editor: */
            m_pEditorRemoteDisplayTimeout->setValidator(new QIntValidator(this));
        }

        /* Auth-method combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pComboRemoteDisplayAuthMethod);
        {
            /* Configure combo-box: */
            m_pComboRemoteDisplayAuthMethod->insertItem(0, ""); /* KAuthType_Null */
            m_pComboRemoteDisplayAuthMethod->insertItem(1, ""); /* KAuthType_External */
            m_pComboRemoteDisplayAuthMethod->insertItem(2, ""); /* KAuthType_Guest */
        }
    }
}

void UIMachineSettingsDisplay::prepareTabRecording()
{
    /* Tab and it's layout created in the .ui file. */
    {
        /* Capture mode selection combo box. */
        AssertPtrReturnVoid(m_pComboBoxCaptureMode);
        {
            m_pComboBoxCaptureMode->insertItem(0, ""); /* UISettingsDefs::RecordingMode_VideoAudio */
            m_pComboBoxCaptureMode->insertItem(1, ""); /* UISettingsDefs::RecordingMode_VideoOnly */
            m_pComboBoxCaptureMode->insertItem(2, ""); /* UISettingsDefs::RecordingMode_AudioOnly */
        }

        /* File-path selector created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCapturePath);
        {
            /* Configure selector: */
            m_pEditorVideoCapturePath->setEditable(false);
            m_pEditorVideoCapturePath->setMode(UIFilePathSelector::Mode_File_Save);
        }

        /* Frame-size combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pComboVideoCaptureSize);
        {
            /* Configure combo-box: */
            m_pComboVideoCaptureSize->addItem(""); /* User Defined */
            m_pComboVideoCaptureSize->addItem("320 x 200 (16:10)",   QSize(320, 200));
            m_pComboVideoCaptureSize->addItem("640 x 480 (4:3)",     QSize(640, 480));
            m_pComboVideoCaptureSize->addItem("720 x 400 (9:5)",     QSize(720, 400));
            m_pComboVideoCaptureSize->addItem("720 x 480 (3:2)",     QSize(720, 480));
            m_pComboVideoCaptureSize->addItem("800 x 600 (4:3)",     QSize(800, 600));
            m_pComboVideoCaptureSize->addItem("1024 x 768 (4:3)",    QSize(1024, 768));
            m_pComboVideoCaptureSize->addItem("1152 x 864 (4:3)",    QSize(1152, 864));
            m_pComboVideoCaptureSize->addItem("1280 x 720 (16:9)",   QSize(1280, 720));
            m_pComboVideoCaptureSize->addItem("1280 x 800 (16:10)",  QSize(1280, 800));
            m_pComboVideoCaptureSize->addItem("1280 x 960 (4:3)",    QSize(1280, 960));
            m_pComboVideoCaptureSize->addItem("1280 x 1024 (5:4)",   QSize(1280, 1024));
            m_pComboVideoCaptureSize->addItem("1366 x 768 (16:9)",   QSize(1366, 768));
            m_pComboVideoCaptureSize->addItem("1440 x 900 (16:10)",  QSize(1440, 900));
            m_pComboVideoCaptureSize->addItem("1440 x 1080 (4:3)",   QSize(1440, 1080));
            m_pComboVideoCaptureSize->addItem("1600 x 900 (16:9)",   QSize(1600, 900));
            m_pComboVideoCaptureSize->addItem("1680 x 1050 (16:10)", QSize(1680, 1050));
            m_pComboVideoCaptureSize->addItem("1600 x 1200 (4:3)",   QSize(1600, 1200));
            m_pComboVideoCaptureSize->addItem("1920 x 1080 (16:9)",  QSize(1920, 1080));
            m_pComboVideoCaptureSize->addItem("1920 x 1200 (16:10)", QSize(1920, 1200));
            m_pComboVideoCaptureSize->addItem("1920 x 1440 (4:3)",   QSize(1920, 1440));
            m_pComboVideoCaptureSize->addItem("2880 x 1800 (16:10)", QSize(2880, 1800));
        }

        /* Frame-width editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureWidth);
        {
            /* Configure editor: */
            uiCommon().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureWidth, 5);
            m_pEditorVideoCaptureWidth->setMinimum(16);
            m_pEditorVideoCaptureWidth->setMaximum(2880);
        }

        /* Frame-height editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureHeight);
        {
            /* Configure editor: */
            uiCommon().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureHeight, 5);
            m_pEditorVideoCaptureHeight->setMinimum(16);
            m_pEditorVideoCaptureHeight->setMaximum(1800);
        }

        /* Frame-rate slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderVideoCaptureFrameRate);
        {
            /* Configure slider: */
            m_pSliderVideoCaptureFrameRate->setMinimum(1);
            m_pSliderVideoCaptureFrameRate->setMaximum(30);
            m_pSliderVideoCaptureFrameRate->setPageStep(1);
            m_pSliderVideoCaptureFrameRate->setSingleStep(1);
            m_pSliderVideoCaptureFrameRate->setTickInterval(1);
            m_pSliderVideoCaptureFrameRate->setSnappingEnabled(true);
            m_pSliderVideoCaptureFrameRate->setOptimalHint(1, 25);
            m_pSliderVideoCaptureFrameRate->setWarningHint(25, 30);
        }

        /* Frame-rate editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureFrameRate);
        {
            /* Configure editor: */
            uiCommon().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureFrameRate, 3);
            m_pEditorVideoCaptureFrameRate->setMinimum(1);
            m_pEditorVideoCaptureFrameRate->setMaximum(30);
        }

        /* Frame quality combo-box created in the .ui file. */
        AssertPtrReturnVoid(m_pContainerLayoutSliderVideoCaptureQuality);
        AssertPtrReturnVoid(m_pSliderVideoCaptureQuality);
        {
            /* Configure quality related widget: */
            m_pContainerLayoutSliderVideoCaptureQuality->setColumnStretch(1, 4);
            m_pContainerLayoutSliderVideoCaptureQuality->setColumnStretch(3, 5);
            m_pSliderVideoCaptureQuality->setMinimum(1);
            m_pSliderVideoCaptureQuality->setMaximum(10);
            m_pSliderVideoCaptureQuality->setPageStep(1);
            m_pSliderVideoCaptureQuality->setSingleStep(1);
            m_pSliderVideoCaptureQuality->setTickInterval(1);
            m_pSliderVideoCaptureQuality->setSnappingEnabled(true);
            m_pSliderVideoCaptureQuality->setOptimalHint(1, 5);
            m_pSliderVideoCaptureQuality->setWarningHint(5, 9);
            m_pSliderVideoCaptureQuality->setErrorHint(9, 10);
        }

        /* Bit-rate editor created in the .ui file. */
        AssertPtrReturnVoid(m_pEditorVideoCaptureBitRate);
        {
            /* Configure editor: */
            uiCommon().setMinimumWidthAccordingSymbolCount(m_pEditorVideoCaptureBitRate, 5);
            m_pEditorVideoCaptureBitRate->setMinimum(VIDEO_CAPTURE_BIT_RATE_MIN);
            m_pEditorVideoCaptureBitRate->setMaximum(VIDEO_CAPTURE_BIT_RATE_MAX);
        }

         /* Frame-rate slider created in the .ui file. */
        AssertPtrReturnVoid(m_pSliderAudioCaptureQuality);
        {
            /* Configure slider: */
            m_pSliderAudioCaptureQuality->setMinimum(1);
            m_pSliderAudioCaptureQuality->setMaximum(3);
            m_pSliderAudioCaptureQuality->setPageStep(1);
            m_pSliderAudioCaptureQuality->setSingleStep(1);
            m_pSliderAudioCaptureQuality->setTickInterval(1);
            m_pSliderAudioCaptureQuality->setSnappingEnabled(true);
            m_pSliderAudioCaptureQuality->setOptimalHint(1, 2);
            m_pSliderAudioCaptureQuality->setWarningHint(2, 3);
        }
    }
}

void UIMachineSettingsDisplay::prepareConnections()
{
    /* Configure 'Screen' connections: */
    connect(m_pVideoMemoryEditor, &UIVideoMemoryEditor::sigValidChanged,
            this, &UIMachineSettingsDisplay::revalidate);
    connect(m_pSliderVideoScreenCount, &QIAdvancedSlider::valueChanged,
            this, &UIMachineSettingsDisplay::sltHandleGuestScreenCountSliderChange);
    connect(m_pEditorVideoScreenCount, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &UIMachineSettingsDisplay::sltHandleGuestScreenCountEditorChange);
    connect(m_pGraphicsControllerEditor, &UIGraphicsControllerEditor::sigValueChanged,
            this, &UIMachineSettingsDisplay::sltHandleGraphicsControllerComboChange);
#ifdef VBOX_WITH_3D_ACCELERATION
    connect(m_pCheckbox3D, &QCheckBox::stateChanged,
            this, &UIMachineSettingsDisplay::sltHandle3DAccelerationCheckboxChange);
#endif

    /* Configure 'Remote Display' connections: */
    connect(m_pCheckboxRemoteDisplay, &QCheckBox::toggled, this, &UIMachineSettingsDisplay::revalidate);
    connect(m_pEditorRemoteDisplayPort, &QLineEdit::textChanged, this, &UIMachineSettingsDisplay::revalidate);
    connect(m_pEditorRemoteDisplayTimeout, &QLineEdit::textChanged, this, &UIMachineSettingsDisplay::revalidate);

    /* Configure 'Recording' connections: */
    connect(m_pCheckboxVideoCapture, &QCheckBox::toggled,
            this, &UIMachineSettingsDisplay::sltHandleRecordingCheckboxToggle);
    connect(m_pComboVideoCaptureSize, static_cast<void(QComboBox::*)(int)>(&QComboBox:: currentIndexChanged),
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoFrameSizeComboboxChange);
    connect(m_pEditorVideoCaptureWidth, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoFrameWidthEditorChange);
    connect(m_pEditorVideoCaptureHeight, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoFrameHeightEditorChange);
    connect(m_pSliderVideoCaptureFrameRate, &QIAdvancedSlider::valueChanged,
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoFrameRateSliderChange);
    connect(m_pEditorVideoCaptureFrameRate, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoFrameRateEditorChange);
    connect(m_pSliderVideoCaptureQuality, &QIAdvancedSlider::valueChanged,
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoQualitySliderChange);
    connect(m_pEditorVideoCaptureBitRate, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &UIMachineSettingsDisplay::sltHandleRecordingVideoBitRateEditorChange);

    connect(m_pComboBoxCaptureMode, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &UIMachineSettingsDisplay::sltHandleRecordingComboBoxChange);
}

void UIMachineSettingsDisplay::prepareWidgets()
{
    if (objectName().isEmpty())
        setObjectName(QStringLiteral("UIMachineSettingsDisplay"));
    resize(350, 300);
    QVBoxLayout *pLayoutMain = new QVBoxLayout(this);
    pLayoutMain->setObjectName(QStringLiteral("pLayoutMain"));
    m_pTabWidget = new QITabWidget();
    m_pTabWidget->setObjectName(QStringLiteral("m_pTabWidget"));
    m_pTabVideo = new QWidget();
    m_pTabVideo->setObjectName(QStringLiteral("m_pTabVideo"));
    QVBoxLayout *pLayoutTabVideo = new QVBoxLayout(m_pTabVideo);
    pLayoutTabVideo->setObjectName(QStringLiteral("pLayoutTabVideo"));
    QWidget *pContainerVideo = new QWidget(m_pTabVideo);
    pContainerVideo->setObjectName(QStringLiteral("pContainerVideo"));
    QGridLayout *pLayoutContainerVideo = new QGridLayout(pContainerVideo);
    pLayoutContainerVideo->setObjectName(QStringLiteral("pLayoutContainerVideo"));
    pLayoutContainerVideo->setContentsMargins(0, 0, 0, 0);
    m_pVideoMemoryLabel = new QLabel(pContainerVideo);
    m_pVideoMemoryLabel->setObjectName(QStringLiteral("m_pVideoMemoryLabel"));
    m_pVideoMemoryLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerVideo->addWidget(m_pVideoMemoryLabel, 0, 0, 1, 1);

    m_pVideoMemoryEditor = new UIVideoMemoryEditor(pContainerVideo);
    m_pVideoMemoryEditor->setObjectName(QStringLiteral("m_pVideoMemoryEditor"));
    pLayoutContainerVideo->addWidget(m_pVideoMemoryEditor, 0, 1, 2, 2);

    m_pLabelVideoScreenCount = new QLabel(pContainerVideo);
    m_pLabelVideoScreenCount->setObjectName(QStringLiteral("m_pLabelVideoScreenCount"));
    m_pLabelVideoScreenCount->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerVideo->addWidget(m_pLabelVideoScreenCount, 2, 0, 1, 1);

    QGridLayout *pLayoutVideoScreenCount = new QGridLayout();
    pLayoutVideoScreenCount->setSpacing(0);
    pLayoutVideoScreenCount->setObjectName(QStringLiteral("pLayoutVideoScreenCount"));
    m_pSliderVideoScreenCount = new QIAdvancedSlider(pContainerVideo);
    m_pSliderVideoScreenCount->setObjectName(QStringLiteral("m_pSliderVideoScreenCount"));
    m_pSliderVideoScreenCount->setOrientation(Qt::Horizontal);
    pLayoutVideoScreenCount->addWidget(m_pSliderVideoScreenCount, 0, 0, 1, 3);

    m_pLabelVideoScreenCountMin = new QLabel(pContainerVideo);
    m_pLabelVideoScreenCountMin->setObjectName(QStringLiteral("m_pLabelVideoScreenCountMin"));
    pLayoutVideoScreenCount->addWidget(m_pLabelVideoScreenCountMin, 1, 0, 1, 1);

    QSpacerItem *pSpacerVideoScreenCount = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    pLayoutVideoScreenCount->addItem(pSpacerVideoScreenCount, 1, 1, 1, 1);

    m_pLabelVideoScreenCountMax = new QLabel(pContainerVideo);
    m_pLabelVideoScreenCountMax->setObjectName(QStringLiteral("m_pLabelVideoScreenCountMax"));

    pLayoutVideoScreenCount->addWidget(m_pLabelVideoScreenCountMax, 1, 2, 1, 1);
    pLayoutContainerVideo->addLayout(pLayoutVideoScreenCount, 2, 1, 2, 1);

    m_pEditorVideoScreenCount = new QSpinBox(pContainerVideo);
    m_pEditorVideoScreenCount->setObjectName(QStringLiteral("m_pEditorVideoScreenCount"));
    pLayoutContainerVideo->addWidget(m_pEditorVideoScreenCount, 2, 2, 1, 1);

    m_pLabelGuestScreenScaleFactorEditor = new QLabel(pContainerVideo);
    m_pLabelGuestScreenScaleFactorEditor->setObjectName(QStringLiteral("m_pLabelGuestScreenScaleFactorEditor"));
    m_pLabelGuestScreenScaleFactorEditor->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerVideo->addWidget(m_pLabelGuestScreenScaleFactorEditor, 4, 0, 1, 1);

    QGridLayout *pLayoutGuestScreenScaleFactorEditor = new QGridLayout();
    pLayoutGuestScreenScaleFactorEditor->setSpacing(0);
    pLayoutGuestScreenScaleFactorEditor->setObjectName(QStringLiteral("pLayoutGuestScreenScaleFactorEditor"));
    m_pScaleFactorEditor = new UIScaleFactorEditor(pContainerVideo);
    m_pScaleFactorEditor->setObjectName(QStringLiteral("m_pScaleFactorEditor"));
    pLayoutGuestScreenScaleFactorEditor->addWidget(m_pScaleFactorEditor, 0, 0, 2, 3);
    pLayoutContainerVideo->addLayout(pLayoutGuestScreenScaleFactorEditor, 4, 1, 2, 2);

    m_pGraphicsControllerLabel = new QLabel(pContainerVideo);
    m_pGraphicsControllerLabel->setObjectName(QStringLiteral("m_pGraphicsControllerLabel"));
    m_pGraphicsControllerLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerVideo->addWidget(m_pGraphicsControllerLabel, 6, 0, 1, 1);

    m_pGraphicsControllerEditor = new UIGraphicsControllerEditor(pContainerVideo);
    m_pGraphicsControllerEditor->setObjectName(QStringLiteral("m_pGraphicsControllerEditor"));
    pLayoutContainerVideo->addWidget(m_pGraphicsControllerEditor, 6, 1, 1, 2);

    m_pLabelVideoOptions = new QLabel(pContainerVideo);
    m_pLabelVideoOptions->setObjectName(QStringLiteral("m_pLabelVideoOptions"));
    m_pLabelVideoOptions->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerVideo->addWidget(m_pLabelVideoOptions, 7, 0, 1, 1);

    m_pLayout3D = new QStackedLayout();
    m_pLayout3D->setObjectName(QStringLiteral("m_pLayout3D"));
    m_pCheckbox3D = new QCheckBox(pContainerVideo);
    m_pCheckbox3D->setObjectName(QStringLiteral("m_pCheckbox3D"));
    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_pCheckbox3D->sizePolicy().hasHeightForWidth());
    m_pCheckbox3D->setSizePolicy(sizePolicy);
    m_pLayout3D->addWidget(m_pCheckbox3D);

    QWidget *pPlaceholder3D = new QWidget(pContainerVideo);
    pPlaceholder3D->setObjectName(QStringLiteral("pPlaceholder3D"));

    m_pLayout3D->addWidget(pPlaceholder3D);
    pLayoutContainerVideo->addLayout(m_pLayout3D, 7, 1, 1, 1);
    pLayoutTabVideo->addWidget(pContainerVideo);

    QSpacerItem *pStretchVideo = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    pLayoutTabVideo->addItem(pStretchVideo);

    m_pTabWidget->addTab(m_pTabVideo, QString());
    m_pTabRemoteDisplay = new QWidget();
    m_pTabRemoteDisplay->setObjectName(QStringLiteral("m_pTabRemoteDisplay"));
    QVBoxLayout *pLayoutTabRemoteDisplay = new QVBoxLayout(m_pTabRemoteDisplay);
    pLayoutTabRemoteDisplay->setObjectName(QStringLiteral("pLayoutTabRemoteDisplay"));
    m_pContainerRemoteDisplay = new QWidget(m_pTabRemoteDisplay);
    m_pContainerRemoteDisplay->setObjectName(QStringLiteral("m_pContainerRemoteDisplay"));
    QGridLayout *pLayoutContainerRemoteDisplay = new QGridLayout(m_pContainerRemoteDisplay);
    pLayoutContainerRemoteDisplay->setObjectName(QStringLiteral("pLayoutContainerRemoteDisplay"));
    pLayoutContainerRemoteDisplay->setContentsMargins(0, 0, 0, 0);
    m_pCheckboxRemoteDisplay = new QCheckBox(m_pContainerRemoteDisplay);
    m_pCheckboxRemoteDisplay->setObjectName(QStringLiteral("m_pCheckboxRemoteDisplay"));
    m_pCheckboxRemoteDisplay->setChecked(false);
    pLayoutContainerRemoteDisplay->addWidget(m_pCheckboxRemoteDisplay, 0, 0, 1, 2);

    QSpacerItem *pSpacerContainerRemoteDisplay = new QSpacerItem(20, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    pLayoutContainerRemoteDisplay->addItem(pSpacerContainerRemoteDisplay, 1, 0, 1, 1);

    m_pContainerRemoteDisplayOptions = new QWidget(m_pContainerRemoteDisplay);
    m_pContainerRemoteDisplayOptions->setObjectName(QStringLiteral("m_pContainerRemoteDisplayOptions"));
    QGridLayout *pLayoutContainerRemoteDisplayServer = new QGridLayout(m_pContainerRemoteDisplayOptions);
    pLayoutContainerRemoteDisplayServer->setObjectName(QStringLiteral("pLayoutContainerRemoteDisplayServer"));
    pLayoutContainerRemoteDisplayServer->setContentsMargins(0, 0, 0, 0);
    m_pLabelRemoteDisplayPort = new QLabel(m_pContainerRemoteDisplayOptions);
    m_pLabelRemoteDisplayPort->setObjectName(QStringLiteral("m_pLabelRemoteDisplayPort"));
    m_pLabelRemoteDisplayPort->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerRemoteDisplayServer->addWidget(m_pLabelRemoteDisplayPort, 0, 0, 1, 1);

    m_pEditorRemoteDisplayPort = new QLineEdit(m_pContainerRemoteDisplayOptions);
    m_pEditorRemoteDisplayPort->setObjectName(QStringLiteral("m_pEditorRemoteDisplayPort"));
    pLayoutContainerRemoteDisplayServer->addWidget(m_pEditorRemoteDisplayPort, 0, 1, 1, 1);

    m_pLabelRemoteDisplayAuthMethod = new QLabel(m_pContainerRemoteDisplayOptions);
    m_pLabelRemoteDisplayAuthMethod->setObjectName(QStringLiteral("m_pLabelRemoteDisplayAuthMethod"));
    m_pLabelRemoteDisplayAuthMethod->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerRemoteDisplayServer->addWidget(m_pLabelRemoteDisplayAuthMethod, 1, 0, 1, 1);

    m_pComboRemoteDisplayAuthMethod = new QComboBox(m_pContainerRemoteDisplayOptions);
    m_pComboRemoteDisplayAuthMethod->setObjectName(QStringLiteral("m_pComboRemoteDisplayAuthMethod"));
    pLayoutContainerRemoteDisplayServer->addWidget(m_pComboRemoteDisplayAuthMethod, 1, 1, 1, 1);

    m_pLabelRemoteDisplayTimeout = new QLabel(m_pContainerRemoteDisplayOptions);
    m_pLabelRemoteDisplayTimeout->setObjectName(QStringLiteral("m_pLabelRemoteDisplayTimeout"));
    m_pLabelRemoteDisplayTimeout->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerRemoteDisplayServer->addWidget(m_pLabelRemoteDisplayTimeout, 2, 0, 1, 1);

    m_pEditorRemoteDisplayTimeout = new QLineEdit(m_pContainerRemoteDisplayOptions);
    m_pEditorRemoteDisplayTimeout->setObjectName(QStringLiteral("m_pEditorRemoteDisplayTimeout"));
    pLayoutContainerRemoteDisplayServer->addWidget(m_pEditorRemoteDisplayTimeout, 2, 1, 1, 1);

    m_pLabelRemoteDisplayOptions = new QLabel(m_pContainerRemoteDisplayOptions);
    m_pLabelRemoteDisplayOptions->setObjectName(QStringLiteral("m_pLabelRemoteDisplayOptions"));
    m_pLabelRemoteDisplayOptions->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pLayoutContainerRemoteDisplayServer->addWidget(m_pLabelRemoteDisplayOptions, 3, 0, 1, 1);

    m_pCheckboxMultipleConn = new QCheckBox(m_pContainerRemoteDisplayOptions);
    m_pCheckboxMultipleConn->setObjectName(QStringLiteral("m_pCheckboxMultipleConn"));
    sizePolicy.setHeightForWidth(m_pCheckboxMultipleConn->sizePolicy().hasHeightForWidth());
    m_pCheckboxMultipleConn->setSizePolicy(sizePolicy);

    pLayoutContainerRemoteDisplayServer->addWidget(m_pCheckboxMultipleConn, 3, 1, 1, 1);
    pLayoutContainerRemoteDisplay->addWidget(m_pContainerRemoteDisplayOptions, 1, 1, 1, 1);
    pLayoutTabRemoteDisplay->addWidget(m_pContainerRemoteDisplay);

    QSpacerItem *pStretchRemoteDisplay = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    pLayoutTabRemoteDisplay->addItem(pStretchRemoteDisplay);

    m_pTabWidget->addTab(m_pTabRemoteDisplay, QString());
    m_pTabVideoCapture = new QWidget();
    m_pTabVideoCapture->setObjectName(QStringLiteral("m_pTabVideoCapture"));
    QVBoxLayout *pLayoutTabVideoCapture = new QVBoxLayout(m_pTabVideoCapture);
    pLayoutTabVideoCapture->setObjectName(QStringLiteral("pLayoutTabVideoCapture"));
    m_pContainerVideoCapture = new QWidget(m_pTabVideoCapture);
    m_pContainerVideoCapture->setObjectName(QStringLiteral("m_pContainerVideoCapture"));
    QGridLayout *pLayoutContainerVideoCapture = new QGridLayout(m_pContainerVideoCapture);
    pLayoutContainerVideoCapture->setObjectName(QStringLiteral("pLayoutContainerVideoCapture"));
    pLayoutContainerVideoCapture->setContentsMargins(0, 0, 0, 0);
    m_pCheckboxVideoCapture = new QCheckBox(m_pContainerVideoCapture);
    m_pCheckboxVideoCapture->setObjectName(QStringLiteral("m_pCheckboxVideoCapture"));
    m_pCheckboxVideoCapture->setChecked(false);
    pLayoutContainerVideoCapture->addWidget(m_pCheckboxVideoCapture, 0, 0, 1, 2);

    QSpacerItem *pLeftSpacer = new QSpacerItem(20, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    pLayoutContainerVideoCapture->addItem(pLeftSpacer, 1, 0, 1, 1);

    QWidget *pContainerVideoCaptureOptions = new QWidget(m_pContainerVideoCapture);
    pContainerVideoCaptureOptions->setObjectName(QStringLiteral("pContainerVideoCaptureOptions"));
    QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(1);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(pContainerVideoCaptureOptions->sizePolicy().hasHeightForWidth());
    pContainerVideoCaptureOptions->setSizePolicy(sizePolicy1);
    QGridLayout *pContainerLayoutVideoCapture = new QGridLayout(pContainerVideoCaptureOptions);
    pContainerLayoutVideoCapture->setObjectName(QStringLiteral("pContainerLayoutVideoCapture"));
    pContainerLayoutVideoCapture->setContentsMargins(0, 0, 0, 0);
    m_pLabelCaptureMode = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelCaptureMode->setObjectName(QStringLiteral("m_pLabelCaptureMode"));
    m_pLabelCaptureMode->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pContainerLayoutVideoCapture->addWidget(m_pLabelCaptureMode, 0, 0, 1, 1);

    m_pComboBoxCaptureMode = new QComboBox(pContainerVideoCaptureOptions);
    m_pComboBoxCaptureMode->setObjectName(QStringLiteral("m_pComboBoxCaptureMode"));
    pContainerLayoutVideoCapture->addWidget(m_pComboBoxCaptureMode, 0, 1, 1, 3);

    m_pLabelVideoCapturePath = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelVideoCapturePath->setObjectName(QStringLiteral("m_pLabelVideoCapturePath"));
    m_pLabelVideoCapturePath->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pContainerLayoutVideoCapture->addWidget(m_pLabelVideoCapturePath, 1, 0, 1, 1);

    m_pEditorVideoCapturePath = new UIFilePathSelector(pContainerVideoCaptureOptions);
    m_pEditorVideoCapturePath->setObjectName(QStringLiteral("m_pEditorVideoCapturePath"));
    pContainerLayoutVideoCapture->addWidget(m_pEditorVideoCapturePath, 1, 1, 1, 3);

    m_pLabelVideoCaptureSize = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelVideoCaptureSize->setObjectName(QStringLiteral("m_pLabelVideoCaptureSize"));
    m_pLabelVideoCaptureSize->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pContainerLayoutVideoCapture->addWidget(m_pLabelVideoCaptureSize, 2, 0, 1, 1);

    m_pComboVideoCaptureSize = new QComboBox(pContainerVideoCaptureOptions);
    m_pComboVideoCaptureSize->setObjectName(QStringLiteral("m_pComboVideoCaptureSize"));
    QSizePolicy sizePolicy2(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(1);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(m_pComboVideoCaptureSize->sizePolicy().hasHeightForWidth());
    m_pComboVideoCaptureSize->setSizePolicy(sizePolicy2);
    pContainerLayoutVideoCapture->addWidget(m_pComboVideoCaptureSize, 2, 1, 1, 1);

    m_pEditorVideoCaptureWidth = new QSpinBox(pContainerVideoCaptureOptions);
    m_pEditorVideoCaptureWidth->setObjectName(QStringLiteral("m_pEditorVideoCaptureWidth"));
    pContainerLayoutVideoCapture->addWidget(m_pEditorVideoCaptureWidth, 2, 2, 1, 1);

    m_pEditorVideoCaptureHeight = new QSpinBox(pContainerVideoCaptureOptions);
    m_pEditorVideoCaptureHeight->setObjectName(QStringLiteral("m_pEditorVideoCaptureHeight"));
    pContainerLayoutVideoCapture->addWidget(m_pEditorVideoCaptureHeight, 2, 3, 1, 1);

    m_pLabelVideoCaptureFrameRate = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelVideoCaptureFrameRate->setObjectName(QStringLiteral("m_pLabelVideoCaptureFrameRate"));
    m_pLabelVideoCaptureFrameRate->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pContainerLayoutVideoCapture->addWidget(m_pLabelVideoCaptureFrameRate, 3, 0, 1, 1);

    m_pContainerSliderVideoCaptureFrameRate = new QWidget(pContainerVideoCaptureOptions);
    m_pContainerSliderVideoCaptureFrameRate->setObjectName(QStringLiteral("m_pContainerSliderVideoCaptureFrameRate"));
    QGridLayout *pContainerLayoutSliderVideoCaptureFrameRate = new QGridLayout(m_pContainerSliderVideoCaptureFrameRate);
    pContainerLayoutSliderVideoCaptureFrameRate->setSpacing(0);
    pContainerLayoutSliderVideoCaptureFrameRate->setObjectName(QStringLiteral("pContainerLayoutSliderVideoCaptureFrameRate"));
    pContainerLayoutSliderVideoCaptureFrameRate->setContentsMargins(0, 0, 0, 0);
    m_pSliderVideoCaptureFrameRate = new QIAdvancedSlider(m_pContainerSliderVideoCaptureFrameRate);
    m_pSliderVideoCaptureFrameRate->setObjectName(QStringLiteral("m_pSliderVideoCaptureFrameRate"));
    m_pSliderVideoCaptureFrameRate->setOrientation(Qt::Horizontal);
    pContainerLayoutSliderVideoCaptureFrameRate->addWidget(m_pSliderVideoCaptureFrameRate, 0, 0, 1, 3);

    m_pLabelVideoCaptureFrameRateMin = new QLabel(m_pContainerSliderVideoCaptureFrameRate);
    m_pLabelVideoCaptureFrameRateMin->setObjectName(QStringLiteral("m_pLabelVideoCaptureFrameRateMin"));
    pContainerLayoutSliderVideoCaptureFrameRate->addWidget(m_pLabelVideoCaptureFrameRateMin, 1, 0, 1, 1);

    QSpacerItem *pStretchVideoCaptureFrameRate = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    pContainerLayoutSliderVideoCaptureFrameRate->addItem(pStretchVideoCaptureFrameRate, 1, 1, 1, 1);

    m_pLabelVideoCaptureFrameRateMax = new QLabel(m_pContainerSliderVideoCaptureFrameRate);
    m_pLabelVideoCaptureFrameRateMax->setObjectName(QStringLiteral("m_pLabelVideoCaptureFrameRateMax"));
    pContainerLayoutSliderVideoCaptureFrameRate->addWidget(m_pLabelVideoCaptureFrameRateMax, 1, 2, 1, 1);
    pContainerLayoutVideoCapture->addWidget(m_pContainerSliderVideoCaptureFrameRate, 3, 1, 2, 1);

    m_pEditorVideoCaptureFrameRate = new QSpinBox(pContainerVideoCaptureOptions);
    m_pEditorVideoCaptureFrameRate->setObjectName(QStringLiteral("m_pEditorVideoCaptureFrameRate"));
    pContainerLayoutVideoCapture->addWidget(m_pEditorVideoCaptureFrameRate, 3, 2, 1, 2);

    m_pLabelVideoCaptureRate = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelVideoCaptureRate->setObjectName(QStringLiteral("m_pLabelVideoCaptureRate"));
    m_pLabelVideoCaptureRate->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pContainerLayoutVideoCapture->addWidget(m_pLabelVideoCaptureRate, 5, 0, 1, 1);

    m_pContainerSliderVideoCaptureQuality = new QWidget(pContainerVideoCaptureOptions);
    m_pContainerSliderVideoCaptureQuality->setObjectName(QStringLiteral("m_pContainerSliderVideoCaptureQuality"));
    m_pContainerLayoutSliderVideoCaptureQuality = new QGridLayout(m_pContainerSliderVideoCaptureQuality);
    m_pContainerLayoutSliderVideoCaptureQuality->setSpacing(0);
    m_pContainerLayoutSliderVideoCaptureQuality->setObjectName(QStringLiteral("m_pContainerLayoutSliderVideoCaptureQuality"));
    m_pContainerLayoutSliderVideoCaptureQuality->setContentsMargins(0, 0, 0, 0);
    m_pSliderVideoCaptureQuality = new QIAdvancedSlider(m_pContainerSliderVideoCaptureQuality);
    m_pSliderVideoCaptureQuality->setObjectName(QStringLiteral("m_pSliderVideoCaptureQuality"));
    m_pSliderVideoCaptureQuality->setOrientation(Qt::Horizontal);
    m_pContainerLayoutSliderVideoCaptureQuality->addWidget(m_pSliderVideoCaptureQuality, 0, 0, 1, 5);

    m_pLabelVideoCaptureQualityMin = new QLabel(m_pContainerSliderVideoCaptureQuality);
    m_pLabelVideoCaptureQualityMin->setObjectName(QStringLiteral("m_pLabelVideoCaptureQualityMin"));
    m_pContainerLayoutSliderVideoCaptureQuality->addWidget(m_pLabelVideoCaptureQualityMin, 1, 0, 1, 1);

    QSpacerItem *pStretchVideoCaptureQualityLeft = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_pContainerLayoutSliderVideoCaptureQuality->addItem(pStretchVideoCaptureQualityLeft, 1, 1, 1, 1);

    m_pLabelVideoCaptureQualityMed = new QLabel(m_pContainerSliderVideoCaptureQuality);
    m_pLabelVideoCaptureQualityMed->setObjectName(QStringLiteral("m_pLabelVideoCaptureQualityMed"));
    m_pContainerLayoutSliderVideoCaptureQuality->addWidget(m_pLabelVideoCaptureQualityMed, 1, 2, 1, 1);

    QSpacerItem *pStretchVideoCaptureQualityRight = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_pContainerLayoutSliderVideoCaptureQuality->addItem(pStretchVideoCaptureQualityRight, 1, 3, 1, 1);

    m_pLabelVideoCaptureQualityMax = new QLabel(m_pContainerSliderVideoCaptureQuality);
    m_pLabelVideoCaptureQualityMax->setObjectName(QStringLiteral("m_pLabelVideoCaptureQualityMax"));

    m_pContainerLayoutSliderVideoCaptureQuality->addWidget(m_pLabelVideoCaptureQualityMax, 1, 4, 1, 1);
    pContainerLayoutVideoCapture->addWidget(m_pContainerSliderVideoCaptureQuality, 5, 1, 2, 1);

    m_pEditorVideoCaptureBitRate = new QSpinBox(pContainerVideoCaptureOptions);
    m_pEditorVideoCaptureBitRate->setObjectName(QStringLiteral("m_pEditorVideoCaptureBitRate"));
    pContainerLayoutVideoCapture->addWidget(m_pEditorVideoCaptureBitRate, 5, 2, 1, 2);

    m_pAudioCaptureQualityLabel = new QLabel(pContainerVideoCaptureOptions);
    m_pAudioCaptureQualityLabel->setObjectName(QStringLiteral("m_pAudioCaptureQualityLabel"));
    m_pAudioCaptureQualityLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    pContainerLayoutVideoCapture->addWidget(m_pAudioCaptureQualityLabel, 7, 0, 1, 1);

    m_pContainerSliderAudioCaptureQuality = new QWidget(pContainerVideoCaptureOptions);
    m_pContainerSliderAudioCaptureQuality->setObjectName(QStringLiteral("m_pContainerSliderAudioCaptureQuality"));
    QGridLayout *pContainerLayoutSliderAudioCaptureQuality = new QGridLayout(m_pContainerSliderAudioCaptureQuality);
    pContainerLayoutSliderAudioCaptureQuality->setSpacing(0);
    pContainerLayoutSliderAudioCaptureQuality->setObjectName(QStringLiteral("pContainerLayoutSliderAudioCaptureQuality"));
    pContainerLayoutSliderAudioCaptureQuality->setContentsMargins(0, 0, 0, 0);
    m_pSliderAudioCaptureQuality = new QIAdvancedSlider(m_pContainerSliderAudioCaptureQuality);
    m_pSliderAudioCaptureQuality->setObjectName(QStringLiteral("m_pSliderAudioCaptureQuality"));
    m_pSliderAudioCaptureQuality->setOrientation(Qt::Horizontal);
    pContainerLayoutSliderAudioCaptureQuality->addWidget(m_pSliderAudioCaptureQuality, 0, 0, 1, 5);

    m_pLabelAudioCaptureQualityMin = new QLabel(m_pContainerSliderAudioCaptureQuality);
    m_pLabelAudioCaptureQualityMin->setObjectName(QStringLiteral("m_pLabelAudioCaptureQualityMin"));

    pContainerLayoutSliderAudioCaptureQuality->addWidget(m_pLabelAudioCaptureQualityMin, 1, 0, 1, 1);

    QSpacerItem *pStretchAudioCaptureQualityLeft = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    pContainerLayoutSliderAudioCaptureQuality->addItem(pStretchAudioCaptureQualityLeft, 1, 1, 1, 1);

    m_pLabelAudioCaptureQualityMed = new QLabel(m_pContainerSliderAudioCaptureQuality);
    m_pLabelAudioCaptureQualityMed->setObjectName(QStringLiteral("m_pLabelAudioCaptureQualityMed"));
    pContainerLayoutSliderAudioCaptureQuality->addWidget(m_pLabelAudioCaptureQualityMed, 1, 2, 1, 1);

    QSpacerItem *pStretchAudioCaptureQualityRight = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    pContainerLayoutSliderAudioCaptureQuality->addItem(pStretchAudioCaptureQualityRight, 1, 3, 1, 1);

    m_pLabelAudioCaptureQualityMax = new QLabel(m_pContainerSliderAudioCaptureQuality);
    m_pLabelAudioCaptureQualityMax->setObjectName(QStringLiteral("m_pLabelAudioCaptureQualityMax"));

    pContainerLayoutSliderAudioCaptureQuality->addWidget(m_pLabelAudioCaptureQualityMax, 1, 4, 1, 1);
    pContainerLayoutVideoCapture->addWidget(m_pContainerSliderAudioCaptureQuality, 7, 1, 2, 1);

    m_pLabelVideoCaptureSizeHint = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelVideoCaptureSizeHint->setObjectName(QStringLiteral("m_pLabelVideoCaptureSizeHint"));
    pContainerLayoutVideoCapture->addWidget(m_pLabelVideoCaptureSizeHint, 9, 1, 1, 1);

    m_pLabelVideoCaptureScreens = new QLabel(pContainerVideoCaptureOptions);
    m_pLabelVideoCaptureScreens->setObjectName(QStringLiteral("m_pLabelVideoCaptureScreens"));
    m_pLabelVideoCaptureScreens->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignTop);
    pContainerLayoutVideoCapture->addWidget(m_pLabelVideoCaptureScreens, 10, 0, 1, 1);

    m_pScrollerVideoCaptureScreens = new UIFilmContainer(pContainerVideoCaptureOptions);
    m_pScrollerVideoCaptureScreens->setObjectName(QStringLiteral("m_pScrollerVideoCaptureScreens"));
    pContainerLayoutVideoCapture->addWidget(m_pScrollerVideoCaptureScreens, 10, 1, 1, 3);

    pLayoutContainerVideoCapture->addWidget(pContainerVideoCaptureOptions, 1, 1, 1, 1);
    pLayoutTabVideoCapture->addWidget(m_pContainerVideoCapture);

    QSpacerItem *pStretchVideoCapture = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    pLayoutTabVideoCapture->addItem(pStretchVideoCapture);
    m_pTabWidget->addTab(m_pTabVideoCapture, QString());
    pLayoutMain->addWidget(m_pTabWidget);

    m_pLabelVideoScreenCount->setBuddy(m_pEditorVideoScreenCount);
    m_pLabelRemoteDisplayPort->setBuddy(m_pEditorRemoteDisplayPort);
    m_pLabelRemoteDisplayAuthMethod->setBuddy(m_pComboRemoteDisplayAuthMethod);
    m_pLabelRemoteDisplayTimeout->setBuddy(m_pEditorRemoteDisplayTimeout);
    m_pLabelCaptureMode->setBuddy(m_pEditorVideoCapturePath);
    m_pLabelVideoCapturePath->setBuddy(m_pEditorVideoCapturePath);
    m_pLabelVideoCaptureSize->setBuddy(m_pComboVideoCaptureSize);
    m_pLabelVideoCaptureFrameRate->setBuddy(m_pSliderVideoCaptureFrameRate);
    m_pLabelVideoCaptureRate->setBuddy(m_pSliderVideoCaptureQuality);
    m_pAudioCaptureQualityLabel->setBuddy(m_pSliderAudioCaptureQuality);
    m_pLabelVideoCaptureScreens->setBuddy(m_pScrollerVideoCaptureScreens);

    QObject::connect(m_pCheckboxRemoteDisplay, &QCheckBox::toggled, m_pContainerRemoteDisplayOptions, &QWidget::setEnabled);
}

void UIMachineSettingsDisplay::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIMachineSettingsDisplay::shouldWeWarnAboutLowVRAM()
{
    bool fResult = true;

    QStringList excludingOSList = QStringList()
        << "Other" << "DOS" << "Netware" << "L4" << "QNX" << "JRockitVE";
    if (m_comGuestOSType.isNull() || excludingOSList.contains(m_comGuestOSType.GetId()))
        fResult = false;

    return fResult;
}

void UIMachineSettingsDisplay::lookForCorrespondingFrameSizePreset()
{
    /* Look for video-capture size preset: */
    lookForCorrespondingPreset(m_pComboVideoCaptureSize,
                               QSize(m_pEditorVideoCaptureWidth->value(),
                                     m_pEditorVideoCaptureHeight->value()));
}

void UIMachineSettingsDisplay::updateGuestScreenCount()
{
    /* Update copy of the cached item to get the desired result: */
    QVector<BOOL> screens = m_pCache->base().m_vecRecordingScreens;
    screens.resize(m_pEditorVideoScreenCount->value());
    m_pScrollerVideoCaptureScreens->setValue(screens);
    m_pScaleFactorEditor->setMonitorCount(m_pEditorVideoScreenCount->value());
}

void UIMachineSettingsDisplay::updateRecordingFileSizeHint()
{
    m_pLabelVideoCaptureSizeHint->setText(tr("<i>About %1MB per 5 minute video</i>").arg(m_pEditorVideoCaptureBitRate->value() * 300 / 8 / 1024));
}

/* static */
void UIMachineSettingsDisplay::lookForCorrespondingPreset(QComboBox *pComboBox, const QVariant &data)
{
    /* Use passed iterator to look for corresponding preset of passed combo-box: */
    const int iLookupResult = pComboBox->findData(data);
    if (iLookupResult != -1 && pComboBox->currentIndex() != iLookupResult)
        pComboBox->setCurrentIndex(iLookupResult);
    else if (iLookupResult == -1 && pComboBox->currentIndex() != 0)
        pComboBox->setCurrentIndex(0);
}

/* static */
int UIMachineSettingsDisplay::calculateBitRate(int iFrameWidth, int iFrameHeight, int iFrameRate, int iQuality)
{
    /* Linear quality<=>bit-rate scale-factor: */
    const double dResult = (double)iQuality
                         * (double)iFrameWidth * (double)iFrameHeight * (double)iFrameRate
                         / (double)10 /* translate quality to [%] */
                         / (double)1024 /* translate bit-rate to [kbps] */
                         / (double)18.75 /* linear scale factor */;
    return (int)dResult;
}

/* static */
int UIMachineSettingsDisplay::calculateQuality(int iFrameWidth, int iFrameHeight, int iFrameRate, int iBitRate)
{
    /* Linear bit-rate<=>quality scale-factor: */
    const double dResult = (double)iBitRate
                         / (double)iFrameWidth / (double)iFrameHeight / (double)iFrameRate
                         * (double)10 /* translate quality to [%] */
                         * (double)1024 /* translate bit-rate to [kbps] */
                         * (double)18.75 /* linear scale factor */;
    return (int)dResult;
}

bool UIMachineSettingsDisplay::saveDisplayData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save display settings from the cache: */
    if (fSuccess && isMachineInValidMode() && m_pCache->wasChanged())
    {
        /* Save 'Screen' data from the cache: */
        if (fSuccess)
            fSuccess = saveScreenData();
        /* Save 'Remote Display' data from the cache: */
        if (fSuccess)
            fSuccess = saveRemoteDisplayData();
        /* Save 'Video Capture' data from the cache: */
        if (fSuccess)
            fSuccess = saveRecordingData();
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveScreenData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Screen' data from the cache: */
    if (fSuccess)
    {
        /* Get old display data from the cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Get graphics adapter for further activities: */
        CGraphicsAdapter comGraphics = m_machine.GetGraphicsAdapter();
        fSuccess = m_machine.isOk() && comGraphics.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save video RAM size: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_iCurrentVRAM != oldDisplayData.m_iCurrentVRAM)
            {
                comGraphics.SetVRAMSize(newDisplayData.m_iCurrentVRAM);
                fSuccess = comGraphics.isOk();
            }
            /* Save guest screen count: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_cGuestScreenCount != oldDisplayData.m_cGuestScreenCount)
            {
                comGraphics.SetMonitorCount(newDisplayData.m_cGuestScreenCount);
                fSuccess = comGraphics.isOk();
            }
            /* Save the Graphics Controller Type: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_graphicsControllerType != oldDisplayData.m_graphicsControllerType)
            {
                comGraphics.SetGraphicsControllerType(newDisplayData.m_graphicsControllerType);
                fSuccess = comGraphics.isOk();
            }
#ifdef VBOX_WITH_3D_ACCELERATION
            /* Save whether 3D acceleration is enabled: */
            if (fSuccess && isMachineOffline() && newDisplayData.m_f3dAccelerationEnabled != oldDisplayData.m_f3dAccelerationEnabled)
            {
                comGraphics.SetAccelerate3DEnabled(newDisplayData.m_f3dAccelerationEnabled);
                fSuccess = comGraphics.isOk();
            }
#endif

            /* Get machine ID for further activities: */
            QUuid uMachineId;
            if (fSuccess)
            {
                uMachineId = m_machine.GetId();
                fSuccess = m_machine.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));

            /* Save guest-screen scale-factor: */
            if (fSuccess && newDisplayData.m_scaleFactors != oldDisplayData.m_scaleFactors)
                /* fSuccess = */ gEDataManager->setScaleFactors(newDisplayData.m_scaleFactors, uMachineId);
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveRemoteDisplayData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Remote Display' data from the cache: */
    if (fSuccess)
    {
        /* Get old display data from the cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        /* Get remote display server for further activities: */
        CVRDEServer comServer = m_machine.GetVRDEServer();
        fSuccess = m_machine.isOk() && comServer.isNotNull();

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
        else
        {
            /* Save whether remote display server is enabled: */
            if (fSuccess && newDisplayData.m_fRemoteDisplayServerEnabled != oldDisplayData.m_fRemoteDisplayServerEnabled)
            {
                comServer.SetEnabled(newDisplayData.m_fRemoteDisplayServerEnabled);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server port: */
            if (fSuccess && newDisplayData.m_strRemoteDisplayPort != oldDisplayData.m_strRemoteDisplayPort)
            {
                comServer.SetVRDEProperty("TCP/Ports", newDisplayData.m_strRemoteDisplayPort);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server auth type: */
            if (fSuccess && newDisplayData.m_remoteDisplayAuthType != oldDisplayData.m_remoteDisplayAuthType)
            {
                comServer.SetAuthType(newDisplayData.m_remoteDisplayAuthType);
                fSuccess = comServer.isOk();
            }
            /* Save remote display server timeout: */
            if (fSuccess && newDisplayData.m_uRemoteDisplayTimeout != oldDisplayData.m_uRemoteDisplayTimeout)
            {
                comServer.SetAuthTimeout(newDisplayData.m_uRemoteDisplayTimeout);
                fSuccess = comServer.isOk();
            }
            /* Save whether remote display server allows multiple connections: */
            if (   fSuccess
                && (isMachineOffline() || isMachineSaved())
                && (newDisplayData.m_fRemoteDisplayMultiConnAllowed != oldDisplayData.m_fRemoteDisplayMultiConnAllowed))
            {
                comServer.SetAllowMultiConnection(newDisplayData.m_fRemoteDisplayMultiConnAllowed);
                fSuccess = comServer.isOk();
            }

            /* Show error message if necessary: */
            if (!fSuccess)
                notifyOperationProgressError(UIErrorString::formatErrorInfo(comServer));
        }
    }
    /* Return result: */
    return fSuccess;
}

bool UIMachineSettingsDisplay::saveRecordingData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save 'Recording' data from the cache: */
    if (fSuccess)
    {
        /* Get old display data from the cache: */
        const UIDataSettingsMachineDisplay &oldDisplayData = m_pCache->base();
        /* Get new display data from the cache: */
        const UIDataSettingsMachineDisplay &newDisplayData = m_pCache->data();

        CRecordingSettings recordingSettings = m_machine.GetRecordingSettings();
        Assert(recordingSettings.isNotNull());

        /** @todo r=andy Make the code below more compact -- too much redundancy here. */

        /* Save new 'Recording' data for online case: */
        if (isMachineOnline())
        {
            /* If 'Recording' was *enabled*: */
            if (oldDisplayData.m_fRecordingEnabled)
            {
                /* Save whether recording is enabled: */
                if (fSuccess && newDisplayData.m_fRecordingEnabled != oldDisplayData.m_fRecordingEnabled)
                {
                    recordingSettings.SetEnabled(newDisplayData.m_fRecordingEnabled);
                    fSuccess = recordingSettings.isOk();
                }

                // We can still save the *screens* option.
                /* Save recording screens: */
                if (fSuccess)
                {
                    CRecordingScreenSettingsVector RecordScreenSettingsVector = recordingSettings.GetScreens();
                    for (int iScreenIndex = 0; fSuccess && iScreenIndex < RecordScreenSettingsVector.size(); ++iScreenIndex)
                    {
                        if (newDisplayData.m_vecRecordingScreens[iScreenIndex] == oldDisplayData.m_vecRecordingScreens[iScreenIndex])
                            continue;

                        CRecordingScreenSettings recordingScreenSettings = RecordScreenSettingsVector.at(iScreenIndex);
                        recordingScreenSettings.SetEnabled(newDisplayData.m_vecRecordingScreens[iScreenIndex]);
                        fSuccess = recordingScreenSettings.isOk();
                    }
                }
            }
            /* If 'Recording' was *disabled*: */
            else
            {
                CRecordingScreenSettingsVector recordingScreenSettingsVector = recordingSettings.GetScreens();
                for (int iScreenIndex = 0; fSuccess && iScreenIndex < recordingScreenSettingsVector.size(); ++iScreenIndex)
                {
                    CRecordingScreenSettings recordingScreenSettings = recordingScreenSettingsVector.at(iScreenIndex);

                    // We should save all the options *before* 'Recording' activation.
                    // And finally we should *enable* Recording if necessary.
                    /* Save recording file path: */
                    if (fSuccess && newDisplayData.m_strRecordingFilePath != oldDisplayData.m_strRecordingFilePath)
                    {
                        recordingScreenSettings.SetFilename(newDisplayData.m_strRecordingFilePath);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                    /* Save recording frame width: */
                    if (fSuccess && newDisplayData.m_iRecordingVideoFrameWidth != oldDisplayData.m_iRecordingVideoFrameWidth)
                    {
                        recordingScreenSettings.SetVideoWidth(newDisplayData.m_iRecordingVideoFrameWidth);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                    /* Save recording frame height: */
                    if (fSuccess && newDisplayData.m_iRecordingVideoFrameHeight != oldDisplayData.m_iRecordingVideoFrameHeight)
                    {
                        recordingScreenSettings.SetVideoHeight(newDisplayData.m_iRecordingVideoFrameHeight);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                    /* Save recording frame rate: */
                    if (fSuccess && newDisplayData.m_iRecordingVideoFrameRate != oldDisplayData.m_iRecordingVideoFrameRate)
                    {
                        recordingScreenSettings.SetVideoFPS(newDisplayData.m_iRecordingVideoFrameRate);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                    /* Save recording frame bit rate: */
                    if (fSuccess && newDisplayData.m_iRecordingVideoBitRate != oldDisplayData.m_iRecordingVideoBitRate)
                    {
                        recordingScreenSettings.SetVideoRate(newDisplayData.m_iRecordingVideoBitRate);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                    /* Save recording options: */
                    if (fSuccess && newDisplayData.m_strRecordingVideoOptions != oldDisplayData.m_strRecordingVideoOptions)
                    {
                        recordingScreenSettings.SetOptions(newDisplayData.m_strRecordingVideoOptions);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                    /* Finally, save the screen's recording state: */
                    /* Note: Must come last, as modifying options with an enabled recording state is not possible. */
                    if (fSuccess && newDisplayData.m_vecRecordingScreens != oldDisplayData.m_vecRecordingScreens)
                    {
                        recordingScreenSettings.SetEnabled(newDisplayData.m_vecRecordingScreens[iScreenIndex]);
                        Assert(recordingScreenSettings.isOk());
                        fSuccess = recordingScreenSettings.isOk();
                    }
                }

                /* Save whether recording is enabled:
                 * Do this last, as after enabling recording no changes via API aren't allowed anymore. */
                if (fSuccess && newDisplayData.m_fRecordingEnabled != oldDisplayData.m_fRecordingEnabled)
                {
                    recordingSettings.SetEnabled(newDisplayData.m_fRecordingEnabled);
                    Assert(recordingSettings.isOk());
                    fSuccess = recordingSettings.isOk();
                }
            }
        }
        /* Save new 'Recording' data for offline case: */
        else
        {
            CRecordingScreenSettingsVector recordingScreenSettingsVector = recordingSettings.GetScreens();
            for (int iScreenIndex = 0; fSuccess && iScreenIndex < recordingScreenSettingsVector.size(); ++iScreenIndex)
            {
                CRecordingScreenSettings recordingScreenSettings = recordingScreenSettingsVector.at(iScreenIndex);

                /* Save recording file path: */
                if (fSuccess && newDisplayData.m_strRecordingFilePath != oldDisplayData.m_strRecordingFilePath)
                {
                    recordingScreenSettings.SetFilename(newDisplayData.m_strRecordingFilePath);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
                /* Save recording frame width: */
                if (fSuccess && newDisplayData.m_iRecordingVideoFrameWidth != oldDisplayData.m_iRecordingVideoFrameWidth)
                {
                    recordingScreenSettings.SetVideoWidth(newDisplayData.m_iRecordingVideoFrameWidth);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
                /* Save recording frame height: */
                if (fSuccess && newDisplayData.m_iRecordingVideoFrameHeight != oldDisplayData.m_iRecordingVideoFrameHeight)
                {
                    recordingScreenSettings.SetVideoHeight(newDisplayData.m_iRecordingVideoFrameHeight);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
                /* Save recording frame rate: */
                if (fSuccess && newDisplayData.m_iRecordingVideoFrameRate != oldDisplayData.m_iRecordingVideoFrameRate)
                {
                    recordingScreenSettings.SetVideoFPS(newDisplayData.m_iRecordingVideoFrameRate);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
                /* Save recording frame bit rate: */
                if (fSuccess && newDisplayData.m_iRecordingVideoBitRate != oldDisplayData.m_iRecordingVideoBitRate)
                {
                    recordingScreenSettings.SetVideoRate(newDisplayData.m_iRecordingVideoBitRate);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
                /* Save capture options: */
                if (fSuccess && newDisplayData.m_strRecordingVideoOptions != oldDisplayData.m_strRecordingVideoOptions)
                {
                    recordingScreenSettings.SetOptions(newDisplayData.m_strRecordingVideoOptions);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
                /* Finally, save the screen's recording state: */
                /* Note: Must come last, as modifying options with an enabled recording state is not possible. */
                if (fSuccess && newDisplayData.m_vecRecordingScreens != oldDisplayData.m_vecRecordingScreens)
                {
                    recordingScreenSettings.SetEnabled(newDisplayData.m_vecRecordingScreens[iScreenIndex]);
                    Assert(recordingScreenSettings.isOk());
                    fSuccess = recordingScreenSettings.isOk();
                }
            }

            /* Save whether recording is enabled:
             * Do this last, as after enabling recording no changes via API aren't allowed anymore. */
            if (fSuccess && newDisplayData.m_fRecordingEnabled != oldDisplayData.m_fRecordingEnabled)
            {
                recordingSettings.SetEnabled(newDisplayData.m_fRecordingEnabled);
                Assert(recordingSettings.isOk());
                fSuccess = recordingSettings.isOk();
            }
        }

        /* Show error message if necessary: */
        if (!fSuccess)
            notifyOperationProgressError(UIErrorString::formatErrorInfo(m_machine));
    }
    /* Return result: */
    return fSuccess;
}

void UIMachineSettingsDisplay::enableDisableRecordingWidgets()
{
    /* Recording options should be enabled only if:
     * 1. Machine is in 'offline' or 'saved' state and check-box is checked,
     * 2. Machine is in 'online' state, check-box is checked, and video recording is *disabled* currently. */
    const bool fIsRecordingOptionsEnabled = ((isMachineOffline() || isMachineSaved()) && m_pCheckboxVideoCapture->isChecked()) ||
                                             (isMachineOnline() && !m_pCache->base().m_fRecordingEnabled && m_pCheckboxVideoCapture->isChecked());

    /* Video Capture Screens option should be enabled only if:
     * Machine is in *any* valid state and check-box is checked. */
    const bool fIsVideoCaptureScreenOptionEnabled = isMachineInValidMode() && m_pCheckboxVideoCapture->isChecked();
    const UISettingsDefs::RecordingMode enmRecordingMode =
        gpConverter->fromString<UISettingsDefs::RecordingMode>(m_pComboBoxCaptureMode->currentText());
    const bool fRecordVideo =    enmRecordingMode == UISettingsDefs::RecordingMode_VideoOnly
                              || enmRecordingMode == UISettingsDefs::RecordingMode_VideoAudio;
    const bool fRecordAudio =    enmRecordingMode == UISettingsDefs::RecordingMode_AudioOnly
                              || enmRecordingMode == UISettingsDefs::RecordingMode_VideoAudio;

    m_pLabelVideoCaptureSize->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pComboVideoCaptureSize->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pEditorVideoCaptureWidth->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pEditorVideoCaptureHeight->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);

    m_pLabelVideoCaptureFrameRate->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pContainerSliderVideoCaptureFrameRate->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pEditorVideoCaptureFrameRate->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);

    m_pLabelVideoCaptureRate->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pContainerSliderVideoCaptureQuality->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pEditorVideoCaptureBitRate->setEnabled(fIsRecordingOptionsEnabled && fRecordVideo);
    m_pScrollerVideoCaptureScreens->setEnabled(fIsVideoCaptureScreenOptionEnabled && fRecordVideo);

    m_pAudioCaptureQualityLabel->setEnabled(fIsRecordingOptionsEnabled && fRecordAudio);
    m_pContainerSliderAudioCaptureQuality->setEnabled(fIsRecordingOptionsEnabled && fRecordAudio);

    m_pLabelVideoCaptureScreens->setEnabled(fIsVideoCaptureScreenOptionEnabled && fRecordVideo);
    m_pLabelVideoCaptureSizeHint->setEnabled(fIsVideoCaptureScreenOptionEnabled && fRecordVideo);
}
