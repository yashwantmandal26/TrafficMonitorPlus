#pragma once
#include "afxwin.h"
#include "BaseDialog.h"

// CMessageDlg dialog box

class CMessageDlg : public CBaseDialog
{
	DECLARE_DYNAMIC(CMessageDlg)

public:
	CMessageDlg(CWnd* pParent = NULL);   // Standard constructor
	virtual ~CMessageDlg();

	void SetWindowTitle(LPCTSTR str);	//Set window title
	void SetInfoText(LPCTSTR str);		//Set message title
	void SetMessageText(LPCTSTR str);	//Set message text
	//void ShowLinkStatic(bool show = true) { m_show_link_ctrl = show; }
	//void SetLinkInfo(LPCTSTR text, LPCTSTR url);
    void SetMessageIcon(HICON hIcon);

	//System standard icon for message dialog box
	enum StandardIcon
	{
		SI_INFORMATION,
		SI_WARNING,
		SI_ERROR
	};

	//Set the system standard icon for the message dialog box
	void SetStandarnMessageIcon(StandardIcon standard_icon);

// Dialog data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MESSAGE_DIALOG };
#endif

protected:
	CEdit m_message_edit;
	CSize m_min_size;		//Minimum size of window
	CStatic m_info_static;

	CString m_title;
	CString m_info;
	CString m_message;

	//CString m_link_text;
	//CString m_link_url;

    HICON m_icon{};
    CPoint m_icon_pos{};        //icon position

	//bool m_show_link_ctrl{ false };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	//afx_msg void OnNMClickSyslink1(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
};
