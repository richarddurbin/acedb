/*  File: test.client.c
 *  Author: mieg
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
 * Description: this program sends a message to a server
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:41 1999 (fw)
 * Created: Thu Aug 26 17:40:49 1999 (fw)
 * CVS info:   $Id: test.client.c,v 1.3 1999-09-01 11:25:49 fw Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "nace_com.h"


void main( argc, argv )
     int argc;
     char *argv[];
{
  net_data data_s;   /* data for server */
  net_data *data_c;  /* result from server*/
  int   erc;
  int   time_out;
  CLIENT *cl;
  
  
  
  if ( argc < 3 ) 
    {
      fprintf(stderr, "usage : %s host message  [time_out]\n",argv[0]);
      exit(1);
    }
  

  bzero( (char *)&data_s, sizeof(data_s) );

  data_s.cmd.cmd_len = (u_int)strlen(argv[2])+1;
  data_s.cmd.cmd_val = (u_char *)strdup(argv[2]); 
  if ( data_s.cmd.cmd_val == NULL ) 
    {
      perror ("strdup");
      exit(1);
    }
  if ( argc > 3 ) 
    {
      sscanf(argv[3],"%i",&time_out);
    }
  else
    {
      time_out = 0;
    }
  
  cl = open_ace_server (argv[1], NET_ACE , NET_ACE_VERS );
  if (cl == NULL)
    exit (1);
  erc = call_ace_server (cl,&data_s,&data_c,time_out);
  if ( erc < 0) 
    {
      fprintf(stderr, "call_ace_server return with : %i\n",erc);
      exit(1);
    }
  printf("server_message : len = %i\n"
         "               : val = %s\n",data_c->cmd.cmd_len, data_c->cmd.cmd_val);

  close_ace_server(cl);
  exit(0);
}

