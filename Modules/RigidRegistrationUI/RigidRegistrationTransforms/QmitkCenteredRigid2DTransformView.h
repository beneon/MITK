/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile: mitkPropertyManager.cpp,v $
Language:  C++
Date:      $Date$
Version:   $Revision: 1.12 $

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef QmitkCenteredRigid2DTransformViewWidgetHIncluded
#define QmitkCenteredRigid2DTransformViewWidgetHIncluded

#include "ui_QmitkCenteredRigid2DTransformControls.h"
#include "MitkRigidRegistrationUIExports.h"
#include "QmitkRigidRegistrationTransformsGUIBase.h"
#include <itkImage.h>

/*!
* \brief Widget for rigid registration
*
* Displays options for rigid registration.
*/
class MITK_RIGIDREGISTRATION_UI_EXPORT QmitkCenteredRigid2DTransformView : public QmitkRigidRegistrationTransformsGUIBase
{

public:

  QmitkCenteredRigid2DTransformView( QWidget* parent = 0, Qt::WindowFlags f = 0 );
  ~QmitkCenteredRigid2DTransformView();


  virtual itk::Object::Pointer GetTransform();

  virtual itk::Array<double> GetTransformParameters();

  virtual void SetTransformParameters(itk::Array<double> transformValues);

  virtual QString GetName();

  virtual void SetupUI(QWidget* parent);

  virtual itk::Array<double> GetScales();

  virtual vtkTransform* Transform(vtkMatrix4x4* vtkmatrix, vtkTransform* vtktransform, itk::Array<double> transformParams);

  virtual int GetNumberOfTransformParameters();

private:

  template < class TPixelType, unsigned int VImageDimension >
  itk::Object::Pointer GetTransform2(itk::Image<TPixelType, VImageDimension>* itkImage1);

protected:

  Ui::QmitkCenteredRigid2DTransformControls m_Controls;

  itk::Object::Pointer m_TransformObject;

  double m_CenterX;
  double m_CenterY;
  double m_CenterZ;

};

#endif