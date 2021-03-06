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

#ifndef mitkPaintContourTool_h_Included
#define mitkPaintContourTool_h_Included

#include "mitkPaintbrushTool.h"
#include "SegmentationExports.h"

namespace mitk
{

 /**
  \brief Paintbrush tool for InteractiveSegmentation

  \sa FeedbackContourTool
  \sa ExtractImageFilter
  \sa OverwriteSliceImageFilter

  \ingroup Interaction
  \ingroup ToolManagerEtAl

  Simple paintbrush drawing tool. Right now there are only circular pens of varying size.

  This class specified only the drawing "color" for the super class PaintbrushTool.

  \warning Only to be instantiated by mitk::ToolManager.
  $Author: maleike $
*/
class Segmentation_EXPORT DrawPaintbrushTool : public PaintbrushTool
{
  public:

    mitkClassMacro(DrawPaintbrushTool, PaintbrushTool);
    itkNewMacro(DrawPaintbrushTool);

    virtual const char** GetXPM() const;
    virtual const char* GetName() const;

  protected:

    DrawPaintbrushTool(); // purposely hidden
    virtual ~DrawPaintbrushTool();

};

} // namespace

#endif

