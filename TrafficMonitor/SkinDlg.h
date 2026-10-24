#pragma once
#include "afxwin.h"
#include"StaticEx.h"
#include "PictureStatic.h"
#include "CSkinPreviewView.h"
#include "LinkStatic.h"
#include "BaseDialog.h"

// CSkinDlg dialog box

class CSkinDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CSkinDlg)

public:
    CSkinDlg(CWnd* pParent = NULL);   // Standard constructor
    virtual ~CSkinDlg();

    int m_skin_selected;			//skin of choice

// Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SKIN_DIALOG };
#endif

    const CSkinFile& GetSkinData() const { return m_skin_data; }

protected:
    virtual CString GetDialogName() const override;
    virtual bool InitializeControls() override;
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
    //control variable
    CListBox m_skin_list_box;   //Select skin list
    CLinkStatic m_skin_course;	//"Skin Making Tutorial" hyperlink
    CLinkStatic m_skin_download;	//"More skin downloads" hyperlink
    CLinkStatic m_open_skin_dir_lnk;
    CSkinPreviewView* m_view;	//Preview area view class
    CStatic m_preview_static;
    CStaticEx m_notify_static;	//Static control that displays prompt information

    CSkinFile m_skin_data;		//skin data

    void ShowPreview();		//Show skin preview
    CRect CalculateViewRect();		//Calculate preview view size based on window size

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnLbnSelchangeList1();
    afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
    afx_msg LRESULT OnLinkClicked(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnBnClickedSkinAutoAdaptButton();
    virtual void OnOK();
    afx_msg void OnBnClickedSkinAutoAdaptCheck();
};
