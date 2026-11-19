// TrafficMonitorPlus - Background GitHub Release Checker Implementation
#include "pch.h"
#include "config/updater.h"
#include "utils/string_utils.h"
#include "../lib/json.hpp"
#include <winhttp.h>

namespace tmp {

using json = nlohmann::json;

void Updater::CheckForUpdatesAsync(std::function<void(const UpdateInfo&)> on_result) {
    if (m_worker.joinable()) m_worker.join();
    m_worker = std::thread([on_result = std::move(on_result)]() {
        UpdateInfo info = CheckForUpdatesSync();
        if (on_result) {
            on_result(info);
        }
    });
}

UpdateInfo Updater::CheckForUpdatesSync() {
    UpdateInfo result{};

    HINTERNET hSession = WinHttpOpen(
        L"TrafficMonitorPlus-Updater/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0
    );
    if (!hSession) return result;

    HINTERNET hConnect = WinHttpConnect(
        hSession, L"api.github.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0
    );
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return result;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET",
        L"/repos/yashwantmandal26/TrafficMonitorPlus/releases/latest",
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    // GitHub API requires a User-Agent header
    WinHttpAddRequestHeaders(
        hRequest,
        L"User-Agent: TrafficMonitorPlus\r\nAccept: application/vnd.github.v3+json\r\n",
        -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
    );

    // 5-second connection & receive timeouts to prevent hangs
    WinHttpSetTimeouts(hRequest, 3000, 3000, 5000, 5000);

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {

        std::string response_data;
        DWORD bytes_available = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0) {
            std::vector<char> buffer(bytes_available + 1);
            DWORD bytes_read = 0;
            if (WinHttpReadData(hRequest, buffer.data(), bytes_available, &bytes_read) && bytes_read > 0) {
                response_data.append(buffer.data(), bytes_read);
            }
        }

        if (!response_data.empty()) {
            try {
                json j = json::parse(response_data, nullptr, false);
                if (!j.is_discarded() && j.contains("tag_name")) {
                    std::string tag = j["tag_name"].get<std::string>();
                    result.latest_version = Utf8ToWide(tag);

                    if (j.contains("html_url")) {
                        result.download_url = Utf8ToWide(j["html_url"].get<std::string>());
                    }
                    if (j.contains("body")) {
                        result.release_notes = Utf8ToWide(j["body"].get<std::string>());
                    }

                    result.update_available = IsNewerVersion(
                        result.latest_version,
                        VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH
                    );
                }
            } catch (...) {
                // Ignore parse errors on network hiccups
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

bool Updater::IsNewerVersion(const std::wstring& remote, int local_major, int local_minor, int local_patch) {
    int r_major = 0, r_minor = 0, r_patch = 0;
    const wchar_t* p = remote.c_str();
    if (*p == L'v' || *p == L'V') p++;

    if (swscanf_s(p, L"%d.%d.%d", &r_major, &r_minor, &r_patch) >= 1) {
        if (r_major > local_major) return true;
        if (r_major < local_major) return false;
        if (r_minor > local_minor) return true;
        if (r_minor < local_minor) return false;
        return r_patch > local_patch;
    }
    return false;
}

} // namespace tmp
