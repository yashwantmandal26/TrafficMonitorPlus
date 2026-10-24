#pragma once
class CWinVersionHelper
{
public:
	CWinVersionHelper();
	~CWinVersionHelper();

	bool IsWindows11OrLater() const;			//Determine whether the current Windows version is Win11 or a newer version
	bool IsWindows10FallCreatorOrLater() const;		//Determine whether the current Windows version is the Win10 Fall Creators Update or a newer version
	bool IsWindows7() const;					//Determine whether the Windows version is Windows7
	bool IsWindows8Or8point1() const;			//Determine whether the Windows version is Windows8 or Windows8.1
    bool IsWindows8Point1OrLater() const;       //Determine whether the Windows version is greater than or equal to Windows8.1
    bool IsWindows8OrLater() const;
    bool IsWindows10OrLater() const;
	bool IsWine() const;						//Determine whether it is in Wine environment

	int GetMajorVersion() const { return m_major_version; }
	int GetMinorVersion() const { return m_minor_version; }
	int GetBuildNumber() const { return m_build_number; }
protected:
	int m_major_version{};
	int m_minor_version{};
	int m_build_number{};
	bool m_is_wine{};
};
