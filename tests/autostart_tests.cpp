#include "pch.h"
#include "config/autostart.h"
#include <taskschd.h>
#include <cassert>
#include <iostream>
#include <atomic>

using namespace tmp;

template<typename T, typename... Args>
Microsoft::WRL::ComPtr<T> Make(Args&&... args) {
    Microsoft::WRL::ComPtr<T> result;
    result.Attach(new T(std::forward<Args>(args)...));
    return result;
}

// Tracking allocators for test validation
static std::atomic<int> g_bstr_alloc_count{0};
static std::atomic<int> g_bstr_free_count{0};

static BSTR TrackedAlloc(const wchar_t* s) {
    g_bstr_alloc_count++;
    return SysAllocString(s);
}

// Custom mock COM classes for isolated Task Scheduler testing
template<typename Interface>
class MockBase : public Interface {
public:
    virtual ~MockBase() = default;
    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_count; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --ref_count;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDispatch) || riid == __uuidof(Interface)) {
            *ppv = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }
        if constexpr(std::is_base_of_v<ITrigger,Interface>){
            if(riid==__uuidof(ITrigger)){*ppv=static_cast<ITrigger*>(this);AddRef();return S_OK;}
        }
        if constexpr(std::is_base_of_v<IAction,Interface>){
            if(riid==__uuidof(IAction)){*ppv=static_cast<IAction*>(this);AddRef();return S_OK;}
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    // IDispatch dummies
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*) override { return E_NOTIMPL; }
private:
    std::atomic<ULONG> ref_count{1};
};

class MockExecAction : public MockBase<IExecAction> {
public:
    std::wstring path;
    bool fail_path = false;

    MockExecAction(std::wstring p) : path(std::move(p)) {}

