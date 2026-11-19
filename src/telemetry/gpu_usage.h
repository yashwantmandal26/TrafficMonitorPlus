#pragma once
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace tmp {
// Sum process contexts belonging to the same physical engine, then select
// the busiest engine. Distinct GPUs/engines must not be added together.
inline double AggregateGpuEngines(const std::vector<std::pair<std::wstring, double>>& samples,
    std::wstring* selected_engine = nullptr, const std::wstring& preferred_engine = {}) {
    std::map<std::wstring, double> engines;
    std::map<std::wstring,double> unique_samples;
    for (const auto& [name, value] : samples) {
        const auto luid = name.find(L"luid_");
        const auto engine = name.find(L"_eng_", luid);
        const auto type = name.find(L"_engtype_", engine);
        if (luid == std::wstring::npos || engine == std::wstring::npos ||
            type == std::wstring::npos || !std::isfinite(value) || value < 0) continue;
        auto [it,added]=unique_samples.emplace(name,value);
        if(!added)it->second=(std::max)(it->second,value);
    }
    for(const auto& [name,value]:unique_samples){
        const auto luid=name.find(L"luid_");
        const auto type=name.find(L"_engtype_",name.find(L"_eng_",luid));
        engines[name.substr(luid,type-luid)]+=value;
    }
    if (selected_engine) selected_engine->clear();
    if (engines.empty()) return -1.0;
    double busiest = -1;
    for (const auto& [engine, value] : engines) {
        if (value > busiest || (value == busiest && engine == preferred_engine)) {
            busiest = value;
            if (selected_engine) *selected_engine = engine;
        }
    }
    return std::clamp(busiest, 0.0, 100.0);
}
}
