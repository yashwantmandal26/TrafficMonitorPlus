/*
CStaticDerived class of class used as a colored text control:
Call SetTextColor to set the text color;
Call SetWindowTextEx to set the control text when needed
*/
#pragma once
#include "afxwin.h"
class CStaticEx :
	public CStatic
{
public:
	CStaticEx();
	~CStaticEx();

	//Alignment
	enum Alignment
	{
		LEFT,       //left aligned
		RIGHT,      //Align right
		CENTER,     //center
	};

	//When using a Static control to draw colored text
public:
	void SetWindowTextEx(LPCTSTR lpszString, Alignment align = Alignment::LEFT);	//Set colored text for the control (need to be used with SetTextColor)
	void SetTextColor(COLORREF textColor);		//Set control text color
	void SetBackColor(COLORREF back_color);		//Set control background color
	CString GetString() const;			//Get control text

protected:
	bool m_color_text{ false };
	COLORREF m_text_color;	//Control text color
	COLORREF m_back_color;
	CString m_text;			//text on control
	Alignment m_align{};		//text alignment
	bool m_draw_background_color{ false };	//Do you need to fill the background color for the control?

protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	virtual void PreSubclassWindow();
};

