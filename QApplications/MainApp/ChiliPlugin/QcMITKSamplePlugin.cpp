#include <chili/task.h>
#include <chili/plugin.xpm>
#include <chili/qclightbox.h>
#include <chili/qclightboxmanager.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qptrlist.h>
#include <qbuttongroup.h>
#include <mitkDataTree.h>
#include <QmitkStdMultiWidget.h>
#include <mitkLightBoxImageReader.h>
#include "SampleApp.h"
#include "QcMITKSamplePlugin.h"
#include "mitkDataTreeNodeFactory.h"
#include "mitkProperties.h"
#include "mitkLevelWindowProperty.h"
#include "mitkStringProperty.h"
#include "ToolBar.h"
#include <mitkChiliPlugin.h>

#include <qmessagebox.h>

#include <ipPic.h>
#include <ipFunc.h>
#include <ipDicom/ipDicom.h>

#include "QmitkCommonFunctionality.h"



QcPlugin* QcMITKSamplePlugin::s_PluginInstance = NULL;

QcMITKSamplePlugin::QcMITKSamplePlugin( QWidget *parent )
  : QcPlugin( parent ), ap(NULL)
{
  task = new QcTask( xpm(), parent, name() );

  toolbar =new ToolBar(task,this);

  QButtonGroup* tb;
  tb=toolbar->GetToolBar();
    
  connect(toolbar,SIGNAL(LightboxSelected(QcLightbox*)),this,SLOT(selectSerie(QcLightbox*)));
  connect(toolbar,SIGNAL(ChangeWidget()),this,SLOT(CreateNewSampleApp()));
  activated =false;
   
  s_PluginInstance = this;

  mitk::ChiliPlugin::SetPluginInstance(s_PluginInstance);
}

QString 
QcMITKSamplePlugin::name() 
{ 
  return QString( "MITK PlugIn" );
}

const char ** 
QcMITKSamplePlugin::xpm()
{ 
  return (const char **)plugin_xpm; 
}

QcMITKSamplePlugin::~QcMITKSamplePlugin ()
{
  task->deleteLater();
}

extern "C" QcEXPORT QObject *
create( QWidget *parent )
{
return new QcMITKSamplePlugin( parent );  
}

void 	QcMITKSamplePlugin::lightboxFilled (QcLightbox* lightbox)
{
  /////itkGenericOutputMacro(<<"lightbox filled");
  QcLightboxManager *lbm=lightboxManager();
  QPtrList<QcLightbox> list;
  list=lbm->getLightboxes();
  QButtonGroup* tb;
  tb=toolbar->GetToolBar();
  int id=tb->id(tb->selected());
  if (id>0)
  {
    if (!tb->find(6)->isOn())
      if (activated&&(list.take(id-1))->isActive())
          selectSerie(lightbox);
  }
  else
    if (tb->find((list.find(lightbox))+1)->isOn())
          selectSerie(lightbox);
       
}

void 	QcMITKSamplePlugin::lightboxTiles (QcLightboxManager *lbm, int tiles)
{
  /////itkGenericOutputMacro(<<"lightbox tiles: "<<tiles);
  toolbar->ConnectButton(tiles);
  
}

