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


#define _USE_MATH_DEFINES
#include "mitkFiberBundleX.h"

#include <mitkPlanarCircle.h>
#include <mitkPlanarPolygon.h>
#include <mitkPlanarFigureComposite.h>

#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPolyLine.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkIdFilter.h>
#include <vtkClipPolyData.h>
#include <vtkPlane.h>
#include <vtkDoubleArray.h>
#include <vtkKochanekSpline.h>
#include <vtkParametricFunctionSource.h>
#include <vtkParametricSpline.h>
#include <vtkPolygon.h>
#include <vtkCleanPolyData.h>
#include <cmath>
#include <boost/progress.hpp>

const char* mitk::FiberBundleX::COLORCODING_ORIENTATION_BASED = "Color_Orient";
//const char* mitk::FiberBundleX::COLORCODING_FA_AS_OPACITY = "Color_Orient_FA_Opacity";
const char* mitk::FiberBundleX::COLORCODING_FA_BASED = "FA_Values";
const char* mitk::FiberBundleX::COLORCODING_CUSTOM = "custom";
const char* mitk::FiberBundleX::FIBER_ID_ARRAY = "Fiber_IDs";

using namespace std;

mitk::FiberBundleX::FiberBundleX( vtkPolyData* fiberPolyData )
    : m_CurrentColorCoding(NULL)
    , m_NumFibers(0)
    , m_FiberSampling(0)
{
    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    if (fiberPolyData != NULL)
    {
        m_FiberPolyData = fiberPolyData;
        //m_FiberPolyData->DeepCopy(fiberPolyData);
        this->DoColorCodingOrientationBased();
    }

    m_NumFibers = m_FiberPolyData->GetNumberOfLines();

    this->UpdateFiberGeometry();
    this->SetColorCoding(COLORCODING_ORIENTATION_BASED);
    this->GenerateFiberIds();
}

mitk::FiberBundleX::~FiberBundleX()
{

}

mitk::FiberBundleX::Pointer mitk::FiberBundleX::GetDeepCopy()
{
    mitk::FiberBundleX::Pointer newFib = mitk::FiberBundleX::New(m_FiberPolyData);
    newFib->SetColorCoding(m_CurrentColorCoding);
    return newFib;
}

vtkSmartPointer<vtkPolyData> mitk::FiberBundleX::GeneratePolyDataByIds(std::vector<long> fiberIds)
{
    MITK_DEBUG << "\n=====FINAL RESULT: fib_id ======\n";
    MITK_DEBUG << "Number of new Fibers: " << fiberIds.size();
    // iterate through the vectorcontainer hosting all desired fiber Ids

    vtkSmartPointer<vtkPolyData> newFiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkCellArray> newLineSet = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPoints> newPointSet = vtkSmartPointer<vtkPoints>::New();

    // if FA array available, initialize fa double array
    // if color orient array is available init color array
    vtkSmartPointer<vtkDoubleArray> faValueArray;
    vtkSmartPointer<vtkUnsignedCharArray> colorsT;
    //colors and alpha value for each single point, RGBA = 4 components
    unsigned char rgba[4] = {0,0,0,0};
    int componentSize = sizeof(rgba);

    if (m_FiberIdDataSet->GetPointData()->HasArray(COLORCODING_FA_BASED)){
        MITK_DEBUG << "FA VALUES AVAILABLE, init array for new fiberbundle";
        faValueArray = vtkSmartPointer<vtkDoubleArray>::New();
    }

    if (m_FiberIdDataSet->GetPointData()->HasArray(COLORCODING_ORIENTATION_BASED)){
        MITK_DEBUG << "colorValues available, init array for new fiberbundle";
        colorsT = vtkUnsignedCharArray::New();
        colorsT->SetNumberOfComponents(componentSize);
        colorsT->SetName(COLORCODING_ORIENTATION_BASED);
    }



    std::vector<long>::iterator finIt = fiberIds.begin();
    while ( finIt != fiberIds.end() )
    {
        if (*finIt < 0 || *finIt>GetNumFibers()){
            MITK_INFO << "FiberID can not be negative or >NumFibers!!! check id Extraction!" << *finIt;
            break;
        }

        vtkSmartPointer<vtkCell> fiber = m_FiberIdDataSet->GetCell(*finIt);//->DeepCopy(fiber);

        vtkSmartPointer<vtkPoints> fibPoints = fiber->GetPoints();

        vtkSmartPointer<vtkPolyLine> newFiber = vtkSmartPointer<vtkPolyLine>::New();
        newFiber->GetPointIds()->SetNumberOfIds( fibPoints->GetNumberOfPoints() );

        for(int i=0; i<fibPoints->GetNumberOfPoints(); i++)
        {
            //            MITK_DEBUG << "id: " << fiber->GetPointId(i);
            //            MITK_DEBUG << fibPoints->GetPoint(i)[0] << " | " << fibPoints->GetPoint(i)[1] << " | " << fibPoints->GetPoint(i)[2];
            newFiber->GetPointIds()->SetId(i, newPointSet->GetNumberOfPoints());
            newPointSet->InsertNextPoint(fibPoints->GetPoint(i)[0], fibPoints->GetPoint(i)[1], fibPoints->GetPoint(i)[2]);


            if (m_FiberIdDataSet->GetPointData()->HasArray(COLORCODING_FA_BASED)){
                //                MITK_DEBUG << m_FiberIdDataSet->GetPointData()->GetArray(FA_VALUE_ARRAY)->GetTuple(fiber->GetPointId(i));
            }

            if (m_FiberIdDataSet->GetPointData()->HasArray(COLORCODING_ORIENTATION_BASED)){
                //                MITK_DEBUG << "ColorValue: " << m_FiberIdDataSet->GetPointData()->GetArray(COLORCODING_ORIENTATION_BASED)->GetTuple(fiber->GetPointId(i))[0];
            }
        }

        newLineSet->InsertNextCell(newFiber);
        ++finIt;
    }

    newFiberPolyData->SetPoints(newPointSet);
    newFiberPolyData->SetLines(newLineSet);
    MITK_DEBUG << "new fiberbundle polydata points: " << newFiberPolyData->GetNumberOfPoints();
    MITK_DEBUG << "new fiberbundle polydata lines: " << newFiberPolyData->GetNumberOfLines();
    MITK_DEBUG << "=====================\n";

    //    mitk::FiberBundleX::Pointer newFib = mitk::FiberBundleX::New(newFiberPolyData);
    return newFiberPolyData;
}

// merge two fiber bundles
mitk::FiberBundleX::Pointer mitk::FiberBundleX::AddBundle(mitk::FiberBundleX* fib)
{
    if (fib==NULL)
    {
        MITK_WARN << "trying to call AddBundle with NULL argument";
        return NULL;
    }

    vtkSmartPointer<vtkPolyData> vNewPolyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkCellArray> vNewLines = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPoints> vNewPoints = vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();

    // add current fiber bundle
    int numFibers = GetNumFibers();
    for( int i=0; i<numFibers; i++ )
    {
        vtkIdType   numPoints(0);
        vtkIdType*  points(NULL);
        vLines->GetNextCell ( numPoints, points );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for( int j=0; j<numPoints; j++)
        {
            vtkIdType id = vNewPoints->InsertNextPoint(m_FiberPolyData->GetPoint(points[j]));
            container->GetPointIds()->InsertNextId(id);
        }
        vNewLines->InsertNextCell(container);
    }

    vLines = fib->m_FiberPolyData->GetLines();
    vLines->InitTraversal();

    // add new fiber bundle
    numFibers = fib->GetNumFibers();
    for( int i=0; i<numFibers; i++ )
    {
        vtkIdType   numPoints(0);
        vtkIdType*  points(NULL);
        vLines->GetNextCell ( numPoints, points );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for( int j=0; j<numPoints; j++)
        {
            vtkIdType id = vNewPoints->InsertNextPoint(fib->m_FiberPolyData->GetPoint(points[j]));
            container->GetPointIds()->InsertNextId(id);
        }
        vNewLines->InsertNextCell(container);
    }

    // initialize polydata
    vNewPolyData->SetPoints(vNewPoints);
    vNewPolyData->SetLines(vNewLines);

    // initialize fiber bundle
    mitk::FiberBundleX::Pointer newFib = mitk::FiberBundleX::New(vNewPolyData);
    return newFib;
}

