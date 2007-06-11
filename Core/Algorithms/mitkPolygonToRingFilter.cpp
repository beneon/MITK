/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
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

#include "mitkPolygonToRingFilter.h"
#include "mitkMesh.h"
#include "mitkSurface.h"
#include "mitkPlaneGeometry.h"

#include <vnl/vnl_cross.h>
#include <vnl/vnl_quaternion.h>
#include <vnl/vnl_quaternion.txx>

#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkCardinalSpline.h>

#include <vector>


mitk::PolygonToRingFilter::PolygonToRingFilter()
: m_RingRadius(3.5f), m_RingResolution(30), m_SplineResolution(20)
{
  m_SplineX = vtkCardinalSpline::New();
  m_SplineY = vtkCardinalSpline::New();
  m_SplineZ = vtkCardinalSpline::New();
}

mitk::PolygonToRingFilter::~PolygonToRingFilter()
{
  m_SplineX->Delete();
  m_SplineY->Delete();
  m_SplineZ->Delete();
}

void mitk::PolygonToRingFilter::GenerateOutputInformation()
{
  mitk::Mesh::ConstPointer input  = this->GetInput();
  mitk::Surface::Pointer output = this->GetOutput();

  itkDebugMacro(<<"GenerateOutputInformation()");

  if(input.IsNull()) return;

  output->SetGeometry(static_cast<Geometry3D*>(input->GetGeometry()->Clone().GetPointer()));
}

void mitk::PolygonToRingFilter::GenerateData()
{
  mitk::Mesh::ConstPointer input  = this->GetInput();
  mitk::Surface::Pointer output = this->GetOutput();

  int t;
  for ( t = 0; t < input->GetPointSetSeriesSize(); ++t )
  {

    vtkPolyData *polyData = vtkPolyData::New();
    vtkPoints *vPoints = vtkPoints::New();
    vtkCellArray *polys = vtkCellArray::New();
    //polys->Initialize();
    //vPoints->Initialize();

    mitk::Mesh::PointType thisPoint;

    //iterate through all cells and build tubes
    Mesh::CellIterator cellIt, cellEnd;
    cellEnd = input->GetMesh( t )->GetCells()->End();
    for ( cellIt = input->GetMesh( t )->GetCells()->Begin();
          cellIt != cellEnd; 
          ++cellIt )
    {
      m_PointList.clear();
      m_VectorList.clear();
      BuildPointAndVectorList(*cellIt->Value(), m_PointList, m_VectorList);
      BuildVtkTube(vPoints, polys, m_PointList, m_VectorList);
    }

    polyData->SetPoints( vPoints );
    vPoints->Delete();
    polyData->SetPolys( polys );
    polys->Delete();

    output->SetVtkPolyData( polyData, t );
    polyData->Delete();
  
  }

}


//sl: Stern Letzter
//sc: Stern Current=aktueller Stern
//idmax: Id des Strahls des aktuellen Sterns (sc), der am besten zum ersten Strahl vom letzten Stern (sl) passt.
//last_p: Mittelpunkt des letzten Sterns
//cur_p: Mittelpunkt des aktuellen Sterns
void mitk::PolygonToRingFilter::DrawCyl(vtkPoints *vPoints, vtkCellArray *polys, 
                                        VectorListType &sl, VectorListType &sc, int idmax, Point3D & last_p, Point3D & cur_p)
{
  unsigned int i;
  //jetzt haben wir alles: sl0 wird mit sc->at(idmax) verbunden usw.
  VectorListType::iterator slit=sl.begin(), scit=sc.begin(), scend=sc.end();
  scit+=idmax;
  Point3D a,b;
  Point3D a_first,b_first;
  int a_firstID = 0, b_firstID = 0;

  int front[4];
  for(i=0;i<m_RingResolution;++i)
  {
    VnlVector v0,v1,v2,v3,normal;
    v0=a.Get_vnl_vector(); v1=b.Get_vnl_vector();
    a=last_p+*slit*m_RingRadius; b=cur_p+*scit*m_RingRadius;
    v2=b.Get_vnl_vector(); v3=a.Get_vnl_vector();
    normal=vnl_cross_3d(v1-v0,v3-v0);

    if(i!=0)
    {
      front[3]=vPoints->InsertNextPoint(v0[0],v0[1],v0[2]);
      front[2]=vPoints->InsertNextPoint(v1[0],v1[1],v1[2]);
      front[1]=vPoints->InsertNextPoint(v2[0],v2[1],v2[2]);
      front[0]=vPoints->InsertNextPoint(v3[0],v3[1],v3[2]);
      polys->InsertNextCell( 4, front );
      if(i==1)
      {
        a_firstID=front[3]; b_firstID=front[2]; //continue;
      }
    }

    ++slit; ++scit; if(scit==scend) scit=sc.begin();
  }
  front[3]=front[0];
  front[2]=front[1];
  front[1]=b_firstID;
  front[0]=a_firstID;
  polys->InsertNextCell( 4, front );
}


