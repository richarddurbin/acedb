/* this ALWAYS GENERATED file contains the RPC server stubs */


/* File created by MIDL compiler version 3.01.75 */
/* at Sat Mar 28 08:12:46 1998
 */
/* Compiler settings for rpcace.idl:
    Os (OptLev=s), W1, Zp4, env=Win32
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )

#include <string.h>
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


extern RPC_DISPATCH_TABLE RPC_ACE_v1_2_DispatchTable;

static const RPC_SERVER_INTERFACE RPC_ACE___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0xd76e4030,0x69f5,0x11d0,{0xa5,0x40,0x00,0x80,0x48,0xb9,0xaa,0x78}},{1,2}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &RPC_ACE_v1_2_DispatchTable,
    0,
    0,
    0,
    0,
    0
    };
RPC_IF_HANDLE RPC_ACE_v1_2_s_ifspec = (RPC_IF_HANDLE)& RPC_ACE___RpcServerInterface;

extern const MIDL_STUB_DESC RPC_ACE_StubDesc;

void __RPC_STUB
RPC_ACE_ace_server(
    PRPC_MESSAGE _pRpcMessage )
{
    handle_t IDL_handle;
    error_status_t _M0;
    MIDL_STUB_MESSAGE _StubMsg;
    LP_ACE_DATA ace_data;
    error_status_t __RPC_FAR *ace_rpc_status;
    RPC_STATUS _Status;
    
    ((void)(_Status));
    NdrServerInitializeNew(
                          _pRpcMessage,
                          &_StubMsg,
                          &RPC_ACE_StubDesc);
    
    NdrRpcSsEnableAllocate(&_StubMsg);
    IDL_handle = _pRpcMessage->Handle;
    ace_data = 0;
    ace_rpc_status = 0;
    RpcTryFinally
        {
        if ( (_pRpcMessage->DataRepresentation & 0X0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION )
            NdrConvert( (PMIDL_STUB_MESSAGE) &_StubMsg, (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0] );
        
        NdrSimpleStructUnmarshall( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR * __RPC_FAR *)&ace_data,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[4],
                                   (unsigned char)0 );
        
        ace_rpc_status = &_M0;
        
        ace_server(
              IDL_handle,
              ace_data,
              ace_rpc_status);
        
        _StubMsg.BufferLength = 0U + 11U;
        NdrSimpleStructBufferSize( (PMIDL_STUB_MESSAGE) &_StubMsg,
                                   (unsigned char __RPC_FAR *)ace_data,
                                   (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[4] );
        
        _pRpcMessage->BufferLength = _StubMsg.BufferLength;
        
        _Status = I_RpcGetBuffer( _pRpcMessage ); 
        if ( _Status )
            RpcRaiseException( _Status );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *) _pRpcMessage->Buffer;
        
        NdrSimpleStructMarshall( (PMIDL_STUB_MESSAGE)& _StubMsg,
                                 (unsigned char __RPC_FAR *)ace_data,
                                 (PFORMAT_STRING) &__MIDL_TypeFormatString.Format[4] );
        
        _StubMsg.Buffer = (unsigned char __RPC_FAR *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);
        *(( error_status_t __RPC_FAR * )_StubMsg.Buffer)++ = *ace_rpc_status;
        
        }
    RpcFinally
        {
        NdrRpcSsDisableAllocate(&_StubMsg);
        }
    RpcEndFinally
    _pRpcMessage->BufferLength = 
        (unsigned int)((long)_StubMsg.Buffer - (long)_pRpcMessage->Buffer);
    
}


static const MIDL_STUB_DESC RPC_ACE_StubDesc = 
    {
    (void __RPC_FAR *)& RPC_ACE___RpcServerInterface,
    NdrRpcSsDefaultAllocate,
    NdrRpcSsDefaultFree,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    0, /* -error bounds_check flag */
    0x10001, /* Ndr library version */
    0,
    0x301004b, /* MIDL Version 3.1.75 */
    0,
    0,
    0,  /* Reserved1 */
    0,  /* Reserved2 */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION RPC_ACE_table[] =
    {
    RPC_ACE_ace_server,
    0
    };
RPC_DISPATCH_TABLE RPC_ACE_v1_2_DispatchTable = 
    {
    1,
    RPC_ACE_table
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
