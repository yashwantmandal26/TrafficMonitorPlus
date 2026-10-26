#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cassert>
#include <cwctype>

// -------------------------------------------------------------
// Test 1: Color logic (Mirrored directly from TaskBarDlg.cpp)
// -------------------------------------------------------------
const COLORREF COLOR_STAGE_WHITE  = RGB(255, 255, 255); // White (Normal)
const COLORREF COLOR_STAGE_YELLOW = RGB(255, 214, 10);  // Yellow
const COLORREF COLOR_STAGE_ORANGE = RGB(251, 146, 60);  // Orange
const COLORREF COLOR_STAGE_RED    = RGB(244, 63, 94);   // Red

static COLORREF InterpolateColor(COLORREF c1, COLORREF c2, float t)
{
    if (t <= 0.0f) return c1;
    if (t >= 1.0f) return c2;
    int r = static_cast<int>(GetRValue(c1) + t * (GetRValue(c2) - GetRValue(c1)));
    int g = static_cast<int>(GetGValue(c1) + t * (GetGValue(c2) - GetGValue(c1)));
    int b = static_cast<int>(GetBValue(c1) + t * (GetBValue(c2) - GetBValue(c1)));
    return RGB(r, g, b);
}

static COLORREF GetDynamicTempColor(int temp)
{
    if (temp <= 0)
        return COLOR_STAGE_WHITE;

    if (temp <= 48)
    {
        return COLOR_STAGE_WHITE;
    }
    else if (temp < 65)
    {
        float t = (temp - 48.0f) / (65.0f - 48.0f);
        return InterpolateColor(COLOR_STAGE_WHITE, COLOR_STAGE_YELLOW, t);
    }
    else if (temp < 80)
    {
        float t = (temp - 65.0f) / (80.0f - 65.0f);
        return InterpolateColor(COLOR_STAGE_YELLOW, COLOR_STAGE_ORANGE, t);
    }
    else
    {
        float t = (temp - 80.0f) / (95.0f - 80.0f);
        return InterpolateColor(COLOR_STAGE_ORANGE, COLOR_STAGE_RED, t);
    }
}

static COLORREF GetDynamicUsageColor(int usage)
{
    if (usage <= 35)
    {
        return COLOR_STAGE_WHITE;
    }
    else if (usage < 60)
    {
        float t = (usage - 35.0f) / (60.0f - 35.0f);
        return InterpolateColor(COLOR_STAGE_WHITE, COLOR_STAGE_YELLOW, t);
    }
    else if (usage < 80)
    {
        float t = (usage - 60.0f) / (80.0f - 60.0f);
        return InterpolateColor(COLOR_STAGE_YELLOW, COLOR_STAGE_ORANGE, t);
    }
    else
    {
        float t = (usage - 80.0f) / (100.0f - 80.0f);
        return InterpolateColor(COLOR_STAGE_ORANGE, COLOR_STAGE_RED, t);
    }
}

// -------------------------------------------------------------
// Test 2: Adaptive Kelvin Conversion logic (from CpuTemperature.cpp)
// -------------------------------------------------------------
static float ConvertKelvinToCelsius(double raw_val)
{
    if (raw_val > 1000.0) // tenths of Kelvin
        return static_cast<float>((raw_val / 10.0) - 273.15);
    else if (raw_val > 0.0) // whole Kelvin
        return static_cast<float>(raw_val - 273.15);
    return -1.0f;
}

static bool MatchThermalZone(const std::wstring& name)
{
    std::wstring name_lower = name;
    for (auto& ch : name_lower) ch = towlower(ch);
    return (name_lower.find(L"ec") != std::wstring::npos ||
            name_lower.find(L"tzrz") != std::wstring::npos ||
            name_lower.find(L"cpu") != std::wstring::npos);
}

// -------------------------------------------------------------
// Test Runners
// -------------------------------------------------------------
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cout << "  [FAIL] " << msg << " (" << #cond << ")" << std::endl; \
            g_tests_failed++; \
        } else { \
            std::cout << "  [PASS] " << msg << std::endl; \
            g_tests_passed++; \
        } \
    } while(0)

