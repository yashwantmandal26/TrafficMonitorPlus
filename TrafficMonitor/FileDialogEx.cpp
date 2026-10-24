#include "stdafx.h"
#include "FileDialogEx.h"
#include <sstream>

CFileDialogEx::CFileDialogEx(BOOL bOpenFileDialog, LPCWSTR lpszDefExt, LPCWSTR lpszFilter)
    : pFileDialog(nullptr), m_bOpenFileDialog(bOpenFileDialog)
{
    // Create an IFileDialog instance
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileDialog));

    if (pFileDialog)
    {
        // Set dialog options
        DWORD dwOptions;
        pFileDialog->GetOptions(&dwOptions);
        pFileDialog->SetOptions(dwOptions | FOS_FORCEFILESYSTEM);  // File system entries only

        if (bOpenFileDialog)
        {
            pFileDialog->SetOptions(dwOptions | FOS_FILEMUSTEXIST); // The file must exist when opening it
        }

        // Set default extension
        if (lpszDefExt)
        {
            pFileDialog->SetDefaultExtension(lpszDefExt);
        }

        // Parse and set filters
        if (lpszFilter)
        {
            ParseFilter(lpszFilter);
            for (size_t i = 0; i < m_filterDescriptions.size() && i < m_filterSpecsStrings.size(); i++)
            {
                COMDLG_FILTERSPEC filterSpec = { m_filterDescriptions[i].c_str(), m_filterSpecsStrings[i].c_str()};
                m_filterSpecs.push_back(filterSpec);
            }
            pFileDialog->SetFileTypes(m_filterSpecs.size(), m_filterSpecs.data());
        }
    }
}

CFileDialogEx::~CFileDialogEx()
{
    if (pFileDialog)
    {
        pFileDialog->Release();
    }
}

int CFileDialogEx::DoModal(HWND hWndOwner)
{
    if (!pFileDialog) return -1;

    // show dialog
    HRESULT hr = pFileDialog->Show(hWndOwner);
    if (FAILED(hr))
        return -1;

    // Get selected file path
    IShellItem* pItem = nullptr;
    hr = pFileDialog->GetResult(&pItem);
    if (SUCCEEDED(hr))
    {
        PWSTR pszFilePath = nullptr;
        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
        if (SUCCEEDED(hr))
        {
            m_filePath = pszFilePath;
            CoTaskMemFree(pszFilePath);
        }
        pItem->Release();
    }

    return (SUCCEEDED(hr)) ? 1 : 0;
}

std::wstring CFileDialogEx::GetPathName() const
{
    return m_filePath;
}

std::wstring CFileDialogEx::GetFileName() const
{
    // Get file name
    size_t pos = m_filePath.find_last_of(L"\\");
    if (pos != std::wstring::npos) {
        return m_filePath.substr(pos + 1);
    }
    return m_filePath;
}

void CFileDialogEx::ParseFilter(LPCWSTR lpszFilter)
{
    m_filterSpecs.clear();
    m_filterDescriptions.clear();
    m_filterSpecsStrings.clear();

    // Split filter string using |
    std::wstringstream filterStream(lpszFilter);
    std::wstring description, spec;

    while (std::getline(filterStream, description, L'|'))
    {
        if (std::getline(filterStream, spec, L'|'))
        {
            // Store descriptions and patterns in m_filterDescriptions and m_filterSpecsStrings
            m_filterDescriptions.push_back(description);
            m_filterSpecsStrings.push_back(spec);
        }
    }
}
