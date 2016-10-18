/*  $Id: nace_cp.c,v 1.1 1994-08-03 20:02:23 rd Exp $  */
#include "nace_com.h"

/*************************************************/
CLIENT * open_ace_server ( server , prog , vers  )
     char * server ;
     u_long prog;
     u_long vers;
{
  CLIENT  *cl;
  cl = clnt_create (server,prog, vers,"tcp");
  if ( cl == NULL ) 
    {
#ifdef RPC_DEBUG
      clnt_pcreateerror(server);
#endif
      return(NULL);
    }
  return(cl);
}

/*************************************************/
void close_ace_server (cl)
     CLIENT *cl;
{
  clnt_destroy (cl);
}

/*************************************************/
int call_ace_server (cl , data_for_srv ,
		     data_for_cln , timeout )
     CLIENT   * cl;
     net_data * data_for_srv;
     net_data **data_for_cln;
     int        timeout;
{
  static ace_server_res  *res = NULL;
  struct timeval tv;
  

  /* set timeout */
  if ( timeout != 0 ) 
    {
      tv.tv_sec = timeout;
      tv.tv_usec = 0;
      clnt_control ( cl,CLSET_TIMEOUT, &tv);
    }
  if (res != NULL)
    {
      xdr_free(xdr_ace_server_res, res);
    }

  res = ace_server_1 ( data_for_srv , cl);

  if (res == NULL) 
    {
#ifdef RPC_DEBUG
      clnt_perror(cl,"");
#endif
      return(-1);
    }
  if (res->errno != 0)
    {
      errno = res->errno;
#ifdef RPC_DEBUG
      perror( "" );
#endif
      return(-1);
    }
  *data_for_cln = &(res->ace_server_res_u.res_data);
  return(0);
}

