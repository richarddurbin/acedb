/*  File: test.server.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
 * -------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
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
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: simple server which make  toupper() and return
 *              message back.
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:41 1999 (fw)
 * Created: Thu Aug 26 17:41:14 1999 (fw)
 * CVS info:   $Id: test.server.c,v 1.3 1999-09-01 11:25:59 fw Exp $
 *-------------------------------------------------------------------
 */

#include <errno.h>
#include <string.h>
#include "nace_com.h"

int r_toupper ( data_in , data_out )
     net_data *data_in;
     net_data *data_out;
{
  int i;
  data_out->cmd.cmd_len = data_in->cmd.cmd_len;
  data_out->cmd.cmd_val = (u_char *)strdup(data_in->cmd.cmd_val);
  for (i = 0; i < data_out->cmd.cmd_len ; i++)
    {
      data_out->cmd.cmd_val[i] = toupper( (char) data_out->cmd.cmd_val[i]);
    }
  
  return(0);
}


int main ()
{
  int  ret;
  ret = wait_for_client ( &r_toupper );
  printf("return with  %i and errno %i \n",ret, errno);
  
}
