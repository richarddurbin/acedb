/* this ALWAYS GENERATED file contains the RPC client stubs */


/* File created by MIDL compiler version 3.01.75 */
/* at Sat Mar 28 08:12:46 1998
 */
/* Compiler settings for rpcace.idl:
    Os (OptLev=s), W1, Zp4, env=Win32
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )

#include <string.h>
#if defined( _ALPHA_ )
#include <stdarg.h>
#endif

#include <malloc.h>
#include "rpcace.h"

#define TYPE_FORMAT_STRING_SIZE   45                                
#define PROC_FORMAT_STRING_SIZE   13                                

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;

/* Standard interface: RPC_ACE, ver. 1.2,
   GUID={0xd76e4030,0x69f5,0x11d0,{0xa5,0x40,0x00,0x80,0x48,0xb9,0xaa,0x78}} */



static const RPC_CLIENT_INTERFACE RPC_ACE___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0xd76e4030,0x69f5,0x11d0,{0xa5,0x40,0x00,0x80,0x48,0xb9,0xaa,0x78}},{1,2}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0
    };
RPC_IF_HANDLE RPC_ACE_v1_2_c_ifspec = (RPC_IF_HANDLE)& RPC_ACE___RpcClientInterface;

extern const MIDL_STUB_DESC RPC_ACE_StubDesc;

static RPC_BINDING_HANDLE RPC_ACE__MIDL_AutoBindHandle;


void ace_server( 
    /* [in] */ handle_t IDL_handle,
    /* [ref][out][in] */ LP_ACE_DATA ace_data,
    /* [fault_status][comm_status][ref][out] */ error_status_t __RPC_FAR *ace_rpc_status)
{

    RPC_BINDING_HANDLE _Handle	=	0;
    
    RPC_MESSAGE _RpcMessage;
    
    RPC_STATUS _Status;
    
    MIDL_STUB_MESSAGE _StubMsg;
    
    RpcTryExcept
        {
        RpcTryFinally
            {
            NdrClientInitializeNew(
                          ( PRPC_MESSAGE  )&_RpcMessage,
                          ( PMIDL_STUB_MESSAGE  )&_StubMsg,
                          ( PMIDL_STUB_DESC  )&RPC_ACE_StubDesc,
                          0);
            
            
            _Handle = IDL_handle;
            
            
            _StubMsg.BufferLength = 0U + 0U;
            NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR *)ace_data,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[4] );
            
            NdrGetBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg, _StubMsg.BufferLength, _Handle );
            
            NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                     (unsigned char __RPC_FAR *)ace_data,
                                     (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[4] );
            
            NdrSendReceive( (PMIDL_STUB_MESSAGE) &_StubMsg, (unsigned char __RPC_FAR *) _StubMsg.Buffer );
            
            if ( (_RpcMessage.DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
                NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
            
            NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                       (unsigned char __RPC_FAR * __RPC_FAR *)&ace_data,
                                       (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[4],
                                       (unsigned char)0 );
            
            _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
            *ace_rpc_status = *(( error_status_t __RPC_FAR * )_StubMsg.Buffer)++;
            
            }
        RpcFinally
            {
            NdrFreeBuffer( (PMIDL_STUB_MESSAGE) &_StubMsg );
            
            }
        RpcEndFinally
        
        }
    RpcExcept( 1 )
        {
        if(_Status = NdrMapCommAndFaultStatus(( PMIDL_STUB_MESSAGE  )&_StubMsg,( unsigned long __RPC_FAR * )ace_rpc_status,( unsigned long __RPC_FAR * )ace_rpc_status,RpcExceptionCode()))
            {
            RpcRaiseException(_Status);
            }
        }
    RpcEndExcept
}

extern MALLOC_FREE_STRUCT _MallocFreeStruct;

static const MIDL_STUB_DESC RPC_ACE_StubDesc = 
    {
    (void __RPC_FAR *)& RPC_ACE___RpcClientInterface,
    NdrRpcSmClientAllocate,
    NdrRpcSmClientFree,
    &RPC_ACE__MIDL_AutoBindHandle,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    0, /* -error bounds_check flag */
    0x10001, /* Ndr library version */
    &_MallocFreeStruct,
    0x301004b, /* MIDL Version 3.1.75 */
    0,
    0,
    0,  /* Reserved1 */
    0,  /* Reserved2 */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static void __RPC_FAR * __RPC_USER
RPC_ACE_malloc_wrapper( size_t _Size )
{
    return( malloc( _Size ) );
}

static void  __RPC_USER
RPC_ACE_free_wrapper( void __RPC_FAR * _p )
{
    free( _p );
}

static MALLOC_FREE_STRUCT _MallocFreeStruct = 
{
    RPC_ACE_malloc_wrapper,
    RPC_ACE_free_wrapper
};

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {
			0x4e,		/* FC_IN_PARAM_BASETYPE */
			0xf,		/* FC_IGNORE */
/*  2 */	
			0x50,		/* FC_IN_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/*  4 */	NdrFcShort( 0x0 ),	/* Type Offset=0 */
/*  6 */	
			0x51,		/* FC_OUT_PARAM */
#ifndef _ALPHA_
			0x1,		/* x86, MIPS & PPC Stack size = 1 */
#else
			0x2,		/* Alpha Stack size = 2 */
#endif
/*  8 */	NdrFcShort( 0x28 ),	/* Type Offset=40 */
/* 10 */	0x5b,		/* FC_END */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			0x11, 0x0,	/* FC_RP */
/*  2 */	NdrFcShort( 0x2 ),	/* Offset= 2 (4) */
/*  4 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/*  6 */	NdrFcShort( 0x20 ),	/* 32 */
/*  8 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 10 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 12 */	NdrFcShort( 0x0 ),	/* 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 18 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 20 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 22 */	NdrFcShort( 0x4 ),	/* 4 */
/* 24 */	NdrFcShort( 0x4 ),	/* 4 */
/* 26 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 28 */	
			0x22,		/* FC_C_CSTRING */
			0x5c,		/* FC_PAD */
/* 30 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 32 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 34 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 36 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 38 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 40 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 42 */	0x10,		/* FC_ERROR_STATUS_T */
			0x5c,		/* FC_PAD */

			0x0
        }
    };
