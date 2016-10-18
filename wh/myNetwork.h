/*  Last edited: Nov 27 19:01 1995 (mieg) */
/*********************************************************
  myNetwork.h
   --------------------------------------------------------
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
 * --------------------------------------------------------
  generated at Tue Jul 25 14:26:05 1995
  by snns2c ( Bernward Kett 1995 ) 
*********************************************************/

/* $Id: myNetwork.h,v 1.4 1999-09-01 11:14:43 fw Exp $ */

extern int myNetwork(float *in, float *out, int init);
#ifdef JUNK
static struct {
  int NoOfInput;    /* Number of Input Units  */
  int NoOfOutput;   /* Number of Output Units */
  int(* propFunc)(float *, float*, int);
} myNetworkREC = {240,4,myNetwork};
#endif
