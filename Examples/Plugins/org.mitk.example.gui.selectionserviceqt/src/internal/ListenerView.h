/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center, 
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without 
even the implied warranty of MERCHANTABILITY or FITNESS FOR 
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef LISTENERVIEW_H_
#define LISTENERVIEW_H_

/// Berry
#include <berryQtViewPart.h>
#include <berryIStructuredSelection.h>
#include <berryISelectionListener.h>


#include "ui_ListenerViewControls.h"


class ListenerView : public berry::QtViewPart
{

  Q_OBJECT

public:

  static const std::string VIEW_ID;

  ListenerView();

  virtual ~ListenerView();

  virtual void CreateQtPartControl(QWidget *parent);

private:
    void SelectionChanged(berry::IWorkbenchPart::Pointer sourcepart,
                     berry::ISelection::ConstPointer selection);

    berry::ISelectionListener::Pointer m_SelectionListener;
    friend struct berry::SelectionChangedAdapter<ListenerView>;

  private slots:
    void ToggleRadioMethod(); 
    //void ToggleRadioMethod(berry::IStructuredSelection::ConstPointer m_CurrentSelection); //Debugging only!

protected:

  berry::IStructuredSelection::ConstPointer m_CurrentSelection;

  void SetFocus();

  Ui::ListenerViewControls m_Controls;

  QWidget* m_Parent;

};

#endif /*LISTENERVIEW_H_*/
