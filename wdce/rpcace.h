/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Sat Mar 28 08:12:46 1998
 */
/* Compiler settings for rpcace.idl:
    Os (OptLev=s), W1, Zp4, env=Win32
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __rpcace_h__
#define __rpcace_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __RPC_ACE_INTERFACE_DEFINED__
#define __RPC_ACE_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: RPC_ACE
 * at Sat Mar 28 08:12:46 1998
 * using MIDL 3.01.75
 ****************************************/
/* [explicit_handle][full][version][uuid] */ 


typedef /* [public][public] */ struct  __MIDL_RPC_ACE_0001
    {
    /* [string] */ unsigned char __RPC_FAR *lpszServerStringBinding;
    handle_t hServerBinding;
    }	SERVER;

typedef SERVER __RPC_FAR *LP_SERVER;

typedef /* [public][public][public] */ struct  __MIDL_RPC_ACE_0002
    {
    /* [unique][string] */ unsigned char __RPC_FAR *question;
    /* [unique][string] */ unsigned char __RPC_FAR *reponse;
    long clientId;
    long magic;
    long cardinal;
    long encore;
    long aceError;
    long kBytes;
    }	ACE_DATA;

typedef ACE_DATA __RPC_FAR *LP_ACE_DATA;

void ace_server( 
    /* [in] */ handle_t IDL_handle,
    /* [ref][out][in] */ LP_ACE_DATA ace_data,
    /* [fault_status][comm_status][ref][out] */ error_status_t __RPC_FAR *ace_rpc_status);



extern RPC_IF_HANDLE RPC_ACE_v1_2_c_ifspec;
extern RPC_IF_HANDLE RPC_ACE_v1_2_s_ifspec;
#endif /* __RPC_ACE_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
