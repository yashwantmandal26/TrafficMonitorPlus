#pragma once
#include <dxgi1_3.h>
#include "HResultException.h"
#include "RenderAPISupport.h"

#pragma comment(lib, "DXGI.lib")

class CDxgiException final : public CHResultException
{
    using CHResultException::CHResultException;
};

template <class SwapChain>
class CDxgiSwapChainResource : public CDeviceResourceBase<SwapChain>
{
public:
    using Base = CDeviceResourceBase<SwapChain>;
    using Base::Base;
    using typename Base::DeviceType;

    /**
     * @brief This function will be called when preparing the Resize swap chain to release all resources related to the swap chain. \n
     Note: This function may be called multiple times, and the number of calls does not necessarily match the number of OnSwapChainResizeEnd.
     Therefore, it must be ensured that the resource can be released repeatedly
     *
     */
    virtual void OnSwapChainResizeBegin() noexcept = 0;
    /**
     * @brief After the swap chain completes Resize, this function will be called to rebuild all resources related to the swap chain.
     *
     */
    virtual void OnSwapChainResizeEnd(DeviceType p_resized_swap_chain) noexcept = 0;
};

/**
 * @brief The corresponding resource of this type is CDxgiSwapChainResource<CDxgiSwapChain1>, not CDeviceResource<CDxgiSwapChain1>
 *
 */
class CDxgiSwapChain1
{
public:
    using Resource = CDxgiSwapChainResource<CDxgiSwapChain1>;
    using Type = Microsoft::WRL::ComPtr<IDXGISwapChain1>;
    using Storage = storage_t<CResourceTracker<Resource>>;

private:
    Type m_p_swap_chain1{};
    CResourceTracker<Resource> m_resource_tracker{std::make_shared<Storage>()};

public:
    void Recreate(Microsoft::WRL::ComPtr<IUnknown> p_device, const DXGI_SWAP_CHAIN_DESC1& ref_desc1, IDXGIOutput* p_output = nullptr);
    auto GetStorage()
        -> std::shared_ptr<Storage>;
    void Resize(std::uint32_t width, std::uint32_t height);
};

class CDxgi1Support2
{
public:
    static bool CheckSupport();
    static IDXGIFactory2* GetFactory();
};
