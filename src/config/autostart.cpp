#include "pch.h"
#include "config/autostart.h"
#include <taskschd.h>
#include <sddl.h>

namespace tmp {
namespace {

// RAII wrapper for allocated BSTR parameters
struct Bstr {
    BSTR value = nullptr;
    explicit Bstr(const std::wstring& text) : value(SysAllocString(text.c_str())) {}
    ~Bstr() { if (value) SysFreeString(value); }
    operator BSTR() const { return value; }
};

// RAII wrapper for out-parameter BSTRs received from COM calls
class ScopedBstr {
public:
    ScopedBstr() = default;
    ~ScopedBstr() { Reset(); }

    ScopedBstr(const ScopedBstr&) = delete;
    ScopedBstr& operator=(const ScopedBstr&) = delete;

    ScopedBstr(ScopedBstr&& other) noexcept : m_bstr(other.m_bstr) {
        other.m_bstr = nullptr;
    }
    ScopedBstr& operator=(ScopedBstr&& other) noexcept {
        if (this != &other) {
            Reset();
            m_bstr = other.m_bstr;
            other.m_bstr = nullptr;
        }
        return *this;
    }

    BSTR* GetAddressOf() {
        Reset();
        return &m_bstr;
    }

    void Reset() {
        if (m_bstr) {
            SysFreeString(m_bstr);
            m_bstr = nullptr;
        }
    }

    BSTR Get() const { return m_bstr; }
    operator BSTR() const { return m_bstr; }
    const wchar_t* c_str() const { return m_bstr ? m_bstr : L""; }

private:
    BSTR m_bstr = nullptr;
};

std::wstring UserSid() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return {};
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> buffer(size);
    const bool valid = GetTokenInformation(token, TokenUser, buffer.data(), size, &size) != FALSE;
    CloseHandle(token);
    LPWSTR sid = nullptr;
    if (!valid || !ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(buffer.data())->User.Sid, &sid)) return {};
    std::wstring result(sid);
    LocalFree(sid);
    return result;
}

ComPtr<ITaskFolder> TaskFolder() {
    ComPtr<ITaskService> service;
    if (FAILED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service)))) return {};
    VARIANT empty{};
    if (FAILED(service->Connect(empty, empty, empty, empty))) return {};
    ComPtr<ITaskFolder> folder;
    service->GetFolder(Bstr(L"\\"), &folder);
    return folder;
}

std::wstring TaskName() { return L"TrafficMonitorPlus-" + UserSid(); }

void RemoveLegacyRun() {
    RegDeleteKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"TrafficMonitorPlus");
}

} // namespace

std::wstring AutoStart::GetTaskName() {
    return TaskName();
}

std::wstring AutoStart::GetCurrentUserSid() {
    return UserSid();
}

bool AutoStart::IsMatchingTask(IRegisteredTask* existing, const std::wstring& exe_path) {
    if (!existing || exe_path.empty()) return false;

    VARIANT_BOOL enabled = VARIANT_FALSE;
    if (FAILED(existing->get_Enabled(&enabled)) || enabled != VARIANT_TRUE) {
        return false;
    }

    ComPtr<ITaskDefinition> definition;
    if (FAILED(existing->get_Definition(&definition)) || !definition) {
        return false;
    }

    ComPtr<IPrincipal> principal;
    TASK_RUNLEVEL_TYPE level{};
    if (FAILED(definition->get_Principal(&principal)) || !principal ||
        FAILED(principal->get_RunLevel(&level)) || level != TASK_RUNLEVEL_HIGHEST) {
        return false;
    }

    ComPtr<ITaskSettings> existing_settings;
    int restart_count = 0;
    int priority = 7;
    if (FAILED(definition->get_Settings(&existing_settings)) || !existing_settings ||
        FAILED(existing_settings->get_RestartCount(&restart_count)) || restart_count != 3 ||
        FAILED(existing_settings->get_Priority(&priority)) || priority != 4) {
        return false;
    }

    ComPtr<ITriggerCollection> triggers;
    ComPtr<ITrigger> trigger;
    ComPtr<ILogonTrigger> logon;
    ScopedBstr delay;
    if (FAILED(definition->get_Triggers(&triggers)) || !triggers ||
        FAILED(triggers->get_Item(1, &trigger)) || !trigger ||
        FAILED(trigger.As(&logon)) || !logon ||
        FAILED(logon->get_Delay(delay.GetAddressOf()))) {
        return false;
    }

    if (delay.Get() && wcslen(delay.Get()) > 0 && wcscmp(delay.Get(), L"PT0S") != 0) {
        return false;
    }

    ComPtr<IActionCollection> actions;
    ComPtr<IAction> action;
    ComPtr<IExecAction> exec;
    ScopedBstr path;
    if (FAILED(definition->get_Actions(&actions)) || !actions ||
        FAILED(actions->get_Item(1, &action)) || !action ||
        FAILED(action.As(&exec)) || !exec ||
        FAILED(exec->get_Path(path.GetAddressOf()))) {
        return false;
    }

    return path.Get() && _wcsicmp(path.Get(), exe_path.c_str()) == 0;
}

