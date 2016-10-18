/*-------------------------------------------------------------------
 * File: client2service.h
 *  Adapted by: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1998
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:  Client-side service management functions
 *
 * Exported: BOOL startService(char *host, char *service) ;
 *
 * HISTORY:
 * Last edited: Mar 4 19:45 1998 (rbrusk)
 * Created: Mar 1 12:45 1998 (rbrusk)
 *-------------------------------------------------------------------
 */

#ifndef CLIENT2SERVICE_H
#define CLIENT2SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL startServer(char *szHostName, 
				 char *szDatabaseName, 
				 char *szServiceName,
				 DWORD dwTimeout) ;

#ifdef __cplusplus
}
#endif

#endif /* CLIENT2SERVICE_H */
