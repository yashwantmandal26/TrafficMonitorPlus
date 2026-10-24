#include "stdafx.h"
#include "PictureStatic.h"


CPictureStatic::CPictureStatic()
{
}


CPictureStatic::~CPictureStatic()
{
	m_memDC.DeleteDC();
	m_bitmap.Detach();
}

void CPictureStatic::SetPicture(UINT pic_id)
{
	m_memDC.DeleteDC();
	if (!m_bitmap.LoadBitmap(pic_id))		//Load bitmap
		return;
	//Get the actual size of the image
	GetObject(m_bitmap, sizeof(BITMAP), &m_bm);
	CDC* pDC = GetDC();
	m_memDC.CreateCompatibleDC(pDC);
	m_memDC.SelectObject(&m_bitmap);
	//Get the control size
	GetClientRect(m_rect);
	//Manual redraw
	Invalidate();
}

void CPictureStatic::SetPicture(HBITMAP hBitmap)
{
	m_memDC.DeleteDC();
	m_bitmap.Detach();
	if (!m_bitmap.Attach(hBitmap))
		return;
	//Get the actual size of the image
	GetObject(m_bitmap, sizeof(BITMAP), &m_bm);
	CDC* pDC = GetDC();
	m_memDC.CreateCompatibleDC(pDC);
	m_memDC.SelectObject(&m_bitmap);
	//Get the control size
	GetClientRect(m_rect);
	//Manual redraw
	Invalidate();
}

BEGIN_MESSAGE_MAP(CPictureStatic, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()


void CPictureStatic::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: add message handler code here
					   // CStatic::OnPaint() is not called for drawing messages
	if (m_bitmap.m_hObject != NULL)
	{
		// The following two lines avoid image distortion
		dc.SetStretchBltMode(HALFTONE);
		dc.SetBrushOrg(0, 0);
		//Draw the image in the memory DC
		dc.StretchBlt(0, 0, m_rect.Width(), m_rect.Height(), &m_memDC, 0, 0, m_bm.bmWidth, m_bm.bmHeight, SRCCOPY);

		//Send a redraw message to the parent window
		CWnd* pParent{ GetParent() };
		if (pParent != nullptr)
			pParent->SendMessage(WM_CONTROL_REPAINT, (WPARAM)this, LPARAM(&dc));
	}
}
