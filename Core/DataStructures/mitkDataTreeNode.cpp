#include "DataTreeNode.h"
#include "MapperFactory.h"
#include <vtkTransform.h>

#include "mitkFloatProperty.h"
#include "mitkBoolProperty.h"
#include "mitkColorProperty.h"
#include "mitkLevelWindowProperty.h"

//##ModelId=3D6A0E8C02CC
mitk::Mapper* mitk::DataTreeNode::GetMapper(MapperSlotId id) const
{
    // TODO
    
    if (mappers[id].IsNull()) {
        
        mappers[id] = MapperFactory::CreateMapper(const_cast<DataTreeNode*>(this),id);
        
    }
    
    // mappers[id]->AddInput(GetData());
    
    return mappers[id];
    
    
    
    //  return NULL;
    
}

//##ModelId=3E32C49D00A8
mitk::BaseData* mitk::DataTreeNode::GetData() const
{
    return m_Data;
}

//##ModelId=3ED91D050121
vtkTransform* mitk::DataTreeNode::GetVtkTransform() const
{
    return m_VtkTransform;
}

//##ModelId=3E33F4E4025B
void mitk::DataTreeNode::SetData(mitk::BaseData* baseData)
{
    if(m_Data!=baseData)
    {
        m_Data=baseData;
        Modified();
    }
}

//##ModelId=3E33F5D702AA
mitk::DataTreeNode::DataTreeNode() : m_Data(NULL)
{
    memset(mappers, 0, sizeof(mappers)); 
    
    m_PropertyList = PropertyList::New();
    
    m_VtkTransform = vtkTransform::New();
}


//##ModelId=3E33F5D702D3
mitk::DataTreeNode::~DataTreeNode()
{
    m_VtkTransform->Delete();
}

//##ModelId=3E33F5D7032D
mitk::DataTreeNode& mitk::DataTreeNode::operator=(const DataTreeNode& right)
{
    mitk::DataTreeNode* node=mitk::DataTreeNode::New();
    node->SetData(right.GetData());
    return *node;
}

mitk::DataTreeNode& mitk::DataTreeNode::operator=(mitk::BaseData* right)
{
    mitk::DataTreeNode* node=mitk::DataTreeNode::New();
    node->SetData(right);
    return *node;
}

#if (_MSC_VER > 1200) || !defined(_MSC_VER)
MBI_STD::istream& mitk::operator>>( MBI_STD::istream& i, mitk::DataTreeNode::Pointer& dtn )
#endif
#if ((defined(_MSC_VER)) && (_MSC_VER <= 1200))
MBI_STD::istream& operator>>( MBI_STD::istream& i, mitk::DataTreeNode::Pointer& dtn ) 
#endif
{
    dtn = mitk::DataTreeNode::New();
    //i >> av.get();
    return i;
}

#if (_MSC_VER > 1200) || !defined(_MSC_VER)
MBI_STD::ostream& mitk::operator<<( MBI_STD::ostream& o, mitk::DataTreeNode::Pointer& dtn)
#endif
#if ((defined(_MSC_VER)) && (_MSC_VER <= 1200))
MBI_STD::ostream& operator<<( MBI_STD::ostream& o, mitk::DataTreeNode::Pointer& dtn)
#endif
{
    if(dtn->GetData()!=NULL)
        o<<dtn->GetData()->GetNameOfClass();
    else
        o<<"empty data";
    return o;
}

//##ModelId=3E69331903C9
void mitk::DataTreeNode::SetMapper(MapperSlotId id, mitk::Mapper* mapper)
{
    mappers[id] = mapper;
    
    if (mapper!=NULL)
        mapper->SetInput(this);
}

//##ModelId=3E860A5C0032
void mitk::DataTreeNode::UpdateOutputInformation()
{
    if (this->GetSource())
    {
        this->GetSource()->UpdateOutputInformation();
    }
}

//##ModelId=3E860A5E011B
void mitk::DataTreeNode::SetRequestedRegionToLargestPossibleRegion()
{
}

//##ModelId=3E860A5F03D9
bool mitk::DataTreeNode::RequestedRegionIsOutsideOfTheBufferedRegion()
{
    return false;
}

//##ModelId=3E860A620080
bool mitk::DataTreeNode::VerifyRequestedRegion()
{
    return GetData()!=NULL;
}

//##ModelId=3E860A640156
void mitk::DataTreeNode::SetRequestedRegion(itk::DataObject *data)
{
}

//##ModelId=3E860A6601DB
void mitk::DataTreeNode::CopyInformation(const itk::DataObject *data)
{
}
//##ModelId=3E3FE0420273
mitk::PropertyList::Pointer mitk::DataTreeNode::GetPropertyList(const mitk::BaseRenderer* renderer) const
{
    if(renderer==NULL)
        return m_PropertyList;
    
    mitk::PropertyList::Pointer & propertyList = m_MapOfPropertyLists[renderer];
    
    if(propertyList.IsNull())
        propertyList = mitk::PropertyList::New();
    
    assert(m_MapOfPropertyLists[renderer].IsNotNull());
    
    return propertyList;
}

