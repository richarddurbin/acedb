/*  File: libcurlobj.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar 30 12:49 2009 (edgrif)
 * Created: Thu May  1 17:06:07 2008 (rds)
 * CVS info:   $Id: libcurlobject.h,v 1.2 2009-03-30 11:50:03 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef CURL_OBJ_H
#define CURL_OBJ_H

#include <glib.h>
#include <glib-object.h>
#include <curl/curl.h>


#define CURL_TYPE_OBJECT            (CURLObjectGetType())
#define CURL_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), CURL_TYPE_OBJECT, curlObject))
#define CURL_OBJECT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST((obj), CURL_TYPE_OBJECT, curlObject const))
#define CURL_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  CURL_TYPE_OBJECT, curlObjectClass))
#define CURL_IS_OBJECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), CURL_TYPE_OBJECT))
#define CURL_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  CURL_TYPE_OBJECT))
#define CURL_OBJECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  CURL_TYPE_OBJECT, curlObjectClass))


typedef enum
  {
    CURL_STATUS_OK,
    CURL_STATUS_FAILED
  } CURLObjectStatus;


/*  */
typedef struct _curlObjectStruct *CURLObject;

typedef struct _curlObjectStruct  curlObject;

/*  */
typedef struct _curlObjectClassStruct *CURLObjectClass;

typedef struct _curlObjectClassStruct  curlObjectClass;


GType CURLObjectGetType(void);

CURLObject       CURLObjectNew         (void);
CURLObjectStatus CURLObjectSet         (CURLObject curlobject, const gchar *first_arg_name, ...);
CURLObjectStatus CURLObjectPerform     (CURLObject curlobject, gboolean use_multi);
CURLObjectStatus CURLObjectErrorMessage(CURLObject curl_object, char **message);
CURLObject       CURLObjectDestroy     (CURLObject curlobject);


#endif	/* CURL_OBJ_H */
