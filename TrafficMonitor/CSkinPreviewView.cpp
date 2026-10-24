// CSkinPreviewView.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "CSkinPreviewView.h"


// CSkinPreviewView

IMPLEMENT_DYNCREATE(CSkinPreviewView, CScrollView)

CSkinPreviewView::CSkinPreviewView()
{

}

CSkinPreviewView::~CSkinPreviewView()
{
}


BEGIN_MESSAGE_MAP(CSkinPreviewView, CScrollView)
END_MESSAGE_MAP()


// CSkinPreviewView drawing

void CSkinPreviewView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CSize sizeTotal;
	// TODO: Calculate the total size of this view
	m_size.cx = 0;
	m_size.cy = 0;
	SetScrollSizes(MM_TEXT, m_size);


}

void CSkinPreviewView::OnDraw(CDC* pDC)
{
    CDrawCommon drawer;
    drawer.Create(pDC, nullptr);
    CRect view_rect;
    GetClientRect(view_rect);
    view_rect.right = (std::max)(view_rect.Width(), static_cast<int>(m_size.cx));
    view_rect.bottom = (std::max)(view_rect.Height(), static_cast<int>(m_size.cy));

    //If the skin is in png format, draw a 10x10 chessboard background
    if (m_skin_data->IsPNG())
    {
        int grid_size = theApp.DPI(10);

        // Check canvas size
        int rows = view_rect.Height() / grid_size + 1;  // Number of lines
        int cols = view_rect.Width() / grid_size + 1;  // Number of columns

        // Traverse each grid
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                // Calculate the rectangular area of the current grid
                CRect rect(col * grid_size, row * grid_size,
                    (col + 1) * grid_size, (row + 1) * grid_size);

                // Determine the current grid color (alternating fill)
                COLORREF color = ((row + col) % 2 == 0) ? RGB(204, 204, 204) : RGB(254, 254, 254);

                // filled rectangle
                drawer.FillRect(rect, color);
            }
        }
    }
    //Draw a solid color background
    else
    {
        drawer.FillRect(view_rect, GetSysColor(COLOR_WINDOW));
    }

    //Draw preview
    CRect draw_rect(CPoint(0, 0), m_size);
    m_skin_data->DrawPreview(pDC, draw_rect);
}


// CSkinPreviewView Diagnostics

//#ifdef _DEBUG
//void CSkinPreviewView::AssertValid() const
//{
//	CScrollView::AssertValid();
//}

//#ifndef _WIN32_WCE
//void CSkinPreviewView::Dump(CDumpContext& dc) const
//{
//	CScrollView::Dump(dc);
//}
//#endif
//#endif //_DEBUG

void CSkinPreviewView::InitialUpdate()
{
	OnInitialUpdate();
}
void CSkinPreviewView::SetSize(int width, int hight)
{
	m_size = CSize(width, hight);
	SetScrollSizes(MM_TEXT, m_size);
}

// CSkinPreviewView message handler
