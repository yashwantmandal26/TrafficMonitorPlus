#pragma once
#include <Windows.h>
#include <type_traits>
#include <functional>

class EnableWriteMemoryGuard
{
private:
    void* const m_p_memory;
    const std::size_t m_memory_size;
    DWORD m_last_flag;
    bool m_state;

public:
    template <class T>
    EnableWriteMemoryGuard(T* p_memory, std::size_t memory_size = sizeof(T))
        : m_p_memory{p_memory}, m_memory_size{memory_size}
    {
        m_state = ::VirtualProtect(m_p_memory, m_memory_size, PAGE_EXECUTE_READWRITE, &m_last_flag);
    }
    ~EnableWriteMemoryGuard();
    bool GetState();
};

template <class FunctionPointer>
struct to_std_function;
template <class R, class... Parameters>
struct to_std_function<R (WINAPI *)(Parameters...)>
{
    using type = std::function<R(Parameters...)>;
};
template <class FunctionPointer>
using to_std_function_t = typename to_std_function<FunctionPointer>::type;

/**
 * @brief is used to manage the behavior of the DrawText series functions in User32.dll after hooking. Currently, only D2D rendering is used.
 *
 */
class User32DrawTextManager
{
private:
    enum class DrawTextType
    {
        DrawTextA,
        DrawTextW,
        DrawTextExA,
        DrawTextExW
    };

    /**
     * @brief is a collection of setting data for each class. In order to facilitate access by internal non-member functions, all internal members are static.
     *
     * @tparam Owner owns the class type that sets the data to prevent multiple classes from sharing a set of data
     * @tparam Function The function pointer type to be stored
     */
    template <class Owner, class Function, class = std::enable_if_t<std::is_pointer<Function>{}>>
    struct CommonSettings
    {
    private:
        static bool m_enabled;

    protected:
        using ReplacedFunction = to_std_function_t<Function>;
        static ReplacedFunction m_replaced_function;

    public:
        static void** m_p_iat_old_function_pointer;
        static Function m_old_function_pointer;

        struct State
        {
            bool m_is_enabled;
            ReplacedFunction m_replaced_function;
        };

        static void SetEnable(bool is_enabled) noexcept
        {
            m_enabled = is_enabled;
        }
        static bool GetEnable() noexcept
        {
            return m_enabled;
        }
        static void SetReplacedFunction(ReplacedFunction replaced_function) noexcept
        {
            m_replaced_function = replaced_function;
        }
        static auto GetReplacedFunction() noexcept
            -> ReplacedFunction
        {
            return m_replaced_function;
        }
        static void SetState(const State& state) noexcept
        {
            m_enabled = state.m_is_enabled;
            if (state.m_is_enabled)
            {
                m_replaced_function = state.m_replaced_function;
            }
            else
            {
                m_replaced_function = nullptr;
            }
        }
        static auto GetOriginalFunction() noexcept
            -> Function
        {
            return m_old_function_pointer;
        }
    };

    template <class T>
    static inline void FunctionReplacer(void** p_found_function_pointer, void* p_args) noexcept
    {
        //Save the original function pointer
        decltype(T::BaseSettings::m_p_iat_old_function_pointer) p_iat_function = &*p_found_function_pointer;
        T::BaseSettings::m_p_iat_old_function_pointer = p_iat_function;
        //Save the address of the original function pointer
        decltype(T::BaseSettings::m_old_function_pointer) iat_function =
            reinterpret_cast<typename T::Function>(*p_found_function_pointer);
        T::BaseSettings::m_old_function_pointer = iat_function;
        //Replace functions in IAT
        EnableWriteMemoryGuard enable_write{p_found_function_pointer};
        auto p_custom_function = T::GetFunction();
        //Circumvent msvc extension
        ::memcpy(p_found_function_pointer, &p_custom_function, sizeof(p_custom_function));
    }

public:
    constexpr static int CUSTOM_SUCCESS = 0x7777;

    class A : public CommonSettings<A, decltype(&::DrawTextA)>
    {
    private:
        static int WINAPI
        CustomDrawTextA(HDC hdc, LPCSTR lpchText, int cchText, LPRECT lprc, UINT format);

    public:
        using BaseSettings = CommonSettings<A, decltype(&::DrawTextA)>;
        using Function = decltype(&CustomDrawTextA);
        using ReplacedFunction = typename BaseSettings::ReplacedFunction;

        static auto GetFunction() noexcept
            -> Function;
        static auto GetReplaceOperation()
        {
            return [](void** p_found_function_pointer, void* p_args) noexcept
            {
                FunctionReplacer<A>(p_found_function_pointer, p_args);
            };
        }
    };

    class W : public CommonSettings<W, decltype(&::DrawTextW)>
    {
    private:
        static int WINAPI
        CustomDrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format);

    public:
        using BaseSettings = CommonSettings<W, decltype(&::DrawTextW)>;
        using Function = decltype(&CustomDrawTextW);
        using ReplacedFunction = typename BaseSettings::ReplacedFunction;

        static auto GetFunction() noexcept
            -> Function;
        static auto GetReplaceOperation()
        {
            return [](void** p_found_function_pointer, void* p_args) noexcept
            {
                FunctionReplacer<W>(p_found_function_pointer, p_args);
            };
        }
    };

    class ExA : public CommonSettings<ExA, decltype(&::DrawTextExA)>
    {
    private:
        static int WINAPI
        CustomDrawTextExA(HDC hdc, LPSTR lpchText, int cchText, LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp);

    public:
        using BaseSettings = CommonSettings<ExA, decltype(&::DrawTextExA)>;
        using Function = decltype(&CustomDrawTextExA);
        using ReplacedFunction = typename BaseSettings::ReplacedFunction;

        static auto GetFunction() noexcept
            -> Function;
        static auto GetReplaceOperation()
        {
            return [](void** p_found_function_pointer, void* p_args) noexcept
            {
                FunctionReplacer<ExA>(p_found_function_pointer, p_args);
            };
        }
    };

    class ExW : public CommonSettings<ExW, decltype(&::DrawTextExW)>
    {
    private:
        static int WINAPI
        CustomDrawTextExW(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp);

    public:
        using BaseSettings = CommonSettings<ExW, decltype(&::DrawTextExW)>;
        using Function = decltype(&CustomDrawTextExW);
        using ReplacedFunction = typename BaseSettings::ReplacedFunction;

        static auto GetFunction() noexcept
            -> Function;
        static auto GetReplaceOperation()
        {
            return [](void** p_found_function_pointer, void* p_args) noexcept
            {
                FunctionReplacer<ExW>(p_found_function_pointer, p_args);
            };
        }
    };

    User32DrawTextManager() = delete;
    ~User32DrawTextManager() = delete;
};

template <class Owner, class Function, class Checker>
bool ::User32DrawTextManager::CommonSettings<Owner, Function, Checker>::m_enabled = false;
template <class Owner, class Function, class Checker>
Function(::User32DrawTextManager::CommonSettings<Owner, Function, Checker>::m_old_function_pointer) = nullptr;
template <class Owner, class Function, class Checker>
void**(::User32DrawTextManager::CommonSettings<Owner, Function, Checker>::m_p_iat_old_function_pointer) = nullptr;
template <class Owner, class Function, class Checker>
to_std_function_t<Function>(::User32DrawTextManager::CommonSettings<Owner, Function, Checker>::m_replaced_function) = {};
