// TrafficMonitorPlus - Application Entry Point
#include "pch.h"
#include "app.h"
#include "config/cli_parser.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR commandLine, int) {
    // Exclude the current working directory and PATH from dependency lookup.
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);

    const auto options = tmp::ParseCommandLine(commandLine ? commandLine : L"");

    if (options.mode == tmp::AppMode::ExitRunning) {
        return tmp::App::RequestRunningInstanceExit() ? 0 : 5;
    }

    if (options.mode == tmp::AppMode::Invalid) {
        return 1;
    }

    if (options.mode == tmp::AppMode::RemoveStartup) {
        const HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        if(FAILED(initialized))return 1;
        const bool removed=tmp::AutoStart::Disable();
        CoUninitialize();
        return removed?0:1;
    }
    // Explicit setup command for Task Scheduler startup registration
    if (options.mode == tmp::AppMode::ConfigureStartup) {
        if (!IsUserAnAdmin()) {
            wchar_t executable[32768]{};
            GetModuleFileNameW(nullptr, executable, 32768);
            SHELLEXECUTEINFOW launch{ sizeof(launch) };
            launch.lpVerb = L"runas";
            launch.lpFile = executable;
            launch.lpParameters = L"--configure-startup";
            launch.nShow = SW_SHOWNORMAL;
            launch.fMask = SEE_MASK_NOCLOSEPROCESS;
            if (ShellExecuteExW(&launch) && launch.hProcess) {
                WaitForSingleObject(launch.hProcess, 30000);
                DWORD exitCode = 0;
                GetExitCodeProcess(launch.hProcess, &exitCode);
                CloseHandle(launch.hProcess);
                return static_cast<int>(exitCode);
            }
            return 1; // Elevation declined or failed
        }

        CoInitialize(nullptr);
        if (!tmp::AutoStart::Enable(tmp::AutoStart::GetCurrentExePath())) {
            CoUninitialize();
            return 2;
        }

        auto settings = tmp::Settings::LoadFromFile(tmp::App::GetConfigPath());
        settings.auto_start = true;
        if (!settings.SaveToFile(tmp::App::GetConfigPath())) {
            // Roll back startup task if settings file write failed
            tmp::AutoStart::Disable();
            CoUninitialize();
            return 3;
        }

        CoUninitialize();
        return 0;
    }

    // Diagnostic sensor report command
    if (options.mode == tmp::AppMode::SensorReport) {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
        CoInitialize(nullptr);

        int exit_code = 0;
        tmp::HardwareMonitor hardware;
        hardware.Init();
        hardware.WaitForFirstSample(std::chrono::seconds(30));
        Sleep(2000); // Allow GPU rate counter second sample

        auto report = tmp::App::GetConfigPath().parent_path() / L"sensor-report.txt";
        std::error_code ec;
        fs::create_directories(report.parent_path(), ec);
        std::ofstream output(report);
        output << tmp::WideToUtf8(hardware.GetStatus());
        if (!output.good()) exit_code = 1;

        CoUninitialize();
        WSACleanup();
        return exit_code;
    }

    // Normal UI launch: standard-user operation supported without mandatory elevation
    if (options.show_settings && tmp::App::RequestRunningInstanceShow()) {
        return 0;
    }

    // Reuse the user's explicitly approved, path-validated highest-privilege
    // task for manual launches as well as sign-in. No repeated UAC prompt.
    if(!IsUserAnAdmin()&&tmp::AutoStart::RunElevated()){
        if(options.show_settings){
            for(int attempt=0;attempt<50;++attempt){
                if(tmp::App::RequestRunningInstanceShow())break;
                Sleep(100);
            }
        }
        return 0;
    }

    // Optional user-requested elevation
    if (options.elevate_requested && !IsUserAnAdmin()) {
        wchar_t executable[32768]{};
        GetModuleFileNameW(nullptr, executable, 32768);
        SHELLEXECUTEINFOW launch{ sizeof(launch) };
        launch.lpVerb = L"runas";
        launch.lpFile = executable;
        launch.lpParameters = commandLine;
        launch.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&launch)) {
            return 0;
        }
        // UAC cancelled: proceed to launch as standard user
    }

    // Single-instance check via user- and session-scoped named mutex
    const std::wstring mutex_name = tmp::App::GetSingleInstanceMutexName();
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutex_name.c_str());
    if (!hMutex) return 1;

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (options.wait_for_instance) {
            if (WaitForSingleObject(hMutex, 15000) != WAIT_OBJECT_0) {
                CloseHandle(hMutex);
                return 0;
            }
        } else {
            // An instance is already running; bring up its settings UI so the user gets immediate visual feedback
            tmp::App::RequestRunningInstanceShow();
            CloseHandle(hMutex);
            return 0;
        }
    }

    // Initialize per-monitor DPI awareness (v2 preferred)
    tmp::DpiHelper::Init();

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        hr = CoInitialize(nullptr);
    }
    const bool com_initialized = SUCCEEDED(hr);

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icex{};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    int exit_code = 0;
    {
        auto& app = tmp::App::Instance();
        if (app.Init(hInstance)) {
            if (options.show_settings) {
                app.OnMenuCommand(tmp::IDM_SETTINGS);
            }
            exit_code = app.Run();
        } else {
            exit_code = 1;
        }
        app.Shutdown();
    }

    if (com_initialized) CoUninitialize();
    WSACleanup();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return exit_code;
}
