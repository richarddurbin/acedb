/*  $Id: nace_sp.c,v 1.1 1994-08-03 20:02:25 rd Exp $  */
/*  Last edited: Nov 26 11:27 1992 (mieg) */
#define main     my_run_rpc
#include "../wrpc/nace_svc.c"
#undef  main




static int (* reg_data_server) (net_data *, net_data *);


int  wait_for_client ( call_back )
     int ( * call_back )( net_data *, net_data*); 

{
  reg_data_server = call_back;
  my_run_rpc() ;
  return(-1);
}


  
/*
** Remote version for "ace_server"
*/

ace_server_res *
ace_server_1 ( net_ace_data )
     net_data *net_ace_data;
{
   static ace_server_res data_for_cln;
   xdr_free(xdr_ace_server_res, &data_for_cln);
   data_for_cln.errno = reg_data_server( net_ace_data , &(data_for_cln.ace_server_res_u.res_data) );
   return (& data_for_cln );
 }

   
  