    HRESULT STDMETHODCALLTYPE get_Path(BSTR* pPath) override {
        if (fail_path) return E_FAIL;
        if (!pPath) return E_POINTER;
        *pPath = TrackedAlloc(path.c_str());
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_Path(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Arguments(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Arguments(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_WorkingDirectory(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_WorkingDirectory(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Id(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Id(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Type(TASK_ACTION_TYPE*) override { return E_NOTIMPL; }
};

class MockActionCollection : public MockBase<IActionCollection> {
public:
    ComPtr<MockExecAction> exec;
    bool fail_get_item = false;

    MockActionCollection(ComPtr<MockExecAction> e) : exec(std::move(e)) {}

    HRESULT STDMETHODCALLTYPE get_Count(long*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Item(long index, IAction** ppAction) override {
        if (fail_get_item) return E_FAIL;
        if (!ppAction) return E_POINTER;
        if (index == 1 && exec) {
            return exec.CopyTo(ppAction);
        }
        return E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Create(TASK_ACTION_TYPE, IAction**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Remove(VARIANT) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Clear() override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Context(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Context(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_XmlText(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_XmlText(BSTR) override { return E_NOTIMPL; }
};

class MockLogonTrigger : public MockBase<ILogonTrigger> {
public:
    std::wstring delay;
    bool fail_delay = false;

    MockLogonTrigger(std::wstring d) : delay(std::move(d)) {}

    HRESULT STDMETHODCALLTYPE get_Delay(BSTR* pDelay) override {
        if (fail_delay) return E_FAIL;
        if (!pDelay) return E_POINTER;
        *pDelay = TrackedAlloc(delay.c_str());
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_Delay(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_UserId(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_UserId(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Type(TASK_TRIGGER_TYPE2*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Id(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Id(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Repetition(IRepetitionPattern**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Repetition(IRepetitionPattern*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_ExecutionTimeLimit(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_ExecutionTimeLimit(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_StartBoundary(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_StartBoundary(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_EndBoundary(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_EndBoundary(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Enabled(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Enabled(VARIANT_BOOL) override { return E_NOTIMPL; }
};

class MockTriggerCollection : public MockBase<ITriggerCollection> {
public:
    ComPtr<MockLogonTrigger> trigger;
    bool fail_get_item = false;

    MockTriggerCollection(ComPtr<MockLogonTrigger> t) : trigger(std::move(t)) {}

    HRESULT STDMETHODCALLTYPE get_Count(long*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Item(long index, ITrigger** ppTrigger) override {
        if (fail_get_item) return E_FAIL;
        if (!ppTrigger) return E_POINTER;
        if (index == 1 && trigger) {
            return trigger.CopyTo(ppTrigger);
        }
        return E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Create(TASK_TRIGGER_TYPE2, ITrigger**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Remove(VARIANT) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Clear() override { return E_NOTIMPL; }
};

class MockPrincipal : public MockBase<IPrincipal> {
public:
    TASK_RUNLEVEL_TYPE run_level = TASK_RUNLEVEL_HIGHEST;
    bool fail_runlevel = false;

    HRESULT STDMETHODCALLTYPE get_Id(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Id(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_DisplayName(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_DisplayName(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_UserId(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_UserId(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_LogonType(TASK_LOGON_TYPE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_LogonType(TASK_LOGON_TYPE) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_GroupId(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_GroupId(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_RunLevel(TASK_RUNLEVEL_TYPE* pRunLevel) override {
        if (fail_runlevel) return E_FAIL;
        if (!pRunLevel) return E_POINTER;
        *pRunLevel = run_level;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_RunLevel(TASK_RUNLEVEL_TYPE) override { return E_NOTIMPL; }
};

class MockTaskSettings : public MockBase<ITaskSettings> {
public:
    int restart_count = 3;
    int priority = 4;
    bool fail_settings = false;

    HRESULT STDMETHODCALLTYPE get_AllowDemandStart(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_AllowDemandStart(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_RestartInterval(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_RestartInterval(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_RestartCount(int* pRestartCount) override {
        if (fail_settings) return E_FAIL;
        if (!pRestartCount) return E_POINTER;
        *pRestartCount = restart_count;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_RestartCount(int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_MultipleInstances(TASK_INSTANCES_POLICY*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_MultipleInstances(TASK_INSTANCES_POLICY) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_StopIfGoingOnBatteries(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_StopIfGoingOnBatteries(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_DisallowStartIfOnBatteries(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_DisallowStartIfOnBatteries(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_AllowHardTerminate(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_AllowHardTerminate(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_StartWhenAvailable(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_StartWhenAvailable(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_XmlText(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_XmlText(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_RunOnlyIfNetworkAvailable(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_RunOnlyIfNetworkAvailable(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_ExecutionTimeLimit(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_ExecutionTimeLimit(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Enabled(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Enabled(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_DeleteExpiredTaskAfter(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_DeleteExpiredTaskAfter(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Priority(int* pPriority) override {
        if (fail_settings) return E_FAIL;
        if (!pPriority) return E_POINTER;
        *pPriority = priority;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_Priority(int) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Compatibility(TASK_COMPATIBILITY*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Compatibility(TASK_COMPATIBILITY) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Hidden(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Hidden(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_IdleSettings(IIdleSettings**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_IdleSettings(IIdleSettings*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_RunOnlyIfIdle(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_RunOnlyIfIdle(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_WakeToRun(VARIANT_BOOL*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_WakeToRun(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_NetworkSettings(INetworkSettings**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_NetworkSettings(INetworkSettings*) override { return E_NOTIMPL; }
};

class MockTaskDefinition : public MockBase<ITaskDefinition> {
public:
    ComPtr<MockPrincipal> principal;
    ComPtr<MockTaskSettings> settings;
    ComPtr<MockTriggerCollection> triggers;
    ComPtr<MockActionCollection> actions;

    HRESULT STDMETHODCALLTYPE get_RegistrationInfo(IRegistrationInfo**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_RegistrationInfo(IRegistrationInfo*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Triggers(ITriggerCollection** ppTriggers) override {
        if (!ppTriggers) return E_POINTER;
        return triggers.CopyTo(ppTriggers);
    }
    HRESULT STDMETHODCALLTYPE put_Triggers(ITriggerCollection*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Settings(ITaskSettings** ppSettings) override {
        if (!ppSettings) return E_POINTER;
        return settings.CopyTo(ppSettings);
    }
    HRESULT STDMETHODCALLTYPE put_Settings(ITaskSettings*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Data(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_Data(BSTR) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Principal(IPrincipal** ppPrincipal) override {
        if (!ppPrincipal) return E_POINTER;
        return principal.CopyTo(ppPrincipal);
    }
    HRESULT STDMETHODCALLTYPE put_Principal(IPrincipal*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Actions(IActionCollection** ppActions) override {
        if (!ppActions) return E_POINTER;
        return actions.CopyTo(ppActions);
    }
    HRESULT STDMETHODCALLTYPE put_Actions(IActionCollection*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_XmlText(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE put_XmlText(BSTR) override { return E_NOTIMPL; }
};

class MockRegisteredTask : public MockBase<IRegisteredTask> {
public:
    VARIANT_BOOL enabled = VARIANT_TRUE;
    ComPtr<MockTaskDefinition> definition;
    bool fail_definition = false;
    bool fail_enabled = false;

    HRESULT STDMETHODCALLTYPE get_Name(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Path(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_State(TASK_STATE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Enabled(VARIANT_BOOL* pEnabled) override {
        if (fail_enabled) return E_FAIL;
        if (!pEnabled) return E_POINTER;
        *pEnabled = enabled;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_Enabled(VARIANT_BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Run(VARIANT, IRunningTask**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RunEx(VARIANT, long, long, BSTR, IRunningTask**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetInstances(long, IRunningTaskCollection**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_LastRunTime(DATE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_LastTaskResult(long*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_NumberOfMissedRuns(long*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_NextRunTime(DATE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE get_Definition(ITaskDefinition** ppDef) override {
        if (fail_definition) return E_FAIL;
        if (!ppDef) return E_POINTER;
        return definition.CopyTo(ppDef);
    }
    HRESULT STDMETHODCALLTYPE get_Xml(BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetSecurityDescriptor(long, BSTR*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetSecurityDescriptor(BSTR, long) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Stop(long) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetRunTimes(const LPSYSTEMTIME, const LPSYSTEMTIME, DWORD*, LPSYSTEMTIME*) override { return E_NOTIMPL; }
};

static ComPtr<MockRegisteredTask> BuildTask(const std::wstring& path, const std::wstring& delay = L"PT0S") {
    auto task = Make<MockRegisteredTask>();
    auto def = Make<MockTaskDefinition>();
    def->principal = Make<MockPrincipal>();
    def->settings = Make<MockTaskSettings>();
    def->triggers = Make<MockTriggerCollection>(Make<MockLogonTrigger>(delay));
    def->actions = Make<MockActionCollection>(Make<MockExecAction>(path));
    task->definition = def;
    return task;
}

int main() {
    CoInitialize(nullptr);

    const std::wstring target_exe = L"C:\\Program Files\\TrafficMonitorPlus\\TrafficMonitorPlus.exe";
    const std::wstring other_exe = L"C:\\OldPath\\TrafficMonitorPlus.exe";

    std::cout << "[Test 1] Matching path returns true without double free\n";
    {
        auto task = BuildTask(target_exe);
        int allocs_before = g_bstr_alloc_count.load();
        bool match = AutoStart::IsMatchingTask(task.Get(), target_exe);
        assert(match == true);
        assert(g_bstr_alloc_count.load() == allocs_before + 2); // 1 for delay, 1 for path
        std::cout << "  Passed. Allocation count verified.\n";
    }

    std::cout << "[Test 2] Different path returns false (Audit Finding 1 regression: NO double free)\n";
    {
        auto task = BuildTask(other_exe); // Task exists with different path
        int allocs_before = g_bstr_alloc_count.load();
        bool match = AutoStart::IsMatchingTask(task.Get(), target_exe);
        assert(match == false);
        assert(g_bstr_alloc_count.load() == allocs_before + 2);
        std::cout << "  Passed. Task with different path safely returned false without double free.\n";
    }

    std::cout << "[Test 3] Null / missing task returns false\n";
    {
        bool match = AutoStart::IsMatchingTask(nullptr, target_exe);
        assert(match == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "[Test 4] Disabled task returns false\n";
    {
        auto task = BuildTask(target_exe);
        task->enabled = VARIANT_FALSE;
        assert(AutoStart::IsMatchingTask(task.Get(), target_exe) == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "[Test 5] Non-highest runlevel returns false\n";
    {
        auto task = BuildTask(target_exe);
        task->definition->principal->run_level = TASK_RUNLEVEL_LUA;
        assert(AutoStart::IsMatchingTask(task.Get(), target_exe) == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "[Test 6] Priority mismatch returns false\n";
    {
        auto task = BuildTask(target_exe);
        task->definition->settings->priority = 7;
        assert(AutoStart::IsMatchingTask(task.Get(), target_exe) == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "[Test 7] Delay mismatch returns false\n";
    {
        auto task = BuildTask(target_exe, L"PT30S");
        assert(AutoStart::IsMatchingTask(task.Get(), target_exe) == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "[Test 8] API failure on get_Delay returns false cleanly\n";
    {
        auto task = BuildTask(target_exe);
        task->definition->triggers->trigger->fail_delay = true;
        assert(AutoStart::IsMatchingTask(task.Get(), target_exe) == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "[Test 9] API failure on get_Path returns false cleanly\n";
    {
        auto task = BuildTask(target_exe);
        task->definition->actions->exec->fail_path = true;
        assert(AutoStart::IsMatchingTask(task.Get(), target_exe) == false);
        std::cout << "  Passed.\n";
    }

    std::cout << "\nAll AutoStart ownership tests passed successfully!\n";
    CoUninitialize();
    return 0;
}
