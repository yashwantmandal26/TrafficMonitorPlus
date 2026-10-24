#pragma once

class CFileDialogEx {
public:
    CFileDialogEx(BOOL bOpenFileDialog, LPCWSTR lpszDefExt = NULL, LPCWSTR lpszFilter = NULL);
    ~CFileDialogEx();

    // show dialog
    int DoModal(HWND hWndOwner = NULL);

    // Get selected file path and file name
    std::wstring GetPathName() const;
    std::wstring GetFileName() const;

private:
    void ParseFilter(LPCWSTR lpszFilter);

private:
    IFileDialog* pFileDialog;
    std::wstring m_filePath;
    BOOL m_bOpenFileDialog;
    std::vector<COMDLG_FILTERSPEC> m_filterSpecs;  // Store filter description and schema
    std::vector<std::wstring> m_filterDescriptions;  // Storage filter description
    std::vector<std::wstring> m_filterSpecsStrings;  // Store filter pattern
};
