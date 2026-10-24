#pragma once
class CFilePathHelper
{
public:
	CFilePathHelper(const wstring& file_path);
	CFilePathHelper(){}
	~CFilePathHelper();

	void SetFilePath(const wstring& file_path) { m_file_path = file_path; }

	wstring GetFileExtension(bool upper = false, bool width_dot = false) const;		//Get the extension of the file (upper: whether it is capitalized; width_dot: whether it contains ".")
	wstring GetFileName() const;							//Get file name
	wstring GetFileNameWithoutExtension() const;			//Get the file name (without extension)
	wstring GetFolderName() const;							//Get folder name
	wstring GetDir() const;									//Get directory
	wstring GetParentDir() const;							//Get superior directory
	wstring GetFilePath() const { return m_file_path; }		//Get full path
	const wstring& ReplaceFileExtension(const wchar_t* new_extension);		//Replaces the extension of the file and returns the full path of the file
    wstring GetFilePathWithoutExtension() const;            //Get the file path (without extension)
protected:
	wstring m_file_path;
};

