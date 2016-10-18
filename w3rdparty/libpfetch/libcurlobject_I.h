/*  File: libcurlobj_I.h
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
 * Last edited: Mar 20 13:47 2009 (edgrif)
 * Created: Thu Jun  5 09:18:29 2008 (rds)
 * CVS info:   $Id: libcurlobject_I.h,v 1.2 2009-03-20 14:13:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <libpfetch/libcurlobject.h>


#define CURL_PARAM_STATIC    (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#define CURL_PARAM_STATIC_RO (CURL_PARAM_STATIC | G_PARAM_READABLE)
#define CURL_PARAM_STATIC_RW (CURL_PARAM_STATIC | G_PARAM_READWRITE)
#define CURL_PARAM_STATIC_WO (CURL_PARAM_STATIC | G_PARAM_WRITABLE)


enum
  {
    CONNECTION_CLOSED_SIGNAL,
    LAST_SIGNAL
  };

typedef struct _curlObjectStruct
{
  GObject __parent__;

  CURL  *easy;
  CURLM *multi;
  
  CURLcode  last_easy_status;
  CURLMcode last_multi_status;

  GQueue *perform_queue;

  gpointer settings_to_destroy;
  gpointer post_data_2_free;

  char error_message[CURL_ERROR_SIZE];

  curl_version_info_data *curl_version;

  GList *pre717strings;

  unsigned int allow_queue : 1;
  unsigned int transfer_in_progress : 1;
  unsigned int manage_post_data : 1;
  unsigned int debug : 1;
} curlObjectStruct;

typedef struct _curlObjectClassStruct
{
  GObjectClass parent_class;

  CURLcode  global_init;

  void (* connection_closed)(CURLObject object);

  guint signals[LAST_SIGNAL];

} curlObjectClassStruct;
