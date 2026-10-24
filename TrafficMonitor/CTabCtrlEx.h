#pragma once


// CTabCtrlEx

class CTabCtrlEx : public CTabCtrl
{
	DECLARE_DYNAMIC(CTabCtrlEx)

public:
	CTabCtrlEx();
	virtual ~CTabCtrlEx();

	void AddWindow(CWnd* pWnd, LPCTSTR lable_text);		//Add a child window to the current tab control
	void SetCurTab(int index);
    CWnd* GetCurrentTab();
    void AdjustTabWindowSize();

protected:
    void CalSubWindowSize();

	DECLARE_MESSAGE_MAP()

protected:
	vector<CWnd*> m_tab_list;		//Save the pointer of each sub-window of the tab control
public:
	afx_msg void OnTcnSelchange(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void PreSubclassWindow();

	CRect m_tab_rect;
    afx_msg void OnSize(UINT nType, int cx, int cy);
};