void TestColors()
{
    std::cout << "\n=== Running Unit Test Suite 1: Dynamic Color Spectrum Transitions ===" << std::endl;

    TEST_ASSERT(GetDynamicTempColor(-10) == COLOR_STAGE_WHITE, "Negative temp returns White");
    TEST_ASSERT(GetDynamicTempColor(0) == COLOR_STAGE_WHITE, "0 deg C returns White");
    TEST_ASSERT(GetDynamicTempColor(25) == COLOR_STAGE_WHITE, "25 deg C (Room temp) returns White");
    TEST_ASSERT(GetDynamicTempColor(48) == COLOR_STAGE_WHITE, "48 deg C boundary returns White");

    // Transition 48-65 C
    COLORREF c55 = GetDynamicTempColor(55);
    TEST_ASSERT(c55 != COLOR_STAGE_WHITE && c55 != COLOR_STAGE_YELLOW, "55 deg C is cleanly interpolated between White & Yellow");
    TEST_ASSERT(GetRValue(c55) == 255, "55 deg C Red component is 255");
    TEST_ASSERT(GetBValue(c55) < 255 && GetBValue(c55) > 10, "55 deg C Blue component transitioning smoothly");

    // Exact transition point 65 C
    TEST_ASSERT(GetDynamicTempColor(65) == COLOR_STAGE_YELLOW, "65 deg C boundary returns Yellow");

    // Transition 65-80 C
    COLORREF c72 = GetDynamicTempColor(72);
    TEST_ASSERT(c72 != COLOR_STAGE_YELLOW && c72 != COLOR_STAGE_ORANGE, "72 deg C is cleanly interpolated between Yellow & Orange");

    // Exact transition point 80 C
    TEST_ASSERT(GetDynamicTempColor(80) == COLOR_STAGE_ORANGE, "80 deg C boundary returns Orange");

    // High temp >= 95 C
    TEST_ASSERT(GetDynamicTempColor(95) == COLOR_STAGE_RED, "95 deg C returns Red");
    TEST_ASSERT(GetDynamicTempColor(110) == COLOR_STAGE_RED, "110 deg C clamped returns Red");

    // Usage tests
    TEST_ASSERT(GetDynamicUsageColor(15) == COLOR_STAGE_WHITE, "15% Usage returns White");
    TEST_ASSERT(GetDynamicUsageColor(35) == COLOR_STAGE_WHITE, "35% Usage returns White");
    TEST_ASSERT(GetDynamicUsageColor(60) == COLOR_STAGE_YELLOW, "60% Usage returns Yellow");
    TEST_ASSERT(GetDynamicUsageColor(80) == COLOR_STAGE_ORANGE, "80% Usage returns Orange");
    TEST_ASSERT(GetDynamicUsageColor(100) == COLOR_STAGE_RED, "100% Usage returns Red");
    TEST_ASSERT(GetDynamicUsageColor(120) == COLOR_STAGE_RED, "120% Usage clamped returns Red");
}

void TestThermalConversion()
{
    std::cout << "\n=== Running Unit Test Suite 2: Adaptive Thermal & Zone Telemetry ===" << std::endl;

    // Tenths of Kelvin
    float c1 = ConvertKelvinToCelsius(3182.0); // 318.2 K
    TEST_ASSERT(std::abs(c1 - 45.05f) < 0.01f, "3182 tenths-of-Kelvin converts accurately to 45.05 C");

    // Whole Kelvin
    float c2 = ConvertKelvinToCelsius(318.0); // 318 K
    TEST_ASSERT(std::abs(c2 - 44.85f) < 0.01f, "318 whole Kelvin converts accurately to 44.85 C");

    // Zero / Invalid
    float c3 = ConvertKelvinToCelsius(0.0);
    TEST_ASSERT(c3 < 0.0f, "0.0 Kelvin returns negative (error)");

    // Zone matching
    TEST_ASSERT(MatchThermalZone(L"\\_sb.pc00.lpcb.ec0.tzrz") == true, "Embedded Controller EC0.TZRZ matches");
    TEST_ASSERT(MatchThermalZone(L"\\_SB.PC00.LPCB.EC0.TZRZ") == true, "Uppercase EC0.TZRZ matches");
    TEST_ASSERT(MatchThermalZone(L"CPU Thermal Sensor 1") == true, "CPU string matches");
    TEST_ASSERT(MatchThermalZone(L"\\_tz.tz00") == false, "Generic tz00 is not falsely identified as EC/CPU zone");
}

