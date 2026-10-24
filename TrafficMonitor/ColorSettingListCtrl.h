#pragma once
#include "afxcmn.h"
#include "DrawCommon.h"
class CColorSettingListCtrl :
	public CListCtrl
{
DECLARE_DYNAMIC(CColorSettingListCtrl)
public:
	CColorSettingListCtrl();
	~CColorSettingListCtrl();

    void SetItemColor(int row, int col, COLORREF color);
    COLORREF GetItemColor(int row, int col);
	void SetDrawItemRangMargin(int margin) { m_margin = margin; }			//Sets the rectangle that draws the edge of the drawn rectangle to the cell border. The larger this value is, the thinner the drawn rectangle is, but it cannot exceed half of the list line spacing.

protected:
	int m_margin{};
	std::map<int, std::map<int, COLORREF>> m_colors;	//Used to save the color of each row and column

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};