// subtract two fiber bundles
mitk::FiberBundleX::Pointer mitk::FiberBundleX::SubtractBundle(mitk::FiberBundleX* fib)
{

    vtkSmartPointer<vtkPolyData> vNewPolyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkCellArray> vNewLines = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPoints> vNewPoints = vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();

    // iterate over current fibers
    int numFibers = GetNumFibers();
    for( int i=0; i<numFibers; i++ )
    {
        vtkIdType   numPoints(0);
        vtkIdType*  points(NULL);
        vLines->GetNextCell ( numPoints, points );

        if (points==NULL)
            continue;

        vtkSmartPointer<vtkCellArray> vLines2 = fib->m_FiberPolyData->GetLines();
        vLines2->InitTraversal();
        int numFibers2 = fib->GetNumFibers();
        bool contained = false;

        for( int i2=0; i2<numFibers2; i2++ )
        {
            vtkIdType   numPoints2(0);
            vtkIdType*  points2(NULL);
            vLines2->GetNextCell ( numPoints2, points2 );

            if (points2==NULL)
                continue;

            // check endpoints
            itk::Point<float, 3> point_start = GetItkPoint(m_FiberPolyData->GetPoint(points[0]));
            itk::Point<float, 3> point_end = GetItkPoint(m_FiberPolyData->GetPoint(points[numPoints-1]));
            itk::Point<float, 3> point2_start = GetItkPoint(fib->m_FiberPolyData->GetPoint(points2[0]));
            itk::Point<float, 3> point2_end = GetItkPoint(fib->m_FiberPolyData->GetPoint(points2[numPoints2-1]));

            if (point_start.SquaredEuclideanDistanceTo(point2_start)<=mitk::eps && point_end.SquaredEuclideanDistanceTo(point2_end)<=mitk::eps ||
                    point_start.SquaredEuclideanDistanceTo(point2_end)<=mitk::eps && point_end.SquaredEuclideanDistanceTo(point2_start)<=mitk::eps)
            {
                // further checking ???
                if (numPoints2==numPoints)
                    contained = true;
            }
        }

        // add to result because fiber is not subtracted
        if (!contained)
        {
            vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
            for( int j=0; j<numPoints; j++)
            {
                vtkIdType id = vNewPoints->InsertNextPoint(m_FiberPolyData->GetPoint(points[j]));
                container->GetPointIds()->InsertNextId(id);
            }
            vNewLines->InsertNextCell(container);
        }
    }
    if(vNewLines->GetNumberOfCells()==0)
        return NULL;
    // initialize polydata
    vNewPolyData->SetPoints(vNewPoints);
    vNewPolyData->SetLines(vNewLines);

    // initialize fiber bundle
    mitk::FiberBundleX::Pointer newFib = mitk::FiberBundleX::New(vNewPolyData);
    return newFib;
}

itk::Point<float, 3> mitk::FiberBundleX::GetItkPoint(double point[3])
{
    itk::Point<float, 3> itkPoint;
    itkPoint[0] = point[0];
    itkPoint[1] = point[1];
    itkPoint[2] = point[2];
    return itkPoint;
}

/*
 * set polydata (additional flag to recompute fiber geometry, default = true)
 */
void mitk::FiberBundleX::SetFiberPolyData(vtkSmartPointer<vtkPolyData> fiberPD, bool updateGeometry)
{
    if (fiberPD == NULL)
        this->m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    else
    {
        m_FiberPolyData->DeepCopy(fiberPD);
        DoColorCodingOrientationBased();
    }

    m_NumFibers = m_FiberPolyData->GetNumberOfLines();

    if (updateGeometry)
        UpdateFiberGeometry();
    SetColorCoding(COLORCODING_ORIENTATION_BASED);
    GenerateFiberIds();
}

/*
 * return vtkPolyData
 */
vtkSmartPointer<vtkPolyData> mitk::FiberBundleX::GetFiberPolyData()
{
    return m_FiberPolyData;
}

void mitk::FiberBundleX::DoColorCodingOrientationBased()
{
    //===== FOR WRITING A TEST ========================
    //  colorT size == tupelComponents * tupelElements
    //  compare color results
    //  to cover this code 100% also polydata needed, where colorarray already exists
    //  + one fiber with exactly 1 point
    //  + one fiber with 0 points
    //=================================================


    /*  make sure that processing colorcoding is only called when necessary */
    if ( m_FiberPolyData->GetPointData()->HasArray(COLORCODING_ORIENTATION_BASED) &&
         m_FiberPolyData->GetNumberOfPoints() ==
         m_FiberPolyData->GetPointData()->GetArray(COLORCODING_ORIENTATION_BASED)->GetNumberOfTuples() )
    {
        // fiberstructure is already colorcoded
        MITK_DEBUG << " NO NEED TO REGENERATE COLORCODING! " ;
        this->ResetFiberOpacity();
        this->SetColorCoding(COLORCODING_ORIENTATION_BASED);
        return;
    }

    /* Finally, execute color calculation */
    vtkPoints* extrPoints = NULL;
    extrPoints = m_FiberPolyData->GetPoints();
    int numOfPoints = 0;
    if (extrPoints!=NULL)
        numOfPoints = extrPoints->GetNumberOfPoints();

    //colors and alpha value for each single point, RGBA = 4 components
    unsigned char rgba[4] = {0,0,0,0};
    //    int componentSize = sizeof(rgba);
    int componentSize = 4;

    vtkSmartPointer<vtkUnsignedCharArray> colorsT = vtkSmartPointer<vtkUnsignedCharArray>::New();
    colorsT->Allocate(numOfPoints * componentSize);
    colorsT->SetNumberOfComponents(componentSize);
    colorsT->SetName(COLORCODING_ORIENTATION_BASED);



    /* checkpoint: does polydata contain any fibers */
    int numOfFibers = m_FiberPolyData->GetNumberOfLines();
    if (numOfFibers < 1) {
        MITK_DEBUG << "\n ========= Number of Fibers is 0 and below ========= \n";
        return;
    }


    /* extract single fibers of fiberBundle */
    vtkCellArray* fiberList = m_FiberPolyData->GetLines();
    fiberList->InitTraversal();
    for (int fi=0; fi<numOfFibers; ++fi) {

        vtkIdType* idList; // contains the point id's of the line
        vtkIdType pointsPerFiber; // number of points for current line
        fiberList->GetNextCell(pointsPerFiber, idList);

        //    MITK_DEBUG << "Fib#: " << fi << " of " << numOfFibers << " pnts in fiber: " << pointsPerFiber ;

        /* single fiber checkpoints: is number of points valid */
        if (pointsPerFiber > 1)
        {
            /* operate on points of single fiber */
            for (int i=0; i <pointsPerFiber; ++i)
            {
                /* process all points except starting and endpoint
         * for calculating color value take current point, previous point and next point */
                if (i<pointsPerFiber-1 && i > 0)
                {
                    /* The color value of the current point is influenced by the previous point and next point. */
                    vnl_vector_fixed< double, 3 > currentPntvtk(extrPoints->GetPoint(idList[i])[0], extrPoints->GetPoint(idList[i])[1],extrPoints->GetPoint(idList[i])[2]);
                    vnl_vector_fixed< double, 3 > nextPntvtk(extrPoints->GetPoint(idList[i+1])[0], extrPoints->GetPoint(idList[i+1])[1], extrPoints->GetPoint(idList[i+1])[2]);
                    vnl_vector_fixed< double, 3 > prevPntvtk(extrPoints->GetPoint(idList[i-1])[0], extrPoints->GetPoint(idList[i-1])[1], extrPoints->GetPoint(idList[i-1])[2]);

                    vnl_vector_fixed< double, 3 > diff1;
                    diff1 = currentPntvtk - nextPntvtk;

                    vnl_vector_fixed< double, 3 > diff2;
                    diff2 = currentPntvtk - prevPntvtk;

                    vnl_vector_fixed< double, 3 > diff;
                    diff = (diff1 - diff2) / 2.0;
                    diff.normalize();

                    rgba[0] = (unsigned char) (255.0 * std::fabs(diff[0]));
                    rgba[1] = (unsigned char) (255.0 * std::fabs(diff[1]));
                    rgba[2] = (unsigned char) (255.0 * std::fabs(diff[2]));
                    rgba[3] = (unsigned char) (255.0);


                } else if (i==0) {
                    /* First point has no previous point, therefore only diff1 is taken */

                    vnl_vector_fixed< double, 3 > currentPntvtk(extrPoints->GetPoint(idList[i])[0], extrPoints->GetPoint(idList[i])[1],extrPoints->GetPoint(idList[i])[2]);
                    vnl_vector_fixed< double, 3 > nextPntvtk(extrPoints->GetPoint(idList[i+1])[0], extrPoints->GetPoint(idList[i+1])[1], extrPoints->GetPoint(idList[i+1])[2]);

                    vnl_vector_fixed< double, 3 > diff1;
                    diff1 = currentPntvtk - nextPntvtk;
                    diff1.normalize();

                    rgba[0] = (unsigned char) (255.0 * std::fabs(diff1[0]));
                    rgba[1] = (unsigned char) (255.0 * std::fabs(diff1[1]));
                    rgba[2] = (unsigned char) (255.0 * std::fabs(diff1[2]));
                    rgba[3] = (unsigned char) (255.0);


                } else if (i==pointsPerFiber-1) {
                    /* Last point has no next point, therefore only diff2 is taken */
                    vnl_vector_fixed< double, 3 > currentPntvtk(extrPoints->GetPoint(idList[i])[0], extrPoints->GetPoint(idList[i])[1],extrPoints->GetPoint(idList[i])[2]);
                    vnl_vector_fixed< double, 3 > prevPntvtk(extrPoints->GetPoint(idList[i-1])[0], extrPoints->GetPoint(idList[i-1])[1], extrPoints->GetPoint(idList[i-1])[2]);

                    vnl_vector_fixed< double, 3 > diff2;
                    diff2 = currentPntvtk - prevPntvtk;
                    diff2.normalize();

                    rgba[0] = (unsigned char) (255.0 * std::fabs(diff2[0]));
                    rgba[1] = (unsigned char) (255.0 * std::fabs(diff2[1]));
                    rgba[2] = (unsigned char) (255.0 * std::fabs(diff2[2]));
                    rgba[3] = (unsigned char) (255.0);

                }

                colorsT->InsertTupleValue(idList[i], rgba);

            } //end for loop

        } else if (pointsPerFiber == 1) {
            /* a single point does not define a fiber (use vertex mechanisms instead */
            continue;
            //      colorsT->InsertTupleValue(0, rgba);

        } else {
            MITK_DEBUG << "Fiber with 0 points detected... please check your tractography algorithm!" ;
            continue;

        }


    }//end for loop

    m_FiberPolyData->GetPointData()->AddArray(colorsT);

    /*=========================
      - this is more relevant for renderer than for fiberbundleX datastructure
      - think about sourcing this to a explicit method which coordinates colorcoding */
    this->SetColorCoding(COLORCODING_ORIENTATION_BASED);
    //  ===========================

    //mini test, shall be ported to MITK TESTINGS!
    if (colorsT->GetSize() != numOfPoints*componentSize)
        MITK_DEBUG << "ALLOCATION ERROR IN INITIATING COLOR ARRAY";


}

