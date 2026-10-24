/*Inherited from the CStatic class and used as a hyperlink:
Call the SetURL function to set the hyperlink;
Call the SetLinkIsURL function to set whether to open a hyperlink or respond to a command after clicking the control
If SetLinkIsURL(false) is called, a WM_LINK_CLICKED message will be sent to the parent window after clicking the control.
And pass the pointer of the control through WPARAM.
Otherwise, the hyperlink opens when the control is clicked
*/
#pragma once

#define WM_LINK_CLICKED (WM_USER + 1002)

// CLinkStatic

class CLinkStatic : public CStatic
{
	DECLARE_DYNAMIC(CLinkStatic)

public:
	CLinkStatic();
	virtual ~CLinkStatic();
    void SetBackgroundColor(COLORREF background_color);

public:
	void SetURL(CString strURL);		//Set up hyperlink
	CString GetURL() const;			//Get hyperlink string
	void SetLinkIsURL(bool enable) { m_link_is_url = enable; }
protected:
	bool m_link_is_url{ true };		//If true, open the hyperlink after clicking it, otherwise send a click message to the parent window
	bool m_bHot{ false };			//True when the mouse points to a hyperlink
	CString m_strURL;				//hyperlink string
    COLORREF m_back_color{ GetSysColor(COLOR_BTNFACE) };
    CFont m_hover_font;
    bool IsLinkValid() const;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
public:
	afx_msg void OnPaint();
	virtual void PreSubclassWindow();
};


