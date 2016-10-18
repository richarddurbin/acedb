// $Id: acemenu.cpp,v 1.1 1996-06-10 10:35:16 rbsk Exp $
// ACEMenu.cpp : implementation file
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "ACEMenu.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CACEMenu message handlers

CString& MenuType( BoxMenuType boxMenuType ) 
{
	static CString typeStr ;
	switch( boxMenuType )
	{
		case NO_MENU:
			typeStr = "No" ;
			break ;
		case NORMAL_MENU:
			typeStr = "Normal" ;
			break ;
		case FREE_MENU:
			typeStr = "Free" ;
			break ;
		case NEWBOX_MENU:
			typeStr = "New Box" ;
			break ;
		default:
		   typeStr = "Unknown" ; 
	}
	return typeStr ;
}

 
