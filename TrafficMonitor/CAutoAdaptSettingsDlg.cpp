// CAutoAdaptSettingsDlg.cpp: implementation file
//

#include "stdafx.h"
#include "TrafficMonitor.h"
#include "CAutoAdaptSettingsDlg.h"
#include "afxdialogex.h"


// CAutoAdaptSettingsDlg dialog box

IMPLEMENT_DYNAMIC(CAutoAdaptSettingsDlg, CBaseDialog)

CAutoAdaptSettingsDlg::CAutoAdaptSettingsDlg(TaskBarSettingData& data, CWnd* pParent /*=nullptr*/)
	: CBaseDialog(IDD_ATUO_ADAPT_SETTING_DIALOG, pParent), m_data(data)
{

}

CAutoAdaptSettingsDlg::~CAutoAdaptSettingsDlg()
{
}

void CAutoAdaptSettingsDlg::InitComboBox(CComboBox& combo_box, int style_sel)
{
    const wchar_t* taskbar_style_names[TASKBAR_DEFAULT_STYLE_NUM] = {
        _T("1: Stealth Minimal Dark"),
        _T("2: 🌸 Pastel Sakura (Cute)"),
        _T("3: ⚡ Cyberpunk Neon (Funky)"),
        _T("4: 🍵 Matcha Mint & Boba (Cute)"),
        _T("5: 🌅 Sunset Peach (Funky)"),
        _T("6: 🌌 Retro Synthwave (Funky)"),
        _T("7: ❄️ Arctic Glacial (Cute)"),
        _T("8: ☀️ Clean Modern Light")
    };
	for (int i = 0; i < TASKBAR_DEFAULT_STYLE_NUM; i++)
    {
        if (i < _countof(taskbar_style_names))
            combo_box.AddString(taskbar_style_names[i]);
        else
		    combo_box.AddString(CCommon::LoadText(IDS_PRESET, std::to_wstring(i + 1).c_str()));
    }
	if (style_sel >= 0 && style_sel < TASKBAR_DEFAULT_STYLE_NUM)
		combo_box.SetCurSel(style_sel);
	else
		combo_box.SetCurSel(0);
}

int CAutoAdaptSettingsDlg::GetComboBoxSel(const CComboBox& combo_box)
{
	int sel = combo_box.GetCurSel();
	if (sel >= 0 && sel < TASKBAR_DEFAULT_STYLE_NUM)
		return sel;
	else
		return 0;
}

void CAutoAdaptSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DARK_MODE_DEFAULT_STYLE_COMBO, m_dark_mode_default_style_combo);
	DDX_Control(pDX, IDC_LIGHT_MODE_DEFAULT_STYLE_COMBO, m_light_mode_default_style_combo);
}

CString CAutoAdaptSettingsDlg::GetDialogName() const
{
    return _T("AutoAdaptSettingsDlg");
}

bool CAutoAdaptSettingsDlg::InitializeControls()
{
    RepositionTextBasedControls({
        { CtrlTextInfo::L1, IDC_DARK_MODE_STATIC },
        { CtrlTextInfo::C0, IDC_DARK_MODE_DEFAULT_STYLE_COMBO },
        { CtrlTextInfo::L1, IDC_LIGHT_MODE_STATIC },
        { CtrlTextInfo::C0, IDC_LIGHT_MODE_DEFAULT_STYLE_COMBO }
    });
    return true;
}


BEGIN_MESSAGE_MAP(CAutoAdaptSettingsDlg, CBaseDialog)
END_MESSAGE_MAP()


// CAutoAdaptSettingsDlg message handler


BOOL CAutoAdaptSettingsDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	// TODO: Add additional initialization here
	InitComboBox(m_dark_mode_default_style_combo, m_data.dark_default_style);
	InitComboBox(m_light_mode_default_style_combo, m_data.light_default_style);
    CheckDlgButton(IDC_AUTO_SAVE_TO_PRESET_CHECK, m_data.auto_save_taskbar_color_settings_to_preset);

    m_toolTip.Create(this);
    m_toolTip.SetMaxTipWidth(theApp.DPI(300));
    m_toolTip.AddTool(GetDlgItem(IDC_AUTO_SAVE_TO_PRESET_CHECK), CCommon::LoadText(IDS_AUTO_SAVE_TO_PRESET_TIP));

	return TRUE;  // return TRUE unless you set the focus to a control
				  // Exception: OCX property page should return FALSE
}


void CAutoAdaptSettingsDlg::OnOK()
{
	// TODO: Add specialized code here and/or call the base class

	//Get the settings in the control
	m_data.dark_default_style = GetComboBoxSel(m_dark_mode_default_style_combo);
	m_data.light_default_style = GetComboBoxSel(m_light_mode_default_style_combo);
    m_data.auto_save_taskbar_color_settings_to_preset = (IsDlgButtonChecked(IDC_AUTO_SAVE_TO_PRESET_CHECK) != 0);

	CBaseDialog::OnOK();
}


BOOL CAutoAdaptSettingsDlg::PreTranslateMessage(MSG* pMsg)
{
    // TODO: Add specialized code here and/or call the base class
    if (pMsg->message == WM_MOUSEMOVE)
        m_toolTip.RelayEvent(pMsg);

    return CBaseDialog::PreTranslateMessage(pMsg);
}
