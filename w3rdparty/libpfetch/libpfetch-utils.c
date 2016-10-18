/*  File: libpfetch-utils.c
 *  Author: Roy Storey (roy@sanger.ac.uk)
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
 * Last edited: Jun 18 13:11 2009 (edgrif)
 * Created: Wed Jun  4 21:34:01 2008 (roy)
 * CVS info:   $Id: libpfetch-utils.c,v 1.3 2009-06-19 08:06:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <libpfetch/libpfetch_P.h>
#include <libpfetch/libpfetch_I.h>

/* Public utility funcs */

/* overkill ATM for what is just a boolean, but inevitably it might expand... */
GType PFetchHandleStatusGetType (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { PFETCH_STATUS_OK,     "PFETCH_STATUS_OK",     "all ok" },
      { PFETCH_STATUS_FAILED, "PFETCH_STATUS_FAILED", "failure" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("PFetchHandleStatus", values);
  }
  return etype;
}



/* Library internal only... */

/* So we can use g_signal_emitv in a nice easy way without re-writing
 * what's below again & again... */
PFetchStatus emit_signal(PFetchHandle handle,
			 guint        signal_id,
			 GQuark       detail,
			 char        *text, 
			 guint       *text_size, 
			 GError      *error)
{
#define HANDLE_N_PARAMS 4
  GValue instance_params[HANDLE_N_PARAMS] = {{0}}, sig_return = {0};
  PFetchStatus default_status = PFETCH_STATUS_FAILED;
  
  g_value_init(instance_params, PFETCH_TYPE_HANDLE);
  g_value_set_object(instance_params, PFETCH_HANDLE(handle));

  if(signal_id != PFETCH_HANDLE_GET_CLASS(handle)->handle_signals[HANDLE_CLOSED_SIGNAL])
    {
      g_value_init(instance_params + 1, G_TYPE_STRING);
      /* Don't copy the string! */
      g_value_set_static_string(instance_params + 1, text);
      
      g_value_init(instance_params + 2, G_TYPE_POINTER);
      g_value_set_pointer(instance_params + 2, text_size);
      
      g_value_init(instance_params + 3, G_TYPE_POINTER);
      g_value_set_pointer(instance_params + 3, error);
    }

  g_value_init(&sig_return, PFETCH_TYPE_HANDLE_STATUS);
  g_value_set_enum(&sig_return, default_status);

  g_object_ref(G_OBJECT(handle));

  g_signal_emitv(instance_params, signal_id, detail, &sig_return);

  g_value_unset(instance_params);

  if(signal_id != PFETCH_HANDLE_GET_CLASS(handle)->handle_signals[HANDLE_CLOSED_SIGNAL])
    {
      int i;
      for(i = 1; i < HANDLE_N_PARAMS; i++)
	{
	  g_value_unset(instance_params + i);
	}
    }

  g_object_unref(G_OBJECT(handle));

#undef HANDLE_N_PARAMS

  default_status = g_value_get_enum(&sig_return);

  return default_status;
}
