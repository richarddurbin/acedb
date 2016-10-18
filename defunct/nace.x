/* $Id: nace.x,v 1.1 1994-08-03 20:03:55 rd Exp $ */
/*
** "net_data" is data structure for transfer from client to
** server. "ace_server_res" is structure for transfer from
** server to client and contains "net_data" also.
** "net_data" structure is not interesting for my functions,
** but "ace_server_res" cannot be changed
*/

struct net_data {
  string   cmd <>;
  opaque   reply <>;
  int      clientId ;
  };
 
union ace_server_res switch ( int errno) {
case 0:
  net_data    res_data;
default:
  void;
};

/*
** Pleas don't change this !!!
*/
program NET_ACE {
  version NET_ACE_VERS   {
    ace_server_res
    ACE_SERVER(net_data ) = 1;
  } = 1;
} = 0x20000101;