void QcMITKSamplePlugin::selectSerie (QcLightbox* lightbox)
{
  /////itkGenericOutputMacro(<<"selectSerie");
  activated=true;
  if(lightbox->getFrames()==0)
    return;
  
  QObject * parent = this->parent();
  QWidget * parentWidget = (QWidget *)parent;

  if (parentWidget->isVisible()) {
    itkGenericOutputMacro(<<"plugin active");
  } else
  {
    itkGenericOutputMacro(<<"plugin not active");
  }
  

  
  mitk::Image::Pointer image = mitk::Image::New();
  
  mitk::LightBoxImageReader::Pointer reader=mitk::LightBoxImageReader::New();
  reader->SetLightBox(lightbox);
  
  image=reader->GetOutput();
  image->Update();

  if ( image.IsNotNull() )
  {
    unsigned long initTime = ap->GetMultiWidget()->GetRenderWindow1()->GetRenderer()->GetMTime();

    mitk::DataTree::Pointer tree = ap->GetTree();
    mitk::DataTreePreOrderIterator it(tree);
    mitk::DataTreeNode::Pointer node = mitk::DataTreeNode::New();
    node->SetData(image);
    
    mitk::LevelWindowProperty::Pointer levWinProp = new mitk::LevelWindowProperty();
    mitk::LevelWindow levelwindow;
    levelwindow.SetAuto( reader->GetOutput() );
    levWinProp->SetLevelWindow( levelwindow );
    node->GetPropertyList()->SetProperty( "levelwindow", levWinProp );
    node->SetProperty("LoadedFromChili", new mitk::BoolProperty( true ) );
    mitk::DataTreeNodeFactory::SetDefaultImageProperties(node);

    ipPicDescriptor *pic = reader->GetOutput()->GetPic();

    ipPicTSV_t* uid= ipPicQueryTag(pic, "IMAGE INSTANCE UID");
    ipPicTSV_t* description= ipPicQueryTag(pic, "SERIES DESCRIPTION");
    ipPicTSV_t* patientName= ipPicQueryTag(pic, "PATIENT NAME");

    int laenge = patientName->n[0];
    char* patName = new char[laenge+1];
    strncpy(patName, (char*)(patientName->value), laenge);
    patName[laenge] = 0;
    //char a;
    //while (*patName != 0) 
    //{
    //  a = *patName;
    //  patName++;
    //}

    std::string test((char*)patientName->value );
    int lenght = test.length();
    
   if (description)
      node->SetProperty("name", new mitk::StringProperty( (char*)uid->value ));
    else if (uid)
      node->SetProperty("name", new mitk::StringProperty( (char*)description->value ));

    else if (patientName)
      //node->SetProperty("name", new mitk::StringProperty( (char*)patientName->value ));
      //node->SetProperty("name", new mitk::StringProperty( test.c_str() ));
      node->SetProperty("name", new mitk::StringProperty( patName ));
    else 
      node->SetProperty("name", new mitk::StringProperty( lightbox->name() ));

    bool notAlreadyInDataTree = true;

    ipPicTSV_t* tsv = ipPicQueryTag(pic, "SOURCE HEADER");
    void* data;
    ipUInt4_t len;
    if (tsv)
    {
      //0x0018, 0x1030 : Protocol Name - ist bei Morphologie/Phase gleich
      //0x0008, 0x103e : Series Description - ist der in der unter "Beschreibung" stehende name (_bh/_bh_P)
      // aber: wahrscheinlich unsicher, da vom Benutzer eingegeben
      //eventuell brauchbar: 0x0018, 0x0024 : Sequence Name
      char * tmp;
      if (dicomFindElement((unsigned char*) tsv->value, 0x0018, 0x1030, &data, &len))
      {
        tmp = new char[len+1];
        strncpy(tmp, (char*)data, len);
        tmp[len]=0;
        node->SetProperty("protocol", new mitk::StringProperty( tmp ));
        delete [] tmp;
      }

      if (dicomFindElement((unsigned char*) tsv->value, 0x0008, 0x0070, &data, &len))
      {
        tmp = new char[len+1];
        strncpy(tmp, (char*)data, len);
        tmp[len]=0;
        node->SetProperty("manufacturer", new mitk::StringProperty( tmp ));
        delete [] tmp;
      }

      if (dicomFindElement((unsigned char*) tsv->value, 0x0020, 0x000e, &data, &len))
      {
        dicomFindElement((unsigned char*) tsv->value, 0x0020, 0x000e, &data, &len);
        tmp = new char[len+1];
        strncpy(tmp, (char*)data, len);
        tmp[len]=0;
        node->SetProperty("series uid", new mitk::StringProperty( tmp ));

        mitk::DataTreeNode* existingNode = CommonFunctionality::GetFirstNodeByProperty(it,"series uid", new mitk::StringProperty( tmp ));

        if ( existingNode )
        {
          notAlreadyInDataTree = false;
        }

        delete [] tmp;
      }      

      
      if ( dicomFindElement((unsigned char*) tsv->value, 0x0008, 0x103e, &data, &len) )
      {
        tmp = new char[len+1];
        strncpy(tmp, (char*)data, len);
        tmp[len]=0;
        node->SetProperty("series description", new mitk::StringProperty( tmp ));
        delete [] tmp;
      }
    }
        //dicomFindElement((unsigned char*) tsv->value, 0x0020, 0x0013, &data, &len);
        //sscanf( (char *) data, "%d", &imageNumber );
        //info.imageNumber=imageNumber;
    

    if (notAlreadyInDataTree)
    {
      it.Add(node);
    }
    
    if((ap->GetMultiWidget()->GetRenderWindow1()->GetRenderer()->GetMTime()==initTime) && (ap->GetStandardViewsInitialized()==false))
    {
      ap->SetStandardViewsInitialized(ap->GetMultiWidget()->InitializeStandardViews(&it));
    }

    ap->GetMultiWidget()->Repaint();
    ap->GetMultiWidget()->Fit();

  }

}


void QcMITKSamplePlugin::CreateNewSampleApp()
{
  if(ap!=NULL)
    delete ap;
  ap = new SampleApp(task,"sample",0);
  toolbar->SetWidget(ap);
}
