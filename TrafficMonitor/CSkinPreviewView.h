#pragma once
#include "DrawCommon.h"
#include "SkinFile.h"


// CSkinPreviewView view


class CSkinPreviewView : public CScrollView
{
	DECLARE_DYNCREATE(CSkinPreviewView)

protected:
	CSkinPreviewView();           // Protected constructor used by dynamic creation
	virtual ~CSkinPreviewView();

public:
//#ifdef _DEBUG
//	virtual void AssertValid() const;
//#ifndef _WIN32_WCE
//	virtual void Dump(CDumpContext& dc) const;
//#endif
//#endif

//member function
public:
	void InitialUpdate();
	void SetSize(int width, int hight);
	void SetSkinData(CSkinFile* skin_data) { m_skin_data = skin_data; }

	//member variables
protected:
	CSize m_size;
	CPoint m_start_point;			//starting position of drawing

    CSkinFile* m_skin_data;

protected:
	virtual void OnDraw(CDC* pDC);      // Override to draw this view
	virtual void OnInitialUpdate();     // first time after construction

	DECLARE_MESSAGE_MAP()
};


