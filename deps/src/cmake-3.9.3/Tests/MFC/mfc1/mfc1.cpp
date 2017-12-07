// mfc1.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"

#include "MainFrm.h"
#include "mfc1.h"

#include "ChildFrm.h"
#include "mfc1Doc.h"
#include "mfc1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Cmfc1App

BEGIN_MESSAGE_MAP(Cmfc1App, CWinApp)
ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
// Standard file based document commands
ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
// Standard print setup command
ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

// Cmfc1App construction

Cmfc1App::Cmfc1App()
{
  // TODO: add construction code here,
  // Place all significant initialization in InitInstance
}

// The one and only Cmfc1App object

Cmfc1App theApp;

// Cmfc1App initialization

BOOL Cmfc1App::InitInstance()
{
  // InitCommonControls() is required on Windows XP if an application
  // manifest specifies use of ComCtl32.dll version 6 or later to enable
  // visual styles.  Otherwise, any window creation will fail.
  InitCommonControls();

  CWinApp::InitInstance();

  // Initialize OLE libraries
  if (!AfxOleInit()) {
    AfxMessageBox(IDP_OLE_INIT_FAILED);
    return FALSE;
  }
  AfxEnableControlContainer();
  // Standard initialization
  // If you are not using these features and wish to reduce the size
  // of your final executable, you should remove from the following
  // the specific initialization routines you do not need
  // Change the registry key under which our settings are stored
  // TODO: You should modify this string to be something appropriate
  // such as the name of your company or organization
  SetRegistryKey(_T("Local AppWizard-Generated Applications"));
  LoadStdProfileSettings(4); // Load standard INI file options (including MRU)
  // Register the application's document templates.  Document templates
  //  serve as the connection between documents, frame windows and views
  CMultiDocTemplate* pDocTemplate;
  pDocTemplate =
    new CMultiDocTemplate(IDR_mfc1TYPE, RUNTIME_CLASS(Cmfc1Doc),
                          RUNTIME_CLASS(CChildFrame), // custom MDI child frame
                          RUNTIME_CLASS(Cmfc1View));
  if (!pDocTemplate)
    return FALSE;
  AddDocTemplate(pDocTemplate);
  // create main MDI Frame window
  CMainFrame* pMainFrame = new CMainFrame;
  if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
    return FALSE;
  m_pMainWnd = pMainFrame;
  // call DragAcceptFiles only if there's a suffix
  //  In an MDI app, this should occur immediately after setting m_pMainWnd
  // Enable drag/drop open
  m_pMainWnd->DragAcceptFiles();
  // Enable DDE Execute open
  EnableShellOpen();
  RegisterShellFileTypes(TRUE);
  // Parse command line for standard shell commands, DDE, file open
  CCommandLineInfo cmdInfo;
  ParseCommandLine(cmdInfo);
  // Dispatch commands specified on the command line.  Will return FALSE if
  // app was launched with /RegServer, /Register, /Unregserver or /Unregister.
  if (!ProcessShellCommand(cmdInfo))
    return FALSE;
  // The main window has been initialized, so show and update it
  pMainFrame->ShowWindow(m_nCmdShow);
  pMainFrame->UpdateWindow();
  return TRUE;
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
  CAboutDlg();

  // Dialog Data
  enum
  {
    IDD = IDD_ABOUTBOX
  };

protected:
  virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

  // Implementation
protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg()
  : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// App command to run the dialog
void Cmfc1App::OnAppAbout()
{
  CAboutDlg aboutDlg;
  aboutDlg.DoModal();
}

// Cmfc1App message handlers
