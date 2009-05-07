/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include <qvalidator.h>
#include <mitkImageCast.h>
#include <stdio.h>
#include <stdlib.h>
#include <mitkLevelWindowProperty.h>
#include <mitkRenderingManager.h>
#include "itkRegularStepGradientDescentOptimizer.h"
#include <qfiledialog.h>
#include <qmessagebox.h>

void QmitkDemonsRegistrationControls::init()
{
  m_FixedNode = NULL;
  m_MovingNode = NULL;
  
  QValidator* validatorHistogramLevels = new QIntValidator(1, 20000000, this);
  m_NumberOfHistogramLevels->setValidator(validatorHistogramLevels);

  QValidator* validatorMatchPoints = new QIntValidator(1, 20000000, this);
  m_NumberOfMatchPoints->setValidator(validatorMatchPoints);

  QValidator* validatorIterations = new QIntValidator(1, 20000000, this);
  m_Iterations->setValidator(validatorIterations);

  QValidator* validatorStandardDeviation = new QDoubleValidator(0, 20000000, 2, this);
  m_StandardDeviation->setValidator(validatorStandardDeviation);
}

int QmitkDemonsRegistrationControls::GetNumberOfIterations()
{
  return atoi(m_Iterations->text().latin1());
}

float QmitkDemonsRegistrationControls::GetStandardDeviation()
{
  return atof(m_StandardDeviation->text().latin1());
}

