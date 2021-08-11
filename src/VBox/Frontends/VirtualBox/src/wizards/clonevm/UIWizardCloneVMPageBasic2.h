/* $Id$ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic2 class declaration.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVMPageBasic2_h
#define FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVMPageBasic2_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes: */
#include "UINativeWizardPage.h"

/* Forward declarations: */
class QAbstractButton;
class QButtonGroup;
class QRadioButton;
class QIRichTextLabel;
class UICloneVMCloneTypeGroupBox;

// /* 2nd page of the Clone Virtual Machine wizard (base part): */
// class UIWizardCloneVMPage2 : public UIWizardPageBase
// {
// protected:

//     /* Constructor: */
//     UIWizardCloneVMPage2(bool fAdditionalInfo);

//     /* Stuff for 'linkedClone' field: */
//     bool linkedClone() const;

//     /* Variables: */

//     /* Widgets: */
// };

/* 2nd page of the Clone Virtual Machine wizard (basic extension): */
class UIWizardCloneVMPageBasic2 : public UINativeWizardPage
{
    Q_OBJECT;
    //Q_PROPERTY(bool linkedClone READ linkedClone);

public:

    /* Constructor: */
    UIWizardCloneVMPageBasic2(bool fAdditionalInfo);

private slots:

    void sltCloneTypeChanged(bool fIsFullClone);

private:

    /* Translation stuff: */
    void retranslateUi();

    /* Prepare stuff: */
    void initializePage();
    void prepare();

    /* Validation stuff: */
    bool validatePage();

    /* Widgets: */
    QIRichTextLabel *m_pLabel;
    bool m_fAdditionalInfo;
    UICloneVMCloneTypeGroupBox *m_pCloneTypeGroupBox;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVMPageBasic2_h */
