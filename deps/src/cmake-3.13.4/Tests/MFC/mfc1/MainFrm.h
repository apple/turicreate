// MainFrm.h : interface of the CMainFrame class
//

#pragma once
class CMainFrame : public CMDIFrameWnd
{
  DECLARE_DYNAMIC(CMainFrame)
public:
  CMainFrame();

  // Attributes
public:
  // Operations
public:
  // Overrides
public:
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

  // Implementation
public:
  virtual ~CMainFrame();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected: // control bar embedded members
  CStatusBar m_wndStatusBar;
  CToolBar m_wndToolBar;

  // Generated message map functions
protected:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  DECLARE_MESSAGE_MAP()
};
