#pragma once
#include "PluginInterface.h"
#include <gdiplus.h>

class CCustomDrawItem2 : public IPluginItem
{
public:
    CCustomDrawItem2();

    // Inherited through IPluginItem
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual bool IsCustomDraw() const override;
    virtual int GetItemWidth() const override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
    virtual bool DrawItemEx(IPluginDrawer* pDrawer, int x, int y, int w, int h, bool dark_mode) override;
    virtual int IsDoubleLineExclusive() const { return 1; }     //Exclusively double rows in taskbar

private:
    Gdiplus::Image* m_png{};
};

