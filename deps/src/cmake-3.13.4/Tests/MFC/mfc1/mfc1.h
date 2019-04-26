// mfc1.h : main header file for the mfc1 application
//
#pragma once

#ifndef __AFXWIN_H__
#  error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h" // main symbols

// Cmfc1App:
// See mfc1.cpp for the implementation of this class
//

class Cmfc1App : public CWinApp
{
public:
  Cmfc1App();

  // Overrides
public:
  virtual BOOL InitInstance();

  // Implementation
  afx_msg void OnAppAbout();
  DECLARE_MESSAGE_MAP()
};

extern Cmfc1App theApp;