//##ModelId=3EF189DB0111
mitk::BaseProperty::Pointer mitk::DataTreeNode::GetProperty(const char *propertyKey, const mitk::BaseRenderer* renderer) const
{
    std::map<const mitk::BaseRenderer*,mitk::PropertyList::Pointer>::const_iterator it;
    
    //does a renderer-specific PropertyList exist?
    it=m_MapOfPropertyLists.find(renderer);
    if(it==m_MapOfPropertyLists.end())
        //no? use the renderer-independent one!
        return m_PropertyList->GetProperty(propertyKey);
    
    //does the renderer-specific PropertyList contain the @a propertyKey?
    mitk::BaseProperty::Pointer property;
    property=it->second->GetProperty(propertyKey);
    if(property.IsNotNull())
        //yes? return it
        return property;
    
    //no? use the renderer-independent one!
    return m_PropertyList->GetProperty(propertyKey);
}//##ModelId=3EF1941C011F

bool mitk::DataTreeNode::GetColor(float rgb[3], mitk::BaseRenderer* renderer, const char* name) const
{
    mitk::ColorProperty::Pointer colorprop = dynamic_cast<mitk::ColorProperty*>(GetProperty(name, renderer).GetPointer());
    if(colorprop.IsNull())
        return false;
    
    memcpy(rgb, colorprop->GetColor().GetDataPointer(), 3*sizeof(float));
    return true;
}

//##ModelId=3EF1941E01D6
bool mitk::DataTreeNode::GetVisibility(bool &visible, mitk::BaseRenderer* renderer, const char* name) const
{
    mitk::BoolProperty::Pointer boolprop = dynamic_cast<mitk::BoolProperty*>(GetProperty(name, renderer).GetPointer());
    if(boolprop.IsNull())
        return false;
    
    visible = boolprop->GetBool();
    return true;
}

//##ModelId=3EF19420016B
bool mitk::DataTreeNode::GetOpacity(float &opacity, mitk::BaseRenderer* renderer, const char* name) const
{
    mitk::FloatProperty::Pointer opacityprop = dynamic_cast<mitk::FloatProperty*>(GetProperty(name, renderer).GetPointer());
    if(opacityprop.IsNull())
        return false;
    
    opacity=opacityprop->GetValue();
    return true;
}

//##ModelId=3EF194220204
bool mitk::DataTreeNode::GetLevelWindow(mitk::LevelWindow &levelWindow, mitk::BaseRenderer* renderer, const char* name) const
{
    mitk::LevelWindowProperty::Pointer levWinProp = dynamic_cast<mitk::LevelWindowProperty*>(GetProperty(name, renderer).GetPointer());
    if(levWinProp.IsNull())
        return false;
    
    levelWindow=levWinProp->GetLevelWindow();
    return true;
}

//##ModelId=3EF19424012B
bool mitk::DataTreeNode::IsVisible(mitk::BaseRenderer* renderer, const char* name) const
{
    bool visible=true;
    GetVisibility(visible, renderer, name);
    return visible;
}

//##ModelId=3EF196360303
void mitk::DataTreeNode::SetColor(const float rgb[3], mitk::BaseRenderer* renderer, const char* name)
{
    mitk::ColorProperty::Pointer prop;
    prop = new mitk::ColorProperty(rgb);
    GetPropertyList(renderer)->SetProperty(name, prop);
}

//##ModelId=3EF1966703D6
void mitk::DataTreeNode::SetVisibility(bool visible, mitk::BaseRenderer* renderer, const char* name)
{
    mitk::BoolProperty::Pointer prop;
    prop = new mitk::BoolProperty(visible);
    GetPropertyList(renderer)->SetProperty(name, prop);
}

//##ModelId=3EF196880095
void mitk::DataTreeNode::SetOpacity(float opacity, mitk::BaseRenderer* renderer, const char* name)
{
    mitk::FloatProperty::Pointer prop;
    prop = new mitk::FloatProperty(opacity);
    GetPropertyList(renderer)->SetProperty(name, prop);
}

//##ModelId=3EF1969A0181
void mitk::DataTreeNode::SetLevelWindow(mitk::LevelWindow levelWindow, mitk::BaseRenderer* renderer, const char* name)
{
    mitk::LevelWindowProperty::Pointer prop;
    prop = new mitk::LevelWindowProperty(levelWindow);
    GetPropertyList(renderer)->SetProperty(name, prop);
}

void mitk::DataTreeNode::SetProperty(const char *propertyKey, 
                                     BaseProperty* propertyValue, 
                                     const mitk::BaseRenderer* renderer)
{
    GetPropertyList(renderer)->SetProperty(propertyKey, propertyValue);
}
