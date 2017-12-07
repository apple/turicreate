// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#endif

// See http://msdn.microsoft.com/en-us/library/6sehtctf.aspx for more info
// on WINVER and _WIN32_WINNT

// Modify the following defines if you have to target a platform prior to the
// ones specified below.
// Refer to MSDN for the latest info on corresponding values for different
// platforms.
#ifndef WINVER // Allow use of features specific to Windows 95 and Windows NT 4
               // or later.
#if _MSC_VER < 1600
#define WINVER                                                                \
  0x0400 // Change this to the appropriate value to target Windows 98 and
         // Windows 2000 or later.
#else
#define WINVER 0x0501 // Target Windows XP and later with VS 10 and later
#endif
#endif

#ifndef _WIN32_WINNT // Allow use of features specific to Windows NT 4 or
                     // later.
#if _MSC_VER < 1600
#define _WIN32_WINNT                                                          \
  0x0400 // Change this to the appropriate value to target Windows 98 and
         // Windows 2000 or later.
#else
#define _WIN32_WINNT 0x0501 // Target Windows XP and later with VS 10 and later
#endif
#endif

#ifndef _WIN32_WINDOWS // Allow use of features specific to Windows 98 or
                       // later.
#if _MSC_VER < 1600
#define _WIN32_WINDOWS                                                        \
  0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif
#endif

#ifndef _WIN32_IE // Allow use of features specific to IE 4.0 or later.
#if _MSC_VER < 1600
#define _WIN32_IE                                                             \
  0x0400 // Change this to the appropriate value to target IE 5.0 or later.
#endif
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be
                                           // explicit

// turns off MFC's hiding of some common and often safely ignored warning
// messages
#define _AFX_ALL_WARNINGS

#include <afxdisp.h> // MFC Automation classes
#include <afxext.h>  // MFC extensions
#include <afxwin.h>  // MFC core and standard components

#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#endif              // _AFX_NO_AFXCMN_SUPPORT
