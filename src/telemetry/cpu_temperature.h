#pragma once
#include <map>
#include <string>
#include <algorithm>
#include <cmath>
#include <cwctype>
namespace tmp {
struct CpuTemperatureSelection { double value=-1;std::wstring source=L"Unavailable"; };
inline CpuTemperatureSelection SelectCpuTemperature(const std::map<std::wstring,float>& sensors){
    CpuTemperatureSelection selected;int best=100;
    for(const auto& [name,value]:sensors){
        if(!std::isfinite(value)||value<=0||value>150)continue;
        auto lower=name;std::transform(lower.begin(),lower.end(),lower.begin(),[](wchar_t c){return std::towlower(c);});
        if(lower.find(L"distance")!=std::wstring::npos||lower.find(L"tjmax")!=std::wstring::npos)continue;
        int rank=100;
        if(lower.find(L"package")!=std::wstring::npos)rank=0;
        else if(lower.find(L"tdie")!=std::wstring::npos)rank=1;
        else if(lower.find(L"die")!=std::wstring::npos&&lower.find(L"average")!=std::wstring::npos)rank=2;
        else if(lower==L"core average")rank=3;
        else if(lower==L"core max")rank=4;
        else if(lower.find(L"core #")!=std::wstring::npos)rank=5;
        if(rank<best||(rank==best&&rank<100&&value>selected.value)){
            best=rank;selected={value,name};
        }
    }
    return selected;
}
}
