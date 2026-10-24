#pragma once
#include "PluginInterface.h"

class CPluginSystemDate : public IPluginItem
{
public:
    CPluginSystemDate();

    // Inherited through IPluginItem
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;

private:
};
