#pragma once
#include "DisplayItem.h"

class CTaskbarItemOrderHelper
{
public:
    CTaskbarItemOrderHelper(bool displayed_only = false);

    void Init();

    //Get each displayed item in the task window according to the set order
    std::vector<CommonDisplayItem> GetAllDisplayItemsWithOrder() const;

    void FromString(const std::wstring& str);
    std::wstring ToString() const;
    void SetOrder(const vector<int>& item_order);
    const vector<int>& GetItemOrderConst() const;
    vector<int>& GetItemOrder();

    static CString GetItemDisplayName(CommonDisplayItem item);
    static bool IsItemDisplayed(CommonDisplayItem item);

private:
    //Standardize the items in m_item_order. If there are serial numbers in m_item_order that exceed the number of displayed items, remove them and add the serial numbers of the missing items later.
    void NormalizeItemOrder();

private:
    vector<int> m_item_order;   //Save the order of each item
    vector<CommonDisplayItem> m_all_item_in_default_order;    //All display items saved in default order
    bool m_displayed_only;
};
