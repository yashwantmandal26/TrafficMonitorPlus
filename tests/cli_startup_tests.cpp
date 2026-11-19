#include "pch.h"
#include "config/cli_parser.h"
#include "config/autostart.h"
#include <cassert>
#include <iostream>

using namespace tmp;

void TestCommandLineParsing() {
    std::cout << "[Test 1] Table-driven CLI argument parser\n";
    assert(ParseCommandLine(L"--remove-startup").mode == AppMode::RemoveStartup);
    assert(ParseCommandLine(L"--remove-startup --elevate").mode == AppMode::Invalid);

    // 1. Default empty string -> NormalUI
    {
        auto opt = ParseCommandLine(L"");
        assert(opt.mode == AppMode::NormalUI);
        assert(!opt.show_settings);
        assert(!opt.wait_for_instance);
        assert(!opt.elevate_requested);
    }
    {
        auto opt = ParseCommandLine(L"   ");
        assert(opt.mode == AppMode::NormalUI);
    }

    // 2. --configure-startup (Audit Finding 20: now reachable and explicit)
    {
        auto opt = ParseCommandLine(L"--configure-startup");
        assert(opt.mode == AppMode::ConfigureStartup);
    }
    {
        auto opt = ParseCommandLine(L"  --configure-startup  ");
        assert(opt.mode == AppMode::ConfigureStartup);
    }

    // 3. --sensor-report
    {
        auto opt = ParseCommandLine(L"--sensor-report");
        assert(opt.mode == AppMode::SensorReport);
    }

    // 4. --show-settings
    {
        auto opt = ParseCommandLine(L"--show-settings");
        assert(opt.mode == AppMode::NormalUI);
        assert(opt.show_settings == true);
    }

    // 5. --exit-running
    {
        auto opt = ParseCommandLine(L"--exit-running");
        assert(opt.mode == AppMode::ExitRunning);
    }

    // 6. --wait-for-instance
    {
        auto opt = ParseCommandLine(L"--wait-for-instance");
        assert(opt.mode == AppMode::NormalUI);
        assert(opt.wait_for_instance == true);
    }

    // 7. --elevate
    {
        auto opt = ParseCommandLine(L"--elevate");
        assert(opt.mode == AppMode::NormalUI);
        assert(opt.elevate_requested == true);
    }

    // 8. Combined arguments: --wait-for-instance --show-settings
    {
        auto opt = ParseCommandLine(L"--wait-for-instance --show-settings");
        assert(opt.mode == AppMode::NormalUI);
        assert(opt.wait_for_instance == true);
        assert(opt.show_settings == true);
    }

    // 9. Invalid flags
    {
        auto opt = ParseCommandLine(L"--bogus-argument");
        assert(opt.mode == AppMode::Invalid);
    }

    std::cout << "  Passed. All command-line modes parsed accurately.\n";
}

void TestSingleInstanceMutexScoping() {
    std::cout << "[Test 2] Single-instance mutex scoping (Finding 18)\n";

    std::wstring mutex_name = GetSingleInstanceMutexName(AutoStart::GetCurrentUserSid());
    std::wcout << L"  Generated mutex name: " << mutex_name << L"\n";

    // Must be scoped under Local\ (not Global\)
    assert(mutex_name.rfind(L"Local\\", 0) == 0);
    assert(mutex_name.find(L"Global\\") == std::wstring::npos);
    assert(mutex_name.length() > 6);

    // Verify two different user SIDs generate distinct mutexes
    std::wstring userA_mutex = GetSingleInstanceMutexName(L"S-1-5-21-1111111111-2222222222-3333333333-1001");
    std::wstring userB_mutex = GetSingleInstanceMutexName(L"S-1-5-21-1111111111-2222222222-3333333333-1002");
    assert(userA_mutex != userB_mutex);
    assert(userA_mutex == L"Local\\TrafficMonitorPlus_S-1-5-21-1111111111-2222222222-3333333333-1001");
    assert(userB_mutex == L"Local\\TrafficMonitorPlus_S-1-5-21-1111111111-2222222222-3333333333-1002");

    // Attempt creation and verify it creates successfully
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutex_name.c_str());
    assert(hMutex != nullptr);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    std::cout << "  Passed. Mutex is properly scoped to the user session.\n";
}

int main() {
    TestCommandLineParsing();
    TestSingleInstanceMutexScoping();
    std::cout << "\nAll CLI and startup tests passed successfully!\n";
    return 0;
}
