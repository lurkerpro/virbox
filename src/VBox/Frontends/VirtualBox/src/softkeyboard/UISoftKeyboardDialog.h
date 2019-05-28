/* $Id$ */
/** @file
 * VBox Qt GUI - UISoftKeyboardDialog class declaration.
 */

/*
 * Copyright (C) 2010-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboardDialog_h
#define FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboardDialog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMainWindow>
#include <QString>

/* COM includes: */
#include "COMEnums.h"
#include "CGuest.h"

/* Forward declarations: */
class UIActionPool;
class UISession;
class UISoftKeyboardDialog;
class CGuest;


class UISoftKeyboardDialog : public QMainWindow
{
    Q_OBJECT;

public:

    /** Constructs Guest Control dialog.
      * @param  pCenterWidget  Passes the widget reference to center according to.
      * @param  pActionPool    Passes the action-pool reference. */
    UISoftKeyboardDialog(QWidget *pParent, UISession *pSession, const QString &strMachineName = QString());

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() /* override */;
    /** @} */

    /** @name Prepare/cleanup cascade.
     * @{ */
        /** Configures all. */
        virtual void configure() /* override */;
        /** Configures central-widget. */
        virtual void configureCentralWidget() /* override */;
        /** Perform final preparations. */
        virtual void finalize() /* override */;
        /** Loads dialog setting such as geometry from extradata. */
        virtual void loadSettings() /* override */;

        /** Saves dialog setting into extradata. */
        virtual void saveSettings() const /* override */;
    /** @} */

    /** @name Functions related to geometry restoration.
     * @{ */
        /** Returns whether the window should be maximized when geometry being restored. */
        virtual bool shouldBeMaximized() const /* override */;
    /** @} */

private slots:

    void sltSetCloseButtonShortCut(QKeySequence shortcut);

private:

    UISession    *m_pSession;
    UIActionPool *m_pActionPool;
    QString      m_strMachineName;
};


#endif /* !FEQT_INCLUDED_SRC_softkeyboard_UISoftKeyboardDialog_h */
