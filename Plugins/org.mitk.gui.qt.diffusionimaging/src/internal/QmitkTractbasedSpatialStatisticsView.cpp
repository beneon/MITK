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

// Blueberry
#include "berryIWorkbenchWindow.h"
#include "berryIWorkbenchPage.h"
#include "berryISelectionService.h"
#include "berryConstants.h"
#include "berryPlatformUI.h"

// Qmitk
#include "QmitkTractbasedSpatialStatisticsView.h"
#include "QmitkStdMultiWidget.h"

#include "mitkDataNodeObject.h"
#include <itkCastImageFilter.h>

// Qt
#include <QMessageBox>
#include <qfiledialog.h>
#include <QDirIterator>
#include <QInputDialog>
#include <QDir>
#include <QStringList>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QClipboard>

#include <itkNiftiImageIO.h>
#include <itkImageFileReader.h>

#include <iostream>
#include <fstream>
#include <limits>


#include <itkMultiplyImageFilter.h>
#include <mitkTractAnalyzer.h>
#include <mitkTbssImporter.h>
#include <mitkProgressBar.h>
#include <mitkPlanarCircle.h>
#include <mitkPlanarFigureInteractor.h>


#include <mitkVectorImageMapper2D.h>
#include "vtkFloatArray.h"
#include "vtkLinearTransform.h"
#include "vtkPoints.h"
#include "mitkSurface.h"
#include <vtkGlyph3D.h>
#include "vtkArrowSource.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include <vtkCellArray.h>
#include <vtkPolyLine.h>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <QMessageBox>

#include "mitkITKImageImport.h"
// #include "mitkImageMapperGL2D.h"
#include "mitkVolumeDataVtkMapper3D.h"
#include "mitkImageAccessByItk.h"
#include "mitkTensorImage.h"

#include "itkDiffusionTensor3D.h"


