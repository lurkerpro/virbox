/* $Id$ */
/** @file
 * VBox Qt GUI - UISoftKeyboard class declaration.
 */

/*
 * Copyright (C) 2016-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboard_h
#define FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* COM includes: */
#include "COMEnums.h"
#include "CGuest.h"
#include "CEventListener.h"

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UISession;
class UISoftKeyboardKey;
class QHBoxLayout;
class QVBoxLayout;
class UISoftKeyboardWidget;
class UIToolBar;


class UISoftKeyboard : public QIWithRetranslateUI<QMainWindow>
{
    Q_OBJECT;

public:

    UISoftKeyboard(QWidget *pParent, UISession *pSession,
                   QString strMachineName = QString());
    ~UISoftKeyboard();

protected:

    virtual void retranslateUi();
    virtual void resizeEvent(QResizeEvent *pEvent);

private slots:

    void sltHandleKeyPress();
    void sltHandleKeyRelease();
    void sltHandleModifierStateChange();

private:

    void prepareObjects();
    void prepareConnections();
    void prepareToolBar();
    void saveSettings();
    void loadSettings();
    void parseLayout();
    void updateLayout();
    CKeyboard& keyboard() const;

    UISession     *m_pSession;
    QHBoxLayout   *m_pMainLayout;
    UISoftKeyboardWidget       *m_pContainerWidget;
    QString       m_strMachineName;
    int           m_iTotalRowHeight;
    int           m_iMaxRowWidth;
    QVector<UISoftKeyboardKey*> m_pressedModifiers;
    QVector<QVector<UISoftKeyboardKey*> > m_keys;
    bool          m_fKeepAspectRatio;

    int m_iXSpacing;
    int m_iYSpacing;
    int m_iLeftMargin;
    int m_iTopMargin;
    int m_iRightMargin;
    int m_iBottomMargin;
};

#endif /* !FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboard_h */