void mitk::PolygonToRingFilter::BuildVtkTube(vtkPoints *vPoints, vtkCellArray *polys, PointListType& ptList, VectorListType& vecList)
{
  PointListType::iterator pit  = ptList.begin(), pend = ptList.end();
  VectorListType::iterator vit = vecList.begin();

  Vector3D axis, last_v, next_v, s;
  Point3D cur_p,last_p;

  //Listen f�r den Stern
  VectorListType *sl, *sc, *swp, sfirst, buf1, buf2;
  sl=&buf1; sc=&buf2;

  Vector3D a,b;
  Matrix3D m;
  //Initialisierung f�r ersten Punkt
  //alternative1:
  //	last_v=*(vl.getLast()); next_v=*vit.current(); axis=last_v+next_v; s.cross(last_v,next_v); s.normalize();
  //alternative2:
  //	last_v=*(vl.getLast()); next_v=*vit.current(); s.cross(last_v,next_v); s.normalize();
  //	axis=next_v-last_v; axis.normalize(); aa.set(s, M_PI/2.0); m.set(aa); m.transform(&axis);
  //alternative3:
  last_v=vecList.back(); next_v=*vit; s.Set_vnl_vector( vnl_cross_3d(last_v.Get_vnl_vector(),next_v.Get_vnl_vector()) ); s.Normalize();
  a=last_v; b=next_v; a.Normalize(); b.Normalize(); axis=a+b; axis.Normalize();

  //Stern am ersten Punkt aufbauen
  m = vnl_quaternion<mitk::ScalarType>(axis.Get_vnl_vector(),2*vnl_math::pi/(double)m_RingResolution).rotation_matrix_transpose();
  unsigned int i;
  for(i=0;i<m_RingResolution;++i)
  {
    sfirst.push_back(s);
    s=m*s;
  }
  *sl=sfirst;
  last_p=*pit;
  ++pit; ++vit;

  //nun die Hauptschleife �ber alle Punkte
  for ( ; pit != pend; ++pit, ++vit )
  {
    //		cur_p=*pit.current(); last_v=next_v; next_v=*vit.current(); axis=last_v+next_v; s.cross(last_v,next_v); s.normalize();
    cur_p=*pit; last_v=next_v; next_v=*vit; s.Set_vnl_vector( vnl_cross_3d(last_v.Get_vnl_vector(),next_v.Get_vnl_vector()) ); s.Normalize();
    //		axis=next_v-last_v; axis.normalize(); aa.set(s, M_PI/2.0); m.set(aa); m.transform(&axis);
    a=last_v; b=next_v; a.Normalize(); b.Normalize(); axis=a+b; axis.Normalize();

    //neuen Stern sc (SternCurrent) bauen und dabei Start f�r neuen Stern suchen
    double max=0; int idmax=0; Vector3D sl0=*(sl->begin()); 
    m = vnl_quaternion<mitk::ScalarType>(axis.Get_vnl_vector(),2*vnl_math::pi/(double)m_RingResolution).rotation_matrix_transpose();
    for(i=0;i<m_RingResolution;++i)
    {
      sc->push_back(s);
      double tmp=s*sl0;
      if(tmp>max)
      {
        max=tmp;
        idmax=i;
      }
      s=m*s;
    }

    //sl: Stern Letzter
    //sc: Stern Current=aktueller Stern
    //idmax: Id des Strahls des aktuellen Sterns (sc), der am besten zum ersten Strahl vom letzten Stern (sl) passt.
    //last_p: Mittelpunkt des letzten Sterns
    //cur_p: Mittelpunkt des aktuellen Sterns
    DrawCyl(vPoints, polys, *sl, *sc, idmax, last_p, cur_p);

    //�bergang zum n�chsten
    last_p=cur_p;
    swp=sl; sl=sc; sc=swp; sc->clear();
  }

  //idmax f�r Verbindung ersten mit letztem ausrechnen:
  double max=0; int idmax=0; Vector3D sl0=*(sl->begin());
  for(i=0;i<m_RingResolution;++i)
  {
    s=sfirst[i];
    double tmp=s*sl0;
    if(tmp>max)
    {
      max=tmp;
      idmax=i;
    }
  }
  cur_p=*ptList.begin();
  DrawCyl(vPoints, polys, *sl, sfirst, idmax, last_p, cur_p);
}

