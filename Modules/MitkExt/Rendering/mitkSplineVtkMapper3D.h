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


#ifndef _MITK_SPLINE_VTK_MAPPER_3D__H
#define _MITK_SPLINE_VTK_MAPPER_3D__H

#include "mitkPointSetVtkMapper3D.h"
#include "MitkExtExports.h"

class vtkActor;
class vtkAssembly;

namespace mitk
{
  //##Documentation
  //## @brief Vtk-based mapper for Splines
  //##
  //## properties looked for:
  //## "line width" = with of the spline
  //## @ingroup Mapper

  /************************************************************************/
  /*
  *   - \b "line width": (FloatProperty) line width of the spline

  */
  /************************************************************************/
class MitkExt_EXPORT SplineVtkMapper3D : public PointSetVtkMapper3D
{
public:

    mitkClassMacro( SplineVtkMapper3D, PointSetVtkMapper3D );

    itkNewMacro( Self );

    virtual vtkProp* GetVtkProp(mitk::BaseRenderer *renderer);
    virtual void UpdateVtkTransform(mitk::BaseRenderer *renderer);

    bool SplinesAreAvailable();

    vtkPolyData* GetSplinesPolyData();

    vtkActor* GetSplinesActor();

    unsigned long GetLastUpdateTime() const;

    virtual void UpdateSpline();

    itkSetMacro( SplineResolution, unsigned int );

    itkGetMacro( SplineResolution, unsigned int );

protected:

    SplineVtkMapper3D();

    virtual ~SplineVtkMapper3D();

    virtual void GenerateDataForRenderer(mitk::BaseRenderer * renderer);

    virtual void ApplyAllProperties(BaseRenderer *renderer, vtkActor *actor);

    vtkActor* m_SplinesActor;

    vtkPropAssembly* m_SplineAssembly;

    bool m_SplinesAvailable;

    bool m_SplinesAddedToAssembly;

    unsigned int m_SplineResolution;

    itk::TimeStamp m_SplineUpdateTime;
};


} //namespace mitk


#endif