void mitk::FiberBundleX::DoColorCodingFaBased()
{
    if(m_FiberPolyData->GetPointData()->HasArray(COLORCODING_FA_BASED) != 1 )
        return;

    this->SetColorCoding(COLORCODING_FA_BASED);
    MITK_DEBUG << "FBX: done CC FA based";
    this->GenerateFiberIds();
}

void mitk::FiberBundleX::DoUseFaFiberOpacity()
{
    if(m_FiberPolyData->GetPointData()->HasArray(COLORCODING_FA_BASED) != 1 )
        return;

    if(m_FiberPolyData->GetPointData()->HasArray(COLORCODING_ORIENTATION_BASED) != 1 )
        return;

    vtkDoubleArray* FAValArray = (vtkDoubleArray*) m_FiberPolyData->GetPointData()->GetArray(COLORCODING_FA_BASED);
    vtkUnsignedCharArray* ColorArray = dynamic_cast<vtkUnsignedCharArray*>  (m_FiberPolyData->GetPointData()->GetArray(COLORCODING_ORIENTATION_BASED));

    for(long i=0; i<ColorArray->GetNumberOfTuples(); i++) {
        double faValue = FAValArray->GetValue(i);
        faValue = faValue * 255.0;
        ColorArray->SetComponent(i,3, (unsigned char) faValue );
    }

    this->SetColorCoding(COLORCODING_ORIENTATION_BASED);
    MITK_DEBUG << "FBX: done CC OPACITY";
    this->GenerateFiberIds();
}

void mitk::FiberBundleX::ResetFiberOpacity() {
    vtkUnsignedCharArray* ColorArray = dynamic_cast<vtkUnsignedCharArray*>  (m_FiberPolyData->GetPointData()->GetArray(COLORCODING_ORIENTATION_BASED));
    if (ColorArray==NULL)
        return;
    for(long i=0; i<ColorArray->GetNumberOfTuples(); i++)
        ColorArray->SetComponent(i,3, 255.0 );
}

void mitk::FiberBundleX::SetFAMap(mitk::Image::Pointer FAimage)
{
    MITK_DEBUG << "SetFAMap";
    vtkSmartPointer<vtkDoubleArray> faValues = vtkSmartPointer<vtkDoubleArray>::New();
    faValues->SetName(COLORCODING_FA_BASED);
    faValues->Allocate(m_FiberPolyData->GetNumberOfPoints());
    faValues->SetNumberOfValues(m_FiberPolyData->GetNumberOfPoints());

    vtkPoints* pointSet = m_FiberPolyData->GetPoints();
    for(long i=0; i<m_FiberPolyData->GetNumberOfPoints(); ++i)
    {
        Point3D px;
        px[0] = pointSet->GetPoint(i)[0];
        px[1] = pointSet->GetPoint(i)[1];
        px[2] = pointSet->GetPoint(i)[2];
        double faPixelValue = 1-FAimage->GetPixelValueByWorldCoordinate(px);
        faValues->InsertValue(i, faPixelValue);
    }

    m_FiberPolyData->GetPointData()->AddArray(faValues);
    this->GenerateFiberIds();

    if(m_FiberPolyData->GetPointData()->HasArray(COLORCODING_FA_BASED))
        MITK_DEBUG << "FA VALUE ARRAY SET";
}

void mitk::FiberBundleX::GenerateFiberIds()
{
    if (m_FiberPolyData == NULL)
        return;

    vtkSmartPointer<vtkIdFilter> idFiberFilter = vtkSmartPointer<vtkIdFilter>::New();
    idFiberFilter->SetInput(m_FiberPolyData);
    idFiberFilter->CellIdsOn();
    //  idFiberFilter->PointIdsOn(); // point id's are not needed
    idFiberFilter->SetIdsArrayName(FIBER_ID_ARRAY);
    idFiberFilter->FieldDataOn();
    idFiberFilter->Update();

    m_FiberIdDataSet = idFiberFilter->GetOutput();

    MITK_DEBUG << "Generating Fiber Ids...[done] | " << m_FiberIdDataSet->GetNumberOfCells();

}

