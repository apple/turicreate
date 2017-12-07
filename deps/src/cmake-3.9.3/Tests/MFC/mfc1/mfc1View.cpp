// mfc1View.cpp : implementation of the Cmfc1View class
//

#include "stdafx.h"

#include "mfc1.h"

#include "mfc1Doc.h"
#include "mfc1View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Cmfc1View

IMPLEMENT_DYNCREATE(Cmfc1View, CView)

BEGIN_MESSAGE_MAP(Cmfc1View, CView)
// Standard printing commands
ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

// Cmfc1View construction/destruction

Cmfc1View::Cmfc1View()
{
  // TODO: add construction code here
}

Cmfc1View::~Cmfc1View()
{
}

BOOL Cmfc1View::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CView::PreCreateWindow(cs);
}

// Cmfc1View drawing

void Cmfc1View::OnDraw(CDC* /*pDC*/)
{
  Cmfc1Doc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  // TODO: add draw code for native data here
}

// Cmfc1View printing

BOOL Cmfc1View::OnPreparePrinting(CPrintInfo* pInfo)
{
  // default preparation
  return DoPreparePrinting(pInfo);
}

void Cmfc1View::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add extra initialization before printing
}

void Cmfc1View::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add cleanup after printing
}

// Cmfc1View diagnostics

#ifdef _DEBUG
void Cmfc1View::AssertValid() const
{
  CView::AssertValid();
}

void Cmfc1View::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

Cmfc1Doc* Cmfc1View::GetDocument() const // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Cmfc1Doc)));
  return (Cmfc1Doc*)m_pDocument;
}
#endif //_DEBUG

// Cmfc1View message handlers
