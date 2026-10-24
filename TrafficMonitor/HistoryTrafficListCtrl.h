#pragma once
#include "afxcmn.h"
#include "DrawCommon.h"
class CHistoryTrafficListCtrl :
	public CListCtrl
{
DECLARE_DYNAMIC(CHistoryTrafficListCtrl)
public:
	CHistoryTrafficListCtrl();
	~CHistoryTrafficListCtrl();

	void EnableDrawItemRange(bool draw = true) { m_draw_item_range = draw; }	//Set whether to draw a rectangle representing the size of a numerical value in a column of cells
	void SetDrawItemRangeRow(int row) { m_draw_item_range_row = row; }			//Set the column to draw a rectangle representing the numerical size. Note that this column must be the rightmost column, otherwise, all columns to the right of this column will not be displayed.
	void SetDrawItemRangeData(int item, double range, COLORREF color);			//Set the item data size (value is 1~1000) and color of a certain row
	void SetDrawItemRangMargin(int margin) { m_margin = margin; }			//Sets the rectangle that draws the edge of the drawn rectangle to the cell border. The larger this value is, the thinner the drawn rectangle is, but it cannot exceed half of the list line spacing.
	void SetDrawItemRangInLogScale(bool log_scale);				//Sets whether to use a logarithmic scale when drawing a rectangle representing the size of a value, otherwise use a linear scale

protected:
	struct ItemData
	{
		double data_value;		//The numerical size represented by the rectangle to be drawn, ranging from 0 to 1000
		COLORREF color;		//The color of the rectangle to draw
	};
	bool m_draw_item_range{ false };		//Do you need to draw a rectangle representing the numerical size in a certain column of cells?
	int m_draw_item_range_row{};		//A column that needs to draw a rectangle representing the size of the value
	int m_margin{};
	vector<ItemData> m_item_rage_data;	//Used to represent the size of the data that needs to be drawn and the color of the drawing.
	bool m_use_log_scale{ false };		//If true then use logarithmic scale to draw the rectangle, otherwise use linear scale

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};