mitk::FiberBundleX::Pointer mitk::FiberBundleX::ExtractFiberSubset(ItkUcharImgType* mask, bool anyPoint)
{
    vtkSmartPointer<vtkPolyData> polyData = this->GetFiberPolyData();
    if (anyPoint)
    {
        float minSpacing = 1;
        if(mask->GetSpacing()[0]<mask->GetSpacing()[1] && mask->GetSpacing()[0]<mask->GetSpacing()[2])
            minSpacing = mask->GetSpacing()[0];
        else if (mask->GetSpacing()[1] < mask->GetSpacing()[2])
            minSpacing = mask->GetSpacing()[1];
        else
            minSpacing = mask->GetSpacing()[2];

        mitk::FiberBundleX::Pointer fibCopy = this->GetDeepCopy();
        fibCopy->ResampleFibers(minSpacing/2);
        polyData = fibCopy->GetFiberPolyData();
    }
    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> vLines = polyData->GetLines();
    vLines->InitTraversal();

    MITK_INFO << "Extracting fibers";
    boost::progress_display disp(m_NumFibers);
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();

        if (numPoints>1)
        {
            if (anyPoint)
            {
                for (int j=0; j<numPoints; j++)
                {
                    double* p = polyData->GetPoint(pointIds[j]);

                    itk::Point<float, 3> itkP;
                    itkP[0] = p[0]; itkP[1] = p[1]; p[2] = p[2];
                    itk::Index<3> idx;
                    mask->TransformPhysicalPointToIndex(itkP, idx);
                    if ( mask->GetPixel(idx)>0 )
                    {
                        for (int j=0; j<numPoints; j++)
                        {
                            double* p = polyData->GetPoint(pointIds[j]);
                            vtkIdType id = vtkNewPoints->InsertNextPoint(p);
                            container->GetPointIds()->InsertNextId(id);
                        }
                        break;
                    }
                }
            }
            else
            {
                double* start = polyData->GetPoint(pointIds[0]);
                itk::Point<float, 3> itkStart;
                itkStart[0] = start[0]; itkStart[1] = start[1]; itkStart[2] = start[2];
                itk::Index<3> idxStart;
                mask->TransformPhysicalPointToIndex(itkStart, idxStart);

                double* end = polyData->GetPoint(pointIds[numPoints-1]);
                itk::Point<float, 3> itkEnd;
                itkEnd[0] = end[0]; itkEnd[1] = end[1]; itkEnd[2] = end[2];
                itk::Index<3> idxEnd;
                mask->TransformPhysicalPointToIndex(itkEnd, idxEnd);

                if ( mask->GetPixel(idxStart)>0 && mask->GetPixel(idxEnd)>0 )
                {
                    for (int j=0; j<numPoints; j++)
                    {
                        double* p = polyData->GetPoint(pointIds[j]);
                        vtkIdType id = vtkNewPoints->InsertNextPoint(p);
                        container->GetPointIds()->InsertNextId(id);
                    }
                }
            }
        }

        vtkNewCells->InsertNextCell(container);
    }

    if (vtkNewCells->GetNumberOfCells()<=0)
        return NULL;

    vtkSmartPointer<vtkPolyData> newPolyData = vtkSmartPointer<vtkPolyData>::New();
    newPolyData->SetPoints(vtkNewPoints);
    newPolyData->SetLines(vtkNewCells);
    return mitk::FiberBundleX::New(newPolyData);
}

mitk::FiberBundleX::Pointer mitk::FiberBundleX::ExtractFiberSubset(mitk::PlanarFigure* pf)
{
    if (pf==NULL)
        return NULL;

    std::vector<long> tmp = ExtractFiberIdSubset(pf);

    if (tmp.size()<=0)
        return mitk::FiberBundleX::New();
    vtkSmartPointer<vtkPolyData> pTmp = GeneratePolyDataByIds(tmp);
    return mitk::FiberBundleX::New(pTmp);
}

