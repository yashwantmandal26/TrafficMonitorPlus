#pragma once
#include "BaseDialog.h"

// CSetItemOrderDlg dialog box

class CSetItemOrderDlg : public CBaseDialog
{
    DECLARE_DYNAMIC(CSetItemOrderDlg)

public:
    CSetItemOrderDlg(CWnd* pParent = nullptr);   // Standard constructor
    virtual ~CSetItemOrderDlg();

    //Set/get display order
    void SetItemOrder(const std::vector<int>& item_order);
    const std::vector<int>& GetItemOrder() const;

    //Set/get display items
    void SetDisplayItem(const DisplayItemSet& display_item);
    DisplayItemSet GetDisplayItem() const;

    void SetPluginDisplayItem(const StringSet& plugin_item);
    const StringSet& GetPluginDisplayItem() const;

    // Dialog data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SELECT_ORDER_DIALOG };
#endif

private:
    CTaskbarItemOrderHelper m_item_order;   //The order in which items are displayed
    CCheckListBox m_list_ctrl;
    DisplayItemSet m_display_item;        //Built-in items to display
    StringSet m_plugin_item;            //Plug-in items to display
    std::vector<CommonDisplayItem> m_all_displayed_item;    //All items shown in the list

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    void ShowItem();
    void EnableCtrl(int list_selected);
    void EnableDlgCtrl(UINT id, bool enable);

    bool GetItemChecked(CommonDisplayItem item);
    void SaveItemChecked(CommonDisplayItem item, bool checked);

    // Inherited through CBaseDialog
    virtual CString GetDialogName() const override;

    DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedMoveUpButton();
    afx_msg void OnBnClickedMoveDownButton();
    afx_msg void OnBnClickedRestoreDefaultButton();
    afx_msg void OnLbnSelchangeList1();
    afx_msg void OnCheckChanged();
    virtual void OnOK();
};
