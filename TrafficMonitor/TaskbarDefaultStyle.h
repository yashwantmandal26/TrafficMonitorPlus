#pragma once
#include "CommonData.h"

#define TASKBAR_DEFAULT_STYLE_NUM 8
#define TASKBAR_DEFAULT_LIGHT_STYLE_INDEX 7
#define TASKBAR_DEFAULT_LIGHT_STYLE(i) (i == 7)

class CTaskbarDefaultStyle
{
public:
	struct TaskBarStyleData
	{
        std::map<CommonDisplayItem, TaskbarItemColor> text_colors{};
		COLORREF back_color{};
		COLORREF transparent_color{};
		COLORREF status_bar_color{};
		bool specify_each_item_color{};
	};

	CTaskbarDefaultStyle();
	~CTaskbarDefaultStyle();

	void LoadConfig();
	void SaveConfig() const;

	void ApplyDefaultStyle(int index, TaskBarSettingData& data) const;		//Apply a color preset
	//static void ApplyDefaultLightStyle(TaskBarSettingData& data); //Apply the default light mode preset
	void ModifyDefaultStyle(int index, TaskBarSettingData& data);     //Save current color settings to a preset

    static bool IsTaskBarStyleDataValid(const TaskBarStyleData& data);     //Determine whether a color mode is valid

private:
	TaskBarStyleData m_default_style[TASKBAR_DEFAULT_STYLE_NUM];    //Default style

};