std::vector<long> mitk::FiberBundleX::ExtractFiberIdSubset(mitk::PlanarFigure* pf)
{
    MITK_DEBUG << "Extracting fibers!";
    // vector which is returned, contains all extracted FiberIds
    std::vector<long> FibersInROI;

    if (pf==NULL)
        return FibersInROI;

    /* Handle type of planarfigure */
    // if incoming pf is a pfc
    mitk::PlanarFigureComposite::Pointer pfcomp= dynamic_cast<mitk::PlanarFigureComposite*>(pf);
    if (!pfcomp.IsNull()) {
        // process requested boolean operation of PFC
        switch (pfcomp->getOperationType()) {
        case 0:
        {
            MITK_DEBUG << "AND PROCESSING";
            //AND
            //temporarly store results of the child in this vector, we need that to accumulate the
            std::vector<long> childResults = this->ExtractFiberIdSubset(pfcomp->getChildAt(0));
            MITK_DEBUG << "first roi got fibers in ROI: " << childResults.size();
            MITK_DEBUG << "sorting...";
            std::sort(childResults.begin(), childResults.end());
            MITK_DEBUG << "sorting done";
            std::vector<long> AND_Assamblage(childResults.size());
            //std::vector<unsigned long> AND_Assamblage;
            fill(AND_Assamblage.begin(), AND_Assamblage.end(), -1);
            //AND_Assamblage.reserve(childResults.size()); //max size AND can reach anyway

            std::vector<long>::iterator it;
            for (int i=1; i<pfcomp->getNumberOfChildren(); ++i)
            {
                std::vector<long> tmpChild = this->ExtractFiberIdSubset(pfcomp->getChildAt(i));
                MITK_DEBUG << "ROI " << i << " has fibers in ROI: " << tmpChild.size();
                sort(tmpChild.begin(), tmpChild.end());

                it = std::set_intersection(childResults.begin(), childResults.end(),
                                           tmpChild.begin(), tmpChild.end(),
                                           AND_Assamblage.begin() );
            }

            MITK_DEBUG << "resize Vector";
            long i=0;
            while (i < AND_Assamblage.size() && AND_Assamblage[i] != -1){ //-1 represents a placeholder in the array
                ++i;
            }
            AND_Assamblage.resize(i);

            MITK_DEBUG << "returning AND vector, size: " << AND_Assamblage.size();
            return AND_Assamblage;
            //            break;

        }
        case 1:
        {
            //OR
            std::vector<long> OR_Assamblage = this->ExtractFiberIdSubset(pfcomp->getChildAt(0));
            std::vector<long>::iterator it;
            MITK_DEBUG << OR_Assamblage.size();

            for (int i=1; i<pfcomp->getNumberOfChildren(); ++i) {
                it = OR_Assamblage.end();
                std::vector<long> tmpChild = this->ExtractFiberIdSubset(pfcomp->getChildAt(i));
                OR_Assamblage.insert(it, tmpChild.begin(), tmpChild.end());
                MITK_DEBUG << "ROI " << i << " has fibers in ROI: " << tmpChild.size() << " OR Assamblage: " << OR_Assamblage.size();
            }

            sort(OR_Assamblage.begin(), OR_Assamblage.end());
            it = unique(OR_Assamblage.begin(), OR_Assamblage.end());
            OR_Assamblage.resize( it - OR_Assamblage.begin() );
            MITK_DEBUG << "returning OR vector, size: " << OR_Assamblage.size();

            return OR_Assamblage;
        }
        case 2:
        {
            //NOT
            //get IDs of all fibers
            std::vector<long> childResults;
            childResults.reserve(this->GetNumFibers());
            vtkSmartPointer<vtkDataArray> idSet = m_FiberIdDataSet->GetCellData()->GetArray(FIBER_ID_ARRAY);
            MITK_DEBUG << "m_NumOfFib: " << this->GetNumFibers() << " cellIdNum: " << idSet->GetNumberOfTuples();
            for(long i=0; i<this->GetNumFibers(); i++)
            {
                MITK_DEBUG << "i: " << i << " idset: " << idSet->GetTuple(i)[0];
                childResults.push_back(idSet->GetTuple(i)[0]);
            }

            std::sort(childResults.begin(), childResults.end());
            std::vector<long> NOT_Assamblage(childResults.size());
            //fill it with -1, otherwise 0 will be stored and 0 can also be an ID of fiber!
            fill(NOT_Assamblage.begin(), NOT_Assamblage.end(), -1);
            std::vector<long>::iterator it;

            for (long i=0; i<pfcomp->getNumberOfChildren(); ++i)
            {
                std::vector<long> tmpChild = ExtractFiberIdSubset(pfcomp->getChildAt(i));
                sort(tmpChild.begin(), tmpChild.end());

                it = std::set_difference(childResults.begin(), childResults.end(),
                                         tmpChild.begin(), tmpChild.end(),
                                         NOT_Assamblage.begin() );

            }

            MITK_DEBUG << "resize Vector";
            long i=0;
            while (NOT_Assamblage[i] != -1){ //-1 represents a placeholder in the array
                ++i;
            }
            NOT_Assamblage.resize(i);

            return NOT_Assamblage;
        }
        default:
            MITK_DEBUG << "we have an UNDEFINED composition... ERROR" ;
            break;

        }
    }
    else
    {
        mitk::Geometry2D::ConstPointer pfgeometry = pf->GetGeometry2D();
        const mitk::PlaneGeometry* planeGeometry = dynamic_cast<const mitk::PlaneGeometry*> (pfgeometry.GetPointer());
        Vector3D planeNormal = planeGeometry->GetNormal();
        planeNormal.Normalize();
        Point3D planeOrigin = planeGeometry->GetOrigin();

        MITK_DEBUG << "planeOrigin: " << planeOrigin[0] << " | " << planeOrigin[1] << " | " << planeOrigin[2] << endl;
        MITK_DEBUG << "planeNormal: " << planeNormal[0] << " | " << planeNormal[1] << " | " << planeNormal[2] << endl;

        std::vector<int> PointsOnPlane; // contains all pointIds which are crossing the cutting plane
        std::vector<int> PointsInROI; // based on PointsOnPlane, all ROI relevant point IDs are stored here

        /* Define cutting plane by ROI (PlanarFigure) */
        vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
        plane->SetOrigin(planeOrigin[0],planeOrigin[1],planeOrigin[2]);
        plane->SetNormal(planeNormal[0],planeNormal[1],planeNormal[2]);

        /* get all points/fibers cutting the plane */
        MITK_DEBUG << "start clipping";
        vtkSmartPointer<vtkClipPolyData> clipper = vtkSmartPointer<vtkClipPolyData>::New();
        clipper->SetInput(m_FiberIdDataSet);
        clipper->SetClipFunction(plane);
        clipper->GenerateClipScalarsOn();
        clipper->GenerateClippedOutputOn();
        vtkSmartPointer<vtkPolyData> clipperout = clipper->GetClippedOutput();
        MITK_DEBUG << "end clipping";

        MITK_DEBUG << "init and update clipperoutput";
        clipperout->GetPointData()->Initialize();
        clipperout->Update();
        MITK_DEBUG << "init and update clipperoutput completed";

        MITK_DEBUG << "STEP 1: find all points which have distance 0 to the given plane";
        /*======STEP 1======
      * extract all points, which are crossing the plane */
        // Scalar values describe the distance between each remaining point to the given plane. Values sorted by point index
        vtkSmartPointer<vtkDataArray> distanceList = clipperout->GetPointData()->GetScalars();
        vtkIdType sizeOfList =  distanceList->GetNumberOfTuples();
        PointsOnPlane.reserve(sizeOfList); /* use reserve for high-performant push_back, no hidden copy procedures are processed then!
                                         * size of list can be optimized by reducing allocation, but be aware of iterator and vector size*/

        for (int i=0; i<sizeOfList; ++i) {
            double *distance = distanceList->GetTuple(i);

            // check if point is on plane.
            // 0.01 due to some approximation errors when calculating distance
            if (distance[0] >= -0.01 && distance[0] <= 0.01)
                PointsOnPlane.push_back(i);
        }


        MITK_DEBUG << "Num Of points on plane: " <<  PointsOnPlane.size();

        MITK_DEBUG << "Step 2: extract Interesting points with respect to given extraction planarFigure";

        PointsInROI.reserve(PointsOnPlane.size());
        /*=======STEP 2=====
     * extract ROI relevant pointIds */

        mitk::PlanarCircle::Pointer circleName = mitk::PlanarCircle::New();
        mitk::PlanarPolygon::Pointer polyName = mitk::PlanarPolygon::New();
        if ( pf->GetNameOfClass() == circleName->GetNameOfClass() )
        {
            //calculate circle radius
            mitk::Point3D V1w = pf->GetWorldControlPoint(0); //centerPoint
            mitk::Point3D V2w  = pf->GetWorldControlPoint(1); //radiusPoint

            double distPF = V1w.EuclideanDistanceTo(V2w);

            for (int i=0; i<PointsOnPlane.size(); i++)
            {
                //distance between circle radius and given point
                double XdistPnt =  sqrt((double) (clipperout->GetPoint(PointsOnPlane[i])[0] - V1w[0]) * (clipperout->GetPoint(PointsOnPlane[i])[0] - V1w[0]) +
                                        (clipperout->GetPoint(PointsOnPlane[i])[1] - V1w[1]) * (clipperout->GetPoint(PointsOnPlane[i])[1] - V1w[1]) +
                                        (clipperout->GetPoint(PointsOnPlane[i])[2] - V1w[2]) * (clipperout->GetPoint(PointsOnPlane[i])[2] - V1w[2])) ;

                if( XdistPnt <= distPF)
                    PointsInROI.push_back(PointsOnPlane[i]);
            }
        }
        else if ( pf->GetNameOfClass() == polyName->GetNameOfClass() )
        {
            //create vtkPolygon using controlpoints from planarFigure polygon
            vtkSmartPointer<vtkPolygon> polygonVtk = vtkSmartPointer<vtkPolygon>::New();

            //get the control points from pf and insert them to vtkPolygon
            unsigned int nrCtrlPnts = pf->GetNumberOfControlPoints();

            for (int i=0; i<nrCtrlPnts; ++i)
            {
                polygonVtk->GetPoints()->InsertNextPoint((double)pf->GetWorldControlPoint(i)[0], (double)pf->GetWorldControlPoint(i)[1], (double)pf->GetWorldControlPoint(i)[2] );
            }

            //prepare everything for using pointInPolygon function
            double n[3];
            polygonVtk->ComputeNormal(polygonVtk->GetPoints()->GetNumberOfPoints(),
                                      static_cast<double*>(polygonVtk->GetPoints()->GetData()->GetVoidPointer(0)), n);

            double bounds[6];
            polygonVtk->GetPoints()->GetBounds(bounds);

            for (int i=0; i<PointsOnPlane.size(); i++)
            {
                double checkIn[3] = {clipperout->GetPoint(PointsOnPlane[i])[0], clipperout->GetPoint(PointsOnPlane[i])[1], clipperout->GetPoint(PointsOnPlane[i])[2]};
                int isInPolygon = polygonVtk->PointInPolygon(checkIn, polygonVtk->GetPoints()->GetNumberOfPoints()
                                                             , static_cast<double*>(polygonVtk->GetPoints()->GetData()->GetVoidPointer(0)), bounds, n);
                if( isInPolygon )
                    PointsInROI.push_back(PointsOnPlane[i]);
            }
        }

        MITK_DEBUG << "Step3: Identify fibers";
        // we need to access the fiberId Array, so make sure that this array is available
        if (!clipperout->GetCellData()->HasArray(FIBER_ID_ARRAY))
        {
            MITK_DEBUG << "ERROR: FiberID array does not exist, no correlation between points and fiberIds possible! Make sure calling GenerateFiberIds()";
            return FibersInROI; // FibersInRoi is empty then
        }
        if (PointsInROI.size()<=0)
            return FibersInROI;

        // prepare a structure where each point id is represented as an indexId.
        // vector looks like: | pntId | fiberIdx |
        std::vector< long > pointindexFiberMap;

        // walk through the whole subline section and create an vector sorted by point index
        vtkCellArray *clipperlines = clipperout->GetLines();
        clipperlines->InitTraversal();
        long numOfLineCells = clipperlines->GetNumberOfCells();
        long numofClippedPoints = clipperout->GetNumberOfPoints();
        pointindexFiberMap.resize(numofClippedPoints);


        //prepare resulting vector
        FibersInROI.reserve(PointsInROI.size());

        MITK_DEBUG << "\n===== Pointindex based structure initialized ======\n";

        // go through resulting "sub"lines which are stored as cells, "i" corresponds to current line id.
        for (int i=0, ic=0 ; i<numOfLineCells; i++, ic+=3)
        { //ic is the index counter for the cells hosting the desired information, eg. 2 | 45 | 46. each cell consits of 3 items.

            vtkIdType npts;
            vtkIdType *pts;
            clipperlines->GetCell(ic, npts, pts);

            // go through point ids in hosting subline, "j" corresponds to current pointindex in current line i. eg. idx[0]=45; idx[1]=46
            for (long j=0; j<npts; j++)
            {
                // MITK_DEBUG << "writing fiber id: " << clipperout->GetCellData()->GetArray(FIBER_ID_ARRAY)->GetTuple(i)[0] << " to pointId: " << pts[j];
                pointindexFiberMap[ pts[j] ] = clipperout->GetCellData()->GetArray(FIBER_ID_ARRAY)->GetTuple(i)[0];
                // MITK_DEBUG << "in array: " << pointindexFiberMap[ pts[j] ];
            }

        }

        MITK_DEBUG << "\n===== Pointindex based structure finalized ======\n";

        // get all Points in ROI with according fiberID
        for (long k = 0; k < PointsInROI.size(); k++)
        {
            //MITK_DEBUG << "point " << PointsInROI[k] << " belongs to fiber " << pointindexFiberMap[ PointsInROI[k] ];
            if (pointindexFiberMap[ PointsInROI[k] ]<=GetNumFibers() && pointindexFiberMap[ PointsInROI[k] ]>=0)
                FibersInROI.push_back(pointindexFiberMap[ PointsInROI[k] ]);
            else
                MITK_INFO << "ERROR in ExtractFiberIdSubset; impossible fiber id detected";
        }

        m_PointsRoi = PointsInROI;

    }

    //  detecting fiberId duplicates
    MITK_DEBUG << "check for duplicates";

    sort(FibersInROI.begin(), FibersInROI.end());
    bool hasDuplicats = false;
    for(long i=0; i<FibersInROI.size()-1; ++i)
    {
        if(FibersInROI[i] == FibersInROI[i+1])
            hasDuplicats = true;
    }

    if(hasDuplicats)
    {
        std::vector<long>::iterator it;
        it = unique (FibersInROI.begin(), FibersInROI.end());
        FibersInROI.resize( it - FibersInROI.begin() );
    }

    return FibersInROI;
}

