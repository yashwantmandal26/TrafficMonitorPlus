#include "config/settings.h"
#include "../lib/json.hpp"
#include <iostream>

int main() {
    const auto dir = fs::temp_directory_path() / (L"TrafficMonitorPlus-tests-" + std::to_wstring(GetCurrentProcessId()));
    const auto path = dir / L"settings.json";
    int failures = 0;
    auto check = [&](bool ok, const char* name) { if (!ok) { std::cerr << name << '\n'; ++failures; } };
    fs::create_directories(dir);
    auto settings = tmp::Settings::GetDefaults();
    auto live = settings, draft = settings;
    live.items[tmp::DisplayItem::UploadSpeed].enabled = false;
    live.widget_x = 240;
    draft.font_size = 18;
    draft.show_unit_signs = false;
    draft.stat_value_gap = 4;
    draft.items[tmp::DisplayItem::GpuUsage].value_width = 112;
    draft.items[tmp::DisplayItem::GpuUsage].label_gap = 6;
    draft.items[tmp::DisplayItem::GpuUsage].offset_x = 10;
    draft.items[tmp::DisplayItem::GpuUsage].show_unit = false;
    const auto merged = tmp::Settings::MergeDraft(live, settings, draft);
    check(!merged.items.at(tmp::DisplayItem::UploadSpeed).enabled && merged.widget_x == 240 && merged.font_size == 18 &&
        !merged.show_unit_signs && merged.stat_value_gap == 4 &&
        merged.items.at(tmp::DisplayItem::GpuUsage).value_width == 112 &&
        merged.items.at(tmp::DisplayItem::GpuUsage).label_gap == 6 &&
        merged.items.at(tmp::DisplayItem::GpuUsage).offset_x == 10 &&
        !merged.items.at(tmp::DisplayItem::GpuUsage).show_unit, "draft merge preserves live tray and position changes");
    auto original_label = settings;
    original_label.items[tmp::DisplayItem::GpuUsage].custom_label = L"Old label";
    check(tmp::Settings::MergeDraft(original_label, original_label, settings).items.at(tmp::DisplayItem::GpuUsage).custom_label.empty(), "custom label can be reset");
    settings.font_family = L"Segoe UI";
    settings.widget_x = -500;
    settings.auto_start = false;
    settings.show_unit_signs = false;
    settings.stat_value_gap = 3;
    settings.items[tmp::DisplayItem::GpuUsage].font_size = 17;
    settings.items[tmp::DisplayItem::GpuUsage].value_width = 100;
    settings.items[tmp::DisplayItem::GpuUsage].label_gap = 5;
    settings.items[tmp::DisplayItem::GpuUsage].offset_x = 8;
    settings.items[tmp::DisplayItem::GpuUsage].show_unit = false;
    settings.items[tmp::DisplayItem::GpuUsage].order = -1;
    settings.taskbar_offset_x = -20;
    check(settings.SaveToFile(path), "save");
    auto loaded = tmp::Settings::LoadFromFile(path);
    check(loaded.font_family == settings.font_family && loaded.widget_x == -500 && !loaded.auto_start &&
        !loaded.show_unit_signs && loaded.stat_value_gap == 3, "round trip");
    check(loaded.items[tmp::DisplayItem::GpuUsage].font_size == 17 && loaded.items[tmp::DisplayItem::GpuUsage].value_width == 100 &&
        loaded.items[tmp::DisplayItem::GpuUsage].label_gap == 5 && loaded.items[tmp::DisplayItem::GpuUsage].offset_x == 8 &&
        !loaded.items[tmp::DisplayItem::GpuUsage].show_unit && loaded.items[tmp::DisplayItem::CpuUsage].show_unit &&
        loaded.GetEnabledItemsSorted().front().first == tmp::DisplayItem::GpuUsage && loaded.taskbar_offset_x == -20, "custom layout roundtrip");
    { std::ofstream out(path); out << R"({"stealth_mode":true,"items":{"network_ping":{"enabled":true},"display_hz":{"enabled":true},"cpu_frequency":{"enabled":true}}})"; }
    loaded = tmp::Settings::LoadFromFile(path);
    check(loaded.items.size() == tmp::DISPLAY_ITEM_COUNT && loaded.items[tmp::DisplayItem::CpuFrequency].enabled, "removed features migrate by key");
    { std::ofstream out(path); out << R"({"font_size":999,"poll_interval_ms":-2,"widget_opacity":200})"; }
    loaded = tmp::Settings::LoadFromFile(path);
    check(loaded.font_size == 40 && loaded.poll_interval_ms == 500 && loaded.widget_opacity == 100, "range validation");
    { std::ofstream out(path); out << "{broken"; }
    loaded = tmp::Settings::LoadFromFile(path);
    check(loaded.font_size == tmp::Settings::GetDefaults().font_size, "corrupt file recovery");
    const auto defaults=tmp::Settings::LoadFromFile(dir / L"missing.json");
    check(defaults.SaveToFile(path), "serialize factory defaults");
    std::ifstream actualFile(path), expectedFile(fs::path(__FILE__).parent_path() / "default-settings.json");
    check(nlohmann::json::parse(actualFile,nullptr,false) == nlohmann::json::parse(expectedFile,nullptr,false),
        "every factory default matches the saved user configuration");
    actualFile.close(); expectedFile.close();
    check(defaults.poll_interval_ms == 1000 && defaults.taskbar_offset_x == 11 && defaults.taskbar_offset_y == 2 &&
        defaults.item_spacing == 5 && !defaults.font_bold && defaults.font_size == 13 &&
        defaults.layout == tmp::LayoutDirection::AutoCompact && defaults.GetEnabledItemsSorted().size() == 6 &&
        !defaults.items.at(tmp::DisplayItem::RamUsage).enabled, "fresh install restores the complete saved preset");
    fs::remove_all(dir);
    return failures ? 1 : 0;
}
