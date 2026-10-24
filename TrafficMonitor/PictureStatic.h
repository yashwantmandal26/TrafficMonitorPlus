/*A static control for displaying images
Call the SetPicture() function to set a static picture when needed.
When the control is redrawn, it will send the WM_CONTROL_REPAINT message to the parent window.
And pass the CWnd pointer of the current control through wParam, and pass the CDC pointer through lParam.
*/
#pragma once
#define WM_CONTROL_REPAINT (WM_USER + 1003)		//Control redraw message

class CPictureStatic : public CStatic
{
public:
	CPictureStatic();
	~CPictureStatic();

	void SetPicture(UINT pic_id);
	void SetPicture(HBITMAP hBitmap);

protected:
	CDC m_memDC;
	CBitmap m_bitmap;
	CRect m_rect;
	BITMAP m_bm;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
};