void mitk::FiberBundleX::UpdateFiberGeometry()
{
    vtkSmartPointer<vtkCleanPolyData> cleaner = vtkSmartPointer<vtkCleanPolyData>::New();
    cleaner->SetInput(m_FiberPolyData);
    cleaner->PointMergingOff();
    cleaner->Update();
    m_FiberPolyData = cleaner->GetOutput();

    m_FiberLengths.clear();
    m_MeanFiberLength = 0;
    m_MedianFiberLength = 0;
    m_LengthStDev = 0;
    m_NumFibers = m_FiberPolyData->GetNumberOfLines();

    if (m_NumFibers<=0) // no fibers present; apply default geometry
    {
        m_MinFiberLength = 0;
        m_MaxFiberLength = 0;
        mitk::Geometry3D::Pointer geometry = mitk::Geometry3D::New();
        geometry->SetImageGeometry(true);
        float b[] = {0, 1, 0, 1, 0, 1};
        geometry->SetFloatBounds(b);
        SetGeometry(geometry);
        return;
    }
    float min = itk::NumericTraits<float>::NonpositiveMin();
    float max = itk::NumericTraits<float>::max();
    float b[] = {max, min, max, min, max, min};

    vtkCellArray* cells = m_FiberPolyData->GetLines();
    cells->InitTraversal();
    for (int i=0; i<m_FiberPolyData->GetNumberOfCells(); i++)
    {
        vtkCell* cell = m_FiberPolyData->GetCell(i);
        int p = cell->GetNumberOfPoints();
        vtkPoints* points = cell->GetPoints();
        float length = 0;
        for (int j=0; j<p; j++)
        {
            // calculate bounding box
            double p1[3];
            points->GetPoint(j, p1);

            if (p1[0]<b[0])
                b[0]=p1[0];
            if (p1[0]>b[1])
                b[1]=p1[0];

            if (p1[1]<b[2])
                b[2]=p1[1];
            if (p1[1]>b[3])
                b[3]=p1[1];

            if (p1[2]<b[4])
                b[4]=p1[2];
            if (p1[2]>b[5])
                b[5]=p1[2];

            // calculate statistics
            if (j<p-1)
            {
                double p2[3];
                points->GetPoint(j+1, p2);

                float dist = std::sqrt((p1[0]-p2[0])*(p1[0]-p2[0])+(p1[1]-p2[1])*(p1[1]-p2[1])+(p1[2]-p2[2])*(p1[2]-p2[2]));
                length += dist;
            }
        }
        m_FiberLengths.push_back(length);
        m_MeanFiberLength += length;
        if (i==0)
        {
            m_MinFiberLength = length;
            m_MaxFiberLength = length;
        }
        else
        {
            if (length<m_MinFiberLength)
                m_MinFiberLength = length;
            if (length>m_MaxFiberLength)
                m_MaxFiberLength = length;
        }
    }
    m_MeanFiberLength /= m_NumFibers;

    std::vector< float > sortedLengths = m_FiberLengths;
    std::sort(sortedLengths.begin(), sortedLengths.end());
    for (int i=0; i<m_NumFibers; i++)
        m_LengthStDev += (m_MeanFiberLength-sortedLengths.at(i))*(m_MeanFiberLength-sortedLengths.at(i));
    if (m_NumFibers>1)
        m_LengthStDev /= (m_NumFibers-1);
    else
        m_LengthStDev = 0;
    m_LengthStDev = std::sqrt(m_LengthStDev);
    m_MedianFiberLength = sortedLengths.at(m_NumFibers/2);

    // provide some border margin
    for(int i=0; i<=4; i+=2)
        b[i] -=10;
    for(int i=1; i<=5; i+=2)
        b[i] +=10;

    mitk::Geometry3D::Pointer geometry = mitk::Geometry3D::New();
    geometry->SetFloatBounds(b);
    this->SetGeometry(geometry);
}

QStringList mitk::FiberBundleX::GetAvailableColorCodings()
{
    QStringList availableColorCodings;
    int numColors = m_FiberPolyData->GetPointData()->GetNumberOfArrays();
    for(int i=0; i<numColors; i++)
    {
        availableColorCodings.append(m_FiberPolyData->GetPointData()->GetArrayName(i));
    }

    //this controlstructure shall be implemented by the calling method
    if (availableColorCodings.isEmpty())
        MITK_DEBUG << "no colorcodings available in fiberbundleX";

    return availableColorCodings;
}


char* mitk::FiberBundleX::GetCurrentColorCoding()
{
    return m_CurrentColorCoding;
}

void mitk::FiberBundleX::SetColorCoding(const char* requestedColorCoding)
{

    if (requestedColorCoding==NULL)
        return;
    MITK_DEBUG << "SetColorCoding:" << requestedColorCoding;

    if( strcmp (COLORCODING_ORIENTATION_BASED,requestedColorCoding) == 0 )    {
        this->m_CurrentColorCoding = (char*) COLORCODING_ORIENTATION_BASED;

    } else if( strcmp (COLORCODING_FA_BASED,requestedColorCoding) == 0 ) {
        this->m_CurrentColorCoding = (char*) COLORCODING_FA_BASED;

    } else if( strcmp (COLORCODING_CUSTOM,requestedColorCoding) == 0 ) {
        this->m_CurrentColorCoding = (char*) COLORCODING_CUSTOM;

    } else {
        MITK_DEBUG << "FIBERBUNDLE X: UNKNOWN COLORCODING in FIBERBUNDLEX Datastructure";
        this->m_CurrentColorCoding = (char*) COLORCODING_CUSTOM; //will cause blank colorcoding of fibers
    }
}

