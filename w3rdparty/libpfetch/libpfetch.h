/*  File: libpfetch.h
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
 * Last edited: Jun 18 13:21 2009 (edgrif)
 * Created: Fri Apr  4 14:20:57 2008 (rds)
 * CVS info:   $Id: libpfetch.h,v 1.3 2009-06-19 08:06:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef __LIBPFETCH_H__
#define __LIBPFETCH_H__

#include <glib.h>
#include <glib-object.h>
#include <libpfetch/libcurlobject.h>


/*
 * Type checking and casting macros
 */

#define PFETCH_TYPE_HANDLE            (PFetchHandleGetType())
#define PFETCH_HANDLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PFETCH_TYPE_HANDLE, pfetchHandle))
#define PFETCH_HANDLE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST((obj), PFETCH_TYPE_HANDLE, pfetchHandle const))
#define PFETCH_HANDLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  PFETCH_TYPE_HANDLE, pfetchHandleClass))
#define PFETCH_IS_HANDLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PFETCH_TYPE_HANDLE))
#define PFETCH_IS_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  PFETCH_TYPE_HANDLE))
#define PFETCH_HANDLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  PFETCH_TYPE_HANDLE, pfetchHandleClass))


typedef enum
  {
    PFETCH_STATUS_OK,
    PFETCH_STATUS_FAILED
  } PFetchStatus;


/*
 * Main object structure
 */

/* synonyms for the private struct:
 *
 * PFetchHandle as a pointer for user coding like...
 * PFetchHandle new_handle = PFetchHandleNew(PFetchHandlePipeGetType());
 *
 * pfetchHandle for the Type checking/casting macros to work
 * PFETCH_HANDLE(obj) expands from
 * PFetchHandle new_handle = PFETCH_HANDLE(gobject);
 * ...to...
 * PFetchHandle new_handle = (pfetchHandle *)(blah_blah(gobject));
 * requirement is due to use of the * here ^. It's a macro thing.
 */

typedef struct _pfetchHandleStruct  pfetchHandle, *PFetchHandle;

/*
 * Class definition
 */

typedef struct _pfetchHandleClassStruct  pfetchHandleClass, *PFetchHandleClass;


/*
 * Public methods
 */

GType PFetchHandleGetType(void);

PFetchHandle  PFetchHandleNew            (GType type);
PFetchStatus  PFetchHandleSettings       (PFetchHandle  pfetch, const gchar *first_arg_name, ...);
PFetchStatus  PFetchHandleSettings_valist(PFetchHandle  pfetch, const gchar *first_arg_name, va_list args);
PFetchStatus  PFetchHandleFetch          (PFetchHandle  pfetch, char *sequence);
PFetchHandle  PFetchHandleDestroy        (PFetchHandle  pfetch);



/* Using pipe to command line client */

#define PFETCH_TYPE_PIPE_HANDLE            (PFetchHandlePipeGetType())
#define PFETCH_PIPE_HANDLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PFETCH_TYPE_PIPE_HANDLE, pfetchHandlePipe))
#define PFETCH_PIPE_HANDLE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST((obj), PFETCH_TYPE_PIPE_HANDLE, pfetchHandlePipe const))
#define PFETCH_PIPE_HANDLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  PFETCH_TYPE_PIPE_HANDLE, pfetchHandlePipeClass))
#define PFETCH_IS_PIPE_HANDLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PFETCH_TYPE_PIPE_HANDLE))
#define PFETCH_IS_PIPE_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  PFETCH_TYPE_PIPE_HANDLE))
#define PFETCH_PIPE_HANDLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  PFETCH_TYPE_PIPE_HANDLE, pfetchHandlePipeClass))

/* 
 * Main Object structure
 */

typedef struct _pfetchHandlePipeStruct  pfetchHandlePipe, *PFetchHandlePipe;

typedef struct _pfetchHandlePipeClassStruct  pfetchHandlePipeClass, *PFetchHandlePipeClass;

GType PFetchHandlePipeGetType(void);


/* Over http... */


/* Object */
typedef struct _pfetchHandleHttpStruct  pfetchHandleHttp, *PFetchHandleHttp;

/* Class */
typedef struct _pfetchHandleHttpClassStruct  pfetchHandleHttpClass, *PFetchHandleHttpClass;


#define PFETCH_TYPE_HTTP_HANDLE            (PFetchHandleHttpGetType ())
#define PFETCH_HTTP_HANDLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PFETCH_TYPE_HTTP_HANDLE, pfetchHandleHttp))
#define PFETCH_HTTP_HANDLE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST((obj), PFETCH_TYPE_HTTP_HANDLE, pfetchHandleHttp const))
#define PFETCH_HTTP_HANDLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  PFETCH_TYPE_HTTP_HANDLE, pfetchHandleHttpClass))
#define PFETCH_IS_HTTP_HANDLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PFETCH_TYPE_HTTP_HANDLE))
#define PFETCH_IS_HTTP_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  PFETCH_TYPE_HTTP_HANDLE))
#define PFETCH_HTTP_HANDLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  PFETCH_TYPE_HTTP_HANDLE, pfetchHandleHttpClass))


GType PFetchHandleHttpGetType(void);


/*
 * Public utility functions...
 */

#define PFETCH_TYPE_HANDLE_STATUS          (PFetchHandleStatusGetType())

GType PFetchHandleStatusGetType (void);


#endif /* __LIBPFETCH_H__ */