void mitk::PolygonToRingFilter::BuildPointAndVectorList(mitk::Mesh::CellType& cell, PointListType& ptList, VectorListType& vecList)
{
  // This method constructs a spline from the given point list and retrieves
  // a number of interpolated points from it to form a ring-like structure.
  //
  // To make the spline "closed", the end point is connected to the start
  // point. For ensuring smoothness at the start-end-point transition, the
  // (intrinsically non-circular) spline array is extended on both sides
  // by wrapping a number of points from the respective other side.
  //
  // The used VTK filters do principally support this kind of "closed" spline,
  // but it does not produce results as consistent as with the method used
  // here. Also, the spline class of VTK 4.4 has only poor support for
  // arbitrary parametric coordinates (t values in vtkSpline). VTK 5.0 has
  // better support, and also provides a new class vtkParametricSpline for
  // directly calculating 3D splines.


  // Remove points from previous call of this method
  m_SplineX->RemoveAllPoints();
  m_SplineY->RemoveAllPoints();
  m_SplineZ->RemoveAllPoints();

  int numberOfPoints = cell.GetNumberOfPoints();

  Mesh::PointType inputPoint;
  vtkFloatingPointType t, tStart(0), tEnd(0);

  // Add input points to the spline and assign each the parametric value t
  // derived from the point euclidean distances.
  int i;
  Mesh::PointIdIterator pit = cell.PointIdsEnd() - 3;
  for ( i = -3, t = 0.0; i < numberOfPoints + 3; ++i )
  {
    if ( i == 0 ) { tStart = t; }
    if ( i == numberOfPoints ) { tEnd = t; }

    inputPoint = this->GetInput()->GetPoint( *pit );
    m_SplineX->AddPoint( t, inputPoint[0] );
    m_SplineY->AddPoint( t, inputPoint[1] );
    m_SplineZ->AddPoint( t, inputPoint[2] );

    ++pit;
    if ( pit == cell.PointIdsEnd() )
    {
      pit = cell.PointIdsBegin();
    }

    t += inputPoint.EuclideanDistanceTo( this->GetInput()->GetPoint( *pit ) );
  }

  // Evaluate the spline for the desired number of points
  // (number of input points) * (spline resolution)
  Point3D point, firstPoint, lastPoint;
  int numberOfSegments = numberOfPoints * m_SplineResolution;
  vtkFloatingPointType step = (tEnd - tStart) / numberOfSegments;
  for ( i = 0, t = tStart; i < numberOfSegments; ++i, t += step )
  {
    FillVector3D( point,
      m_SplineX->Evaluate(t), m_SplineY->Evaluate(t), m_SplineZ->Evaluate(t)
    );

    ptList.push_back( point );

    if ( i == 0 )
    {
      firstPoint = point;
    }
    else
    {
      vecList.push_back( point - lastPoint );
    }
    lastPoint = point;
  }
  vecList.push_back( firstPoint - lastPoint );
}