void mitk::FiberBundleX::RotateAroundAxis(double x, double y, double z)
{
    MITK_INFO << "Rotating fibers";
    x = x*M_PI/180;
    y = y*M_PI/180;
    z = z*M_PI/180;

    vnl_matrix_fixed< double, 3, 3 > rotX; rotX.set_identity();
    rotX[1][1] = cos(x);
    rotX[2][2] = rotX[1][1];
    rotX[1][2] = -sin(x);
    rotX[2][1] = -rotX[1][2];

    vnl_matrix_fixed< double, 3, 3 > rotY; rotY.set_identity();
    rotY[0][0] = cos(y);
    rotY[2][2] = rotY[0][0];
    rotY[0][2] = sin(y);
    rotY[2][0] = -rotY[0][2];

    vnl_matrix_fixed< double, 3, 3 > rotZ; rotZ.set_identity();
    rotZ[0][0] = cos(z);
    rotZ[1][1] = rotZ[0][0];
    rotZ[0][1] = -sin(z);
    rotZ[1][0] = -rotZ[0][1];

    mitk::Geometry3D::Pointer geom = this->GetGeometry();
    mitk::Point3D center = geom->GetCenter();

    boost::progress_display disp(m_NumFibers);

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp ;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for (int j=0; j<numPoints; j++)
        {
            double* p = m_FiberPolyData->GetPoint(pointIds[j]);
            vnl_vector_fixed< double, 3 > dir;
            dir[0] = p[0]-center[0];
            dir[1] = p[1]-center[1];
            dir[2] = p[2]-center[2];
            dir = rotZ*rotY*rotX*dir;
            dir[0] += center[0];
            dir[1] += center[1];
            dir[2] += center[2];
            vtkIdType id = vtkNewPoints->InsertNextPoint(dir.data_block());
            container->GetPointIds()->InsertNextId(id);
        }
        vtkNewCells->InsertNextCell(container);
    }

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);
    UpdateColorCoding();
    UpdateFiberGeometry();
}

void mitk::FiberBundleX::ScaleFibers(double x, double y, double z)
{
    MITK_INFO << "Scaling fibers";
    boost::progress_display disp(m_NumFibers);

    mitk::Geometry3D* geom = this->GetGeometry();
    mitk::Point3D c = geom->GetCenter();

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp ;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for (int j=0; j<numPoints; j++)
        {
            double* p = m_FiberPolyData->GetPoint(pointIds[j]);
            p[0] -= c[0]; p[1] -= c[1]; p[2] -= c[2];
            p[0] *= x;
            p[1] *= y;
            p[2] *= z;
            p[0] += c[0]; p[1] += c[1]; p[2] += c[2];
            vtkIdType id = vtkNewPoints->InsertNextPoint(p);
            container->GetPointIds()->InsertNextId(id);
        }
        vtkNewCells->InsertNextCell(container);
    }

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);
    UpdateColorCoding();
    UpdateFiberGeometry();
}

void mitk::FiberBundleX::TranslateFibers(double x, double y, double z)
{
    MITK_INFO << "Translating fibers";
    boost::progress_display disp(m_NumFibers);

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp ;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for (int j=0; j<numPoints; j++)
        {
            double* p = m_FiberPolyData->GetPoint(pointIds[j]);
            p[0] += x;
            p[1] += y;
            p[2] += z;
            vtkIdType id = vtkNewPoints->InsertNextPoint(p);
            container->GetPointIds()->InsertNextId(id);
        }
        vtkNewCells->InsertNextCell(container);
    }

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);
    UpdateColorCoding();
    UpdateFiberGeometry();
}

void mitk::FiberBundleX::MirrorFibers(unsigned int axis)
{
    if (axis>2)
        return;

    MITK_INFO << "Mirroring fibers";
    boost::progress_display disp(m_NumFibers);

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp ;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for (int j=0; j<numPoints; j++)
        {
            double* p = m_FiberPolyData->GetPoint(pointIds[j]);
            p[axis] = -p[axis];
            vtkIdType id = vtkNewPoints->InsertNextPoint(p);
            container->GetPointIds()->InsertNextId(id);
        }
        vtkNewCells->InsertNextCell(container);
    }

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);
    UpdateColorCoding();
    UpdateFiberGeometry();
}

bool mitk::FiberBundleX::ApplyCurvatureThreshold(float minRadius, bool deleteFibers)
{
    if (minRadius<0)
        return true;

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> vtkOldCells = m_FiberPolyData->GetLines();
    vtkOldCells->InitTraversal();

    MITK_INFO << "Applying curvature threshold";
    boost::progress_display disp(m_FiberPolyData->GetNumberOfCells());
    for (int i=0; i<m_FiberPolyData->GetNumberOfCells(); i++)
    {
        ++disp ;
        vtkIdType   numPoints(0);
        vtkIdType*  points(NULL);
        vtkOldCells->GetNextCell ( numPoints, points );

        // calculate curvatures
        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
        for (int j=0; j<numPoints-2; j++)
        {
            double p1[3];
            m_FiberPolyData->GetPoint(points[j], p1);
            double p2[3];
            m_FiberPolyData->GetPoint(points[j+1], p2);
            double p3[3];
            m_FiberPolyData->GetPoint(points[j+2], p3);

            vnl_vector_fixed< float, 3 > v1, v2, v3;

            v1[0] = p2[0]-p1[0];
            v1[1] = p2[1]-p1[1];
            v1[2] = p2[2]-p1[2];

            v2[0] = p3[0]-p2[0];
            v2[1] = p3[1]-p2[1];
            v2[2] = p3[2]-p2[2];

            v3[0] = p1[0]-p3[0];
            v3[1] = p1[1]-p3[1];
            v3[2] = p1[2]-p3[2];

            float a = v1.magnitude();
            float b = v2.magnitude();
            float c = v3.magnitude();
            float r = a*b*c/std::sqrt((a+b+c)*(a+b-c)*(b+c-a)*(a-b+c)); // radius of triangle via Heron's formula (area of triangle)

            vtkIdType id = vtkNewPoints->InsertNextPoint(p1);
            container->GetPointIds()->InsertNextId(id);

            if (deleteFibers && r<minRadius)
                break;

            if (r<minRadius)
            {
                j += 2;
//                id = vtkNewPoints->InsertNextPoint(p2);
//                container->GetPointIds()->InsertNextId(id);
                vtkNewCells->InsertNextCell(container);
                container = vtkSmartPointer<vtkPolyLine>::New();
            }
            else if (j==numPoints-3)
            {
                id = vtkNewPoints->InsertNextPoint(p2);
                container->GetPointIds()->InsertNextId(id);
                id = vtkNewPoints->InsertNextPoint(p3);
                container->GetPointIds()->InsertNextId(id);
                vtkNewCells->InsertNextCell(container);
            }
        }
    }

    if (vtkNewCells->GetNumberOfCells()<=0)
        return false;


    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);

    UpdateColorCoding();
    UpdateFiberGeometry();
    return true;
}

bool mitk::FiberBundleX::RemoveShortFibers(float lengthInMM)
{
    if (lengthInMM<=0 || lengthInMM<m_MinFiberLength)
        return true;

    if (lengthInMM>m_MaxFiberLength)    // can't remove all fibers
        return false;

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    float min = m_MaxFiberLength;

    MITK_INFO << "Removing short fibers";
    boost::progress_display disp(m_NumFibers);
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        if (m_FiberLengths.at(i)>=lengthInMM)
        {
            vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
            for (int j=0; j<numPoints; j++)
            {
                double* p = m_FiberPolyData->GetPoint(pointIds[j]);
                vtkIdType id = vtkNewPoints->InsertNextPoint(p);
                container->GetPointIds()->InsertNextId(id);
            }
            vtkNewCells->InsertNextCell(container);
            if (m_FiberLengths.at(i)<min)
                min = m_FiberLengths.at(i);
        }
    }

    if (vtkNewCells->GetNumberOfCells()<=0)
        return false;

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);

    UpdateColorCoding();
    UpdateFiberGeometry();
    return true;
}

bool mitk::FiberBundleX::RemoveLongFibers(float lengthInMM)
{
    if (lengthInMM<=0 || lengthInMM>m_MaxFiberLength)
        return true;

    if (lengthInMM<m_MinFiberLength)    // can't remove all fibers
        return false;

    vtkSmartPointer<vtkPoints> vtkNewPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> vtkNewCells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();

    MITK_INFO << "Removing long fibers";
    boost::progress_display disp(m_NumFibers);
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        if (m_FiberLengths.at(i)<=lengthInMM)
        {
            vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();
            for (int j=0; j<numPoints; j++)
            {
                double* p = m_FiberPolyData->GetPoint(pointIds[j]);
                vtkIdType id = vtkNewPoints->InsertNextPoint(p);
                container->GetPointIds()->InsertNextId(id);
            }
            vtkNewCells->InsertNextCell(container);
        }
    }

    if (vtkNewCells->GetNumberOfCells()<=0)
        return false;

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkNewPoints);
    m_FiberPolyData->SetLines(vtkNewCells);
    UpdateColorCoding();
    UpdateFiberGeometry();
    return true;
}

