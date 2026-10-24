#pragma once
#include"Common.h"
#include "afxcmn.h"
#include "AdapterCommon.h"
#include "BaseDialog.h"
#include "ListCtrlEx.h"

// CNetworkInfoDlg dialog box

class CNetworkInfoDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CNetworkInfoDlg)

public:
    CNetworkInfoDlg(vector<NetWorkConection>& adapters, MIB_IFROW* pIfRow, int connection_selected, CWnd* pParent = NULL);   // Standard constructor
    virtual ~CNetworkInfoDlg();

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_NETWORK_INFO_DIALOG };
#endif

    SYSTEMTIME m_start_time;		//Program start time

protected:

    vector<NetWorkConection>& m_connections;
    MIB_IFROW* m_pIfRow;
    int m_connection_selected;		//The connection currently displayed in the dialog box
    int m_current_connection;		//Initial selected connection

    CListCtrlEx m_info_list;
    CMenu m_menu;
    CString m_selected_string;
    CFont m_font_bold;		//Default font bold

    CWinThread* m_pGetIPThread;			//Thread to obtain external network IP
    bool m_ip_acquired{ false };        //True if the external IP address has been obtained

    //void GetIPAddress(); //Get IP address
    void ShowInfo();
    void GetProgramElapsedTime();
    MIB_IFROW& GetConnectIfTable(int connection_index);    //Gets the MIB_IFROW object of the currently selected network connection. connection_index is the index in m_connections
    NetWorkConection GetConnection(int connection_index); //Gets the NetWorkConection object of the currently selected network connection. connection_index is the index in m_connections

    //Thread function to obtain external network IP
    static UINT GetInternetIPThreadFunc(LPVOID lpParam);

    virtual CString GetDialogName() const override;

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()

    virtual BOOL OnInitDialog();
public:
    afx_msg void OnCopyText();
    afx_msg void OnNMRClickInfoList1(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnClose();
    afx_msg void OnBnClickedPreviousButton();
    afx_msg void OnBnClickedNextButton();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnNMDblclkInfoList1(NMHDR* pNMHDR, LRESULT* pResult);
};
