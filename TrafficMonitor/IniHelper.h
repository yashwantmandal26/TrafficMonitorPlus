//ini reading and writing class
//Pass the ini file path through the constructor parameter when using
//When writing data to an ini file, the Save() function needs to be called at the end to save the changes to the file
//By default, it is saved in UTF8_BOM encoding. If you want to save in ANSI, please call SetSaveAsUTF8(false);
#pragma once

class CIniHelper
{
public:
	// Load ini file from disk
	// file_path: file path
	// force_utf8: If true, it will be forced to be parsed in UTF8 encoding. Otherwise, it will be parsed in UTF8 encoding only when it contains a UTF8 BOM.
	CIniHelper(const wstring& file_path, bool force_utf8 = false);
    // Load ini from resource file (read only)
    CIniHelper(UINT id, bool is_utf8 = true);
    CIniHelper();
	~CIniHelper();

    void FromDirectString(const wstring& str_content);

	void SetSaveAsUTF8(bool utf8);

	void WriteString(const wchar_t* AppName, const wchar_t* KeyName, const wstring& str);
	wstring GetString(const wchar_t* AppName, const wchar_t* KeyName, const wchar_t* default_str) const;
	bool GetString(const wchar_t* AppName, const wchar_t* KeyName, wstring& str) const;
	void WriteInt(const wchar_t * AppName, const wchar_t * KeyName, int value);
	int GetInt(const wchar_t * AppName, const wchar_t * KeyName, int default_value) const;
	void WriteBool(const wchar_t * AppName, const wchar_t * KeyName, bool value);
	bool GetBool(const wchar_t * AppName, const wchar_t * KeyName, bool default_value) const;
	void WriteIntArray(const wchar_t * AppName, const wchar_t * KeyName, const int* values, int size);		//Write an int array with the number of elements size
	void GetIntArray(const wchar_t * AppName, const wchar_t * KeyName, int* values, int size, int default_value = 0) const;		//Read an int array and store it in values. The number of elements is size.
	void WriteBoolArray(const wchar_t * AppName, const wchar_t * KeyName, const bool* values, int size);
	void GetBoolArray(const wchar_t * AppName, const wchar_t * KeyName, bool* values, int size, bool default_value = false) const;
    void WriteStringList(const wchar_t* AppName, const wchar_t* KeyName, const vector<wstring>& values);      //Write a list of strings. Since quotation marks will be added before and after the string when saving to the ini file, the string cannot contain quotation marks.
    void GetStringList(const wchar_t* AppName, const wchar_t* KeyName, vector<wstring>& values, const vector<wstring>& default_value) const;

    // Get all AppNames with the specified prefix (without prefix)
    vector<wstring> GetAllAppName(const wstring& prefix) const;
    // Get all key-value pairs under an AppName
    void GetAllKeyValues(const wstring& AppName, std::map<wstring, wstring>& map) const;

	//remove a segment
	bool RemoveSection(const wstring& AppName);

	bool Save();		//Save ini file to file, return true successfully

protected:
	wstring m_file_path;
	wstring m_ini_str;
	bool m_save_as_utf8{ true };		//Whether and UTF8 encoding to save

    static void UnEscapeString(wstring& str);
    void _WriteString(const wchar_t* AppName, const wchar_t* KeyName, const wstring& str);
	bool _GetString(const wchar_t* AppName, const wchar_t* KeyName, wstring& str) const;

    static wstring MergeStringList(const vector<wstring>& values);
    static void SplitStringList(vector<wstring>& values, const wstring& str_value);
};