void mitk::FiberBundleX::DoFiberSmoothing(int pointsPerCm, double tension, double continuity, double bias )
{
    vtkSmartPointer<vtkPoints> vtkSmoothPoints = vtkSmartPointer<vtkPoints>::New(); //in smoothpoints the interpolated points representing a fiber are stored.

    //in vtkcells all polylines are stored, actually all id's of them are stored
    vtkSmartPointer<vtkCellArray> vtkSmoothCells = vtkSmartPointer<vtkCellArray>::New(); //cellcontainer for smoothed lines
    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    vtkIdType pointHelperCnt = 0;

    MITK_INFO << "Resampling fibers";
    boost::progress_display disp(m_NumFibers);
    for (int i=0; i<m_NumFibers; i++)
    {
        ++disp;
        vtkIdType   numPoints(0);
        vtkIdType*  pointIds(NULL);
        vLines->GetNextCell ( numPoints, pointIds );

        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        for (int j=0; j<numPoints; j++)
            points->InsertNextPoint(m_FiberPolyData->GetPoint(pointIds[j]));

        float length = m_FiberLengths.at(i);
        length /=10;
        int sampling = pointsPerCm*length;

        vtkSmartPointer<vtkKochanekSpline> xSpline = vtkSmartPointer<vtkKochanekSpline>::New();
        vtkSmartPointer<vtkKochanekSpline> ySpline = vtkSmartPointer<vtkKochanekSpline>::New();
        vtkSmartPointer<vtkKochanekSpline> zSpline = vtkSmartPointer<vtkKochanekSpline>::New();
        xSpline->SetDefaultBias(bias); xSpline->SetDefaultTension(tension); xSpline->SetDefaultContinuity(continuity);
        ySpline->SetDefaultBias(bias); ySpline->SetDefaultTension(tension); ySpline->SetDefaultContinuity(continuity);
        zSpline->SetDefaultBias(bias); zSpline->SetDefaultTension(tension); zSpline->SetDefaultContinuity(continuity);

        vtkSmartPointer<vtkParametricSpline> spline = vtkSmartPointer<vtkParametricSpline>::New();
        spline->SetXSpline(xSpline);
        spline->SetYSpline(ySpline);
        spline->SetZSpline(zSpline);
        spline->SetPoints(points);

        vtkSmartPointer<vtkParametricFunctionSource> functionSource = vtkSmartPointer<vtkParametricFunctionSource>::New();
        functionSource->SetParametricFunction(spline);
        functionSource->SetUResolution(sampling);
        functionSource->SetVResolution(sampling);
        functionSource->SetWResolution(sampling);
        functionSource->Update();

        vtkPolyData* outputFunction = functionSource->GetOutput();
        vtkPoints* tmpSmoothPnts = outputFunction->GetPoints(); //smoothPoints of current fiber

        vtkSmartPointer<vtkPolyLine> smoothLine = vtkSmartPointer<vtkPolyLine>::New();
        smoothLine->GetPointIds()->SetNumberOfIds(tmpSmoothPnts->GetNumberOfPoints());

        for (int j=0; j<smoothLine->GetNumberOfPoints(); j++)
        {
            smoothLine->GetPointIds()->SetId(j, j+pointHelperCnt);
            vtkSmoothPoints->InsertNextPoint(tmpSmoothPnts->GetPoint(j));
        }
        vtkSmoothCells->InsertNextCell(smoothLine);
        pointHelperCnt += tmpSmoothPnts->GetNumberOfPoints();
    }

    m_FiberPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_FiberPolyData->SetPoints(vtkSmoothPoints);
    m_FiberPolyData->SetLines(vtkSmoothCells);
    UpdateColorCoding();
    UpdateFiberGeometry();
    m_FiberSampling = pointsPerCm;
}

void mitk::FiberBundleX::DoFiberSmoothing(int pointsPerCm)
{
    DoFiberSmoothing(pointsPerCm, 0, 0, 0 );
}

// Resample fiber to get equidistant points
void mitk::FiberBundleX::ResampleFibers(float pointDistance)
{
    if (pointDistance<=0.00001)
        return;

    vtkSmartPointer<vtkPolyData> newPoly = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkCellArray> newCellArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkPoints>    newPoints = vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkCellArray> vLines = m_FiberPolyData->GetLines();
    vLines->InitTraversal();
    int numberOfLines = m_NumFibers;

    MITK_INFO << "Resampling fibers";
    boost::progress_display disp(m_NumFibers);
    for (int i=0; i<numberOfLines; i++)
    {
        ++disp;
        vtkIdType   numPoints(0);
        vtkIdType*  points(NULL);
        vLines->GetNextCell ( numPoints, points );

        vtkSmartPointer<vtkPolyLine> container = vtkSmartPointer<vtkPolyLine>::New();

        double* point = m_FiberPolyData->GetPoint(points[0]);
        vtkIdType pointId = newPoints->InsertNextPoint(point);
        container->GetPointIds()->InsertNextId(pointId);

        float dtau = 0;
        int cur_p = 1;
        itk::Vector<float,3> dR;
        float normdR = 0;

        for (;;)
        {
            while (dtau <= pointDistance && cur_p < numPoints)
            {
                itk::Vector<float,3> v1;
                point = m_FiberPolyData->GetPoint(points[cur_p-1]);
                v1[0] = point[0];
                v1[1] = point[1];
                v1[2] = point[2];
                itk::Vector<float,3> v2;
                point = m_FiberPolyData->GetPoint(points[cur_p]);
                v2[0] = point[0];
                v2[1] = point[1];
                v2[2] = point[2];

                dR  = v2 - v1;
                normdR = std::sqrt(dR.GetSquaredNorm());
                dtau += normdR;
                cur_p++;
            }

            if (dtau >= pointDistance)
            {
                itk::Vector<float,3> v1;
                point = m_FiberPolyData->GetPoint(points[cur_p-1]);
                v1[0] = point[0];
                v1[1] = point[1];
                v1[2] = point[2];

                itk::Vector<float,3> v2 = v1 - dR*( (dtau-pointDistance)/normdR );
                pointId = newPoints->InsertNextPoint(v2.GetDataPointer());
                container->GetPointIds()->InsertNextId(pointId);
            }
            else
            {
                point = m_FiberPolyData->GetPoint(points[numPoints-1]);
                pointId = newPoints->InsertNextPoint(point);
                container->GetPointIds()->InsertNextId(pointId);
                break;
            }
            dtau = dtau-pointDistance;
        }

        newCellArray->InsertNextCell(container);
    }

    newPoly->SetPoints(newPoints);
    newPoly->SetLines(newCellArray);
    m_FiberPolyData = newPoly;
    UpdateFiberGeometry();
    UpdateColorCoding();
    m_FiberSampling = 10/pointDistance;
}

// reapply selected colorcoding in case polydata structure has changed
void mitk::FiberBundleX::UpdateColorCoding()
{
    char* cc = GetCurrentColorCoding();

    if( strcmp (COLORCODING_ORIENTATION_BASED,cc) == 0 )
        DoColorCodingOrientationBased();
    else if( strcmp (COLORCODING_FA_BASED,cc) == 0 )
        DoColorCodingFaBased();
}

// reapply selected colorcoding in case polydata structure has changed
bool mitk::FiberBundleX::Equals(mitk::FiberBundleX* fib)
{
    if (fib==NULL)
        return false;

    mitk::FiberBundleX::Pointer tempFib = this->SubtractBundle(fib);
    mitk::FiberBundleX::Pointer tempFib2 = fib->SubtractBundle(this);

    if (tempFib.IsNull() && tempFib2.IsNull())
        return true;

    return false;
}

/* ESSENTIAL IMPLEMENTATION OF SUPERCLASS METHODS */
void mitk::FiberBundleX::UpdateOutputInformation()
{

}
void mitk::FiberBundleX::SetRequestedRegionToLargestPossibleRegion()
{

}
bool mitk::FiberBundleX::RequestedRegionIsOutsideOfTheBufferedRegion()
{
    return false;
}
bool mitk::FiberBundleX::VerifyRequestedRegion()
{
    return true;
}
void mitk::FiberBundleX::SetRequestedRegion( itk::DataObject *data )
{

}