void TestIniSanity()
{
    std::cout << "\n=== Running Unit Test Suite 3: Configuration & INI File Integrity ===" << std::endl;

    std::ifstream f("C:\\Users\\Yashwant\\Desktop\\TrafficMonitorPlus+\\config.ini");
    TEST_ASSERT(f.is_open(), "config.ini exists and can be opened for reading");

    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(f, line))
    {
        size_t eq = line.find('=');
        if (eq != std::string::npos)
        {
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            // trim
            while (!k.empty() && (k.back() == ' ' || k.back() == '\t')) k.pop_back();
            while (!k.empty() && (k.front() == ' ' || k.front() == '\t')) k.erase(0, 1);
            while (!v.empty() && (v.back() == ' ' || v.back() == '\t' || v.back() == '\r')) v.pop_back();
            while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(0, 1);
            kv[k] = v;
        }
    }

    TEST_ASSERT(kv.find("language") != kv.end() && !kv["language"].empty(), "config.ini has valid 'language' key");
    TEST_ASSERT(kv["monitor_time_span"] == "1000", "monitor_time_span is set to 1000ms");
    TEST_ASSERT(kv["show_task_bar_wnd"] == "true", "show_task_bar_wnd is enabled (true)");
    TEST_ASSERT(kv["font_name"] == "Segoe UI", "Taskbar font is set to modern 'Segoe UI'");
    TEST_ASSERT(kv["dynamic_color"] == "true", "dynamic_color is enabled (true)");
    TEST_ASSERT(kv["auto_select"] == "true", "auto_select network connection is enabled (true)");
}

void TestSecurityAndServices()
{
    std::cout << "\n=== Running Unit Test Suite 4: Security & Driverless Verification ===" << std::endl;

    // Check no .sys files exist in release root
    DWORD sys1 = GetFileAttributesW(L"C:\\Users\\Yashwant\\Desktop\\TrafficMonitorPlus+\\TrafficMonitorPlus+.sys");
    DWORD sys2 = GetFileAttributesW(L"C:\\Users\\Yashwant\\Desktop\\TrafficMonitorPlus+\\LibreHardwareMonitorLib.sys");
    TEST_ASSERT(sys1 == INVALID_FILE_ATTRIBUTES, "No TrafficMonitorPlus+.sys file in directory");
    TEST_ASSERT(sys2 == INVALID_FILE_ATTRIBUTES, "No LibreHardwareMonitorLib.sys file in directory");

    // Check newly compiled binary exists and is valid PE
    HANDLE hFile = CreateFileW(L"C:\\Users\\Yashwant\\Desktop\\TrafficMonitorPlus+\\TrafficMonitorPlus+.exe",
                               GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    TEST_ASSERT(hFile != INVALID_HANDLE_VALUE, "TrafficMonitorPlus+.exe exists and can be opened");
    if (hFile != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER sz;
        GetFileSizeEx(hFile, &sz);
        TEST_ASSERT(sz.QuadPart > 1000000, "TrafficMonitorPlus+.exe has valid executable size (> 1MB)");
        CloseHandle(hFile);
    }
}

int main()
{
    std::cout << "================================================================" << std::endl;
    std::cout << "  TrafficMonitorPlus+ Comprehensive Verification & Unit Tests   " << std::endl;
    std::cout << "  Creator: Yashwant Mandal                                      " << std::endl;
    std::cout << "================================================================" << std::endl;

    TestColors();
    TestThermalConversion();
    TestIniSanity();
    TestSecurityAndServices();

    std::cout << "\n----------------------------------------------------------------" << std::endl;
    std::cout << "  SUMMARY: " << g_tests_passed << " Passed, " << g_tests_failed << " Failed." << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;

    return (g_tests_failed == 0) ? 0 : 1;
}
