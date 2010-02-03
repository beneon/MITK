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

#include "Poco/ClassLibrary.h"

#include <berryIBundleActivator.h>
#include "src/internal/berryQtLogPlugin.h"

#include <berryIViewPart.h>
#include "src/internal/berryLogView.h"

POCO_BEGIN_MANIFEST(berry::IBundleActivator)
  POCO_EXPORT_CLASS(berry::QtLogPlugin)
POCO_END_MANIFEST

POCO_BEGIN_NAMED_MANIFEST(berryIViewPart, berry::IViewPart)
  POCO_EXPORT_CLASS(berry::LogView)
POCO_END_MANIFEST