#define SEARCHSIGMA 10 /* length in linear voxel dimens
  {
    // create new ones
    m_PointSetNode = mitk::PointSet::New();ions */
#define MAXSEARCHLENGTH (3*SEARCHSIGMA)

const std::string QmitkTractbasedSpatialStatisticsView::VIEW_ID = "org.mitk.views.tractbasedspatialstatistics";

using namespace berry;


struct TbssSelListener : ISelectionListener
{

  berryObjectMacro(TbssSelListener)

  TbssSelListener(QmitkTractbasedSpatialStatisticsView* view)
  {
    m_View = view;
  }


  void DoSelectionChanged(ISelection::ConstPointer selection)
  {
    // save current selection in member variable
    m_View->m_CurrentSelection = selection.Cast<const IStructuredSelection>();

    // do something with the selected items
    if(m_View->m_CurrentSelection)
    {
      bool foundTbssRoi = false;
      bool foundTbss = false;
      bool found3dImage = false;
      bool found4dImage = false;
      bool foundFiberBundle = false;
      bool foundStartRoi = false;
      bool foundEndRoi = false;

      mitk::TbssRoiImage* roiImage;
      mitk::TbssImage* image;
      mitk::Image* img;
      mitk::FiberBundleX* fib;
      mitk::PlanarFigure* start;
      mitk::PlanarFigure* end;

      m_View->m_CurrentStartRoi = NULL;
      m_View->m_CurrentEndRoi = NULL;


      // iterate selection
      for (IStructuredSelection::iterator i = m_View->m_CurrentSelection->Begin();
        i != m_View->m_CurrentSelection->End(); ++i)
      {

        // extract datatree node
        if (mitk::DataNodeObject::Pointer nodeObj = i->Cast<mitk::DataNodeObject>())
        {
          mitk::DataNode::Pointer node = nodeObj->GetDataNode();

          // only look at interesting types
          // check for valid data
          mitk::BaseData* nodeData = node->GetData();
          if( nodeData )
          {
            if(QString("TbssRoiImage").compare(nodeData->GetNameOfClass())==0)
            {
              foundTbssRoi = true;
              roiImage = static_cast<mitk::TbssRoiImage*>(nodeData);
            }
            else if (QString("TbssImage").compare(nodeData->GetNameOfClass())==0)
            {
              foundTbss = true;
              image = static_cast<mitk::TbssImage*>(nodeData);
            }
            else if(QString("Image").compare(nodeData->GetNameOfClass())==0)
            {
              img = static_cast<mitk::Image*>(nodeData);
              if(img->GetDimension() == 3)
              {
                found3dImage = true;
              }
              else if(img->GetDimension() == 4)
              {
                found4dImage = true;
              }
            }

            else if (QString("FiberBundleX").compare(nodeData->GetNameOfClass())==0)
            {
              foundFiberBundle = true;
              fib = static_cast<mitk::FiberBundleX*>(nodeData);
              m_View->m_CurrentFiberNode = node;
            }


            std::string name = nodeData->GetNameOfClass();
            if(QString("PlanarCircle").compare(nodeData->GetNameOfClass())==0)
            {
              if(!foundStartRoi)
              {
                start = dynamic_cast<mitk::PlanarFigure*>(nodeData);
                m_View->m_CurrentStartRoi = node;
                foundStartRoi =  true;
              }
              else
              {
                end = dynamic_cast<mitk::PlanarFigure*>(nodeData);
                m_View->m_CurrentEndRoi = node;
                foundEndRoi = true;
              }
            }
          } // end CHECK nodeData != NULL

        }

      }

      m_View->m_Controls->m_CreateRoi->setEnabled(found3dImage);
      m_View->m_Controls->m_ImportFsl->setEnabled(found4dImage);



      if(foundTbss && foundTbssRoi)
      {
        m_View->Plot(image, roiImage);
      }

      if(found3dImage && foundFiberBundle && foundStartRoi && foundEndRoi)
      {
        m_View->PlotFiberBundle(fib, img, start, end);

      }
      else if(found3dImage == true && foundFiberBundle)
      {
        m_View->PlotFiberBundle(fib, img);
      }

      if(found3dImage)
      {
        m_View->InitPointsets();
      }

      m_View->m_Controls->m_Cut->setEnabled(foundFiberBundle && foundStartRoi && foundEndRoi);
      m_View->m_Controls->m_SegmentLabel->setEnabled(foundFiberBundle && foundStartRoi && foundEndRoi && found3dImage);
      m_View->m_Controls->m_Segments->setEnabled(foundFiberBundle && foundStartRoi && foundEndRoi && found3dImage);
      m_View->m_Controls->m_Average->setEnabled(foundFiberBundle && foundStartRoi && foundEndRoi && found3dImage);


    }
  }


  void SelectionChanged(IWorkbenchPart::Pointer part, ISelection::ConstPointer selection)
  {
    // check, if selection comes from datamanager
    if (part)
    {
      QString partname(part->GetPartName().c_str());
      if(partname.compare("Datamanager")==0)
      {
        // apply selection
        DoSelectionChanged(selection);
      }
    }
  }

  QmitkTractbasedSpatialStatisticsView* m_View;
};


QmitkTractbasedSpatialStatisticsView::QmitkTractbasedSpatialStatisticsView()
: QmitkFunctionality()
, m_Controls( 0 )
, m_MultiWidget( NULL )
{

}

QmitkTractbasedSpatialStatisticsView::~QmitkTractbasedSpatialStatisticsView()
{
}


void QmitkTractbasedSpatialStatisticsView::PerformChange()
{
  m_Controls->m_RoiPlotWidget->ModifyPlot(m_Controls->m_Segments->value(), m_Controls->m_Average->isChecked());
}

void QmitkTractbasedSpatialStatisticsView::OnSelectionChanged(std::vector<mitk::DataNode*> nodes)
{
  //datamanager selection changed
  if (!this->IsActivated())
    return;


  // Get DataManagerSelection
  if (!this->GetDataManagerSelection().empty())
  {
    mitk::DataNode::Pointer sourceImageNode = this->GetDataManagerSelection().front();
    mitk::Image::Pointer sourceImage = dynamic_cast<mitk::Image*>(sourceImageNode->GetData());

    if (!sourceImage)
    {
      m_Controls->m_TbssImageLabel->setText(
        QString( sourceImageNode->GetName().c_str() ) + " is no image"
        );

      return;
    }

    // set Text
    m_Controls->m_TbssImageLabel->setText(
      QString( sourceImageNode->GetName().c_str() ) + " (" +
      QString::number(sourceImage->GetDimension()) + "D)"
      );


  }
  else
  {
    m_Controls->m_TbssImageLabel->setText("Please select an image");
  }

}

void QmitkTractbasedSpatialStatisticsView::InitPointsets()
{
  // Check if PointSetStart exsits, if not create it.
  m_P1 = this->GetDefaultDataStorage()->GetNamedNode("PointSetNode");

  if (m_PointSetNode)
  {
    //m_PointSetNode = dynamic_cast<mitk::PointSet*>(m_P1->GetData());
    return;
  }

  if ((!m_P1) || (!m_PointSetNode))
  {
    // create new ones
    m_PointSetNode = mitk::PointSet::New();
    m_P1 = mitk::DataNode::New();
    m_P1->SetData( m_PointSetNode );
    m_P1->SetProperty( "name", mitk::StringProperty::New( "PointSet" ) );
    m_P1->SetProperty( "opacity", mitk::FloatProperty::New( 1 ) );
    m_P1->SetProperty( "helper object", mitk::BoolProperty::New(true) ); // CHANGE if wanted
    m_P1->SetProperty( "pointsize", mitk::FloatProperty::New( 0.1 ) );
    m_P1->SetColor( 1.0, 0.0, 0.0 );
    this->GetDefaultDataStorage()->Add(m_P1);
    m_Controls->m_PointWidget->SetPointSetNode(m_P1);
    m_Controls->m_PointWidget->SetMultiWidget(GetActiveStdMultiWidget());
  }
}

void QmitkTractbasedSpatialStatisticsView::CreateQtPartControl( QWidget *parent )
{
  // build up qt view, unless already done
  if ( !m_Controls )
  {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls = new Ui::QmitkTractbasedSpatialStatisticsViewControls;
    m_Controls->setupUi( parent );
    this->CreateConnections();
  }

  m_SelListener = berry::ISelectionListener::Pointer(new TbssSelListener(this));
  this->GetSite()->GetWorkbenchWindow()->GetSelectionService()->AddPostSelectionListener(/*"org.mitk.views.datamanager",*/ m_SelListener);
  berry::ISelection::ConstPointer sel(
    this->GetSite()->GetWorkbenchWindow()->GetSelectionService()->GetSelection("org.mitk.views.datamanager"));
  m_CurrentSelection = sel.Cast<const IStructuredSelection>();
  m_SelListener.Cast<TbssSelListener>()->DoSelectionChanged(sel);

  m_IsInitialized = false;



  // Table for the FSL TBSS import
  m_GroupModel = new QmitkTbssTableModel();
  m_Controls->m_GroupInfo->setModel(m_GroupModel);


}

void QmitkTractbasedSpatialStatisticsView::Activated()
{
  QmitkFunctionality::Activated();

  berry::ISelection::ConstPointer sel(
    this->GetSite()->GetWorkbenchWindow()->GetSelectionService()->GetSelection("org.mitk.views.datamanager"));
  m_CurrentSelection = sel.Cast<const IStructuredSelection>();
  m_SelListener.Cast<TbssSelListener>()->DoSelectionChanged(sel);
}

void QmitkTractbasedSpatialStatisticsView::Deactivated()
{
  QmitkFunctionality::Deactivated();
}

void QmitkTractbasedSpatialStatisticsView::CreateConnections()
{
  if ( m_Controls )
  {
    connect( (QObject*)(m_Controls->m_CreateRoi), SIGNAL(clicked()), this, SLOT(CreateRoi()) );
    connect( (QObject*)(m_Controls->m_ImportFsl), SIGNAL(clicked()), this, SLOT(TbssImport()) );
    connect( (QObject*)(m_Controls->m_AddGroup), SIGNAL(clicked()), this, SLOT(AddGroup()) );
    connect( (QObject*)(m_Controls->m_RemoveGroup), SIGNAL(clicked()), this, SLOT(RemoveGroup()) );
    connect( (QObject*)(m_Controls->m_Clipboard), SIGNAL(clicked()), this, SLOT(CopyToClipboard()) );
    connect( m_Controls->m_RoiPlotWidget->m_PlotPicker, SIGNAL(selected(const QwtDoublePoint&)), SLOT(Clicked(const QwtDoublePoint&) ) );
    connect( m_Controls->m_RoiPlotWidget->m_PlotPicker, SIGNAL(moved(const QwtDoublePoint&)), SLOT(Clicked(const QwtDoublePoint&) ) );
    connect( (QObject*)(m_Controls->m_Cut), SIGNAL(clicked()), this, SLOT(Cut()) );
    connect( (QObject*)(m_Controls->m_Average), SIGNAL(stateChanged(int)), this, SLOT(PerformChange()) );
    connect( (QObject*)(m_Controls->m_Segments), SIGNAL(valueChanged(int)), this, SLOT(PerformChange()) );

  }
}


void QmitkTractbasedSpatialStatisticsView::CopyToClipboard()
{



  if(m_Controls->m_RoiPlotWidget->IsPlottingFiber())
  {
    // Working with fiber bundles
    std::vector <std::vector<double> > profiles = m_Controls->m_RoiPlotWidget->GetIndividualProfiles();



    QString clipboardText;
    for (std::vector<std::vector<double> >::iterator it = profiles.begin(); it
                                                           != profiles.end(); ++it)
    {
      for (std::vector<double>::iterator it2 = (*it).begin(); it2 !=
            (*it).end(); ++it2)
      {
        clipboardText.append(QString("%1 \t").arg(*it2));
      }
      clipboardText.append(QString("\n"));
    }



    if(m_Controls->m_Average->isChecked())
    {
      std::vector<double> averages = m_Controls->m_RoiPlotWidget->GetAverageProfile();
      clipboardText.append(QString("\nAverage\n"));

      for (std::vector<double>::iterator it2 = averages.begin(); it2 !=
            averages.end(); ++it2)
      {
        clipboardText.append(QString("%1 \t").arg(*it2));

      }
    }

    QApplication::clipboard()->setText(clipboardText, QClipboard::Clipboard);

  }

  else{

    // Working with TBSS Data
    if(m_Controls->m_Average->isChecked())
    {
      std::vector<std::vector<double> > vals = m_Controls->m_RoiPlotWidget->GetVals();
      QString clipboardText;
      for (std::vector<std::vector<double> >::iterator it = vals.begin(); it
                                                             != vals.end(); ++it)
      {
        for (std::vector<double>::iterator it2 = (*it).begin(); it2 !=
              (*it).end(); ++it2)
        {
          clipboardText.append(QString("%1 \t").arg(*it2));

          double d = *it2;
          std::cout << d <<std::endl;
        }
        clipboardText.append(QString("\n"));
      }

      QApplication::clipboard()->setText(clipboardText, QClipboard::Clipboard);
    }
    else
    {
      std::vector<std::vector<double> > vals = m_Controls->m_RoiPlotWidget->GetIndividualProfiles();
      QString clipboardText;
      for (std::vector<std::vector<double> >::iterator it = vals.begin(); it
                                                             != vals.end(); ++it)
      {
        for (std::vector<double>::iterator it2 = (*it).begin(); it2 !=
              (*it).end(); ++it2)
        {
          clipboardText.append(QString("%1 \t").arg(*it2));

          double d = *it2;
          std::cout << d <<std::endl;
        }
        clipboardText.append(QString("\n"));
      }
      QApplication::clipboard()->setText(clipboardText, QClipboard::Clipboard);
    }

  }


}

void QmitkTractbasedSpatialStatisticsView::RemoveGroup()
{

  QTableView *temp = static_cast<QTableView*>(m_Controls->m_GroupInfo);
 // QSortFilterProxyModel *proxy = static_cast<QSortFilterProxyModel*>(temp->model());
  QItemSelectionModel *selectionModel = temp->selectionModel();

  QModelIndexList indices = selectionModel->selectedRows();

  QModelIndex index;

  foreach(index, indices)
  {
    int row = index.row();
    m_GroupModel->removeRows(row, 1, QModelIndex());
  }

}

std::string QmitkTractbasedSpatialStatisticsView::ReadFile(std::string whatfile)
{
  std::string s = "Select a" + whatfile;
  QFileDialog* w = new QFileDialog(this->m_Controls->m_ImportFsl, QString(s.c_str()) );
  w->setFileMode(QFileDialog::ExistingFiles);
  w->setDirectory("/home");

  if(whatfile == "gradient image")
  {
    w->setNameFilter("Tbss gradient images (*.tgi)");
  }

  // RETRIEVE SELECTION
  if ( w->exec() != QDialog::Accepted )
  {
    return "";
    MITK_INFO << "Failed to load";
  }

  QStringList filenames = w->selectedFiles();
  if (filenames.size() > 0)
  {
    std::string retval = filenames.at(0).toStdString();
    return retval;
  }
  return "";
}

void QmitkTractbasedSpatialStatisticsView::AddGroup()
{
  QString group("Group");
  int number = 0;
  QPair<QString, int> pair(group, number);
  QList< QPair<QString, int> >list = m_GroupModel->getList();


  if(!list.contains(pair))
  {
    m_GroupModel->insertRows(0, 1, QModelIndex());

    QModelIndex index = m_GroupModel->index(0, 0, QModelIndex());
    m_GroupModel->setData(index, group, Qt::EditRole);
    index = m_GroupModel->index(0, 1, QModelIndex());
    m_GroupModel->setData(index, number, Qt::EditRole);

  }
  else
  {
    //QMessageBox::information(this, "Duplicate name");
  }


}


void QmitkTractbasedSpatialStatisticsView::TbssImport()
{

  // Read groups from the interface
  mitk::TbssImporter::Pointer importer = mitk::TbssImporter::New();

  QList< QPair<QString, int> >list = m_GroupModel->getList();

  if(list.size() == 0)
  {
    QMessageBox msgBox;
    msgBox.setText("No study group information has been set yet.");
    msgBox.exec();
    return;
  }


  std::vector < std::pair<std::string, int> > groups;
  for(int i=0; i<list.size(); i++)
  {
    QPair<QString, int> pair = list.at(i);
    std::string s = pair.first.toStdString();
    int n = pair.second;

    std::pair<std::string, int> p;
    p.first = s;
    p.second = n;
    groups.push_back(p);
  }

  importer->SetGroupInfo(groups);

  std::string minfo = m_Controls->m_MeasurementInfo->text().toStdString();
  importer->SetMeasurementInfo(minfo);


  std::string name = "";
  for (IStructuredSelection::iterator i = m_CurrentSelection->Begin();
    i != m_CurrentSelection->End(); ++i)
  {
    // extract datatree node
    if (mitk::DataNodeObject::Pointer nodeObj = i->Cast<mitk::DataNodeObject>())
    {
      mitk::DataNode::Pointer node = nodeObj->GetDataNode();

      if(QString("Image").compare(node->GetData()->GetNameOfClass())==0)
      {
        mitk::Image* img = static_cast<mitk::Image*>(node->GetData());
        if(img->GetDimension() == 4)
        {
          importer->SetImportVolume(img);
          name = node->GetName();
        }
      }
    }
  }

  mitk::TbssImage::Pointer tbssImage;

  tbssImage = importer->Import();
  name += "_tbss";
  AddTbssToDataStorage(tbssImage, name);


}


void QmitkTractbasedSpatialStatisticsView::AddTbssToDataStorage(mitk::Image* image, std::string name)
{
  mitk::LevelWindow levelwindow;
  levelwindow.SetAuto( image );
  mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
  levWinProp->SetLevelWindow( levelwindow );

  mitk::DataNode::Pointer result = mitk::DataNode::New();
  result->SetProperty( "name", mitk::StringProperty::New(name) );
  result->SetData( image );
  result->SetProperty( "levelwindow", levWinProp );


  // add new image to data storage and set as active to ease further processing
  GetDefaultDataStorage()->Add( result );

  // show the results
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void QmitkTractbasedSpatialStatisticsView::Clicked(const QwtDoublePoint& pos)
{
  int index = (int)pos.x();

  if(m_Roi.size() > 0 && m_CurrentGeometry != NULL && !m_Controls->m_RoiPlotWidget->IsPlottingFiber() )
  {

    index = std::min( (int)m_Roi.size()-1, std::max(0, index) );
    itk::Index<3> ix = m_Roi.at(index);

    mitk::Vector3D i;
    i[0] = ix[0];
    i[1] = ix[1];
    i[2] = ix[2];

    mitk::Vector3D w;

    m_CurrentGeometry->IndexToWorld(i, w);

    mitk::Point3D origin = m_CurrentGeometry->GetOrigin();

    mitk::Point3D p;
    p[0] = w[0] + origin[0];
    p[1] = w[1] + origin[1];
    p[2] = w[2] + origin[2];

    m_MultiWidget->MoveCrossToPosition(p);
    m_Controls->m_RoiPlotWidget->drawBar(index);
  }

  else if(m_Controls->m_RoiPlotWidget->IsPlottingFiber() )
  {

    mitk::Point3D point = m_Controls->m_RoiPlotWidget->GetPositionInWorld(index);
    m_MultiWidget->MoveCrossToPosition(point);

    /*
    mitk::BaseData* fibData = m_CurrentFiberNode->GetData();
    mitk::FiberBundleX* fib = static_cast<mitk::FiberBundleX*>(fibData);

    mitk::BaseData* startData = m_CurrentStartRoi->GetData();
    mitk::PlanarFigure* startRoi = static_cast<mitk::PlanarFigure*>(startData);

    mitk::BaseData* endData = m_CurrentEndRoi->GetData();
    mitk::PlanarFigure* endRoi = static_cast<mitk::PlanarFigure*>(endData);

    mitk::Point3D startCenter = startRoi->GetWorldControlPoint(0); //center Point of start roi
    mitk::Point3D endCenter = endRoi->GetWorldControlPoint(0); //center Point of end roi


    mitk::FiberBundleX::Pointer inStart = fib->ExtractFiberSubset(startRoi);
    mitk::FiberBundleX::Pointer inBoth = inStart->ExtractFiberSubset(endRoi);

    // check wich fiber is selected. if they are all shown take the first one from the bundle
    int selectedFiber = std::max(m_Controls->m_FiberSelector->currentIndex()-1, 0);
    int num = inBoth->GetNumFibers();

    if(num < selectedFiber)
      return;

    std::vector<long> ids;
    ids.push_back(selectedFiber);

    vtkSmartPointer<vtkPolyData> singlefib = inBoth->GeneratePolyDataByIds(ids);
    vtkCellArray* lines = singlefib->GetLines();
    lines->InitTraversal();




    // Select fiber by selectedFiber
    vtkIdType   numPointsInCell(0);
    vtkIdType*  pointsInCell(NULL);
    lines->GetCell(selectedFiber, numPointsInCell, pointsInCell);


    int startId = 0;
    int endId = 0;

    float minDistStart = std::numeric_limits<float>::max();
    float minDistEnd = std::numeric_limits<float>::max();


    for( int pointInCellID( 0 ); pointInCellID < numPointsInCell ; pointInCellID++)
    {


      double *p = singlefib->GetPoint( pointsInCell[ pointInCellID ] );


      mitk::Point3D point;
      point[0] = p[0];
      point[1] = p[1];
      point[2] = p[2];

      float distanceToStart = point.EuclideanDistanceTo(startCenter);
      float distanceToEnd = point.EuclideanDistanceTo(endCenter);

      if(distanceToStart < minDistStart)
      {
        minDistStart = distanceToStart;
        startId = pointInCellID;
      }

      if(distanceToEnd < minDistEnd)
      {
        minDistEnd = distanceToEnd;
        endId = pointInCellID;
      }

    }


    if(startId < endId)
    {
      index = startId + index;
    }
    else
    {
      index = startId- index;
    }


    // In case the possible range of indices exceeds the range of fiber oints
    index = std::min( (int)numPointsInCell-1, std::max(0, index) );

    double *p = singlefib->GetPoint( pointsInCell[ index ] );
    PointType point;
    point[0] = p[0];
    point[1] = p[1];
    point[2] = p[2];


*/

  }


}

void QmitkTractbasedSpatialStatisticsView::Cut()
{
  mitk::BaseData* fibData = m_CurrentFiberNode->GetData();
  mitk::FiberBundleX* fib = static_cast<mitk::FiberBundleX*>(fibData);

  mitk::BaseData* startData = m_CurrentStartRoi->GetData();
  mitk::PlanarFigure* startRoi = static_cast<mitk::PlanarFigure*>(startData);
  mitk::PlaneGeometry* startGeometry2D = dynamic_cast<mitk::PlaneGeometry*>( const_cast<mitk::Geometry2D*>(startRoi->GetGeometry2D()) );

  mitk::BaseData* endData = m_CurrentEndRoi->GetData();
  mitk::PlanarFigure* endRoi = static_cast<mitk::PlanarFigure*>(endData);
  mitk::PlaneGeometry* endGeometry2D = dynamic_cast<mitk::PlaneGeometry*>( const_cast<mitk::Geometry2D*>(endRoi->GetGeometry2D()) );

  mitk::Point3D startCenter = startRoi->GetWorldControlPoint(0); //center Point of start roi
  mitk::Point3D endCenter = endRoi->GetWorldControlPoint(0); //center Point of end roi

  mitk::FiberBundleX::Pointer inStart = fib->ExtractFiberSubset(startRoi);
  mitk::FiberBundleX::Pointer inBoth = inStart->ExtractFiberSubset(endRoi);

  int num = inBoth->GetNumFibers();

  vtkSmartPointer<vtkPolyData> fiberPolyData = inBoth->GetFiberPolyData();
  vtkCellArray* lines = fiberPolyData->GetLines();
  lines->InitTraversal();


  // initialize new vtk polydata
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

  int pointIndex=0;


  // find start and endpoint
  for( int fiberID( 0 ); fiberID < num; fiberID++ )
  {
    vtkIdType   numPointsInCell(0);
    vtkIdType*  pointsInCell(NULL);
    lines->GetNextCell ( numPointsInCell, pointsInCell );

    int startId = 0;
    int endId = 0;

    float minDistStart = std::numeric_limits<float>::max();
    float minDistEnd = std::numeric_limits<float>::max();


    vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
    int lineIndex=0;


    for( int pointInCellID( 0 ); pointInCellID < numPointsInCell ; pointInCellID++)
    {
      double *p = fiberPolyData->GetPoint( pointsInCell[ pointInCellID ] );

      mitk::Point3D point;
      point[0] = p[0];
      point[1] = p[1];
      point[2] = p[2];

      float distanceToStart = point.EuclideanDistanceTo(startCenter);
      float distanceToEnd = point.EuclideanDistanceTo(endCenter);

      if(distanceToStart < minDistStart)
      {
        minDistStart = distanceToStart;
        startId = pointInCellID;
      }

      if(distanceToEnd < minDistEnd)
      {
        minDistEnd = distanceToEnd;
        endId = pointInCellID;
      }



    }



    /* We found the start and end points of of the part that should be plottet for
       the current fiber. now we need to plot them. If the endId is smaller than the startId the plot order
       must be reversed*/

    if(startId < endId)
    {

      double *p = fiberPolyData->GetPoint( pointsInCell[ startId ] );

      mitk::Vector3D p0;
      p0[0] = p[0];
      p0[1] = p[1];
      p0[2] = p[2];

      p = fiberPolyData->GetPoint( pointsInCell[ startId+1 ] );

      mitk::Vector3D p1;
      p1[0] = p[0];
      p1[1] = p[1];
      p1[2] = p[2];


      // Check if p and p2 are both on the same side of the plane
      mitk::Vector3D normal = startGeometry2D->GetNormal();

      mitk::Point3D pStart;
      pStart[0] = p0[0];
      pStart[1] = p0[1];
      pStart[2] = p0[2];

      bool startOnPositive = startGeometry2D->IsAbove(pStart);

      mitk::Point3D pSecond;
      pSecond[0] = p1[0];
      pSecond[1] = p1[1];
      pSecond[2] = p1[2];

      bool secondOnPositive = startGeometry2D->IsAbove(pSecond);


      // Calculate intersection with the plane

      mitk::Vector3D onPlane;
      onPlane[0] = startCenter[0];
      onPlane[1] = startCenter[1];
      onPlane[2] = startCenter[2];


      if(! (secondOnPositive ^ startOnPositive) )
      {
        /* startId and startId+1 lie on the same side of the plane, so we need
           need startId-1 to calculate the intersection with the planar figure*/
        p = fiberPolyData->GetPoint( pointsInCell[ startId-1 ] );
        p1[0] = p[0];
        p1[1] = p[1];
        p1[2] = p[2];
      }


      double d = ( (onPlane-p0)*normal) / ( (p0-p1) * normal );

      mitk::Vector3D newPoint = (p0-p1);

      newPoint[0] = d*newPoint[0] + p0[0];
      newPoint[1] = d*newPoint[1] + p0[1];
      newPoint[2] = d*newPoint[2] + p0[2];


      double insertPoint[3];
      insertPoint[0] = newPoint[0];
      insertPoint[1] = newPoint[1];
      insertPoint[2] = newPoint[2];


      // First insert the intersection with the start roi
      points->InsertNextPoint(insertPoint);
      polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
      lineIndex++;
      pointIndex++;

      if(! (secondOnPositive ^ startOnPositive) )
      {
        /* StartId and startId+1 lie on the same side of the plane
           so startId is also part of the ROI*/

        double *start = fiberPolyData->GetPoint( pointsInCell[startId] );
        points->InsertNextPoint(start);
        polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
        lineIndex++;
        pointIndex++;


      }

      // Insert the rest up and to including endId-1
      for( int pointInCellID( startId+1 ); pointInCellID < endId ; pointInCellID++)
      {
        // create new polyline for new polydata
        double *p = fiberPolyData->GetPoint( pointsInCell[ pointInCellID ] );
        points->InsertNextPoint(p);


        // add point to line
        polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
        lineIndex++;
        pointIndex++;

      }



      /* endId must be included if endId and endId-1 lie on the same side of the
         plane defined by endRoi*/


      p = fiberPolyData->GetPoint( pointsInCell[ endId ] );
      p0[0] = p[0];      p0[1] = p[1];      p0[2] = p[2];


      p = fiberPolyData->GetPoint( pointsInCell[ endId-1 ] );
      p1[0] = p[0];      p1[1] = p[1];      p1[2] = p[2];


      mitk::Point3D pLast;
      pLast[0] = p0[0];      pLast[1] = p0[1];      pLast[2] = p0[2];

      mitk::Point3D pBeforeLast;
      pBeforeLast[0] = p1[0];      pBeforeLast[1] = p1[1];      pBeforeLast[2] = p1[2];

      bool lastOnPositive = endGeometry2D->IsAbove(pLast);
      bool secondLastOnPositive = endGeometry2D->IsAbove(pBeforeLast);
      normal = endGeometry2D->GetNormal();


      onPlane[0] = endCenter[0];
      onPlane[1] = endCenter[1];
      onPlane[2] = endCenter[2];



      if(! (lastOnPositive ^ secondLastOnPositive) )
      {
        /* endId and endId-1 lie on the same side of the plane, so we need
           need endId+1 to calculate the intersection with the planar figure.
           this should exist since we know that the fiber crosses the planar figure
           endId is part of the roi so can also be included here*/
        p = fiberPolyData->GetPoint( pointsInCell[ endId+1 ] );
        p1[0] = p[0];
        p1[1] = p[1];
        p1[2] = p[2];


        double *end = fiberPolyData->GetPoint( pointsInCell[endId] );
        points->InsertNextPoint(end);
        polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
        lineIndex++;
        pointIndex++;
      }

      d = ( (onPlane-p0)*normal) / ( (p0-p1) * normal );

      newPoint = (p0-p1);

      newPoint[0] = d*newPoint[0] + p0[0];
      newPoint[1] = d*newPoint[1] + p0[1];
      newPoint[2] = d*newPoint[2] + p0[2];

      insertPoint[0] = newPoint[0];
      insertPoint[1] = newPoint[1];
      insertPoint[2] = newPoint[2];

      //Insert the Last Point (intersection with the end roi)

      points->InsertNextPoint(insertPoint);
      polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
      lineIndex++;
      pointIndex++;


    }

    // Need to reverse walking order
    else{
      double *p = fiberPolyData->GetPoint( pointsInCell[ startId ] );

      mitk::Vector3D p0;
      p0[0] = p[0];
      p0[1] = p[1];
      p0[2] = p[2];

      p = fiberPolyData->GetPoint( pointsInCell[ startId-1 ] );

      mitk::Vector3D p1;
      p1[0] = p[0];
      p1[1] = p[1];
      p1[2] = p[2];


      // Check if p and p2 are both on the same side of the plane
      mitk::Vector3D normal = startGeometry2D->GetNormal();

      mitk::Point3D pStart;
      pStart[0] = p0[0];
      pStart[1] = p0[1];
      pStart[2] = p0[2];

      bool startOnPositive = startGeometry2D->IsAbove(pStart);

      mitk::Point3D pSecond;
      pSecond[0] = p1[0];
      pSecond[1] = p1[1];
      pSecond[2] = p1[2];

      bool secondOnPositive = startGeometry2D->IsAbove(pSecond);


      // Calculate intersection with the plane

      mitk::Vector3D onPlane;
      onPlane[0] = startCenter[0];
      onPlane[1] = startCenter[1];
      onPlane[2] = startCenter[2];


      if(! (secondOnPositive ^ startOnPositive) )
      {
        /* startId and startId-1 lie on the same side of the plane, so we need
           need startId+1 to calculate the intersection with the planar figure*/
        p = fiberPolyData->GetPoint( pointsInCell[ startId+1 ] );
        p1[0] = p[0];
        p1[1] = p[1];
        p1[2] = p[2];
      }


      double d = ( (onPlane-p0)*normal) / ( (p0-p1) * normal );

      mitk::Vector3D newPoint = (p0-p1);

      newPoint[0] = d*newPoint[0] + p0[0];
      newPoint[1] = d*newPoint[1] + p0[1];
      newPoint[2] = d*newPoint[2] + p0[2];


      double insertPoint[3];
      insertPoint[0] = newPoint[0];
      insertPoint[1] = newPoint[1];
      insertPoint[2] = newPoint[2];


      // First insert the intersection with the start roi
      points->InsertNextPoint(insertPoint);
      polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
      lineIndex++;
      pointIndex++;

      if(! (secondOnPositive ^ startOnPositive) )
      {
        /* startId and startId-1 lie on the same side of the plane
           so endId is also part of the ROI*/

        double *start = fiberPolyData->GetPoint( pointsInCell[startId] );
        points->InsertNextPoint(start);
        polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
        lineIndex++;
        pointIndex++;


      }

      // Insert the rest up and to including endId-1
      for( int pointInCellID( startId-1 ); pointInCellID > endId ; pointInCellID--)
      {
        // create new polyline for new polydata
        double *p = fiberPolyData->GetPoint( pointsInCell[ pointInCellID ] );
        points->InsertNextPoint(p);


        // add point to line
        polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
        lineIndex++;
        pointIndex++;

      }



      /* startId must be included if startId and startId+ lie on the same side of the
         plane defined by endRoi*/


      p = fiberPolyData->GetPoint( pointsInCell[ endId ] );
      p0[0] = p[0];
      p0[1] = p[1];
      p0[2] = p[2];


      p = fiberPolyData->GetPoint( pointsInCell[ endId+1 ] );
      p1[0] = p[0];
      p1[1] = p[1];
      p1[2] = p[2];


      mitk::Point3D pLast;
      pLast[0] = p0[0];
      pLast[1] = p0[1];
      pLast[2] = p0[2];

      bool lastOnPositive = endGeometry2D->IsAbove(pLast);

      mitk::Point3D pBeforeLast;
      pBeforeLast[0] = p1[0];
      pBeforeLast[1] = p1[1];
      pBeforeLast[2] = p1[2];

      bool secondLastOnPositive = endGeometry2D->IsAbove(pBeforeLast);

      onPlane[0] = endCenter[0];
      onPlane[1] = endCenter[1];
      onPlane[2] = endCenter[2];



      if(! (lastOnPositive ^ secondLastOnPositive) )
      {
        /* endId and endId+1 lie on the same side of the plane, so we need
           need endId-1 to calculate the intersection with the planar figure.
           this should exist since we know that the fiber crosses the planar figure*/
        p = fiberPolyData->GetPoint( pointsInCell[ endId-1 ] );
        p1[0] = p[0];
        p1[1] = p[1];
        p1[2] = p[2];


        /* endId and endId+1 lie on the same side of the plane
           so startId is also part of the ROI*/

        double *end = fiberPolyData->GetPoint( pointsInCell[endId] );
        points->InsertNextPoint(end);
        polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
        lineIndex++;
        pointIndex++;
      }

      d = ( (onPlane-p0)*normal) / ( (p0-p1) * normal );

      newPoint = (p0-p1);

      newPoint[0] = d*newPoint[0] + p0[0];
      newPoint[1] = d*newPoint[1] + p0[1];
      newPoint[2] = d*newPoint[2] + p0[2];


      insertPoint[0] = newPoint[0];
      insertPoint[1] = newPoint[1];
      insertPoint[2] = newPoint[2];



      //Insert the Last Point (intersection with the end roi)
      points->InsertNextPoint(insertPoint);
      polyLine->GetPointIds()->InsertId(lineIndex,pointIndex);
      lineIndex++;
      pointIndex++;



    }


    // add polyline to vtk cell array
     cells->InsertNextCell(polyLine);

  }


  // Add the points to the dataset
  polyData->SetPoints(points);

  // Add the lines to the dataset
  polyData->SetLines(cells);

  mitk::FiberBundleX::Pointer cutBundle = mitk::FiberBundleX::New(polyData);


  mitk::DataNode::Pointer cutNode = mitk::DataNode::New();
  cutNode->SetData(cutBundle);
  std::string name = "fiberCut";
  cutNode->SetName(name);
  GetDataStorage()->Add(cutNode);



}


void QmitkTractbasedSpatialStatisticsView::StdMultiWidgetAvailable (QmitkStdMultiWidget &stdMultiWidget)
{
  m_MultiWidget = &stdMultiWidget;
}


void QmitkTractbasedSpatialStatisticsView::StdMultiWidgetNotAvailable()
{
  m_MultiWidget = NULL;
}

void QmitkTractbasedSpatialStatisticsView::AdjustPlotMeasure(const QString & text)
{
  berry::ISelection::ConstPointer sel(
    this->GetSite()->GetWorkbenchWindow()->GetSelectionService()->GetSelection("org.mitk.views.datamanager"));
  m_CurrentSelection = sel.Cast<const IStructuredSelection>();
  m_SelListener.Cast<TbssSelListener>()->DoSelectionChanged(sel);
}

void QmitkTractbasedSpatialStatisticsView::Clustering()
{
/*


  // Create a mask using the distance map
  typedef itk::ImageFileReader< VectorImageType > DirectionReader;
  DirectionReader::Pointer directionReader = DirectionReader::New();
  directionReader->SetFileName(m_TbssWorkspaceManager.GetInputDir().toStdString() + "/tbss/" + m_TbssWorkspaceManager.GetGradient().toStdString());
  directionReader->Update();
  VectorImageType::Pointer directions = directionReader->GetOutput();

  FloatReaderType::Pointer distMapReader = FloatReaderType::New();
  distMapReader->SetFileName(m_TbssWorkspaceManager.GetInputDir().toStdString() + "/stats/" + m_TbssWorkspaceManager.GetDistanceMap().toStdString());
  distMapReader->Update();
  FloatImageType::Pointer distanceMap = distMapReader->GetOutput();


  std::string line;
  std::string path = "/mnt/E130-Projekte/NeuroDiffusion/BRAM DTI/TBSS/rois/cc.txt";
  std::ifstream file(path.c_str());
  std::vector< itk::Index< 3 > > roi;

  if(file.is_open())
  {

    while(getline(file,line))
    {

      std::vector<std::string> tokens;
      Tokenize(line, tokens);

      itk::Index<3> ix;
      ix[0] = atoi(tokens[0].c_str());
      ix[1] = atoi(tokens[1].c_str());
      ix[2] = atoi(tokens[2].c_str());
      roi.push_back(ix);
    }
  }

  if(roi.size() == 0)
  {
    return;
  }



  // Some code from the projection algorithm of tbss to create a mask


  std::vector< std::vector< itk::Index< 3 > > > rois;

  for(int j=0; j<roi.size(); j++)
  {



    std::vector< itk::Index<3> > indices;

    FloatImageType::SizeType size = distanceMap->GetLargestPossibleRegion().GetSize();

    bool roiDone = false;

    while(!roiDone && j<roi.size())
    {
      if(j != 0 && j%5==0)
      {
        roiDone = true;
      }

      itk::Index<3> ix = roi[j];
      int x=ix[0];
      int y=ix[1];
      int z=ix[2];
      VectorImageType::PixelType dir = directions->GetPixel(ix);

      indices.push_back(ix);


      for(int iters=0;iters<2;iters++)
      {
        float distance=0;

        for(int d=1;d<MAXSEARCHLENGTH;d++)
        {
          int D=d;
          if (iters==1) D=-d;

          int dx = x+dir[0]*D, dy = y+dir[1]*D, dz = z+dir[2]*D;
          ix[0] = dx;
          ix[1] = dy;
          ix[2] = dz;

          if(dx<0 || dy<0 || dz<0
            || dx>=size[0] && dy<=size[1] && dz<=size[2])
          {
            d=MAXSEARCHLENGTH;
          }
          else if(distanceMap->GetPixel(ix)>=distance)
          {
            distance = distanceMap->GetPixel(ix);
            indices.push_back(ix);
          }
          else{
            d=MAXSEARCHLENGTH;
          }

        }

      }
      j++;
    }


    // Create a mask from indices
    UCharImageType::Pointer maskItk = UCharImageType::New();
    maskItk->SetRegions(distanceMap->GetRequestedRegion());
    maskItk->SetDirection(distanceMap->GetDirection());
    maskItk->SetSpacing(distanceMap->GetSpacing());
    maskItk->SetOrigin(distanceMap->GetOrigin());
    maskItk->Allocate();







    // For every point on the roi create a mask and feed it to the partial voluming algorithm


    //maskItk->FillBuffer(0);



    // Create a bounding box from current ROI
    int xMin = numeric_limits<int>::max();
    int yMin = numeric_limits<int>::max();
    int zMin = numeric_limits<int>::max();
    int xMax = numeric_limits<int>::min();
    int yMax = numeric_limits<int>::min();
    int zMax = numeriUCharImageType::Pointer newMask = UCharImageType::New();c_limits<int>::min();


    for(int i=0; i<indices.size(); i++)
    {
      itk::Index<3> ix = indices[i];

      if(ix[0] < xMin)
        xMin=ix[0];

      if(ix[1] < yMin)
        yMin=ix[1];

      if(ix[2] < zMin)
        zMin=ix[2];

      if(ix[0] > xMax)
        xMax=ix[0];

      if(ix[1] > yMax)
        yMax=ix[1];

      if(ix[2] > zMax)
        zMax=ix[2];
    }

    FloatImageType::PointType origin = distanceMap->GetOrigin();
    CharImageType::PointType originMask;
    originMask[0] = origin[0] + xMin;
    originMask[1] = origin[1] + -yMin;
    originMask[2] = origin[2] + zMin;

    CharImageType::RegionType region;
    CharImageType::RegionType::SizeType s;
    s[0] = xMax-xMin + 1;
    s[1] = yMax-yMin + 1;
    s[2] = zMax-zMin + 1;
    region.SetSize(s);


    UCharImageType::Pointer newMask = UCharImageType::New();
    newMask->SetSpacing( distanceMap->GetSpacing() );   // Set the image spacing
    newMask->SetOrigin( originMask );     // Set the image origin
    newMask->SetDirection( distanceMap->GetDirection() );  // Set the image direction
    newMask->SetRegions( region );
    newMask->Allocate();
    newMask->FillBuffer(0);



    for(int i=0; i<indices.size(); i++)
    {
      //convert the index to the new pixel index
      itk::Index<3> ix = indices[i];

      itk::Point< double, 3 > point;
      itk::Index<  3 > index;

      distanceMap->TransformIndexToPhysicalPoint (ix, point);



      newMask->TransformPhysicalPointToIndex(point, index);



      newMask->SetPixel(index, 1);
    }



*/



/*



    UCharImageType::Pointer newMask = UCharImageType::New();
    UCharReaderType::Pointer cReader = UCharReaderType::New();
    cReader->SetFileName("/mnt/E130-Projekte/NeuroDiffusion/BRAM DTI/ClusteringFornix/fornix_central_maxFA_path_Dilated_by_3.nrrd");
    cReader->Update();
    newMask = cReader->GetOutput();


    // mitk::DataNode::Pointer maskNode = readNode("itk image/mnt/E130-Projekte/NeuroDiffusion/BRAM DTI/TBSS/clusterMasks/area2.nii");
    // mitk::Image::Pointer mask = dynamic_cast<mitk::Image*>(maskNode->GetData());
    mitk::Image::Pointer mask;
    mitk::CastToMitkImage(newMask, mask);

    typedef mitk::PartialVolumeAnalysisHistogramCalculator  HistorgramCalculator;
    typedef mitk::PartialVolumeAnalysisClusteringCalculator ClusteringType;
    typedef HistorgramCalculator::HistogramType             HistogramType;

    HistorgramCalculator::Pointer histogramCalculator = HistorgramCalculator::New();





    // Make list of subjects

    std::vector<std::string> paths;
    paths.push_back("/mnt/E130-Projekte/NeuroDiffusion/BRAM DTI/TBSS/FA/SORTEDBYCONDITION/FA/subset");
   // paths.push_back("/mnt/E130-Projekte/NeuroDiffusion/BRAM DTI/TBSS/FA/SORTEDBYCONDITION/AXD/");

    for(int j=0; j<paths.size(); j++)
    {
      QDir currentDir = QDir(QString(paths.at(j).c_str()));
      currentDir.setFilter( QDir::Files );
      QStringList entries = currentDir.entryList();

      std::vector<double> values;

      for(int i=0; i<entries.size(); i++)
      {



        std::string file = paths.at(j) + entries.at(i).toStdString();
        mitk::DataNode::Pointer node = readNode(file);
        mitk::Image::Pointer image = dynamic_cast<mitk::Image*>(node->GetData());


        histogramCalculator->SetImage(image);
        histogramCalculator->SetImageMask( mask );
        histogramCalculator->SetMaskingModeToImage();
        histogramCalculator->SetNumberOfBins(25);
        histogramCalculator->SetUpsamplingFactor(5);
        histogramCalculator->SetGaussianSigma(0.0);
        histogramCalculator->SetForceUpdate(true);

        bool statisticsChanged = histogramCalculator->ComputeStatistics( );


        ClusteringType::ParamsType *cparams = 0;
        ClusteringType::ClusterResultType *cresult = 0;
        ClusteringType::HistType *chist = 0;
        ClusteringType::HelperStructPerformClusteringRetval *currentPerformClusteringResults;

        try{


          mitk::Image* tmpImg = histogramCalculator->GetInternalImage();
          mitk::Image::ConstPointer imgToCluster = tmpImg;

          if(imgToCluster.IsNotNull())
          {

            // perform clustering
            const HistogramType *histogram = histogramCalculator->GetHistogram( );
            ClusteringType::Pointer clusterer = ClusteringType::New();
            clusterer->SetStepsNumIntegration(200);
            clusterer->SetMaxIt(1000);

            mitk::Image::Pointer pFiberImg;

            currentPerformClusteringResults =
                clusterer->PerformClustering(imgToCluster, histogram, 2);

            pFiberImg = currentPerformClusteringResults->clusteredImage;
            cparams = currentPerformClusteringResults->params;
            cresult = currentPerformClusteringResults->result;
            chist = currentPerformClusteringResults->hist;


           // m_Controls->m_HistogramWidget->SetParameters(
              //  cparams, cresult, chist );



            std::vector<double> *xVals = chist->GetXVals();

            std::vector<double> *fiberVals = new std::vector<double>(cresult->GetFiberVals());



            double fiberFA = 0.0;
            double weights = 0.0;

           // std::cout << "x, y, fiber, nonFiber, mixed, combi" << std::endl;
            for(int k=0; k<xVals->size(); ++k)
            {

              fiberFA += xVals->at(k) * fiberVals->at(k);
              weights += fiberVals->at(k);


            }

            fiberFA = fiberFA / weights;
            std::cout << "FA: " << fiberFA << std::endl;
            values.push_back(fiberFA);

          }


        }

        catch ( const std::runtime_error &e )
        {
          std::cout << "noooooooooooooooooooooooooooooooo!";
        }

      //MITK_INFO << "number of voxels: " << indices.size();
      }

      std::vector<double>::iterator it = values.begin();
      while(it!=values.end())
      {
        std::cout << *it << std::endl;
        ++it;
      }

    }*/
}


void QmitkTractbasedSpatialStatisticsView::CreateRoi()
{

  // It is important to load the MeanFASkeletonMask image in MITK to make sure that point selection and
  // pathfinding is done on the same image
  //string filename = m_TbssWorkspaceManager.GetInputDir().toStdString() + "/stats/" + m_TbssWorkspaceManager.GetMeanFASkeletonMask().toStdString();

  // Implement a way to obtain skeleton and skeletonFA without sml workspace

  double threshold = QInputDialog::getDouble(m_Controls->m_CreateRoi, tr("Set an FA threshold"),
                                                      tr("Threshold:"), QLineEdit::Normal,
                                                      0.2);


  mitk::Image::Pointer image;

  for (IStructuredSelection::iterator i = m_CurrentSelection->Begin();
    i != m_CurrentSelection->End(); ++i)
  {
    // extract datatree node
    if (mitk::DataNodeObject::Pointer nodeObj = i->Cast<mitk::DataNodeObject>())
    {
      mitk::DataNode::Pointer node = nodeObj->GetDataNode();

      if(QString("Image").compare(node->GetData()->GetNameOfClass())==0)
      {
        mitk::Image* img = static_cast<mitk::Image*>(node->GetData());
        if(img->GetDimension() == 3)
        {
          image = img;
        }
      }
    }
  }



  if(image.IsNull())
  {
    return;
  }

  mitk::TractAnalyzer analyzer;
  analyzer.SetInputImage(image);
  analyzer.SetThreshold(threshold);



  int n = 0;
  if(m_PointSetNode.IsNotNull())
  {
    n = m_PointSetNode->GetSize();
    if(n==0)
    {
      QMessageBox msgBox;
      msgBox.setText("No points have been set yet.");
      msgBox.exec();
    }
  }
  else{
    QMessageBox msgBox;
    msgBox.setText("No points have been set yet.");
    msgBox.exec();
  }

  std::string pathDescription = "";
  std::vector< itk::Index<3> > totalPath;

  if(n>0)
  {
    for(int i=0; i<n-1; ++i)
    {
      mitk::Point3D p = m_PointSetNode->GetPoint(i);
      mitk::Point3D p2 = m_PointSetNode->GetPoint(i+1);


      itk::Index<3> StartPoint; mitk::ProgressBar::GetInstance()->Progress();

      itk::Index<3> EndPoint;
      image->GetGeometry()->WorldToIndex(p,StartPoint);
      image->GetGeometry()->WorldToIndex(p2,EndPoint);
      MITK_INFO << "create roi";


      analyzer.BuildGraph(StartPoint, EndPoint);
      std::vector< itk::Index<3> > path = analyzer.GetPath();


      for(std::vector< itk::Index<3> >::iterator it = path.begin();
          it != path.end(); it++)
      {
        itk::Index<3> ix = *it;

        if (!(ix==EndPoint))
        { mitk::ProgressBar::GetInstance()->Progress();

          totalPath.push_back(ix);
          std::stringstream ss;
          ss << ix[0] << " " << ix[1] << " " << ix[2] << "\n";
          pathDescription += ss.str();
        }
        else
        {
          // Only when dealing with the last segment the last point should be added. This one will not occur
          // as the first point of the next roi segment.
          if(i == (n-2))
          {
            totalPath.push_back(EndPoint);
            std::stringstream ss;
            ss << EndPoint[0] << " " << EndPoint[1] << " " << EndPoint[2] << "\n";
            pathDescription += ss.str();
          }

        }
      }



    }

    m_Controls->m_PathTextEdit->setPlainText(QString(pathDescription.c_str()));


    FloatImageType::Pointer itkImg = FloatImageType::New();
    mitk::CastToItkImage(image, itkImg);

    CharImageType::Pointer roiImg = CharImageType::New();
    roiImg->SetRegions(itkImg->GetLargestPossibleRegion().GetSize());
    roiImg->SetOrigin(itkImg->GetOrigin());
    roiImg->SetSpacing(itkImg->GetSpacing());
    roiImg->SetDirection(itkImg->GetDirection());
    roiImg->Allocate();
    roiImg->FillBuffer(0);


    std::vector< itk::Index<3> > roi;

    std::vector< itk::Index<3> >::iterator it;
    for(it = totalPath.begin();
        it != totalPath.end();
        it++)
    {
      itk::Index<3> ix = *it;
      roiImg->SetPixel(ix, 1);
      roi.push_back(ix);
    }


    mitk::TbssRoiImage::Pointer tbssRoi = mitk::TbssRoiImage::New();

    tbssRoi->SetRoi(roi);
    tbssRoi->SetImage(roiImg);
    tbssRoi->SetStructure(m_Controls->m_Structure->text().toStdString());
    tbssRoi->InitializeFromImage();

    AddTbssToDataStorage(tbssRoi, m_Controls->m_RoiName->text().toStdString());


  }

}



void QmitkTractbasedSpatialStatisticsView:: PlotFiberBundle(mitk::FiberBundleX *fib, mitk::Image* img,
                                                           mitk::PlanarFigure* startRoi, mitk::PlanarFigure* endRoi)
{
  bool avg = m_Controls->m_Average->isChecked();
  int segments = m_Controls->m_Segments->value();
  m_Controls->m_RoiPlotWidget->PlotFiberBetweenRois(fib, img, startRoi ,endRoi, avg, segments);
  m_Controls->m_RoiPlotWidget->SetPlottingFiber(true);
  mitk::RenderingManager::GetInstance()->ForceImmediateUpdateAll();
}


void QmitkTractbasedSpatialStatisticsView::Plot(mitk::TbssImage* image, mitk::TbssRoiImage* roiImage)
{
  if(m_Controls->m_TabWidget->currentWidget() == m_Controls->m_MeasureTAB)
  {

    std::vector< itk::Index<3> > roi = roiImage->GetRoi();
    m_Roi = roi;
    m_CurrentGeometry = image->GetGeometry();


    std::string resultfile = "";
    std::string structure = roiImage->GetStructure();

    m_Controls->m_RoiPlotWidget->SetGroups(image->GetGroupInfo());
    m_Controls->m_RoiPlotWidget->SetProjections(image->GetImage());
    m_Controls->m_RoiPlotWidget->SetRoi(roi);
    m_Controls->m_RoiPlotWidget->SetStructure(structure);
    m_Controls->m_RoiPlotWidget->SetMeasure( image->GetMeasurementInfo() );
    m_Controls->m_RoiPlotWidget->DrawProfiles(resultfile);
  }

  m_Controls->m_RoiPlotWidget->SetPlottingFiber(false);

}




void QmitkTractbasedSpatialStatisticsView::Masking()
{
  //QString filename = m_Controls->m_WorkingDirectory->text();
  QString filename = "E:/Experiments/tbss";
  QString faFiles = filename + "/AxD";
  QString maskFiles = filename + "/bin_masks";


  QDirIterator faDirIt(faFiles, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
  QDirIterator maskDirIt(maskFiles, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

  std::vector<std::string> faFilenames;
  std::vector<std::string> maskFilenames;
  std::vector<std::string> outputFilenames;

  while(faDirIt.hasNext() && maskDirIt.hasNext())
  {
    faDirIt.next();
    maskDirIt.next();
    if((faDirIt.fileInfo().completeSuffix() == "nii"
       || faDirIt.fileInfo().completeSuffix() == "mhd"
       || faDirIt.fileInfo().completeSuffix() == "nii.gz")
       && (maskDirIt.fileInfo().completeSuffix() == "nii"
       || maskDirIt.fileInfo().completeSuffix() == "mhd"
       || maskDirIt.fileInfo().completeSuffix() == "nii.gz"))
    {
      faFilenames.push_back(faDirIt.filePath().toStdString());
      outputFilenames.push_back(faDirIt.fileName().toStdString());
      maskFilenames.push_back(maskDirIt.filePath().toStdString());
    }
  }

  std::vector<std::string>::iterator faIt = faFilenames.begin();
  std::vector<std::string>::iterator maskIt = maskFilenames.begin();
  std::vector<std::string>::iterator outputIt = outputFilenames.begin();

  // Now multiply all FA images with their corresponding masks

  QString outputDir = filename;
  while(faIt != faFilenames.end() && maskIt != maskFilenames.end() && outputIt != outputFilenames.end())
  {
    std::cout << "Mask " << *faIt << " with " << *maskIt << std::endl;

    typedef itk::MultiplyImageFilter<FloatImageType, CharImageType, FloatImageType> MultiplicationFilterType;

    FloatReaderType::Pointer floatReader = FloatReaderType::New();
    CharReaderType::Pointer charReader = CharReaderType::New();

    floatReader->SetFileName(*faIt);
    //floatReader->Update();
    //FloatImageType::Pointer faImage = floatReader->GetOutput();

    charReader->SetFileName(*maskIt);
    //charReader->Update();
    // CharImageType::Pointer maskImage = charReader->GetOutput();

    MultiplicationFilterType::Pointer multiplicationFilter = MultiplicationFilterType::New();
    multiplicationFilter->SetInput1(floatReader->GetOutput());
    multiplicationFilter->SetInput2(charReader->GetOutput());
    multiplicationFilter->Update();

    //FloatImageType::Pointer maskedImage = FloatImageType::New();
    //maskedImage = MultiplicationFilter->GetOutput();

    FloatWriterType::Pointer floatWriter = FloatWriterType::New();
    std::string s = faFiles.toStdString().append("/"+*outputIt);
    floatWriter->SetFileName(s.c_str());
    floatWriter->SetInput(multiplicationFilter->GetOutput());
    floatWriter->Update();

    ++faIt;
    ++maskIt;
    ++outputIt;
  }
}


VectorImageType::Pointer QmitkTractbasedSpatialStatisticsView::ConvertToVectorImage(mitk::Image::Pointer mitkImage)
{
  VectorImageType::Pointer vecImg = VectorImageType::New();

  mitk::Geometry3D* geo = mitkImage->GetGeometry();
  mitk::Vector3D spacing = geo->GetSpacing();
  mitk::Point3D origin = geo->GetOrigin();

  VectorImageType::SpacingType vecSpacing;
  vecSpacing[0] = spacing[0];
  vecSpacing[1] = spacing[1];
  vecSpacing[2] = spacing[2];

  VectorImageType::PointType vecOrigin;
  vecOrigin[0] = origin[0];
  vecOrigin[1] = origin[1];
  vecOrigin[2] = origin[2];

  VectorImageType::SizeType size;
  size[0] = mitkImage->GetDimension(0);
  size[1] = mitkImage->GetDimension(1);
  size[2] = mitkImage->GetDimension(2);


  vecImg->SetSpacing(vecSpacing);
  vecImg->SetOrigin(vecOrigin);
  vecImg->SetRegions(size);
  vecImg->SetVectorLength(mitkImage->GetDimension(3));
  vecImg->Allocate();


  for(int x=0; x<size[0]; x++)
  {
    for(int y=0; y<size[1]; y++)
    {
      for(int z=0; z<size[2]; z++)
      {
        mitk::Index3D ix;
        ix[0] = x;
        ix[1] = y;
        ix[2] = z;


        itk::VariableLengthVector<float> pixel = vecImg->GetPixel(ix);

        for (int t=0; t<pixel.Size(); t++)
        {
          float f = mitkImage->GetPixelValueByIndex(ix, t);
          pixel.SetElement(t, f);
        }
        vecImg->SetPixel(ix, pixel);
      }
    }
  }

  return vecImg;

}


/*
void QmitkTractbasedSpatialStatisticsView::InitializeGridByVectorImage()
{
  // Read vector image from file
  typedef itk::ImageFileReader< FloatVectorImageType > VectorReaderType;
  VectorReaderType::Pointer vectorReader = VectorReaderType::New();
  vectorReader->SetFileName("E:\\tbss\\testing\\Gradient.mhd");
  vectorReader->Update();
  FloatVectorImageType::Pointer directions = vectorReader->GetOutput();


  // Read roi from file.
  CharReaderType::Pointer roiReader = CharReaderType::New();
  roiReader->SetFileName("E:\\tbss\\testing\\debugging skeletonization\\segment2.mhd");
  roiReader->Update();
  CharImageType::Pointer roi = roiReader->GetOutput();

  DoInitializeGridByVectorImage(directions, roi, std::string("directions"));


}




void QmitkTractbasedSpatialStatisticsView::DoInitializeGridByVectorImage(FloatVectorImageType::Pointer vectorpic, CharImageType::Pointer roi, std::string name)
{
  //vtkStructuredGrid* grid = vtkStructuredGrid::New();
  itk::Matrix<double,3,3> itkdirection = vectorpic->GetDirection();
  itk::Matrix<double,3,3> itkinversedirection = itk::Matrix<double,3,3>(itkdirection.GetInverse());
  std::vector<VectorType> GridPoints;
  vtkPoints *points = vtkPoints::New();
  mitk::Geometry3D::Pointer geom = mitk::Geometry3D::New();
  vtkLinearTransform *vtktransform;
  vtkLinearTransform *inverse;
  mitk::Image::Pointer geomget = mitk::Image::New();
  geomget->InitializeByItk(vectorpic.GetPointer());
  geom = geomget->GetGeometry();
  vtktransform = geom->GetVtkTransform();
  inverse = vtktransform->GetLinearInverse();
  vtkFloatArray * directions = vtkFloatArray::New();
  directions->SetName("Vectors");
  directions->SetNumberOfComponents(3);

  // Iterator for the vector image
  itk::ImageRegionIterator<FloatVectorImageType> it_input(vectorpic, vectorpic->GetLargestPossibleRegion());
  FloatVectorType nullvector;
  nullvector.Fill(0);
  double lengthsum = 0;
  int id = 0;

  // Iterator for the roi
  itk::ImageRegionIterator<CharImageType> roiIt(roi, roi->GetLargestPossibleRegion());
  roiIt.GoToBegin();

  for(it_input.GoToBegin(); !( it_input.IsAtEnd() || roiIt.IsAtEnd() ); ++it_input)
  {
    //VectorType val = it_input.Value();
    if(it_input.Value() != nullvector && roiIt.Get() != 0)
    {
      //itk::Point<double,3> point;
      mitk::Point3D mitkpoint, mitkworldpoint;
      mitk::Point3D mitkendpoint, mitkworldendpoint;
      mitk::Vector3D mitkvector, mitktransvector;
      itk::Point<float,3> direction = it_input.Value().GetDataPointer();
      //itk::Index<3> in_input = it_input.GetIndex();
      //itk::ContinuousIndex<int,3> cindirection;
      FloatVectorType transvec = it_input.Value();
      mitkvector[0] = transvec[0];
      mitkvector[1] = transvec[1];
      mitkvector[2] = transvec[2];
      //mitkvector[2] = 0.0;


      mitkpoint[0] = it_input.GetIndex()[0];
      mitkpoint[1] = it_input.GetIndex()[1];
      mitkpoint[2] = it_input.GetIndex()[2];

      mitkendpoint[0] = mitkpoint[0] + mitkvector[0];
      mitkendpoint[1] = mitkpoint[1] + mitkvector[1];
      mitkendpoint[2] = mitkpoint[2] + mitkvector[2];

      //mitkpoint.setXYZ((ScalarType)point[0],(ScalarType)point[1],(ScalarType)point[2]);
      geom->IndexToWorld(mitkpoint, mitkworldpoint);
      geom->IndexToWorld(mitkendpoint, mitkworldendpoint);
      mitktransvector[0] = mitkworldendpoint[0] - mitkworldpoint[0];
      mitktransvector[1] = mitkworldendpoint[1] - mitkworldpoint[1];
      mitktransvector[2] = mitkworldendpoint[2] - mitkworldpoint[2];

      lengthsum += mitktransvector.GetNorm();

      directions->InsertTuple3(id,mitktransvector[0],mitktransvector[1],mitktransvector[2]);

      points->InsertPoint(id,mitkworldpoint[0],mitkworldpoint[1],mitkworldpoint[2]);
      id++;
      //for (unsigned short loop = 0; (loop < 20) && (!it_input.IsAtEnd()); loop++)
      //{
      //  ++it_input;
      //}
      if(it_input.IsAtEnd())
      {
        break;
      }
    }
    ++roiIt;
  }
  double meanlength = lengthsum / id;
  vtkGlyph3D* glyph = vtkGlyph3D::New();
  vtkUnstructuredGrid* ugrid = vtkUnstructuredGrid::New();
  ugrid->SetPoints(points);
  ugrid->GetPointData()->SetVectors(directions);
  glyph->SetInput(ugrid);
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.5);
  glyph->SetColorModeToColorByScalar();
  //glyph->ClampingOn();
  vtkArrowSource* arrow = vtkArrowSource::New();
  if(meanlength > 5) {arrow->SetTipLength(0);arrow->SetTipRadius(0);}

  arrow->SetShaftRadius(0.03/meanlength);
  //arrow->SetTipRadius(0.05/meanlength);

  glyph->SetSource(arrow->GetOutput());
  glyph->Update();

  mitk::Surface::Pointer glyph_surface = mitk::Surface::New();

  glyph_surface->SetVtkPolyData(glyph->GetOutput());
  glyph_surface->UpdateOutputInformation();

  mitk::DataNode::Pointer gridNode = mitk::DataNode::New();
  gridNode->SetProperty( "name", mitk::StringProperty::New(name.c_str()) );
  //m_GridNode->SetProperty( "color" , m_GridColor);
  gridNode->SetProperty( "visible", mitk::BoolProperty::New(true) );
  gridNode->SetProperty( "segmentation", mitk::BoolProperty::New(true) );
  gridNode->SetProperty( "ID-Tag", mitk::StringProperty::New("grid") );
  gridNode->SetProperty( "shader", mitk::StringProperty::New("mitkShaderLightning") );
  gridNode->SetData( glyph_surface );
  GetDefaultDataStorage()->Add(gridNode);


}

*/
