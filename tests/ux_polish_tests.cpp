#include "pch.h"
#include "types.h"
#include "config/settings.h"
#include "config/updater.h"
#include "rendering/d2d_renderer.h"
#include "rendering/text_renderer.h"
#include "rendering/theme.h"
#include "windows/settings_window.h"
#include "windows/flyout_window.h"
#include "windows/tray_icon.h"
#include "../res/version.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>

int main() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int failures = 0;
    auto check = [&](bool ok, const char* desc) {
        if (!ok) {
            std::cerr << "FAIL: " << desc << "\n";
            ++failures;
        } else {
            std::cout << "PASS: " << desc << "\n";
        }
    };

    std::cout << "=== Phase 3 Regressions: UX, Visual Polish & Packaging ===\n";

    // ─── Test 1: Finding 6 - Version Unification & Updater Tests ─────────────
    std::cout << "\n[Test 1] Version Unification & Updater Consistency\n";
    {
        check(tmp::VERSION_MAJOR == VER_MAJOR, "VERSION_MAJOR matches res/version.h");
        check(tmp::VERSION_MINOR == VER_MINOR, "VERSION_MINOR matches res/version.h");
        check(tmp::VERSION_PATCH == VER_PATCH, "VERSION_PATCH matches res/version.h");
        check(tmp::VERSION_MAJOR == 2 && tmp::VERSION_MINOR == 0 && tmp::VERSION_PATCH == 0, "Current release version is 2.0.0");

        // Verify manifest contains matching version
        std::ifstream manifest("res/app.manifest");
        if (!manifest.is_open()) {
            manifest.open("../res/app.manifest");
        }
        std::string line;
        bool found_manifest_ver = false;
        while (std::getline(manifest, line)) {
            if (line.find("version=\"2.0.0.0\"") != std::string::npos) {
                found_manifest_ver = true;
                break;
            }
        }
        check(found_manifest_ver, "app.manifest specifies version 2.0.0.0");
        check(!tmp::Updater::IsNewerVersion(L"v1.86",2,0,0), "Public legacy 1.86 does not supersede 2.0.0");

        // Verify Updater version comparison logic
        check(!tmp::Updater::IsNewerVersion(L"v1.2.25", 1, 2, 25), "Current installed version does not notify update");
        check(!tmp::Updater::IsNewerVersion(L"1.2.25", 1, 2, 25), "Current version without 'v' does not notify update");
        check(!tmp::Updater::IsNewerVersion(L"v1.2.14", 1, 2, 25), "Older version 1.2.14 does not notify update");
        check(!tmp::Updater::IsNewerVersion(L"v1.2.24", 1, 2, 25), "Older version 1.2.24 does not notify update");
        check(!tmp::Updater::IsNewerVersion(L"v1.1.99", 1, 2, 25), "Older minor version 1.1.99 does not notify update");
        check(tmp::Updater::IsNewerVersion(L"v1.2.26", 1, 2, 25), "Newer patch 1.2.26 notifies update");
        check(tmp::Updater::IsNewerVersion(L"v1.3.0", 1, 2, 25), "Newer minor 1.3.0 notifies update");
        check(tmp::Updater::IsNewerVersion(L"v2.0.0", 1, 2, 25), "Newer major 2.0.0 notifies update");
    }

    // ─── Test 2: Finding 8 - Auto-compact layout extent and gap inclusion ────
    std::cout << "\n[Test 2] Auto-Compact Layout Extent & Gap Calculation\n";
    {
        HWND dummy_hwnd = CreateWindowExW(0, L"STATIC", L"LayoutTest", WS_POPUP, 0, 0, 800, 200, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        tmp::D2DRenderer renderer;
        if (renderer.Init(dummy_hwnd)) {
            tmp::Theme theme;
            theme.Init(true);
            tmp::TextRenderer text(&renderer, &theme);
            renderer.SetFont(L"Segoe UI", 13.0f, true);

            tmp::TelemetryData sample{};
            sample.cpu_usage_percent = 45.0;
            sample.gpu_usage_percent = 30.0;

            // Configure paired CPU and GPU items with custom label_gap and value_width
            std::vector<std::pair<tmp::DisplayItem, tmp::DisplayItemConfig>> configs;
            tmp::DisplayItemConfig cpu_cfg{ true, 0, L"CPU:" };
            cpu_cfg.label_gap = 50;
            cpu_cfg.value_width = 80;
            cpu_cfg.offset_x = 0;

            tmp::DisplayItemConfig gpu_cfg{ true, 1, L"GPU:" };
            gpu_cfg.label_gap = 50;
            gpu_cfg.value_width = 80;
            gpu_cfg.offset_x = 0;

            configs.push_back({ tmp::DisplayItem::CpuUsage, cpu_cfg });
            configs.push_back({ tmp::DisplayItem::GpuUsage, gpu_cfg });

            auto prepared = text.PrepareItems(sample, configs, false);
            check(prepared.size() == 2, "Prepared 2 items for paired group");

            renderer.BeginDraw();
            // In 2-row auto-compact, available_height is sufficient for 2 rows (e.g. 40 DIPs, row_height = 18 DIPs)
            float calculated_width = text.RenderAutoCompact(prepared, 0, 0, 8.0f, 40.0f, 18.0f);
            renderer.EndDraw();

            // Expected width: label width + label_gap (50) + value_width (80)
            // Minimum expected width must be >= 50 + 80 = 130 DIPs
            check(calculated_width >= 130.0f, "RenderAutoCompact includes label_gap (50) and value_width (80), width >= 130");
            std::cout << "  Calculated auto-compact width: " << calculated_width << " DIPs\n";
        }
        if (dummy_hwnd) DestroyWindow(dummy_hwnd);
    }

    // ─── Test 3: Finding 16 - Light Theme Contrast & Thermal Palette ─────────
    std::cout << "\n[Test 3] Light Theme Contrast & Thermal Palette\n";
    {
        tmp::Theme light_theme;
        light_theme.Init(false); // Light mode

        auto text_col = light_theme.GetTextColor();
        auto label_col = light_theme.GetLabelColor();
        auto thermal_zero = light_theme.GetThermalColor(0.0f);

        // Relative luminance helper (sRGB)
        auto lum = [](D2D1_COLOR_F c) {
            return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
        };

        // On a white background (lum = 1.0), dark text has high contrast: (1.0 + 0.05) / (lum + 0.05)
        float text_lum = lum(text_col);
        float label_lum = lum(label_col);
        float thermal_zero_lum = lum(thermal_zero);

        float text_contrast = (1.05f) / (text_lum + 0.05f);
        float label_contrast = (1.05f) / (label_lum + 0.05f);
        float thermal_contrast = (1.05f) / (thermal_zero_lum + 0.05f);

        // WCAG AA for normal text requires >= 4.5:1 contrast
        check(text_contrast >= 4.5f, "Light theme text color meets WCAG AA contrast (>= 4.5:1)");
        check(label_contrast >= 4.0f, "Light theme label color has strong contrast against white background");
        check(thermal_contrast >= 4.5f, "Light theme thermal base stop (0.0 heat) has strong contrast (not white-on-white)");
        check(thermal_zero.r < 0.5f && thermal_zero.g < 0.5f && thermal_zero.b < 0.5f, "Thermal 0% color in light mode is dark text");

        // Verify dark theme
        tmp::Theme dark_theme;
        dark_theme.Init(true);
        auto dark_thermal_zero = dark_theme.GetThermalColor(0.0f);
        check(dark_thermal_zero.r > 0.9f && dark_thermal_zero.g > 0.9f && dark_thermal_zero.b > 0.9f, "Dark theme thermal 0% color is crisp white");
    }

    // ─── Test 4: Finding 15 - Settings Draft Persistence & Lifecycle ─────────
    std::cout << "\n[Test 4] Settings Draft Persistence Across Deactivation & Lifecycle\n";
    {
        tmp::SettingsWindow window;
        if (window.Init(GetModuleHandleW(nullptr))) {
            tmp::Settings original = tmp::Settings::GetDefaults();
            tmp::Settings live = original;
            bool saved = false;
            bool preview_active = false;

            window.live_settings = [&]() { return live; };
            window.on_settings_changed = [&](const tmp::Settings& s) {
                live = s;
                saved = true;
                return true;
            };
            window.on_layout_preview = [&](const tmp::Settings&) {
                preview_active = true;
            };
            window.on_preview_finished = [&]() {
                preview_active = false;
            };

            window.Show(original);
            check(!window.HasUnsavedChanges(), "Initially no unsaved changes");

            // Simulate user dragging slider (taskbar_offset_x = -42)
            HWND slider = GetDlgItem(window.GetHwnd(), 1100);
            if (slider) {
                SendMessageW(slider, TBM_SETPOS, TRUE, -42);
                SendMessageW(window.GetHwnd(), WM_HSCROLL, TB_THUMBPOSITION, reinterpret_cast<LPARAM>(slider));
            }
            check(window.HasUnsavedChanges(), "Draft modified: HasUnsavedChanges() is true");
            check(preview_active, "Live preview activated on slider drag");

            // Simulate window deactivation (user Alt-Tabs to another application)
            SendMessageW(window.GetHwnd(), WM_ACTIVATE, WA_INACTIVE, 0);
            check(window.IsVisible(), "Settings remain visible on deactivation");
            check(preview_active, "Live preview persists while comparing another application");
            check(window.HasUnsavedChanges(), "Unsaved draft retained while switching applications");
            SendMessageW(window.GetHwnd(), WM_CLOSE, 0, 0);
            check(!window.IsVisible(), "Close hides Settings");
            check(!preview_active, "Close reverts the live preview");

            // Reopen settings window (e.g. clicked tray icon or hotkey)
            window.Show(live);
            UpdateWindow(window.GetHwnd());
            check(window.IsVisible(), "Window reopened");
            check(window.HasUnsavedChanges(), "Unsaved draft survived reopening after deactivation");
            check(preview_active, "Live preview re-engaged on reopening dirty draft");

            // Test Discard action
            window.Discard();
            UpdateWindow(window.GetHwnd());
            check(!window.HasUnsavedChanges(), "Discard() resets unsaved draft to original settings");
            check(!preview_active, "Discard() reverted live preview");

            // Modify again and test Save
            if (slider) {
                SendMessageW(slider, TBM_SETPOS, TRUE, 19);
                SendMessageW(window.GetHwnd(), WM_HSCROLL, TB_THUMBPOSITION, reinterpret_cast<LPARAM>(slider));
            }
            UpdateWindow(window.GetHwnd());
            check(window.HasUnsavedChanges(), "Draft modified again");

            // Click Apply & save
            RECT client{};
            GetClientRect(window.GetHwnd(), &client);
            float scale = (std::min)(client.right / 820.0f, client.bottom / 600.0f);
            // Save button is at (735, 565)
            SendMessageW(window.GetHwnd(), WM_LBUTTONUP, 0, MAKELPARAM(static_cast<int>(735 * scale), static_cast<int>(565 * scale)));
            UpdateWindow(window.GetHwnd());
            check(saved, "Save() called on_settings_changed");
            check(live.taskbar_offset_x == 19, "Saved settings updated with new value 19");
            check(!window.HasUnsavedChanges(), "HasUnsavedChanges() cleared after successful save");
            check(!preview_active, "Live preview finished after save");

            window.Hide();
        }
    }

    // ─── Test 5: Finding 19 - Flyout Pure Rendering & Decoupled I/O ──────────
    std::cout << "\n[Test 5] Flyout Pure Rendering & Zero Paint-Path I/O\n";
    {
        tmp::FlyoutWindow flyout;
        if (flyout.Init(GetModuleHandleW(nullptr))) {
            std::atomic<int> slow_storage_calls{ 0 };
            std::atomic<int> slow_ip_calls{ 0 };

            // Inject a mock storage provider that simulates a 200ms latency spinning drive
            flyout.storage_provider = [&]() {
                ++slow_storage_calls;
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                std::vector<tmp::DiskInfo> mock_drives;
                tmp::DiskInfo c{};
                c.letter = L'C';
                c.label = L"FastNVMe";
                c.total = 1000000000000ULL;
                c.free = 600000000000ULL;
                c.used = 400000000000ULL;
                c.used_pct = 40.0;
                mock_drives.push_back(c);
                return mock_drives;
            };

            flyout.ip_provider = [&]() {
                ++slow_ip_calls;
                return L"192.168.1.100";
            };

            // Update with initial snapshot
            tmp::TelemetryData telem{};
            telem.ram_usage_percent = 50.0;
            telem.ram_total_bytes = 16ULL * 1024 * 1024 * 1024;
            telem.ram_used_bytes = 8ULL * 1024 * 1024 * 1024;

            // Populate cached snapshot first so it is not empty
            flyout.RefreshStorageSnapshot();
            check(slow_storage_calls.load() == 1, "Initial sample refreshed storage");

            POINT pt{ 400, 400 };
            flyout.Show(pt, true);
            UpdateWindow(flyout.GetHwnd());

            const int count_before_paint = slow_storage_calls.load();

            // Now, simulate rapid painting (e.g. 10 consecutive repaints from mouse moves / high FPS)
            auto paint_start = std::chrono::steady_clock::now();
            for (int i = 0; i < 10; ++i) {
                SendMessageW(flyout.GetHwnd(), WM_PAINT, 0, 0);
            }
            auto paint_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - paint_start).count();

            // In the buggy version, 10 paints would call GetLogicalDrives/GetDiskFreeSpaceExW synchronously
            // which with slow_storage would take 2000ms+. With pure cached rendering, 10 paints take < 50ms!
            check(slow_storage_calls.load() == count_before_paint, "Paint path did ZERO storage provider calls (storage decoupled from paint)");
            check(paint_duration < 150, "10 paint cycles completed in under 150ms total (pure visual rendering)");
            std::cout << "  10 paint cycles completed in: " << paint_duration << " ms\n";

            flyout.Hide();
        }
    }

    CoUninitialize();
    std::cout << "\n=======================================================\n";
    if (failures == 0) {
        std::cout << "ALL PHASE 3 REGRESSIONS PASSED SUCCESSFULLY!\n";
        return 0;
    } else {
        std::cerr << failures << " TEST FAILURES OCCURRED.\n";
        return 1;
    }
}
