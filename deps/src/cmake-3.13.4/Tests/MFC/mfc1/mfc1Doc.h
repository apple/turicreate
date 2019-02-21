// mfc1Doc.h : interface of the Cmfc1Doc class
//

#pragma once

class Cmfc1Doc : public CDocument
{
protected: // create from serialization only
  Cmfc1Doc();
  DECLARE_DYNCREATE(Cmfc1Doc)

  // Attributes
public:
  // Operations
public:
  // Overrides
public:
  virtual BOOL OnNewDocument();
  virtual void Serialize(CArchive& ar);

  // Implementation
public:
  virtual ~Cmfc1Doc();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected:
  // Generated message map functions
protected:
  DECLARE_MESSAGE_MAP()
};
