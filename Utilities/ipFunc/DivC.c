/*****************************************************************************

 Copyright (c) 1993-2000,  Div. Medical and Biological Informatics, 
 Deutsches Krebsforschungszentrum, Heidelberg, Germany
 All rights reserved.

 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice, 
   this list of conditions and the following disclaimer in the documentation 
   and/or other materials provided with the distribution.

 - All advertising materials mentioning features or use of this software must 
   display the following acknowledgement: 
          
     "This product includes software developed by the Div. Medical and 
      Biological Informatics, Deutsches Krebsforschungszentrum, Heidelberg, 
      Germany."

 - Neither the name of the Deutsches Krebsforschungszentrum nor the names of 
   its contributors may be used to endorse or promote products derived from 
   this software without specific prior written permission. 

   THIS SOFTWARE IS PROVIDED BY THE DIVISION MEDICAL AND BIOLOGICAL
   INFORMATICS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE DIVISION MEDICAL AND BIOLOGICAL INFORMATICS,
   THE DEUTSCHES KREBSFORSCHUNGSZENTRUM OR CONTRIBUTORS BE LIABLE FOR 
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
   IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

 Send comments and/or bug reports to:
   mbi-software@dkfz-heidelberg.de

*****************************************************************************/

/**@file
*  this function divides the greyvalues of an image by a value
*/

/** @brief divides the greyvalues of an image by a value
 *
 *  @param pic_1 pointer to the first image
 *  @param value constant which is multiplied to image
 *  @param keep tells whether the image type could be changed when
 *                necessary
 *  @arg @c ipFuncNoKeep : image data type could be changed
 *  @arg @c ipFuncKeep   : image data type of original pictures is
 *                             kept (if there will be an over-/underflow
 *                             the max. or min. possible greyvalue is taken)
 *  @param pic_return memory which could be used for pic_new. If
 *                pic_retrun == NULL new memory is allocated
 *
 *  @return pointer to the new image
 *
 * AUTHOR & DATE
 */

/* include-Files                                                        */

#include "ipFuncP.h"

ipPicDescriptor *ipFuncDivC ( ipPicDescriptor *pic_old,
                              ipFloat8_t      value, 
                              ipFuncFlagI_t   keep,
                              ipPicDescriptor *pic_return );

#ifndef DOXYGEN_IGNORE

#ifndef lint
  static char *what = { "@(#)ipFuncDivC\t\tDKFZ (Dept. MBI)\t"__DATE__ };
#endif




/* definition of macros                                                 */

#define DIVC( type_1, pic_1, pic_new, value )                            \
{                                                                        \
  mitkIpPicFORALL_3 ( DIVC2, pic_new, pic_1, type_1, value );                \
}                                                                        \
 
#define DIVC2( type_n, pic_new, pic_1, type_1, value )                   \
{                                                                        \
  ipUInt4_t  i, no_elem;                                                 \
                                                                         \
  no_elem = _ipPicElements ( pic_1 );                                    \
  for ( i = 0; i < no_elem; i++ )                                        \
    {                                                                    \
       (( type_n * ) pic_new->data ) [i] = ( type_n )                    \
               ( ( ipFloat8_t )(( type_1 * ) pic_1->data ) [i] / value );\
    }                                                                    \
}

#define DIVC3( type_n, pic_1, pic_new, value )                           \
{                                                                        \
  ipUInt4_t  i, no_elem;                                                 \
  ipFloat8_t help;                                                       \
                                                                         \
  no_elem = _ipPicElements ( pic_1 );                                    \
  for ( i = 0; i < no_elem; i++ )                                        \
    {                                                                    \
       help  = ( ipFloat8_t )(( type_n * ) pic_1->data ) [i];            \
       (( type_n * ) pic_new->data ) [i] =                               \
          ( max_gv > ( ipFloat8_t ) help / value ) ?                     \
             (( min_gv < ( ipFloat8_t ) help / value ) ?                 \
                 ( (type_n)( help / value ) ) : ( type_n ) min_gv ) :    \
             ( type_n ) max_gv;                                          \
    }                                                                    \
}



/* -------------------------------------------------------------------  */
/*
*/
/* -------------------------------------------------------------------  */

