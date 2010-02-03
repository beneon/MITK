/*=========================================================================
 
 Program:   BlueBerry Platform
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


#ifndef BERRYIDEBUGOBJECTLISTENER_H_
#define BERRYIDEBUGOBJECTLISTENER_H_

#include <berryObject.h>
#include <berryMacros.h>

#include "berryOSGiDll.h"

namespace berry
{

struct BERRY_OSGI IDebugObjectListener: public Object
{

  berryInterfaceMacro(IDebugObjectListener, berry);

  struct BERRY_OSGI Events {

    enum Type {
     NONE                   = 0x00000000,
     OBJECT_CREATED         = 0x00000001,
     OBJECT_DESTROYED       = 0x00000002,
     OBJECT_TRACING         = 0x00000004,
     SMARTPOINTER_CREATED   = 0x00000008,
     SMARTPOINTER_DESTROYED = 0x00000010,

     ALL       = 0xffffffff
    };

    BERRY_DECLARE_FLAGS(Types, Type)

    typedef Message1<const Object*> ObjectEventType;
    typedef Message3<unsigned int, bool, const Object*> TracingEventType;
    typedef Message2<unsigned int, const Object*> SmartPointerEventType;

    ObjectEventType objCreatedEvent;
    ObjectEventType objDestroyedEvent;
    TracingEventType objTracingEvent;
    SmartPointerEventType spCreatedEvent;
    SmartPointerEventType spDestroyedEvent;

    void AddListener(IDebugObjectListener::Pointer listener);
    void RemoveListener(IDebugObjectListener::Pointer listener);

    typedef MessageDelegate1<IDebugObjectListener, const Object*> ObjDelegate;
    typedef MessageDelegate3<IDebugObjectListener, unsigned int, bool, const Object*> TraceDelegate;
    typedef MessageDelegate2<IDebugObjectListener, unsigned int, const Object*> SPDelegate;
  };

  virtual ~IDebugObjectListener()
  {
  }

  virtual Events::Types GetEventTypes() const = 0;

  virtual void ObjectCreated(const Object*  /*obj*/)
  {
  }

  virtual void ObjectDestroyed(const Object*  /*obj*/)
  {
  }

  virtual void ObjectTracingChanged(unsigned int  /*traceId*/, bool  /*enabled*/ = true, const Object*  /*obj*/ = 0)
  {
  }

  virtual void SmartPointerCreated(unsigned int  /*id*/, const Object*  /*obj*/)
  {
  }

  virtual void SmartPointerDestroyed(unsigned int  /*id*/, const Object*  /*obj*/)
  {
  }

};

}

BERRY_DECLARE_OPERATORS_FOR_FLAGS(berry::IDebugObjectListener::Events::Types)

#endif /* BERRYIDEBUGOBJECTLISTENER_H_ */