const mitk::Mesh *mitk::PolygonToRingFilter::GetInput(void)
{
  if (this->GetNumberOfInputs() < 1)
  {
    return 0;
  }

  return static_cast<const mitk::Mesh * >
    (this->ProcessObject::GetInput(0) );
}

void mitk::PolygonToRingFilter::SetInput(const mitk::Mesh *input)
{
  // Process object is not const-correct so the const_cast is required here
  this->ProcessObject::SetNthInput(0, 
    const_cast< mitk::Mesh * >( input ) );
}

//void mitk::PolygonToRingFilter::BuildContourLines(/*mitk::VectorOfContourLines*/void* contours1)
//{
//  mitk::Mesh::ConstPointer input  = this->GetInput();
//  mitk::Surface::Pointer output = this->GetOutput();
//
//  mitk::VectorOfContourLines & contours = *((mitk::VectorOfContourLines*)contours1);
//
//  mitk::Mesh::PointType thisPoint;
//
//  mitk::PlaneGeometry::Pointer oben, rechts;
//  oben   = mitk::PlaneGeometry::New();
//  rechts = mitk::PlaneGeometry::New();
//
//  //iterate through all cells and then iterate through all indexes of points in that cell
//  Mesh::CellIterator cellIt, cellEnd;
//  Mesh::PointIdIterator pointIdBegin, pointIdEnd;
//  Mesh::PointIdIterator pointIdIt, stopPointIdIt, tempPointIdIt;
//
//  cellEnd = input->GetMesh()->GetCells()->End();
//  for( cellIt = input->GetMesh()->GetCells()->Begin(); cellIt != cellEnd; ++cellIt )
//  {
//    unsigned int numOfPointsInCell = cellIt->Value()->GetNumberOfPoints();
//    if (numOfPointsInCell>=3)
//    {
//      pointIdBegin = cellIt->Value()->PointIdsBegin();
//      pointIdEnd   = cellIt->Value()->PointIdsEnd();
//
//      //Zur Bestimmung, ob Polygon links- oder rechtsdrehend (also auf welcher Seite innen ist), 
//      //  wird erstmal ein echtes Linienst�ck gesucht
//      int rl_end_id=1; //Nummer des Endpunkts des gefundenen echten Linienst�cks
//      pointIdIt=stopPointIdIt=pointIdBegin; ++pointIdIt;
//      while((pointIdIt!=pointIdEnd) && (mitk::Equal(input->GetPoint(*pointIdIt),input->GetPoint(*stopPointIdIt))))
//      {
//        stopPointIdIt=pointIdIt;
//        ++pointIdIt; ++rl_end_id;
//      }
//
//      if(pointIdIt!=pointIdEnd) //andernfalls ist das gar kein richtiges Polygon, sondern nur ein Punkt
//      {
//        //Den n�chsten Punkt der nicht kollinear auf dem ersten echten Linienst�ck liegt, suchen. 
//        Vector3D direction;
//        direction = input->GetPoint(*pointIdIt)-input->GetPoint(*stopPointIdIt);
//        direction.Normalize();
//        VnlVector stopVnl = input->GetPoint(*stopPointIdIt).Get_vnl_vector();
//        VnlVector directionVnl=direction.Get_vnl_vector();
//        VnlVector normalVnl;
//        for(tempPointIdIt=pointIdBegin; tempPointIdIt!=pointIdEnd; ++tempPointIdIt)
//        {
//          normalVnl = vnl_cross_3d(directionVnl, stopVnl-input->GetPoint(*tempPointIdIt).Get_vnl_vector());
//          if( mitk::Equal(normalVnl.squared_magnitude(),0.0f) == false )
//            break;
//        }
//        if(tempPointIdIt!=pointIdEnd) //andernfalls ist das gar kein richtiges Polygon, sondern nur eine Linie
//        {
//          //Da wir z.Z. keine Normale in Zelle haben, aus ersten echten Linienst�ck und dem Punkt die Normale
//          //  der Ebene berechnen
//          normalVnl.normalize();
//
//          //Ebenen f�r Drehrichtungstest initialisieren
//          Point3D p; Vector3D mid;
//          VnlVector n_rechtsVnl;
//          Vector3D n_rechts;
//          vtk2itk(input->GetPoint(*pointIdIt), mid);
//          mid += input->GetPoint(*stopPointIdIt);
//          mid *= 0.5;
//          vtk2itk(mid, p);
//          oben->InitializePlane(p,direction);
//          n_rechtsVnl=vnl_cross_3d(directionVnl, normalVnl); n_rechtsVnl.normalize();
//          n_rechts.Set_vnl_vector(n_rechtsVnl);
//          rechts->InitializePlane(p,n_rechts);
//
//          //Drehrichtungstest: wenn die Anzahl der echten Crossings der "oben"-Plane 
//          //  (Vorzeichenwechsel des gerichteten Abstands von Polygonpunkt zu "oben"-Ebene)
//          //  auf der rechten Seite (gerichteter Abstand des Polygonpunkt von "rechts"-Ebene echt positiv)
//          //  ungerade ist, ist die Drehrichtung rechts, sonst links.
//          int crossings=0;
//          bool oberhalb;
//          oberhalb=true;
//
//          ++pointIdIt;	if(pointIdIt==pointIdEnd) pointIdIt=pointIdBegin;
//          while(pointIdIt!=stopPointIdIt)
//          {
//            thisPoint = input->GetPoint(*pointIdIt);
//            if(rechts->SignedDistance(thisPoint)>0)
//            {
//              float dist=oben->SignedDistance(thisPoint);
//              if(oberhalb)
//              {
//                if(dist<0)
//                {
//                  oberhalb=false;
//                  ++crossings;
//                }
//              }
//              else
//              {
//                if(dist>0)
//                {
//                  oberhalb=true;
//                  ++crossings;
//                }
//              }
//            }
//            ++pointIdIt;	if(pointIdIt==pointIdEnd) pointIdIt=pointIdBegin;
//          }
//
//          bool rechts_drehend=((crossings%2)==1);
//
//          //Zwischenpunkte (Punkte auf Polygon-Linienst�ck) in Einheitsabstand einf�gen
//          MBI_STD::auto_ptr<PointList> points3D(new PointList);
//          Point3f first, last, point;
//          Vector3f dir;
//          int i, len;
//          pointIdIt=pointIdBegin;
//          itk2vm(input->GetPoint(*pointIdIt), last);
//          first=last;
//          ++pointIdIt;
//          while(pointIdIt!=pointIdEnd)
//          {
//            itk2vm(input->GetPoint(*pointIdIt), point);
//            dir=point-last; len=dir.length(); dir.normalize();
//            for(i=0;i<len;++i,last+=dir)
//              points3D->push_back(last);
//            last=point;
//            ++pointIdIt;
//          }
//          dir=first-last; len=dir.length(); dir.normalize();
//          for(i=0;i<len;++i,last+=dir)
//            points3D->push_back(last);
//          //erster Punkt muss gleich letzter sein, sonst gilt die contour nicht als geschlossen (sh. ContourLine::contourIsClosed)
//          points3D->push_back(first);
//
//          ContourLine* contour;
//          Point3f originVm;
//          Vector3f normalVm, directionVm;
//          itk2vm(input->GetPoint(*stopPointIdIt), originVm);
//          itk2vm(normalVnl, normalVm);
//          itk2vm(direction, directionVm);
//          View3f view(Plane3f(originVm, normalVm), directionVm);
//          contour = new ContourLine( points3D, view, rechts_drehend );
//
//          contours.push_back(contour);
//        }
//      }
//    }
//  }
//}
