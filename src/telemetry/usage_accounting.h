#pragma once
#include "telemetry/usage_history.h"
#include <limits>

namespace tmp {
// Delayed Windows records must not replace bytes already observed on the adapter.
// Select one whole source: summing native and tracked counters would double-count.
inline bool PreferWindowsUsage(bool current, UsageTotal native, UsageTotal tracked) {
    return current && native.download >= tracked.download && native.upload >= tracked.upload;
}
inline std::wstring UsageIdentity(std::wstring adapter,std::wstring profile){
    for(auto& c:adapter)if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';
    // SSIDs/profile names can differ by case. Only adapter GUID casing is an alias.
    return adapter+L"\n"+profile;
}
struct UsageWindowAccumulator {
    UsageTotal total{};
    std::set<std::wstring> profiles;
    bool Add(const std::wstring& identity,UsageTotal bytes){
        if(profiles.contains(identity))return false;
        if(bytes.download>UINT64_MAX-total.download||bytes.upload>UINT64_MAX-total.upload)
            throw std::overflow_error("Network byte total overflow");
        profiles.insert(identity);total.download+=bytes.download;total.upload+=bytes.upload;return true;
    }
};
struct AttributedBytes {std::wstring id,name;UsageTotal bytes;};
inline std::vector<AttributedBytes> ValidateAttribution(const std::vector<AttributedBytes>& samples,UsageTotal total){
    std::map<std::wstring,AttributedBytes> unique;
    for(const auto& sample:samples){
        const auto& key=sample.id;
        auto [it,added]=unique.emplace(key,sample);
        if(!added&&(it->second.bytes.download!=sample.bytes.download||it->second.bytes.upload!=sample.bytes.upload))
            throw std::runtime_error("Conflicting duplicate Windows attribution");
    }
    std::vector<AttributedBytes> result;
    for(const auto& [key,sample]:unique){
        if(sample.bytes.download>total.download||sample.bytes.upload>total.upload)
            throw std::runtime_error("Windows attribution exceeds measured traffic");
        total.download-=sample.bytes.download;total.upload-=sample.bytes.upload;
        result.push_back(sample);
    }
    return result;
}
}
