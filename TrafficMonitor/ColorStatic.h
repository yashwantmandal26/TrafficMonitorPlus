// CColorStatic
//Derived from CStatic, a Static control used to display colors
#pragma once
#define WM_STATIC_CLICKED (WM_USER + 1001)

class CColorStatic : public CStatic
{
	DECLARE_DYNAMIC(CColorStatic)

public:
	CColorStatic();
	virtual ~CColorStatic();

	void SetFillColor(COLORREF fill_color);		//Set a single background color to fill
	void SetColorNum(int color_num);			//Set the number of colors
	void SetFillColor(int index, COLORREF fill_color);
	void SetLinkCursor(bool link_cursor = true);		//Set the cursor to become a hyperlink shape when pointing to a control
    void EnableWindow(bool enable);

protected:
	//COLORREF m_fill_color{ RGB(255, 255,255) };
	vector<COLORREF> m_colors;
	bool m_hover{ false };		//True when the mouse points to the control
	bool m_link_cursor{ false };	//Whether to enable hyperlink-shaped cursor

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual void PreSubclassWindow();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};
