// TrafficMonitorPlus - Network Monitor Implementation
#include "pch.h"
#include "telemetry/network_monitor.h"

#include <ws2tcpip.h>
#include <netioapi.h>

namespace tmp {

// ---------------------------------------------------------------------------
// Helper: check if an adapter type is a real physical / wifi adapter
// ---------------------------------------------------------------------------
static bool IsPhysicalAdapter(const MIB_IF_ROW2& row) {
    // Skip loopback
    if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK)
        return false;
    // Skip tunnel interfaces
    if (row.Type == IF_TYPE_TUNNEL)
        return false;
    // Accept Ethernet and Wi-Fi (IEEE 802.11)
    if (row.Type == IF_TYPE_ETHERNET_CSMACD ||
        row.Type == IF_TYPE_IEEE80211)
        return true;
    // Also accept gigabit ethernet / 802.3
    if (row.Type == 117 /* IF_TYPE_GIGABITETHERNET */)
        return true;
    return false;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
NetworkMonitor::NetworkMonitor(const std::wstring& adapter_filter)
    : m_adapter_filter(adapter_filter)
    , m_is_manual_selection(!adapter_filter.empty())
{
}

// ---------------------------------------------------------------------------
// SetAdapterFilter
// ---------------------------------------------------------------------------
void NetworkMonitor::SetAdapterFilter(const std::wstring& name) {
    m_adapter_filter = name;
    m_is_manual_selection = !name.empty();
    m_adapter_index = 0;
    m_adapter_luid = {};
    m_adapter_connected = false;
    m_initialized   = false;
    m_download_bps  = -1.0;
    m_upload_bps    = -1.0;
}

// ---------------------------------------------------------------------------
// MatchesFilter - match by description, alias, or GUID
// ---------------------------------------------------------------------------
bool NetworkMonitor::MatchesFilter(const MIB_IF_ROW2& row, const std::wstring& filter) const {
    if (filter.empty()) return false;

    auto to_lower = [](std::wstring s) {
        for (auto& c : s) {
            if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        }
        return s;
    };
    const std::wstring filter_lower = to_lower(filter);

    const std::wstring desc_lower = to_lower(row.Description);
    if (desc_lower.find(filter_lower) != std::wstring::npos) return true;

    const std::wstring alias_lower = to_lower(row.Alias);
    if (alias_lower.find(filter_lower) != std::wstring::npos) return true;

    wchar_t guid_buf[64]{};
    if (StringFromGUID2(row.InterfaceGuid, guid_buf, static_cast<int>(std::size(guid_buf))) > 0) {
        const std::wstring guid_lower = to_lower(guid_buf);
        if (guid_lower.find(filter_lower) != std::wstring::npos) return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// ResetBaseline - clean counter reset on connect or adapter change
// ---------------------------------------------------------------------------
void NetworkMonitor::ResetBaseline(const MIB_IF_ROW2& row, std::chrono::steady_clock::time_point now) {
    m_prev_in_bytes  = row.InOctets;
    m_prev_out_bytes = row.OutOctets;
    m_prev_time      = now;
    m_download_bps   = -1.0;
    m_upload_bps     = -1.0;
    m_initialized    = true;
}

// ---------------------------------------------------------------------------
// ComputeRates - calculate instantaneous rate and accumulate session bytes
// ---------------------------------------------------------------------------
void NetworkMonitor::ComputeRates(const MIB_IF_ROW2& row, std::chrono::steady_clock::time_point now) {
    const double elapsed_sec = std::chrono::duration<double>(now - m_prev_time).count();
    if (elapsed_sec > 0.001) {
        const uint64_t in_bytes  = row.InOctets;
        const uint64_t out_bytes = row.OutOctets;

        const uint64_t in_delta  = (in_bytes  >= m_prev_in_bytes)  ? (in_bytes  - m_prev_in_bytes)  : 0;
        const uint64_t out_delta = (out_bytes >= m_prev_out_bytes) ? (out_bytes - m_prev_out_bytes) : 0;

        m_download_bps = in_bytes>=m_prev_in_bytes?static_cast<double>(in_delta)/elapsed_sec:-1.0;
        m_upload_bps   = out_bytes>=m_prev_out_bytes?static_cast<double>(out_delta)/elapsed_sec:-1.0;

        m_total_download_bytes += in_delta;
        m_total_upload_bytes   += out_delta;

        m_prev_in_bytes  = in_bytes;
        m_prev_out_bytes = out_bytes;
        m_prev_time      = now;
    }
}

// ---------------------------------------------------------------------------
// MarkDisconnected - mark current adapter as unavailable
// ---------------------------------------------------------------------------
void NetworkMonitor::MarkDisconnected() {
    m_adapter_index = 0;
    m_adapter_luid = {};
    m_adapter_connected = false;
    m_download_bps = -1.0;
    m_upload_bps   = -1.0;
    m_adapter_name = m_adapter_filter.empty() ? L"No active network" : (m_adapter_filter + L" (Disconnected)");
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void NetworkMonitor::Init() {
    PMIB_IF_TABLE2 table = nullptr;
    DWORD result = GetIfTable2(&table);
    if (result != NO_ERROR || !table) {
        m_download_bps = m_upload_bps = -1.0;
        m_initialized = false;
        m_adapter_connected = false;
        return;
    }

    SelectAdapter(table);
    if (m_adapter_index == 0 || !m_adapter_connected) {
        m_download_bps = m_upload_bps = -1.0;
        m_initialized = false;
        FreeMibTable(table);
        return;
    }

    // Capture initial byte counters
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2& row = table->Table[i];
        if (row.InterfaceIndex == m_adapter_index) {
            ResetBaseline(row, std::chrono::steady_clock::now());
            m_download_bps = -1.0;
            m_upload_bps   = -1.0;
            break;
        }
    }

    FreeMibTable(table);
    UpdateLocalIP();
    m_initialized = true;
}

// ---------------------------------------------------------------------------
// SelectAdapter - pick the best adapter from the table
// ---------------------------------------------------------------------------
void NetworkMonitor::SelectAdapter(PMIB_IF_TABLE2 table) {
    m_adapter_index = 0;
    m_adapter_luid = {};
    m_adapter_name.clear();
    m_adapter_connected = false;

    if (!table || table->NumEntries == 0) return;

    if (m_is_manual_selection) {
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IF_ROW2& row = table->Table[i];
            if (row.OperStatus == IfOperStatusUp && MatchesFilter(row, m_adapter_filter)) {
                m_adapter_index = row.InterfaceIndex;
                m_adapter_luid  = row.InterfaceLuid;
                m_adapter_name  = row.Description;
                m_adapter_connected = true;
                return;
            }
        }
        MarkDisconnected();
        return;
    }

    // Auto mode: prefer interface used for external IPv4 destination
    SOCKADDR_INET destination{};
    destination.Ipv4.sin_family = AF_INET;
    InetPtonW(AF_INET, L"8.8.8.8", &destination.Ipv4.sin_addr);
    MIB_IPFORWARD_ROW2 route{};
    SOCKADDR_INET source{};
    if (GetBestRoute2(nullptr, 0, nullptr, &destination, 0, &route, &source) == NO_ERROR) {
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IF_ROW2& row = table->Table[i];
            if (row.InterfaceIndex == route.InterfaceIndex && row.OperStatus == IfOperStatusUp && IsPhysicalAdapter(row)) {
                m_adapter_index = row.InterfaceIndex;
                m_adapter_luid = row.InterfaceLuid;
                m_adapter_name = row.Description;
                m_adapter_connected = true;
                return;
            }
        }
    }

    // Fallback: first available physical interface that is UP
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2& row = table->Table[i];
        if (row.OperStatus == IfOperStatusUp && IsPhysicalAdapter(row)) {
            m_adapter_index = row.InterfaceIndex;
            m_adapter_luid  = row.InterfaceLuid;
            m_adapter_name  = row.Description;
            m_adapter_connected = true;
            return;
        }
    }

