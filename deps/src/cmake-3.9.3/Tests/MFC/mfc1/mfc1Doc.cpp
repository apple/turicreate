// mfc1Doc.cpp : implementation of the Cmfc1Doc class
//

#include "stdafx.h"

#include "mfc1.h"

#include "mfc1Doc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Cmfc1Doc

IMPLEMENT_DYNCREATE(Cmfc1Doc, CDocument)

BEGIN_MESSAGE_MAP(Cmfc1Doc, CDocument)
END_MESSAGE_MAP()

// Cmfc1Doc construction/destruction

Cmfc1Doc::Cmfc1Doc()
{
  // TODO: add one-time construction code here
}

Cmfc1Doc::~Cmfc1Doc()
{
}

BOOL Cmfc1Doc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  // TODO: add reinitialization code here
  // (SDI documents will reuse this document)

  return TRUE;
}

// Cmfc1Doc serialization

void Cmfc1Doc::Serialize(CArchive& ar)
{
  if (ar.IsStoring()) {
    // TODO: add storing code here
  } else {
    // TODO: add loading code here
  }
}

// Cmfc1Doc diagnostics

#ifdef _DEBUG
void Cmfc1Doc::AssertValid() const
{
  CDocument::AssertValid();
}

void Cmfc1Doc::Dump(CDumpContext& dc) const
{
  CDocument::Dump(dc);
}
#endif //_DEBUG

// Cmfc1Doc commands
