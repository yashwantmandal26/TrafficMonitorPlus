// TrafficMonitorPlus - Precompiled Header
#pragma once

// Target Windows 10 1809+ (build 17763)
#define WINVER          0x0A00
#define _WIN32_WINNT    0x0A00
#define NTDDI_VERSION   NTDDI_WIN10_RS5

// Lean Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Networking headers MUST be before windows.h
#include <winsock2.h>
#include <ws2tcpip.h>

// Core Windows headers
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <shlwapi.h>

// IP Helper & Network API
#include <iphlpapi.h>
#include <netioapi.h>
#include <icmpapi.h>

// Direct2D / DirectWrite
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wincodec.h>

// System info
#include <pdh.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <powerbase.h>

// NTSTATUS definition
typedef LONG NTSTATUS;
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

// COM
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// C++ Standard Library
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <memory>
#include <functional>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <optional>
#include <variant>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <format>

namespace fs = std::filesystem;