    MarkDisconnected();
}

// ---------------------------------------------------------------------------
// PollWithTable - injectable test seam with simulated table and timestamp
// ---------------------------------------------------------------------------
void NetworkMonitor::PollWithTable(PMIB_IF_TABLE2 table, std::chrono::steady_clock::time_point now) {
    if (!table || table->NumEntries == 0) {
        MarkDisconnected();
        return;
    }

    if (m_is_manual_selection) {
        // Manual mode: look strictly for preferred adapter by description, alias, or GUID
        const MIB_IF_ROW2* matching_row = nullptr;
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IF_ROW2& row = table->Table[i];
            if (MatchesFilter(row, m_adapter_filter)) {
                matching_row = &row;
                break;
            }
        }

        if (matching_row && matching_row->OperStatus == IfOperStatusUp) {
            if (!m_adapter_connected || m_adapter_index != matching_row->InterfaceIndex || m_adapter_luid.Value!=matching_row->InterfaceLuid.Value) {
                // First connect or reconnect after disconnect:
                // Re-establish baseline without computing delta to prevent artificial traffic spikes
                m_adapter_index = matching_row->InterfaceIndex;
                m_adapter_luid  = matching_row->InterfaceLuid;
                m_adapter_name  = matching_row->Description;
                m_adapter_connected = true;
                ResetBaseline(*matching_row, now);
            } else {
                ComputeRates(*matching_row, now);
            }
        } else {
            // Preferred adapter is disconnected or down:
            // Do NOT fall back to other adapters; keep manual choice and mark disconnected
            MarkDisconnected();
        }
    } else {
        // Auto mode: select best active route or physical adapter
        const NET_IFINDEX prev_index = m_adapter_index;
        const NET_LUID prev_luid = m_adapter_luid;

        SelectAdapter(table);

        if (m_adapter_connected && m_adapter_index != 0) {
            const MIB_IF_ROW2* current_row = nullptr;
            for (ULONG i = 0; i < table->NumEntries; ++i) {
                if (table->Table[i].InterfaceIndex == m_adapter_index) {
                    current_row = &table->Table[i];
                    break;
                }
            }

            if (current_row && current_row->OperStatus == IfOperStatusUp) {
                if (!m_initialized || prev_index != m_adapter_index || prev_luid.Value != m_adapter_luid.Value) {
                    ResetBaseline(*current_row, now);
                } else {
                    ComputeRates(*current_row, now);
                }
            } else {
                MarkDisconnected();
            }
        } else {
            MarkDisconnected();
        }
    }
}

