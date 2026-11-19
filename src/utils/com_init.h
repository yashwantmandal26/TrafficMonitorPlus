// TrafficMonitorPlus - RAII COM Initialization
#pragma once
#include "../pch.h"

namespace tmp {

// RAII wrapper for COM initialization.
// Construct on the thread that needs COM; destructor calls CoUninitialize.
class ComInit {
public:
    explicit ComInit(DWORD flags = COINIT_MULTITHREADED) noexcept
        : initialized_(false)
    {
        HRESULT hr = ::CoInitializeEx(nullptr, flags);
        // S_OK = success, S_FALSE = already initialized on this thread (still must uninit)
        if (hr == S_OK || hr == S_FALSE) {
            initialized_ = true;
        }
    }

    ~ComInit() noexcept {
        if (initialized_) {
            ::CoUninitialize();
        }
    }

    // Non-copyable, non-movable (tied to the calling thread)
    ComInit(const ComInit&) = delete;
    ComInit& operator=(const ComInit&) = delete;
    ComInit(ComInit&&) = delete;
    ComInit& operator=(ComInit&&) = delete;

    [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

private:
    bool initialized_;
};

} // namespace tmp
