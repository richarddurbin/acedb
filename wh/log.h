/*  File: log.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2001
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Interface to acedb logging interface. Can be used for
 *              general operations on an acedb format log file.
 *              Currently used for log.wrm and serverlog.wrm.
 * HISTORY:
 * Last edited: Oct  7 15:40 2002 (edgrif)
 * Created: Mon Feb 26 13:17:06 2001 (edgrif)
 * CVS info:   $Id: log.h,v 1.5 2002-10-18 12:43:45 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_LOG_H
#define ACEDB_LOG_H

typedef enum _aceLogExitType {ACELOG_NORMAL_EXIT = 0, ACELOG_ABNORMAL_EXIT,
			      ACELOG_CRASH_EXIT, ACELOG_CLOSE_LOG} aceLogExitType ;

/* Start a completely new log.                                               */
BOOL openAceLogFile(char *logfile_name, ACEOUT *logfile_out, BOOL new_session) ;

/* Turn on/off timestamps in log messages except at session start/end. */
void setTimeStampsAceLogFile(BOOL timestamps_on) ;

/* Just close and then reopen the existing log, used in conjunction with     */
/* "rotatelogs" script found on Linux and other Unixes.                      */
BOOL reopenAceLogFile(ACEOUT *current_log_inout) ;

/* Close existing log and open a new one with the same name, the old one is  */
/* renamed so that the name contains a date/time stamp.                      */
/* Optionally rename the log to "name_for_old_log", and if timestamp is TRUE */
/* add a timestamp to the name of the log.                                   */
/* Optionally add a message and start a new session in the log.              */
ACEOUT startNewAceLogFile(ACEOUT current_log, char *name_for_old_log, BOOL timestamp,
			  char *log_msg, BOOL new_session) ;

/* Add a message to the log.                                                 */
BOOL appendAceLogFile(ACEOUT current_log, char *text) ;

/* Close the log file, exit_type determines what sort of message is added    */
/* to the log, if ACELOG_CLOSE_LOG then no message is written.               */
BOOL closeAceLogFile(ACEOUT current_log, aceLogExitType exit_type) ;

#endif	/* ACEDB_LOG_H */