void QmitkDemonsRegistrationControls::CalculateTransformation()
{
  if (m_FixedNode != NULL && m_MovingNode != NULL)
  {
    mitk::Image::Pointer fimage = dynamic_cast<mitk::Image*>(m_FixedNode->GetData());
    mitk::Image::Pointer mimage = dynamic_cast<mitk::Image*>(m_MovingNode->GetData());
    if ( m_RegistrationSelection->currentItem() == 0)
    {
      QString deformationFieldname;
      QString resultImagename;
      mitk::DemonsRegistration::Pointer registration = mitk::DemonsRegistration::New();
      int saveResult = QMessageBox::question(this, "Save results", "Do you want to save the results?",QMessageBox::Yes,QMessageBox::No);
      if(saveResult == QMessageBox::Yes)
      {
        // ask for filenames to save deformation field and result image
        bool isValid = false;
        while(!isValid)
        {
          deformationFieldname = QFileDialog::getSaveFileName(
            "deformationField.mhd",
            "*.mhd",
            this,  
            "Choose filename",
            "Where do you want to save the deformation field?");
          if ( !deformationFieldname.isEmpty() && deformationFieldname.right(4) != ".mhd" ) 
          {
            deformationFieldname += ".mhd";
          }
          else if (deformationFieldname.isEmpty())
          {
            QMessageBox::information(this, "Save file", "You have to specify a file name!",QMessageBox::Ok);
            isValid = false;
          }
          //check if file exists
          if(QFile::exists( deformationFieldname ))
          {
            //ask the user whether the file shall be overwritten
            int overwrite = QMessageBox::question(this, "Save file", "File already exists. Shall the file be overwritten?",QMessageBox::Yes,QMessageBox::No);
            if(overwrite == QMessageBox::Yes)
            {
              isValid = true;
            } 
            else
            {
              isValid  = false;
            }
          }
          else
          {
            isValid = true;
          }
        }        
        isValid = false;
        while(!isValid)
        {
          resultImagename = QFileDialog::getSaveFileName(
            "resultImage.mhd",
            "*.mhd",
            this,
            "Choose directory",
            "Where do you want to save the result image?");
          if ( !resultImagename.isEmpty() && resultImagename.right(4) != ".mhd" ) 
          {
            resultImagename += ".mhd";
          }
          else if (resultImagename.isEmpty())
          {
            QMessageBox::information(this, "Save file", "You have to specify a file name!",QMessageBox::Ok);
            isValid = false;
          }
          //check if file exists
          if (resultImagename == deformationFieldname)
          {
            QMessageBox::information(this, "Save file", "You have to specify a different file name than for the deformation field!",QMessageBox::Ok);
            isValid = false;
          }
          else if(QFile::exists( resultImagename ))
          {
            //ask the user whether the file shall be overwritten
            int overwrite = QMessageBox::question(this, "Save file", "File already exists. Shall the file be overwritten?",QMessageBox::Yes,QMessageBox::No);
            if(overwrite == QMessageBox::Yes)
            {
              isValid = true;
            } 
            else 
            {
              isValid  = false;
            }
          }
          else
          {
            isValid = true;
          }
        }        
        registration->SetSaveDeformationField(true);
        registration->SetSaveResult(true);
        registration->SetDeformationFieldFileName(deformationFieldname);
        registration->SetResultFileName(resultImagename);
      }
      else
      {
        registration->SetSaveDeformationField(false);
        registration->SetSaveResult(false);
      }
      
      registration->SetReferenceImage(fimage);
      registration->SetNumberOfIterations(atoi(m_Iterations->text().latin1()));
      registration->SetStandardDeviation(atof(m_StandardDeviation->text().latin1()));
      if (m_UseHistogramMatching->isChecked())
      {
        mitk::HistogramMatching::Pointer histogramMatching = mitk::HistogramMatching::New();
        histogramMatching->SetReferenceImage(fimage);
        histogramMatching->SetInput(mimage);
        histogramMatching->SetNumberOfHistogramLevels(atoi(m_NumberOfHistogramLevels->text().latin1()));
        histogramMatching->SetNumberOfMatchPoints(atoi(m_NumberOfMatchPoints->text().latin1()));
        histogramMatching->SetThresholdAtMeanIntensity(m_ThresholdAtMeanIntensity->isChecked());
        histogramMatching->Update();
        mitk::Image::Pointer histimage = histogramMatching->GetOutput();
        if (histimage.IsNotNull())
        {
          registration->SetInput(histimage);
        }
        else
        {
          registration->SetInput(mimage);
        }
      }
      else
      {
        registration->SetInput(mimage);
      }
      try
      {
        registration->Update();
      }
      catch (itk::ExceptionObject& excpt)
      {
        QMessageBox::information( this, "Registration exception", excpt.GetDescription(), QMessageBox::Ok );
      }
      mitk::Image::Pointer image = registration->GetOutput();
      if (image.IsNotNull())
      {
        m_MovingNode->SetData(image);
        mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
        mitk::LevelWindow levelWindow;
        levelWindow.SetAuto( image );
        levWinProp->SetLevelWindow(levelWindow);
        m_MovingNode->GetPropertyList()->SetProperty("levelwindow",levWinProp);
      }
    }
    else if(m_RegistrationSelection->currentItem() == 1)
    {
      QString deformationFieldname;
      QString resultImagename;
      mitk::SymmetricForcesDemonsRegistration::Pointer registration = mitk::SymmetricForcesDemonsRegistration::New();
      int saveResult = QMessageBox::question(this, "Save results", "Do you want to save the results?",QMessageBox::Yes,QMessageBox::No);
      if(saveResult == QMessageBox::Yes)
      {
        // ask for filenames to save deformation field and result image
        bool isValid = false;
        while(!isValid)
        {
          deformationFieldname = QFileDialog::getSaveFileName(
            "deformationField.mhd",
            "*.mhd",
            this,  
            "Choose filename",
            "Where do you want to save the deformation field?");
          if ( !deformationFieldname.isEmpty() && deformationFieldname.right(4) != ".mhd" ) 
          {
            deformationFieldname += ".mhd";
          }
          else if (deformationFieldname.isEmpty())
          {
            QMessageBox::information(this, "Save file", "You have to specify a file name!",QMessageBox::Ok);
            isValid = false;
          }
          //check if file exists
          if(QFile::exists( deformationFieldname ))
          {
            //ask the user whether the file shall be overwritten
            int overwrite = QMessageBox::question(this, "Save file", "File already exists. Shall the file be overwritten?",QMessageBox::Yes,QMessageBox::No);
            if(overwrite == QMessageBox::Yes)
            {
              isValid = true;
            } 
            else
            {
              isValid  = false;
            }
          }
          else
          {
            isValid = true;
          }
        }        
        isValid = false;
        while(!isValid)
        {
          resultImagename = QFileDialog::getSaveFileName(
            "resultImage.mhd",
            "*.mhd",
            this,
            "Choose directory",
            "Where do you want to save the result image?");
          if ( !resultImagename.isEmpty() && resultImagename.right(4) != ".mhd" ) 
          {
            resultImagename += ".mhd";
          }
          else if (resultImagename.isEmpty())
          {
            QMessageBox::information(this, "Save file", "You have to specify a file name!",QMessageBox::Ok);
            isValid = false;
          }
          //check if file exists
          if (resultImagename == deformationFieldname)
          {
            QMessageBox::information(this, "Save file", "You have to specify a different file name than for the deformation field!",QMessageBox::Ok);
            isValid = false;
          }
          else if(QFile::exists( resultImagename ))
          {
            //ask the user whether the file shall be overwritten
            int overwrite = QMessageBox::question(this, "Save file", "File already exists. Shall the file be overwritten?",QMessageBox::Yes,QMessageBox::No);
            if(overwrite == QMessageBox::Yes)
            {
              isValid = true;
            } 
            else 
            {
              isValid  = false;
            }
          }
          else
          {
            isValid = true;
          }
        }        
        registration->SetSaveDeformationField(true);
        registration->SetSaveResult(true);
        registration->SetDeformationFieldFileName(deformationFieldname);
        registration->SetResultFileName(resultImagename);
      }
      else
      {
        registration->SetSaveDeformationField(false);
        registration->SetSaveResult(false);
      }

      registration->SetReferenceImage(fimage);
      registration->SetNumberOfIterations(atoi(m_Iterations->text().latin1()));
      registration->SetStandardDeviation(atof(m_StandardDeviation->text().latin1()));
      if (m_UseHistogramMatching->isChecked())
      {
        mitk::HistogramMatching::Pointer histogramMatching = mitk::HistogramMatching::New();
        histogramMatching->SetReferenceImage(fimage);
        histogramMatching->SetInput(mimage);
        histogramMatching->SetNumberOfHistogramLevels(atoi(m_NumberOfHistogramLevels->text().latin1()));
        histogramMatching->SetNumberOfMatchPoints(atoi(m_NumberOfMatchPoints->text().latin1()));
        histogramMatching->SetThresholdAtMeanIntensity(m_ThresholdAtMeanIntensity->isChecked());
        histogramMatching->Update();
        mitk::Image::Pointer histimage = histogramMatching->GetOutput();
        if (histimage.IsNotNull())
        {
          registration->SetInput(histimage);
        }
        else
        {
          registration->SetInput(mimage);
        }
      }
      else
      {
        registration->SetInput(mimage);
      }
      registration->Update();
      mitk::Image::Pointer image = registration->GetOutput();
      if (image.IsNotNull())
      {
        m_MovingNode->SetData(image);
        mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
        mitk::LevelWindow levelWindow;
        levelWindow.SetAuto( image );
        levWinProp->SetLevelWindow(levelWindow);
        m_MovingNode->GetPropertyList()->SetProperty("levelwindow",levWinProp);
      }
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  }
}

void QmitkDemonsRegistrationControls::SetFixedNode( mitk::DataTreeNode * fixedNode )
{
  m_FixedNode = fixedNode;
}

void QmitkDemonsRegistrationControls::SetMovingNode( mitk::DataTreeNode * movingNode )
{
  m_MovingNode = movingNode;
}

void QmitkDemonsRegistrationControls::UseHistogramMatching( bool useHM )
{
  if (useHM)
  {
    numberOfHistogramLevels->setEnabled(true);
    m_NumberOfHistogramLevels->setEnabled(true);
    numberOfMatchPoints->setEnabled(true);
    m_NumberOfMatchPoints->setEnabled(true);
    thresholdAtMeanIntensity->setEnabled(true);
    m_ThresholdAtMeanIntensity->setEnabled(true);
  }
  else
  {
    numberOfHistogramLevels->setEnabled(false);
    m_NumberOfHistogramLevels->setEnabled(false);
    numberOfMatchPoints->setEnabled(false);
    m_NumberOfMatchPoints->setEnabled(false);
    thresholdAtMeanIntensity->setEnabled(false);
    m_ThresholdAtMeanIntensity->setEnabled(false);
  }
}
