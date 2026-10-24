#include "stdafx.h"
#include "HistoryTrafficListCtrl.h"
#include "DrawCommonHelper.h"

IMPLEMENT_DYNAMIC(CHistoryTrafficListCtrl, CListCtrl)

CHistoryTrafficListCtrl::CHistoryTrafficListCtrl()
{
}


CHistoryTrafficListCtrl::~CHistoryTrafficListCtrl()
{
}
void CHistoryTrafficListCtrl::SetDrawItemRangeData(int item, double range, COLORREF color)
{
	if (item < 0) return;
	if (item >= static_cast<int>(m_item_rage_data.size()))
		m_item_rage_data.resize(item + 1);
	m_item_rage_data[item].data_value = range;
	m_item_rage_data[item].color = color;
}

void CHistoryTrafficListCtrl::SetDrawItemRangInLogScale(bool log_scale)
{
	m_use_log_scale = log_scale;
	Invalidate();
}

BEGIN_MESSAGE_MAP(CHistoryTrafficListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CHistoryTrafficListCtrl::OnNMCustomdraw)
END_MESSAGE_MAP()


void CHistoryTrafficListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_draw_item_range)
	{
		*pResult = CDRF_DODEFAULT;
		LPNMLVCUSTOMDRAW lplvdr = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);
		NMCUSTOMDRAW& nmcd = lplvdr->nmcd;
		switch (lplvdr->nmcd.dwDrawStage)	//Judgment status   
		{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:			//If you want to change the color before painting ITEM
			if (nmcd.dwItemSpec >= 0 && nmcd.dwItemSpec < m_item_rage_data.size())
			{
				double range = m_item_rage_data[nmcd.dwItemSpec].data_value;
				CDC* pDC = CDC::FromHandle(nmcd.hdc); //Get drawing DC
				CRect item_rect, draw_rect;
				GetSubItemRect(nmcd.dwItemSpec,m_draw_item_range_row, LVIR_BOUNDS, item_rect); //Get the rectangular area of the drawing cell
				CDrawCommon::SetDrawRect(pDC, item_rect); //Set the drawing area to the current column
                //Drawing using double buffering
                {
                    CDrawDoubleBuffer draw_double_buffer(pDC, item_rect);
                    //fill background
                    draw_rect = item_rect;
                    draw_rect.MoveToXY(0, 0);
                    draw_double_buffer.GetMemDC()->FillSolidRect(draw_rect, GetSysColor(COLOR_WINDOW));
                    if (draw_rect.Height() > 2 * m_margin)
                    {
                        draw_rect.top += m_margin;
                        draw_rect.bottom -= m_margin;
                    }
                    int width;
                    if (m_use_log_scale) //Use logarithmic scale (y=ln(x+1))
                    {
                        range = std::log(range + 1);
                        width = static_cast<int>(range*draw_rect.Width() / std::log(1000 + 1));
                    }
                    else //Use linear scale (y=x)
                    {
                        width = static_cast<int>(range*draw_rect.Width() / 1000);
                    }
                    draw_rect.right = draw_rect.left + width;
                    draw_double_buffer.GetMemDC()->FillSolidRect(draw_rect, m_item_rage_data[nmcd.dwItemSpec].color);
                }

				//After the current column is drawn, set the drawing area to the left area to prevent the current column area from being overwritten.
				CRect rect1{ item_rect };
				rect1.left = 0;
				rect1.right = item_rect.left;
				CDrawCommon::SetDrawRect(pDC, rect1);
			}
			*pResult = CDRF_DODEFAULT;
			break;
		}
	}
}