// ---------------------------------------------------------------------------
// Poll - compute current network rates
// ---------------------------------------------------------------------------
void NetworkMonitor::Poll() {
    PMIB_IF_TABLE2 table = nullptr;
    DWORD result = GetIfTable2(&table);
    if (result != NO_ERROR || !table) {
        MarkDisconnected();
        return;
    }

    PollWithTable(table, std::chrono::steady_clock::now());
    FreeMibTable(table);
    UpdateLocalIP();
}

// ---------------------------------------------------------------------------
// UpdateLocalIP - resolve the IP address of the active adapter
// ---------------------------------------------------------------------------
void NetworkMonitor::UpdateLocalIP() {
    const auto now = std::chrono::steady_clock::now();
    if (m_ip_adapter_index == m_adapter_index && now - m_ip_refresh < std::chrono::seconds(5)) return;
    m_ip_adapter_index = m_adapter_index;
    m_ip_refresh = now;
    m_local_ip.clear();

    if (m_adapter_index == 0)
        return;

    // Query unicast address table
    PMIB_UNICASTIPADDRESS_TABLE addr_table = nullptr;
    DWORD result = GetUnicastIpAddressTable(AF_INET, &addr_table);
    if (result != NO_ERROR || !addr_table)
        return;

    for (ULONG i = 0; i < addr_table->NumEntries; ++i) {
        const MIB_UNICASTIPADDRESS_ROW& addr = addr_table->Table[i];
        if (addr.InterfaceLuid.Value == m_adapter_luid.Value &&
            addr.Address.si_family == AF_INET) {
            wchar_t buf[64] = {};
            const SOCKADDR_IN* sa = reinterpret_cast<const SOCKADDR_IN*>(&addr.Address);
            InetNtopW(AF_INET, &sa->sin_addr, buf, _countof(buf));
            m_local_ip = buf;
            break;
        }
    }

    FreeMibTable(addr_table);
}

std::vector<std::wstring> NetworkMonitor::GetAvailableAdapters() const {
    std::vector<std::wstring> list;
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) == NO_ERROR && table) {
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IF_ROW2& row = table->Table[i];
            if (row.OperStatus == IfOperStatusUp && IsPhysicalAdapter(row)) {
                list.push_back(row.Description);
            }
        }
        FreeMibTable(table);
    }
    return list;
}

} // namespace tmp
