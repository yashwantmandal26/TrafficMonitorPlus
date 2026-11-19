#include "telemetry/gpu_usage.h"
#include <iostream>
#include <limits>
int main() {
    int failed = 0;
    auto check = [&](double actual, double expected) {
        if (actual != expected) { std::cerr << actual << " != " << expected << '\n'; ++failed; }
    };
    check(tmp::AggregateGpuEngines({}), -1);
    check(tmp::AggregateGpuEngines({
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D",40},
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D",40}}),40);
    check(tmp::AggregateGpuEngines({{L"invalid", 20}}), -1);
    std::wstring selected;
    const std::wstring igpu = L"luid_0x0_0x1_phys_0_eng_0";
    const std::wstring dgpu = L"luid_0x0_0x2_phys_0_eng_0";
    check(tmp::AggregateGpuEngines({
        {L"pid_1_" + igpu + L"_engtype_3D", 5},
        {L"pid_2_" + dgpu + L"_engtype_3D", 80}}, &selected), 80);
    if (selected != dgpu) ++failed;
    check(tmp::AggregateGpuEngines({
        {L"pid_1_" + igpu + L"_engtype_3D", 90},
        {L"pid_2_" + dgpu + L"_engtype_3D", 0}}, &selected), 90);
    if (selected != igpu) ++failed;
    check(tmp::AggregateGpuEngines({
        {L"pid_1_" + igpu + L"_engtype_3D", 0},
        {L"pid_2_" + dgpu + L"_engtype_3D", 0}}, &selected, dgpu), 0);
    if (selected != dgpu) ++failed;
    check(tmp::AggregateGpuEngines({
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D", 20},
        {L"pid_2_luid_0x0_0x1_phys_0_eng_0_engtype_3D", 30},
        {L"pid_1_luid_0x0_0x1_phys_0_eng_1_engtype_Copy", 40}}), 50);
    check(tmp::AggregateGpuEngines({
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D", 60},
        {L"pid_1_luid_0x0_0x2_phys_0_eng_0_engtype_3D", 70}}), 70);
    check(tmp::AggregateGpuEngines({
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D", 120}}), 100);
    check(tmp::AggregateGpuEngines({
        {L"pid_1_luid_0x0_0x1_phys_0_eng_0_engtype_3D", std::numeric_limits<double>::quiet_NaN()}}), -1);
    return failed ? 1 : 0;
}