ipPicDescriptor *ipFuncDivC ( ipPicDescriptor *pic_old,
                              ipFloat8_t      value, 
                              ipFuncFlagI_t   keep,
                              ipPicDescriptor *pic_return )
{

  ipPicDescriptor *pic_new;         /* pointer to new image             */
  ipFloat8_t      max_gv;           /* max. possible greyvalue          */
  ipFloat8_t      min_gv;           /* min. possible greyvalue          */
  ipFloat8_t      min1, max1;       /* extreme greyvalues of 1. image   */ 
  ipFloat8_t      smin, smax;       /* product of extreme greyvalues    */


  /* check image data                                                   */

  if ( _ipFuncError ( pic_old ) != mitkIpFuncOK ) return ( mitkIpFuncERROR );

  /* check value                                                        */

  if ( value == 0. )  return ( mitkIpFuncERROR );

  /* calculate max. and min. possible greyvalues for data type of images*/

  if ( _ipFuncExtT ( pic_old->type, pic_old->bpe, &min_gv, &max_gv ) != mitkIpFuncOK )
    return ( mitkIpFuncERROR );

  /* find out data type of new iamge                                    */

  if ( keep == ipFuncKeep )
    {
       pic_new = _ipFuncMalloc ( pic_old, pic_return, mitkIpOVERWRITE );     
       if ( pic_new == NULL ) return ( mitkIpFuncERROR );
    }
  else if ( keep == ipFuncNoKeep )
    {
       /* calculate max. and min. greyvalues of both images             */

       if ( ipFuncExtr ( pic_old, &min1, &max1 ) != mitkIpFuncOK ) return ( mitkIpFuncERROR );

       smax      = max1 / value;
       smin      = min1 / value;

       /* change image type of images of type ipPicInt                  */

       if ( pic_old->type == ipPicInt )
         {
           if ( smax < max_gv && smin > min_gv ) 
             {
                pic_new = ipPicCopyHeader ( pic_old, NULL );
             }
           else
             {
                pic_new       = ipPicCopyHeader ( pic_old, NULL );
                pic_new->type = ipPicFloat;
                pic_new->bpe  = 64;
                _ipFuncExtT ( pic_new->type, pic_new->bpe, &min_gv, &max_gv );
             }
         }

       /* change image type of images of type ipPicUInt                 */

       else if ( pic_old->type == ipPicUInt )
         {
           if ( smax < max_gv && smin > min_gv )
             {
                pic_new = ipPicCopyHeader ( pic_old, NULL );
             }
           else
             {
                pic_new = ipPicCopyHeader ( pic_old, NULL );
                pic_new->type = ipPicInt;
                pic_new->bpe  = 16;
                _ipFuncExtT ( pic_new->type, pic_new->bpe, &min_gv, &max_gv );
                if ( smax > max_gv || smin < min_gv )
                  {
                     pic_new->type = ipPicFloat;
                     pic_new->bpe  = 64;
                     _ipFuncExtT ( pic_new->type, pic_new->bpe, &min_gv, &max_gv );
                  }
             }
         } 

       /* change image type of images of type ipPicUInt                 */
 
       else if ( pic_old->type == ipPicFloat )
         {
            pic_new = ipPicCopyHeader ( pic_old, NULL );
         }
       else 
         {     
            _ipFuncSetErrno ( mitkIpFuncTYPE_ERROR );
            return ( mitkIpFuncERROR );
         }
       
    }
  else
    {
       _ipFuncSetErrno ( mitkIpFuncFLAG_ERROR );
       return ( mitkIpFuncERROR );
    }

  if ( pic_new == NULL )
    {
       _ipFuncSetErrno ( mitkIpFuncPICNEW_ERROR );
       return ( mitkIpFuncERROR );
    }
    
  if ( keep == ipFuncNoKeep )
    pic_new->data = malloc ( _ipPicSize  ( pic_new ) );
  if ( pic_new->data == NULL )
    {
       ipPicFree ( pic_new );
       _ipFuncSetErrno ( mitkIpFuncMALLOC_ERROR );
       return ( mitkIpFuncERROR );
    }


  /* macro to invert the picture (for all data types)                   */

  if ( keep == ipFuncNoKeep )
    mitkIpPicFORALL_2 ( DIVC, pic_old, pic_new, value )
  else if ( keep == ipFuncKeep )
    mitkIpPicFORALL_2 ( DIVC3, pic_old, pic_new, value )

  /* Copy Tags */

  ipFuncCopyTags(pic_new, pic_old);
  
  


  return pic_new;
}
#endif