bool AutoStart::IsEnabled() {
    auto folder = TaskFolder();
    ComPtr<IRegisteredTask> task;
    VARIANT_BOOL enabled = VARIANT_FALSE;
    return folder && SUCCEEDED(folder->GetTask(Bstr(TaskName()), &task)) &&
        SUCCEEDED(task->get_Enabled(&enabled)) && enabled == VARIANT_TRUE;
}

bool AutoStart::RunElevated() {
    const HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(com)&&com!=RPC_E_CHANGED_MODE)return false;
    const bool started=[](){
        auto folder=TaskFolder();
        ComPtr<IRegisteredTask> task;
        if(!folder||FAILED(folder->GetTask(Bstr(TaskName()),&task))||
           !IsMatchingTask(task.Get(),GetCurrentExePath()))return false;
        VARIANT parameters{};
        ComPtr<IRunningTask> running;
        return SUCCEEDED(task->Run(parameters,&running));
    }();
    if(SUCCEEDED(com))CoUninitialize();
    return started;
}

bool AutoStart::Enable(const std::wstring& exe_path) {
    if (exe_path.empty()) return false;
    const auto sid = UserSid();
    if (sid.empty()) return false;
    auto folder = TaskFolder();
    if (!folder) return false;

    // Avoid rewriting a working elevated task on every startup (also works
    // when a standard-user launch cannot create elevated tasks).
    ComPtr<IRegisteredTask> existing;
    if (SUCCEEDED(folder->GetTask(Bstr(TaskName()), &existing)) && existing) {
        if (IsMatchingTask(existing.Get(), exe_path)) {
            RemoveLegacyRun();
            return true;
        }
    }

    ComPtr<ITaskService> service;
    VARIANT empty{};
    if (FAILED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&service))) ||
        FAILED(service->Connect(empty, empty, empty, empty))) return false;
    ComPtr<ITaskDefinition> task;
    if (FAILED(service->NewTask(0, &task))) return false;
    ComPtr<IPrincipal> principal;
    if (FAILED(task->get_Principal(&principal)) || FAILED(principal->put_UserId(Bstr(sid))) ||
        FAILED(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN)) ||
        FAILED(principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST))) return false;
    ComPtr<ITaskSettings> settings;
    if (FAILED(task->get_Settings(&settings)) ||
        FAILED(settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE)) ||
        FAILED(settings->put_StopIfGoingOnBatteries(VARIANT_FALSE)) ||
        FAILED(settings->put_StartWhenAvailable(VARIANT_TRUE)) ||
        FAILED(settings->put_ExecutionTimeLimit(Bstr(L"PT0S"))) ||
        FAILED(settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW)) ||
        FAILED(settings->put_Priority(4)) ||
        FAILED(settings->put_RestartCount(3)) || FAILED(settings->put_RestartInterval(Bstr(L"PT1M")))) return false;
    ComPtr<ITriggerCollection> triggers;
    ComPtr<ITrigger> trigger;
    ComPtr<ILogonTrigger> logon;
    if (FAILED(task->get_Triggers(&triggers)) || FAILED(triggers->Create(TASK_TRIGGER_LOGON, &trigger)) ||
        FAILED(trigger.As(&logon)) || FAILED(logon->put_UserId(Bstr(sid))) || FAILED(logon->put_Delay(Bstr(L"PT0S")))) return false;
    ComPtr<IActionCollection> actions;
    ComPtr<IAction> action;
    ComPtr<IExecAction> exec;
    if (FAILED(task->get_Actions(&actions)) || FAILED(actions->Create(TASK_ACTION_EXEC, &action)) ||
        FAILED(action.As(&exec)) || FAILED(exec->put_Path(Bstr(exe_path))) ||
        FAILED(exec->put_WorkingDirectory(Bstr(fs::path(exe_path).parent_path().wstring())))) return false;
    ComPtr<IRegisteredTask> registered;
    // Interactive token: no stored password and no launch before user sign-in.
    const auto status = folder->RegisterTaskDefinition(Bstr(TaskName()), task.Get(), TASK_CREATE_OR_UPDATE,
        empty, empty, TASK_LOGON_INTERACTIVE_TOKEN, empty, &registered);
    if (FAILED(status)) return false;
    RemoveLegacyRun();
    return true;
}

bool AutoStart::Disable() {
    RemoveLegacyRun();
    auto folder = TaskFolder();
    if (!folder) return false;
    auto hr = folder->DeleteTask(Bstr(TaskName()), 0);
    return SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

std::wstring AutoStart::GetCurrentExePath() {
    std::vector<wchar_t> path(32768);
    DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return size && size < path.size() ? std::wstring(path.data(), size) : L"";
}

} // namespace tmp
