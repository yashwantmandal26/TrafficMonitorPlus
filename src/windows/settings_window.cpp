#include "windows/settings_window.h"
#include "utils/string_utils.h"
#include <commdlg.h>
#include <algorithm>
#include <uxtheme.h>

namespace tmp { namespace {
constexpr float W=820,H=600;
constexpr auto kClass=L"TrafficMonitorPlusSettings";
const wchar_t* Names[]={L"Download speed",L"Upload speed",L"CPU usage",L"CPU temperature",L"RAM usage",L"GPU usage",L"GPU temperature",L"Battery power",L"CPU frequency"};
D2D1_COLOR_F RGBF(unsigned rgb,float alpha=1){return D2D1::ColorF(rgb,alpha);}
bool Hit(D2D1_RECT_F r,float x,float y){return x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom;}
}
SettingsWindow::SettingsWindow(){m_theme.Init(true);}
SettingsWindow::~SettingsWindow(){if(m_hwnd)DestroyWindow(m_hwnd);if(m_edit_font)DeleteObject(m_edit_font);if(m_edit_brush)DeleteObject(m_edit_brush);}
float SettingsWindow::Scale()const{RECT r{};GetClientRect(m_hwnd,&r);return (std::max)(0.5f,(std::min)(r.right/W,r.bottom/H));}
bool SettingsWindow::Init(HINSTANCE instance){
    if(m_hwnd)return true;m_instance=instance;
    WNDCLASSEXW c{sizeof(c)};c.lpfnWndProc=WndProc;c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.lpszClassName=kClass;
    if(!RegisterClassExW(&c)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return false;
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    const float scale=(std::min)(GetDpiForSystem()/96.0f,(work.bottom-work.top-52)/H);
    int width=static_cast<int>(W*scale),height=static_cast<int>(H*scale+38);
    m_hwnd=CreateWindowExW(WS_EX_APPWINDOW,kClass,L"Yash Monitor - Settings",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN,
        work.left+(work.right-work.left-width)/2,work.top+(work.bottom-work.top-height)/2,width,height,nullptr,nullptr,instance,this);
    if(!m_hwnd)return false;
    BOOL dark=TRUE;DwmSetWindowAttribute(m_hwnd,20,&dark,sizeof(dark));DWORD corners=2;DwmSetWindowAttribute(m_hwnd,33,&corners,sizeof(corners));
    auto icon=LoadIconW(instance,MAKEINTRESOURCEW(101));SendMessageW(m_hwnd,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(icon));
    m_renderer=std::make_unique<D2DRenderer>();if(!m_renderer->Init(m_hwnd))return false;
    m_preview=std::make_unique<TextRenderer>(m_renderer.get(),&m_theme);
    m_edit_brush=CreateSolidBrush(RGB(10,16,27));
    m_label_edit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,0,0,m_hwnd,reinterpret_cast<HMENU>(1001),instance,nullptr);
    SendMessageW(m_label_edit,EM_SETLIMITTEXT,40,0);
    SetWindowSubclass(m_label_edit,LabelProc,1,reinterpret_cast<DWORD_PTR>(this));
    INITCOMMONCONTROLSEX bars{sizeof(bars),ICC_BAR_CLASSES};InitCommonControlsEx(&bars);
    for(int i=0;i<3;++i){
        m_layout_sliders[i]=CreateWindowExW(0,TRACKBAR_CLASSW,L"",WS_CHILD|WS_TABSTOP|TBS_HORZ|TBS_NOTICKS|TBS_TOOLTIPS,
            0,0,0,0,m_hwnd,reinterpret_cast<HMENU>(static_cast<INT_PTR>(1100+i)),instance,nullptr);
        if(!m_layout_sliders[i])return false;
        const int mins[]={-500,-100,-100},maxs[]={500,100,60};
        SendMessageW(m_layout_sliders[i],TBM_SETRANGE,TRUE,MAKELPARAM(mins[i],maxs[i]));
        SendMessageW(m_layout_sliders[i],TBM_SETPAGESIZE,0,i==0?10:i==1?5:2);
        SetWindowTheme(m_layout_sliders[i],L"DarkMode_Explorer",nullptr);
        const wchar_t* names[]={L"Taskbar left / right",L"Taskbar up / down",L"Metric gap / overlap"};
        SetWindowTextW(m_layout_sliders[i],names[i]);
    }
    PositionLabel();return true;
}
void SettingsWindow::Show(const Settings& settings){
    if(!m_hwnd&&!Init(m_instance))return;
    if(IsVisible()){SetForegroundWindow(m_hwnd);return;}
    if(!HasUnsavedChanges()){
        m_draft=m_original=settings;
        m_reset=false;
    }else{
        m_original=settings;
    }
    m_focus=m_hover=-1;
    m_status.clear();
    RefreshOrder();
    m_startup_ready=startup_status&&startup_status();SelectMetric(m_metric);
    ShowWindow(m_hwnd,SW_SHOW);SetForegroundWindow(m_hwnd);PositionLabel();InvalidateRect(m_hwnd,nullptr,FALSE);
    if(HasUnsavedChanges()&&on_layout_preview)on_layout_preview(m_draft);
}
void SettingsWindow::Hide(){ShowWindow(m_hwnd,SW_HIDE);if(on_preview_finished)on_preview_finished();}
bool SettingsWindow::HasUnsavedChanges()const{return m_reset||!(m_draft==m_original);}
void SettingsWindow::Discard(){
    m_draft=m_original;m_reset=false;m_status=L"Draft discarded.";
    RefreshOrder();SelectMetric(m_metric);PositionSliders();
    if(on_preview_finished)on_preview_finished();
    InvalidateRect(m_hwnd,nullptr,FALSE);
}
void SettingsWindow::RefreshOrder(){
    m_order.clear();for(uint32_t i=0;i<DISPLAY_ITEM_COUNT;++i)m_order.push_back(static_cast<DisplayItem>(i));
    std::stable_sort(m_order.begin(),m_order.end(),[&](auto a,auto b){return m_draft.items[a].order<m_draft.items[b].order;});
}
bool SettingsWindow::IsVisible()const{return m_hwnd&&IsWindowVisible(m_hwnd);}
void SettingsWindow::Page(int page){if(m_page!=page){m_usage_scroll=0;m_selected_bucket_idx=-1;}m_page=page;m_focus=m_hover=-1;m_bucket_zone_start=-1;m_bucket_zone_count=0;PositionLabel();}
void SettingsWindow::PositionLabel(){
    if(!m_label_edit)return;float scale=Scale();
    SetWindowPos(m_label_edit,nullptr,static_cast<int>(461*scale),static_cast<int>(350*scale),static_cast<int>(304*scale),static_cast<int>(24*scale),SWP_NOZORDER|SWP_NOACTIVATE);
    HFONT font=CreateFontW(-static_cast<int>(13*scale),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    SendMessageW(m_label_edit,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);if(m_edit_font)DeleteObject(m_edit_font);m_edit_font=font;
    ShowWindow(m_label_edit,m_page==3&&m_metric_tab==0?SW_SHOW:SW_HIDE);
    PositionSliders();
}
void SettingsWindow::PositionSliders(){
    const float scale=Scale();
    const int values[]={m_draft.taskbar_offset_x,m_draft.taskbar_offset_y,m_draft.item_spacing};
    for(int i=0;i<3;++i){if(!m_layout_sliders[i])continue;
        SetWindowPos(m_layout_sliders[i],nullptr,static_cast<int>(234*scale),static_cast<int>((225+i*88)*scale),
            static_cast<int>(534*scale),static_cast<int>(30*scale),SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(m_layout_sliders[i],TBM_SETPOS,TRUE,values[i]);
        ShowWindow(m_layout_sliders[i],m_page==2&&m_layout_tab==1?SW_SHOW:SW_HIDE);
    }
}
void SettingsWindow::SelectMetric(DisplayItem item){m_metric=item;SetWindowTextW(m_label_edit,m_draft.items[item].custom_label.c_str());}
void SettingsWindow::MoveMetric(int direction){auto at=std::find(m_order.begin(),m_order.end(),m_metric);int i=static_cast<int>(at-m_order.begin()),next=i+direction;
    if(next<0||next>=static_cast<int>(m_order.size()))return;std::swap(m_order[i],m_order[next]);for(size_t n=0;n<m_order.size();++n)m_draft.items[m_order[n]].order=static_cast<int>(n);}
void SettingsWindow::Save(){
    auto merged=m_reset?m_draft:Settings::MergeDraft(live_settings?live_settings():m_original,m_original,m_draft);
    merged.custom_label_colors.clear();
    if(on_settings_changed&&!on_settings_changed(merged))return;
    m_original=m_draft=merged;m_reset=false;m_status=L"Saved. Changes are active.";
    if(on_preview_finished)on_preview_finished();
    PositionSliders();InvalidateRect(m_hwnd,nullptr,FALSE);
}
void SettingsWindow::Activate(float x,float y){for(auto& z:m_zones)if(Hit(z.rect,x,y)){auto action=z.action;m_status.clear();action();if(IsVisible()&&HasUnsavedChanges()&&on_layout_preview)on_layout_preview(m_draft);InvalidateRect(m_hwnd,nullptr,FALSE);break;}}
LRESULT CALLBACK SettingsWindow::LabelProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    auto self=reinterpret_cast<SettingsWindow*>(data);
    if(msg==WM_KEYDOWN&&(wp==VK_TAB||wp==VK_RETURN||wp==VK_ESCAPE)){SetFocus(self->m_hwnd);return 0;}
    return DefSubclassProc(hwnd,msg,wp,lp);
}
LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    auto self=reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(message==WM_NCCREATE){self=static_cast<SettingsWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->m_hwnd=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
    if(!self)return DefWindowProcW(hwnd,message,wp,lp);
    switch(message){
        case WM_PAINT:{PAINTSTRUCT p;BeginPaint(hwnd,&p);self->Render();EndPaint(hwnd,&p);return 0;}
        case WM_ERASEBKGND:return 1;
        case WM_SIZE:self->PositionLabel();InvalidateRect(hwnd,nullptr,FALSE);return 0;
        case WM_DPICHANGED:{auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(hwnd,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);return 0;}
        case WM_COMMAND:if(LOWORD(wp)==1001&&HIWORD(wp)==EN_CHANGE){wchar_t label[41]{};GetWindowTextW(self->m_label_edit,label,41);self->m_draft.items[self->m_metric].custom_label=label;self->m_status.clear();if(self->IsVisible()&&self->on_layout_preview)self->on_layout_preview(self->m_draft);InvalidateRect(hwnd,nullptr,FALSE);}return 0;
        case WM_HSCROLL:{
            const HWND control=reinterpret_cast<HWND>(lp);int* values[]={&self->m_draft.taskbar_offset_x,&self->m_draft.taskbar_offset_y,&self->m_draft.item_spacing};
            for(int i=0;i<3;++i)if(control==self->m_layout_sliders[i]){*values[i]=static_cast<int>(SendMessageW(control,TBM_GETPOS,0,0));self->m_status.clear();if(self->on_layout_preview)self->on_layout_preview(self->m_draft);InvalidateRect(hwnd,nullptr,FALSE);return 0;}
            break;
        }
        case WM_CTLCOLORSTATIC:if(GetDlgCtrlID(reinterpret_cast<HWND>(lp))>=1100&&GetDlgCtrlID(reinterpret_cast<HWND>(lp))<=1102){SetDCBrushColor(reinterpret_cast<HDC>(wp),RGB(17,24,36));return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));}break;
        case WM_CTLCOLOREDIT:SetTextColor(reinterpret_cast<HDC>(wp),RGB(235,241,255));SetBkColor(reinterpret_cast<HDC>(wp),RGB(10,16,27));return reinterpret_cast<LRESULT>(self->m_edit_brush);
        case WM_MOUSEMOVE:{float scale=self->Scale();float x=GET_X_LPARAM(lp)/scale,y=GET_Y_LPARAM(lp)/scale;int hover=-1;
            for(size_t i=0;i<self->m_zones.size();++i)if(Hit(self->m_zones[i].rect,x,y)){hover=static_cast<int>(i);break;}
            if(hover!=self->m_hover){self->m_hover=hover;InvalidateRect(hwnd,nullptr,FALSE);}SetCursor(LoadCursorW(nullptr,hover>=0?IDC_HAND:IDC_ARROW));
            if(!self->m_tracking){TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);self->m_tracking=true;}return 0;}
        case WM_MOUSELEAVE:self->m_tracking=false;self->m_hover=-1;InvalidateRect(hwnd,nullptr,FALSE);return 0;
        case WM_MOUSEWHEEL:{
            if(self->m_page==4&&self->m_usage_max_scroll>0){
                const int delta=GET_WHEEL_DELTA_WPARAM(wp);
                const int step=delta>0?-1:1;
                const int next=std::clamp(self->m_usage_scroll+step,0,self->m_usage_max_scroll);
                if(next!=self->m_usage_scroll){self->m_usage_scroll=next;InvalidateRect(hwnd,nullptr,FALSE);}
                return 0;
            }
            break;
        }
        case WM_LBUTTONUP:self->Activate(GET_X_LPARAM(lp)/self->Scale(),GET_Y_LPARAM(lp)/self->Scale());return 0;
        case WM_KEYDOWN:
            if(wp==VK_ESCAPE){self->Hide();return 0;}
            if(wp==VK_TAB&&!self->m_zones.empty()){int direction=GetKeyState(VK_SHIFT)<0?-1:1;self->m_focus=(self->m_focus+direction+static_cast<int>(self->m_zones.size()))%static_cast<int>(self->m_zones.size());InvalidateRect(hwnd,nullptr,FALSE);return 0;}
            if((wp==VK_RETURN||wp==VK_SPACE)&&self->m_focus>=0&&self->m_focus<static_cast<int>(self->m_zones.size())){auto action=self->m_zones[self->m_focus].action;action();if(self->IsVisible()&&self->HasUnsavedChanges()&&self->on_layout_preview)self->on_layout_preview(self->m_draft);InvalidateRect(hwnd,nullptr,FALSE);return 0;}break;
        case WM_CLOSE:self->Hide();return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
void SettingsWindow::Text(const std::wstring& text,float x,float y,float width,float size,bool bold,D2D1_COLOR_F color){m_renderer->SetFont(L"Segoe UI",size,bold);m_renderer->DrawText(text,D2D1::RectF(x,y,x+width,y+size*1.6f+2),color);}
std::wstring SettingsWindow::Ellipsize(const std::wstring& text,float max_width,float size,bool bold){
    if(text.empty()||max_width<=8.0f)return L"";
    m_renderer->SetFont(L"Segoe UI",size,bold);
    if(m_renderer->MeasureText(text).width<=max_width)return text;
    std::wstring trimmed=text;
    while(trimmed.size()>1&&m_renderer->MeasureText(trimmed+L"...").width>max_width){
        trimmed.pop_back();
    }
    return trimmed+L"...";
}
void SettingsWindow::TextRight(const std::wstring& text,float right_x,float y,float max_width,float size,bool bold,D2D1_COLOR_F color){
    const std::wstring fitted=Ellipsize(text,max_width,size,bold);
    if(fitted.empty())return;
    const float w=m_renderer->MeasureText(fitted).width;
    m_renderer->DrawText(fitted,D2D1::RectF(right_x-w-2.0f,y,right_x+2.0f,y+size*1.6f+2),color);
}
void SettingsWindow::TextCenter(const std::wstring& text,float x,float y,float width,float size,bool bold,D2D1_COLOR_F color){
    const std::wstring fitted=Ellipsize(text,width-4.0f,size,bold);
    if(fitted.empty())return;
    const float w=m_renderer->MeasureText(fitted).width;
    const float tx=x+(std::max)(0.0f,(width-w)*0.5f);
    m_renderer->DrawText(fitted,D2D1::RectF(tx,y,x+width,y+size*1.6f+2),color);
}
void SettingsWindow::Card(float x,float y,float width,float height){auto r=D2D1::RectF(x,y,x+width,y+height);m_renderer->FillRoundedRect(r,14,RGBF(0x111824));m_renderer->DrawRoundedRect(r,14,RGBF(0x263349),1);}
void SettingsWindow::Section(float y,float height,const std::wstring& title,const std::wstring& description){
    Card(206,y,592,height);
    Text(title,226,y+14,550,13,true);
    if(!description.empty())Text(description,226,y+36,550,11,false,RGBF(0x91A0B9));
}
void SettingsWindow::Navigation(int page,float y,const std::wstring& title){
    const auto rect=D2D1::RectF(22,y,170,y+32);
    const int index=static_cast<int>(m_zones.size());const bool selected=m_page==page;
    if(selected||index==m_hover)m_renderer->FillRoundedRect(rect,6,RGBF(selected?0x19314E:0x182231));
    if(selected)m_renderer->FillRoundedRect(D2D1::RectF(22,y+8,25,y+24),1.5f,RGBF(0x70B9FF));
    if(index==m_focus)m_renderer->DrawRoundedRect(rect,6,RGBF(0x8DC8FF),1.5f);
    Text(title,34,y+7,132,11.5f,selected,RGBF(selected?0xDDEEFF:0xA5B5CB));
    m_zones.push_back({rect,[this,page](){Page(page);}});
}
void SettingsWindow::Button(D2D1_RECT_F rect,const std::wstring& text,std::function<void()> action,bool selected){
    int index=static_cast<int>(m_zones.size());bool hover=index==m_hover;
    m_renderer->FillRoundedRect(rect,7,RGBF(selected?0x193D64:hover?0x26384F:0x182231));
    m_renderer->DrawRoundedRect(rect,7,RGBF(selected?0x65ADFA:hover?0x4F78AC:0x34435B),1);
    if(index==m_focus)m_renderer->DrawRoundedRect(rect,9,RGBF(0x8DC8FF),2);
    Text(text,rect.left+12,rect.top+(rect.bottom-rect.top-18)/2,rect.right-rect.left-20,12,selected);
    m_zones.push_back({rect,std::move(action)});
}
void SettingsWindow::PillButton(D2D1_RECT_F rect,const std::wstring& text,std::function<void()> action,bool selected,bool enabled,float font_size){
    int index=static_cast<int>(m_zones.size());
    bool hover=enabled&&(index==m_hover);
    m_renderer->FillRoundedRect(rect,7,RGBF(!enabled?0x101722:selected?0x176BD1:hover?0x26384F:0x182231));
    m_renderer->DrawRoundedRect(rect,7,RGBF(!enabled?0x1F293A:selected?0x6AB8FF:hover?0x4F78AC:0x34435B),1);
    if(enabled&&index==m_focus)m_renderer->DrawRoundedRect(rect,7,RGBF(0x8DC8FF),2);
    const D2D1_COLOR_F col=!enabled?RGBF(0x526178):selected?RGBF(0xFFFFFF):RGBF(0xD3DFEF);
    const float ty=rect.top+(rect.bottom-rect.top-(font_size*1.35f))*0.5f;
    TextCenter(text,rect.left,ty,rect.right-rect.left,font_size,selected,col);
    if(enabled)m_zones.push_back({rect,std::move(action)});
}
void SettingsWindow::Toggle(float y,const std::wstring& title,const std::wstring& subtitle,bool& value,float x,float width){
    Text(title,x,y,width-90,13,true);Text(subtitle,x,y+22,width-90,11,false,RGBF(0x8E9DB4));
    const int index=static_cast<int>(m_zones.size());const float left=x+width-44;
    TextRight(value?L"On":L"Off",left-10,y+8,34,11,false,RGBF(0xA9BAD0));
    const auto rect=D2D1::RectF(left,y+6,x+width,y+30);
    m_renderer->FillRoundedRect(rect,12,RGBF(value?0x3C91ED:0x334155));
    if(index==m_hover||index==m_focus)m_renderer->DrawRoundedRect(rect,12,RGBF(0xA3D2FF),1.5f);
    const float knob=left+(value?23.0f:3.0f);
    m_renderer->FillRoundedRect(D2D1::RectF(knob,y+9,knob+18,y+27),9,RGBF(0xFFFFFF));
    m_zones.push_back({D2D1::RectF(x,y,x+width,y+36),[&value](){value=!value;}});
}
void SettingsWindow::Stepper(float x,float y,const std::wstring& title,int& value,int min,int max,int step,float width){
    Text(title,x,y,width-138,12);
    Button(D2D1::RectF(x+width-134,y-5,x+width-100,y+25),L"-",[&value,min,max,step](){value=(std::max)(min,value-step);if(min==0&&max==40&&value>0&&value<6&&step==1)value=0;});
    Text(std::to_wstring(value),x+width-88,y,48,13,true);
    Button(D2D1::RectF(x+width-34,y-5,x+width,y+25),L"+",[&value,max,step](){value=(std::min)(max,value+step);if(max==40&&value>0&&value<6&&step==1)value=6;});
}
void SettingsWindow::Checkbox(float x,float y,const std::wstring& title,bool& value,float width,std::function<void(bool)> on_change){
    const float box_size=18.0f;
    auto box_rect=D2D1::RectF(x,y+1,x+box_size,y+1+box_size);
    int index=static_cast<int>(m_zones.size());
    bool hover=index==m_hover;
    m_renderer->FillRoundedRect(box_rect,4,RGBF(value?0x176BD1:hover?0x1E2B3D:0x111824));
    m_renderer->DrawRoundedRect(box_rect,4,RGBF(value?0x6AB8FF:hover?0x4F78AC:0x34435B),1);
    if(index==m_focus)m_renderer->DrawRoundedRect(box_rect,4,RGBF(0x8DC8FF),2);
    if(value){
        m_renderer->DrawLine(D2D1::Point2F(x+4.5f,y+10.0f),D2D1::Point2F(x+7.5f,y+13.5f),RGBF(0xFFFFFF),2.0f);
        m_renderer->DrawLine(D2D1::Point2F(x+7.5f,y+13.5f),D2D1::Point2F(x+13.5f,y+5.5f),RGBF(0xFFFFFF),2.0f);
    }
    Text(title,x+26,y+2,width-26,12,value,value?RGBF(0xFFFFFF):RGBF(0xC5D1E0));
    m_zones.push_back({D2D1::RectF(x,y,x+width,y+22),[&value,on_change](){value=!value;if(on_change)on_change(value);}});
}
void SettingsWindow::Render(){
    if(!m_renderer)return;RECT r{};GetClientRect(m_hwnd,&r);if(!r.right||!r.bottom)return;
    m_renderer->Resize(r.right,r.bottom);
    if(!m_renderer->BeginDraw(RGBF(0x080C13)))return;
    auto* target=m_renderer->GetTarget();
    if(!target)return;
    float dx=96,dy=96;target->GetDpi(&dx,&dy);float transform=Scale()/(dx/96.0f);
    target->SetTransform(D2D1::Matrix3x2F::Scale(transform,transform));m_zones.clear();m_bucket_zone_start=-1;m_bucket_zone_count=0;
    m_renderer->FillRoundedRect(D2D1::RectF(10,10,182,H-10),16,RGBF(0x0E1521));
    m_renderer->FillGradientRoundedRect(D2D1::RectF(25,29,57,61),10,RGBF(0x53A9FF),RGBF(0x2859D9));
    Text(L"Y",34,31,20,20,true);Text(L"Yash Monitor",68,29,110,15,true);Text(L"Settings",68,51,100,10,false,RGBF(0x91A0B9));
    Navigation(0,85,L"Overview");
    Text(L"WIDGET",34,132,130,9,true,RGBF(0x7388A6));
    Navigation(1,152,L"Appearance");Navigation(2,187,L"Layout & position");
    Navigation(3,222,L"Metrics & labels");Navigation(6,257,L"Value colours");
    Text(L"MONITORING",34,306,130,9,true,RGBF(0x7388A6));
    Navigation(4,326,L"Data usage");Navigation(7,361,L"Alerts");Navigation(8,396,L"Performance & sensors");
    Text(L"APPLICATION",34,445,130,9,true,RGBF(0x7388A6));
    Navigation(5,465,L"Startup & updates");
    m_renderer->FillRoundedRect(D2D1::RectF(24,520,168,574),9,RGBF(0x111D2D));
    m_renderer->FillRoundedRect(D2D1::RectF(36,535,42,541),3,RGBF(HasUnsavedChanges()?0xF3BC6A:0x67D8AB));
    Text(HasUnsavedChanges()?L"Unsaved changes":L"All changes saved",49,529,112,10,true);
    Text(L"Live preview enabled",36,550,126,9.5f,false,RGBF(0x8E9DB4));
    const wchar_t* titles[]={L"Overview",L"Appearance",L"Layout & position",L"Metrics & labels",L"Data usage",L"Startup & updates",L"Value colours",L"Alerts",L"Performance & sensors"};
    const wchar_t* descriptions[]={L"Your live widget, current readings and quick settings.",L"Set the widget theme and default typography.",L"Arrange your metrics and fine-tune their taskbar position.",L"Select a metric to adjust its visibility, label and size.",L"Explore app, service and timeline usage reported by Windows.",L"Manage sign-in behaviour and application updates.",L"Choose a fixed colour or let values change with intensity.",L"Choose when high usage and temperature trigger a notification.",L"Balance refresh speed with background work and inspect sensors."};
    Text(titles[m_page],206,28,600,22,true);Text(descriptions[m_page],207,69,590,12,false,RGBF(0x91A0B9));
    auto data=data_snapshot?data_snapshot():TelemetryData{};
    auto usage=usage_snapshot?usage_snapshot():std::pair<UsageTotal,UsageTotal>{};
    if(m_page==0){
        Card(206,110,592,140);Text(L"YOUR LIVE WIDGET",224,125,300,10,true,RGBF(0x8AA7D1));
        m_renderer->FillRoundedRect(D2D1::RectF(222,158,780,232),11,RGBF(0x090E16));
        m_renderer->SetFont(m_draft.font_family,static_cast<float>(m_draft.font_size),m_draft.font_bold);
        m_preview->SetSpeedFormat(m_draft.compact,m_draft.speed_unit_bits,m_draft.speed_unit);
        m_preview->SetShowUnitSigns(m_draft.show_unit_signs);
        m_preview->SetStatValueGap(m_draft.stat_value_gap);
        m_preview->SetThresholds(m_draft);
        auto items=m_preview->PrepareItems(data,m_draft.GetEnabledItemsSorted(),m_draft.thermal_colors,nullptr,&m_draft.custom_value_colors);
        m_renderer->GetTarget()->PushAxisAlignedClip(D2D1::RectF(228,163,774,228),D2D1_ANTIALIAS_MODE_ALIASED);
        if(m_draft.layout==LayoutDirection::AutoCompact)m_preview->RenderAutoCompact(items,232,166,static_cast<float>(m_draft.item_spacing),52,26);
        else if(m_draft.layout==LayoutDirection::Stacked)m_preview->RenderStacked(items,232,166,static_cast<float>(m_draft.item_spacing),26);
        else if(m_draft.layout==LayoutDirection::TwoRows)m_preview->RenderTwoRows(items,232,166,static_cast<float>(m_draft.item_spacing),26);
        else m_preview->RenderHorizontal(items,232,178,static_cast<float>(m_draft.item_spacing));
        m_renderer->GetTarget()->PopAxisAlignedClip();
        Card(206,266,286,96);Card(508,266,290,96);
        Text(L"TODAY'S DATA",225,280,240,10,true,RGBF(0x91A0B9));Text(FormatMemory(usage.first.Combined()),225,304,240,24,true);
        Text(L"CPU TEMPERATURE",527,280,240,10,true,RGBF(0x91A0B9));Text(FormatTemperature(data.cpu_temp_celsius),527,304,240,24,true,RGBF(0x74BDFF));
        Card(206,378,592,136);
        Toggle(395,L"Start with Windows",L"Sign in and your widget is ready, with sensor privileges.",m_draft.auto_start,224,552);
        Toggle(452,L"Hide for full-screen apps",L"Keep games and videos distraction-free.",m_draft.hide_on_fullscreen,224,552);
    }else if(m_page==1){
        Section(110,120,L"Widget theme",L"Follow Windows or choose a fixed appearance for the taskbar widget.");
        const wchar_t* themes[]={L"auto",L"dark",L"light"};const wchar_t* themeNames[]={L"Match Windows",L"Dark",L"Light"};
        for(int i=0;i<3;++i)Button(D2D1::RectF(226.0f+i*187,177,401.0f+i*187,211),themeNames[i],[this,i](){const wchar_t* values[]={L"auto",L"dark",L"light"};m_draft.theme=values[i];},m_draft.theme==themes[i]);
        Section(244,174,L"Typography",L"Default font for all metrics. Individual overrides are in Metrics & labels.");
        const wchar_t* families[]={L"Segoe UI",L"Consolas",L"Tahoma"};
        for(int i=0;i<3;++i)Button(D2D1::RectF(226.0f+i*187,310,401.0f+i*187,344),families[i],[this,i](){const wchar_t* fonts[]={L"Segoe UI",L"Consolas",L"Tahoma"};m_draft.font_family=fonts[i];},m_draft.font_family==families[i]);
        Stepper(226,373,L"Font size",m_draft.font_size,6,40,1,258);
        Button(D2D1::RectF(518,366,633,400),L"Regular",[this](){m_draft.font_bold=false;},!m_draft.font_bold);
        Button(D2D1::RectF(645,366,776,400),L"Semibold",[this](){m_draft.font_bold=true;},m_draft.font_bold);
        Card(206,432,592,82);Text(L"Looking for value colours?",226,446,315,13,true);
        Text(L"Automatic colours, fixed colours and limits live together.",226,474,355,10.5f,false,RGBF(0x91A0B9));
        Button(D2D1::RectF(617,453,776,489),L"Value colours  >",[this](){Page(6);});
    }else if(m_page==2){
        PillButton(D2D1::RectF(206,110,492,146),L"Arrangement",[this](){m_layout_tab=0;PositionSliders();},m_layout_tab==0);
        PillButton(D2D1::RectF(502,110,798,146),L"Position & spacing",[this](){m_layout_tab=1;PositionSliders();},m_layout_tab==1);
        if(m_layout_tab==1){
            Card(206,162,592,352);
            const wchar_t* names[]={L"Left / right position",L"Up / down position",L"Metric gap / overlap"};
            const wchar_t* left[]={L"Left",L"Up",L"Overlap"};const wchar_t* right[]={L"Right",L"Down",L"Wider"};
            const int values[]={m_draft.taskbar_offset_x,m_draft.taskbar_offset_y,m_draft.item_spacing};
            for(int i=0;i<3;++i){const float y=192.0f+i*88;
                Text(names[i],234,y,420,13,true);Text(std::format(L"{} px",values[i]),681,y,95,13,true,RGBF(0x74BDFF));
                Text(left[i],238,y+63,240,11,false,RGBF(0x91A0B9));Text(right[i],717,y+63,70,11,false,RGBF(0x91A0B9));
            }
            Text(L"Auto keeps a small gutter. Negative spacing allows overlap.",234,449,540,11,false,RGBF(0x91A0B9));
            Button(D2D1::RectF(234,475,436,504),L"Reset position / spacing",[this](){const auto defaults=Settings::GetDefaults();m_draft.taskbar_offset_x=defaults.taskbar_offset_x;m_draft.taskbar_offset_y=defaults.taskbar_offset_y;m_draft.item_spacing=defaults.item_spacing;m_draft.stat_value_gap=defaults.stat_value_gap;PositionSliders();});
            Button(D2D1::RectF(450,475,774,504),L"Auto fit to save space",[this](){m_draft.layout=LayoutDirection::AutoCompact;},m_draft.layout==LayoutDirection::AutoCompact);
        }else{
            Section(162,196,L"Metric arrangement",L"Choose how labels and values fit in the Windows taskbar.");
            Button(D2D1::RectF(226,221,493,253),L"Taskbar & tray monitor",[this](){m_draft.display_mode=DisplayMode::Taskbar;},true);
            LayoutDirection layouts[]={LayoutDirection::AutoCompact,LayoutDirection::Stacked,LayoutDirection::TwoRows,LayoutDirection::Horizontal,LayoutDirection::Vertical};const wchar_t* layoutNames[]={L"Auto compact",L"Label / value",L"Two rows",L"One row",L"Vertical"};
            for(int i=0;i<5;++i)PillButton(D2D1::RectF(226.0f+i*112,270,330.0f+i*112,306),layoutNames[i],[this,i](){LayoutDirection l[]={LayoutDirection::AutoCompact,LayoutDirection::Stacked,LayoutDirection::TwoRows,LayoutDirection::Horizontal,LayoutDirection::Vertical};m_draft.layout=l[i];},m_draft.layout==layouts[i]);
            Text(L"Auto compact fits the available space. Two rows fills each column top to bottom.",226,325,550,10.5f,false,RGBF(0x91A0B9));
            Section(372,142,L"Label and value spacing",L"Use Metrics & labels for individual width, spacing and label overrides.");
            Stepper(226,451,L"Label / value gap",m_draft.stat_value_gap,0,50,1,550);
            Text(L"For widget placement and gaps between metrics, open Position & spacing above.",226,487,550,10.5f,false,RGBF(0x91A0B9));
        }
    }else if(m_page==3){
        Card(206,110,222,404);Card(442,110,356,404);
        Text(L"METRICS / DISPLAY ORDER",222,126,198,10,true,RGBF(0x91A0B9));
        for(size_t i=0;i<m_order.size();++i){auto item=m_order[i];float y=154+static_cast<float>(i)*36;auto name=(m_draft.items[item].enabled?L"+  ":L"-  ")+std::wstring(Names[static_cast<int>(item)]);Button(D2D1::RectF(219,y,415,y+29),name,[this,item](){SelectMetric(item);},m_metric==item);}
        Checkbox(222,486,L"Units for all metrics",m_draft.show_unit_signs,200,[this](bool val){for(auto& [k,cfg]:m_draft.items)cfg.show_unit=val;});
        auto& config=m_draft.items[m_metric];Text(Names[static_cast<int>(m_metric)],460,126,314,16,true);
        PillButton(D2D1::RectF(460,164,608,198),L"Content",[this](){m_metric_tab=0;PositionLabel();},m_metric_tab==0);
        PillButton(D2D1::RectF(620,164,776,198),L"Size & spacing",[this](){m_metric_tab=1;PositionLabel();},m_metric_tab==1);
        if(m_metric_tab==0){
        Toggle(218,L"Show this metric",L"Hidden items don't occupy a slot.",config.enabled,460,315);
        const wchar_t* unitLabel=L"Show unit sign next to value";
        switch(m_metric){
            case DisplayItem::CpuUsage:case DisplayItem::RamUsage:case DisplayItem::GpuUsage:unitLabel=L"Show unit sign (%) next to value";break;
            case DisplayItem::CpuTemp:case DisplayItem::GpuTemp:unitLabel=L"Show unit sign (\u00b0C) next to value";break;
            case DisplayItem::BatteryWattage:unitLabel=L"Show unit sign (W) next to value";break;
            case DisplayItem::CpuFrequency:unitLabel=L"Show unit sign (GHz) next to value";break;
            case DisplayItem::DownloadSpeed:case DisplayItem::UploadSpeed:unitLabel=L"Show speed unit (KB/s, MB/s)";break;
            default:break;
        }
        Checkbox(460,278,unitLabel,config.show_unit,315,[this](bool val){
            if(val)m_draft.show_unit_signs=true;
            else{bool any=false;for(const auto& [k,cfg]:m_draft.items)if(cfg.show_unit){any=true;break;}m_draft.show_unit_signs=any;}
        });
        Text(L"CUSTOM LABEL  (blank = automatic)",460,320,315,10,true,RGBF(0x91A0B9));
        m_renderer->FillRoundedRect(D2D1::RectF(456,346,777,377),7,RGBF(0x0A101B));m_renderer->DrawRoundedRect(D2D1::RectF(456,346,777,377),7,RGBF(0x34435B),1);
        Text(L"DISPLAY ORDER",460,408,315,10,true,RGBF(0x91A0B9));
        Button(D2D1::RectF(460,434,608,468),L"Move left / up",[this](){MoveMetric(-1);});Button(D2D1::RectF(624,434,776,468),L"Move right / down",[this](){MoveMetric(1);});
        Text(L"Two rows fills each column top, then bottom.",460,485,315,10,false,RGBF(0x91A0B9));
        }else{
        Stepper(460,231,L"Font size",config.font_size,0,40,1,315);
        Text(L"0 inherits the default size from Appearance.",460,262,315,10.5f,false,RGBF(0x91A0B9));
        Stepper(460,302,L"Number width",config.value_width,0,500,4,315);
        Text(L"Width in pixels; 0 sizes the number automatically.",460,333,315,10.5f,false,RGBF(0x91A0B9));
        Stepper(460,373,L"Horizontal offset",config.offset_x,-50,100,1,315);
        Text(L"Shift this metric left or right, in pixels.",460,404,315,10.5f,false,RGBF(0x91A0B9));
        Stepper(460,444,L"Label / value gap",config.label_gap,0,50,1,315);
        Text(L"0 uses the global gap from Layout & position.",460,475,315,10.5f,false,RGBF(0x91A0B9));
        }
    }else if(m_page==4){
        NetworkUsageReport report{};
        if(detailed_usage_snapshot){
            report=detailed_usage_snapshot(m_usage_range,m_usage_scope,m_usage_force_refresh);
        }
        m_usage_force_refresh=false;
        if(report.ready)m_last_report=report;
        const UsageTotal today_total=report.ready?report.today_summary:usage.first;
        const UsageTotal month_total=report.ready?report.month_summary:usage.second;

        // Top summary cards
        Card(206,102,292,60);Card(506,102,292,60);
        Text(L"TODAY",220,110,60,10,true,RGBF(0x91A0B9));
        const std::wstring profile_text=report.profile_label.empty()
            ?(m_usage_scope==UsageNetworkScope::AllNetworks?L"All networks":L"Active network")
            :report.profile_label;
        TextRight(profile_text,486,110,200,10,false,RGBF(0x74BDFF));
        const bool valid_today=!report.ready||report.today_summary_valid;
        const bool valid_month=!report.ready||report.month_summary_valid;
        Text(valid_today?FormatMemory(today_total.Combined()):L"--",220,127,124,20,true);
        Text(L"\u2193 "+(valid_today?FormatMemory(today_total.download):L"--"),346,133,74,11,true,RGBF(0x78BCFF));
        Text(L"\u2191 "+(valid_today?FormatMemory(today_total.upload):L"--"),422,133,72,11,true,RGBF(0x91DFC1));

        Text(L"THIS MONTH",520,110,88,10,true,RGBF(0x91A0B9));
        const std::wstring sync_text = [&]() -> std::wstring {
            if (report.status == ReportStatus::Loading) return L"Loading OS history...";
            if (report.status == ReportStatus::Failed) return L"Query failed";
            if (report.status == ReportStatus::Stale) return L"Stale (last: " + (report.updated_at.empty() ? L"earlier" : report.updated_at) + L")";
            if (report.status == ReportStatus::Partial) return L"Partial (" + (report.updated_at.empty() ? L"Synced" : report.updated_at) + L")";
            return report.updated_at.empty() ? L"Live SRUM" : L"Synced " + report.updated_at;
        }();
        TextRight(sync_text,786,110,174,10,false,RGBF(0x8E9DB4));
        Text(valid_month?FormatMemory(month_total.Combined()):L"--",520,127,124,20,true);
        Text(L"\u2193 "+(valid_month?FormatMemory(month_total.download):L"--"),646,133,74,11,true,RGBF(0x78BCFF));
        Text(L"\u2191 "+(valid_month?FormatMemory(month_total.upload):L"--"),722,133,72,11,true,RGBF(0x91DFC1));

        // Time range ("WHEN"), category filter, and network scope bar
        auto selectRange=[this](UsageTimeRange r){
            if(m_usage_range!=r){
                m_usage_range=r;
                m_usage_scroll=0;
                m_selected_bucket_idx=-1;
                m_last_report.ready=false;
                m_last_report.status=ReportStatus::Loading;
            }
        };
        PillButton(D2D1::RectF(206,170,270,198),L"Last 1h",[=](){selectRange(UsageTimeRange::LastHour);},m_usage_range==UsageTimeRange::LastHour,true,11.0f);
        PillButton(D2D1::RectF(275,170,333,198),L"Today",[=](){selectRange(UsageTimeRange::Today);},m_usage_range==UsageTimeRange::Today,true,11.0f);
        PillButton(D2D1::RectF(338,170,400,198),L"7 Days",[=](){selectRange(UsageTimeRange::Last7Days);},m_usage_range==UsageTimeRange::Last7Days,true,11.0f);
        PillButton(D2D1::RectF(405,170,487,198),L"This Month",[=](){selectRange(UsageTimeRange::ThisMonth);},m_usage_range==UsageTimeRange::ThisMonth,true,11.0f);

        auto selectCategory=[this](int cat){
            if(m_usage_category_filter!=cat){m_usage_category_filter=cat;m_usage_scroll=0;m_selected_bucket_idx=-1;}
        };
        PillButton(D2D1::RectF(502,170,544,198),L"All",[=](){selectCategory(0);},m_usage_category_filter==0,true,11.0f);
        PillButton(D2D1::RectF(549,170,603,198),L"Apps",[=](){selectCategory(1);},m_usage_category_filter==1,true,11.0f);
        PillButton(D2D1::RectF(608,170,676,198),L"Services",[=](){selectCategory(2);},m_usage_category_filter==2,true,11.0f);

        const bool all_nets=(m_usage_scope==UsageNetworkScope::AllNetworks);
        PillButton(D2D1::RectF(688,170,798,198),all_nets?L"Scope: All nets":L"Scope: Active",[this](){
            m_usage_scope=(m_usage_scope==UsageNetworkScope::ActiveOnly)
                ?UsageNetworkScope::AllNetworks:UsageNetworkScope::ActiveOnly;
            m_usage_scroll=0;m_selected_bucket_idx=-1;
            m_last_report.ready=false;
            m_last_report.status=ReportStatus::Loading;
        },all_nets,true,10.5f);

        // Filter items by App / Service
        std::vector<const AppUsageEntry*> filtered;
        uint64_t filtered_total=0;
        filtered.reserve(report.items.size());
        for(const auto& entry:report.items){
            if(m_usage_category_filter==1&&entry.category!=AppUsageCategory::App)continue;
            if(m_usage_category_filter==2&&entry.category!=AppUsageCategory::Service)continue;
            filtered.push_back(&entry);
            filtered_total+=entry.Combined();
        }
        const int total_items=static_cast<int>(filtered.size());
        constexpr int kVisibleRows=5;
        m_usage_max_scroll=(std::max)(0,total_items-kVisibleRows);
        m_usage_scroll=std::clamp(m_usage_scroll,0,m_usage_max_scroll);

        const AppUsageEntry* selected_entry=nullptr;
        if(!m_selected_app_id.empty()){
            for(const auto* e:filtered){
                if(e->id==m_selected_app_id){selected_entry=e;break;}
            }
        }

        // Left pane: Ranked Apps & Services list
        Card(206,206,334,276);
        const wchar_t* list_title=m_usage_category_filter==1?L"APPLICATIONS":(m_usage_category_filter==2?L"SERVICES & OS":L"APPS & SERVICES");
        Text(std::format(L"{} ({})",list_title,total_items),220,214,204,10,true,RGBF(0x91A0B9));
        const uint64_t scope_range_total=(m_usage_category_filter==0&&report.ready)
            ?(report.total_download+report.total_upload):filtered_total;
        TextRight(FormatMemory(scope_range_total),526,214,94,10,true,RGBF(0xE8F0FF));

        if(filtered.empty()){
            TextCenter(report.ready?L"No recorded data in this window.":L"Querying Windows SRUM usage...",216,320,314,12,false,RGBF(0x8E9DB4));
        }else{
            const uint64_t top_bytes=(std::max)(uint64_t{1},filtered.front()->Combined());
            for(int r=0;r<kVisibleRows&&(m_usage_scroll+r)<total_items;++r){
                const AppUsageEntry* item=filtered[m_usage_scroll+r];
                const float ry=236.0f+static_cast<float>(r)*42.0f;
                const D2D1_RECT_F row_rect=D2D1::RectF(216.0f,ry,530.0f,ry+38.0f);
                const int zone_idx=static_cast<int>(m_zones.size());
                const bool is_sel=(selected_entry==item);
                const bool is_hov=(zone_idx==m_hover);
                m_renderer->FillRoundedRect(row_rect,7.0f,RGBF(is_sel?0x162944:is_hov?0x182334:0x0C121C));
                m_renderer->DrawRoundedRect(row_rect,7.0f,RGBF(is_sel?0x58A6FF:is_hov?0x3D5980:0x1E293B),1.0f);
                const std::wstring row_id=item->id;
                m_zones.push_back({row_rect,[this,row_id](){
                    m_selected_app_id=(m_selected_app_id==row_id)?L"":row_id;
                    m_selected_bucket_idx=-1;
                }});

                const bool is_svc=(item->category==AppUsageCategory::Service);
                m_renderer->FillRoundedRect(D2D1::RectF(222.0f,ry+5.0f,252.0f,ry+18.5f),4.0f,RGBF(is_svc?0x16332D:0x16314F));
                TextCenter(is_svc?L"SVC":L"APP",222.0f,ry+5.5f,30.0f,8.5f,true,is_svc?RGBF(0x91DFC1):RGBF(0x78BCFF));
                Text(Ellipsize(item->display_name,180.0f,11.5f,true),258.0f,ry+3.0f,182.0f,11.5f,true,RGBF(0xEDF3FF));
                TextRight(FormatMemory(item->Combined()),524.0f,ry+3.0f,84.0f,11.5f,true,RGBF(0xFFFFFF));

                const float track_x=222.0f,track_w=302.0f,bar_y=ry+21.5f;
                m_renderer->FillRoundedRect(D2D1::RectF(track_x,bar_y,track_x+track_w,bar_y+3.0f),1.5f,RGBF(0x182232));
                const uint64_t comb=item->Combined();
                if(comb>0){
                    const float ratio=static_cast<float>(comb)/static_cast<float>(top_bytes);
                    const float fill_w=(std::max)(3.0f,ratio*track_w);
                    const float down_w=fill_w*(static_cast<float>(item->download)/static_cast<float>(comb));
                    if(down_w>0.5f)m_renderer->FillRoundedRect(D2D1::RectF(track_x,bar_y,track_x+down_w,bar_y+3.0f),1.5f,RGBF(0x53A9FF));
                    if(fill_w-down_w>0.5f)m_renderer->FillRoundedRect(D2D1::RectF(track_x+down_w,bar_y,track_x+fill_w,bar_y+3.0f),1.5f,RGBF(0x67D8AB));
                }

                const std::wstring dl_ul=std::format(L"\u2193{}  \u2191{}",FormatMemory(item->download),FormatMemory(item->upload));
                Text(Ellipsize(dl_ul,156.0f,9.5f,false),222.0f,ry+25.0f,158.0f,9.5f,false,RGBF(0x8FA4C2));
                const std::wstring when_str=!item->peak_when.empty()?L"Peak "+item->peak_when:item->last_when;
                TextRight(when_str,524.0f,ry+25.0f,146.0f,9.5f,false,RGBF(0x8DC8FF));
            }
        }

        // Pagination footer
        PillButton(D2D1::RectF(216,448,282,474),L"< Prev",[this](){
            m_usage_scroll=(std::max)(0,m_usage_scroll-kVisibleRows);
        },false,m_usage_scroll>0,10.5f);
        const std::wstring page_label=total_items>0
            ?std::format(L"{}-{} of {}  (Scroll)",m_usage_scroll+1,(std::min)(total_items,m_usage_scroll+kVisibleRows),total_items)
            :L"0 items";
        TextCenter(page_label,286,454,174,10.0f,false,RGBF(0x91A0B9));
        PillButton(D2D1::RectF(464,448,530,474),L"Next >",[this](){
            m_usage_scroll=(std::min)(m_usage_max_scroll,m_usage_scroll+kVisibleRows);
        },false,(m_usage_scroll+kVisibleRows)<total_items,10.5f);

        // Right pane: "WHEN" Activity Timeline & Active Services Inspector
        Card(548,206,250,276);
        Text(L"WHEN ACTIVITY TIMELINE",562,214,154,10,true,RGBF(0x91A0B9));
        if(selected_entry){
            PillButton(D2D1::RectF(722,210,786,230),L"Show all",[this](){
                m_selected_app_id.clear();m_selected_bucket_idx=-1;
            },false,true,10.0f);
        }
        const std::wstring inspector_title=selected_entry
            ?selected_entry->display_name
            :(m_usage_category_filter==1?L"All Applications":(m_usage_category_filter==2?L"All Services & OS":L"All Network Traffic"));
        const std::wstring inspector_sub=selected_entry
            ?(selected_entry->detail.empty()?selected_entry->id:selected_entry->detail)
            :L"Click a bar or app row to inspect when";
        Text(Ellipsize(inspector_title,224.0f,12.5f,true),562,231,224,12.5f,true,RGBF(0xFFFFFF));
        Text(Ellipsize(inspector_sub,224.0f,9.5f,false),562,248,224,9.5f,false,RGBF(0x8FA4C2));

        std::vector<UsageBucket> chart_timeline;
        if(selected_entry){
            chart_timeline=selected_entry->timeline;
        }else if(m_usage_category_filter==0){
            chart_timeline=report.timeline;
        }else{
            chart_timeline=report.timeline;
            for(auto& b:chart_timeline){b.download=0;b.upload=0;}
            for(const auto* e:filtered){
                const size_t n=(std::min)(chart_timeline.size(),e->timeline.size());
                for(size_t i=0;i<n;++i){
                    chart_timeline[i].download+=e->timeline[i].download;
                    chart_timeline[i].upload+=e->timeline[i].upload;
                }
            }
        }

        uint64_t max_bucket=0;
        int peak_idx=-1;
        for(size_t i=0;i<chart_timeline.size();++i){
            const uint64_t c=chart_timeline[i].Combined();
            if(c>max_bucket){max_bucket=c;peak_idx=static_cast<int>(i);}
        }

        const float chart_x=562.0f,chart_w=224.0f,plot_top=272.0f,plot_bot=352.0f,plot_h=plot_bot-plot_top;
        m_renderer->DrawLine(D2D1::Point2F(chart_x,plot_top),D2D1::Point2F(chart_x+chart_w,plot_top),RGBF(0x182130),1.0f);
        m_renderer->DrawLine(D2D1::Point2F(chart_x,plot_top+plot_h*0.5f),D2D1::Point2F(chart_x+chart_w,plot_top+plot_h*0.5f),RGBF(0x182130),1.0f);
        m_renderer->DrawLine(D2D1::Point2F(chart_x,plot_bot),D2D1::Point2F(chart_x+chart_w,plot_bot),RGBF(0x2B3B55),1.0f);

        if(!chart_timeline.empty()){
            const size_t count=chart_timeline.size();
            const float slot_w=chart_w/static_cast<float>(count);
            const float bar_w=std::clamp(slot_w*0.64f,3.0f,18.0f);
            m_bucket_zone_start=static_cast<int>(m_zones.size());
            m_bucket_zone_count=static_cast<int>(count);
            for(size_t i=0;i<count;++i){
                const float sx=chart_x+static_cast<float>(i)*slot_w;
                const D2D1_RECT_F hit_rect=D2D1::RectF(sx,plot_top-4.0f,sx+slot_w,plot_bot+16.0f);
                const int zone_idx=static_cast<int>(m_zones.size());
                const int bi=static_cast<int>(i);
                const bool active=(zone_idx==m_hover)||(bi==m_selected_bucket_idx);
                if(active){
                    m_renderer->FillRoundedRect(D2D1::RectF(sx+1.0f,plot_top,sx+slot_w-1.0f,plot_bot),3.0f,RGBF(0x172942));
                }
                const uint64_t comb=chart_timeline[i].Combined();
                if(comb>0&&max_bucket>0){
                    const float h_total=(std::max)(2.5f,(static_cast<float>(comb)/static_cast<float>(max_bucket))*plot_h);
                    const float bx=sx+(slot_w-bar_w)*0.5f;
                    const float up_h=h_total*(static_cast<float>(chart_timeline[i].upload)/static_cast<float>(comb));
                    const float down_h=h_total-up_h;
                    if(down_h>0.5f){
                        m_renderer->FillRoundedRect(D2D1::RectF(bx,plot_bot-down_h,bx+bar_w,plot_bot),1.5f,RGBF(active?0x6EC0FF:0x479DF5));
                    }
                    if(up_h>0.5f){
                        m_renderer->FillRoundedRect(D2D1::RectF(bx,plot_bot-h_total,bx+bar_w,plot_bot-down_h),1.5f,RGBF(active?0x8CF2C9:0x5FCE9F));
                    }
                }
                m_zones.push_back({hit_rect,[this,bi](){
                    m_selected_bucket_idx=(m_selected_bucket_idx==bi)?-1:bi;
                }});
            }
            Text(chart_timeline.front().label,chart_x,plot_bot+3.0f,68.0f,9.0f,false,RGBF(0x7F93B0));
            if(count>=3)TextCenter(chart_timeline[count/2].label,chart_x+70.0f,plot_bot+3.0f,84.0f,9.0f,false,RGBF(0x7F93B0));
            if(count>=2)TextRight(chart_timeline.back().label,chart_x+chart_w,plot_bot+3.0f,68.0f,9.0f,false,RGBF(0x7F93B0));
        }

        int hovered_bucket=-1;
        if(m_bucket_zone_start>=0&&m_hover>=m_bucket_zone_start&&m_hover<m_bucket_zone_start+m_bucket_zone_count){
            hovered_bucket=m_hover-m_bucket_zone_start;
        }
        const int inspect_idx=hovered_bucket>=0?hovered_bucket
            :(m_selected_bucket_idx>=0&&m_selected_bucket_idx<static_cast<int>(chart_timeline.size())?m_selected_bucket_idx:peak_idx);

        m_renderer->FillRoundedRect(D2D1::RectF(562,374,786,418),6.0f,RGBF(0x0B1019));
        m_renderer->DrawRoundedRect(D2D1::RectF(562,374,786,418),6.0f,RGBF(0x223046),1.0f);
        if(inspect_idx>=0&&inspect_idx<static_cast<int>(chart_timeline.size())){
            const auto& b=chart_timeline[inspect_idx];
            const std::wstring tag=hovered_bucket>=0?L"Window: ":(m_selected_bucket_idx>=0?L"Selected: ":L"Peak: ");
            Text(Ellipsize(tag+b.range_label,148.0f,10.5f,true),570,379,150,10.5f,true,RGBF(0xE8F0FF));
            TextRight(FormatMemory(b.Combined()),779,379,66,10.5f,true,RGBF(0xFFFFFF));
            Text(L"\u2193 "+FormatMemory(b.download),570,397,100,10.0f,true,RGBF(0x78BCFF));
            Text(L"\u2191 "+FormatMemory(b.upload),676,397,100,10.0f,true,RGBF(0x91DFC1));
        }else{
            TextCenter(report.ready?L"No traffic recorded in this time window.":L"Loading timeline...",566,389,216,10.0f,false,RGBF(0x8E9DB4));
        }

        // Bottom inspector: Live active Windows services or Peak/Last Active timestamps
        m_renderer->FillRoundedRect(D2D1::RectF(562,424,786,474),6.0f,RGBF(0x0C121C));
        m_renderer->DrawRoundedRect(D2D1::RectF(562,424,786,474),6.0f,RGBF(0x1D283A),1.0f);
        if(selected_entry&&selected_entry->category!=AppUsageCategory::Service){
            Text(L"ACTIVITY TIMESTAMPS",570,429,208,9.0f,true,RGBF(0x91A0B9));
            Text(Ellipsize(L"Peak: "+(selected_entry->peak_when.empty()?L"\u2014":selected_entry->peak_when),210.0f,10.0f,false),570,443,210,10.0f,false,RGBF(0xD5E1F2));
            Text(Ellipsize(L"Last active: "+(selected_entry->last_when.empty()?L"\u2014":selected_entry->last_when),210.0f,10.0f,false),570,457,210,10.0f,false,RGBF(0x8DC8FF));
        }else{
            Text(std::format(L"ACTIVE OS SERVICES NOW ({})",report.active_services.size()),570,429,210,9.0f,true,RGBF(0x91DFC1));
            std::wstring line1,line2;
            for(const auto& svc:report.active_services){
                const std::wstring candidate=line1.empty()?svc:(line1+L", "+svc);
                m_renderer->SetFont(L"Segoe UI",9.5f,false);
                if(line2.empty()&&m_renderer->MeasureText(candidate).width<=210.0f){
                    line1=candidate;
                }else{
                    const std::wstring c2=line2.empty()?svc:(line2+L", "+svc);
                    if(m_renderer->MeasureText(c2).width<=210.0f)line2=c2;
                    else{if(!line2.empty())line2+=L", ...";break;}
                }
            }
            if(line1.empty())line1=L"No active service sockets detected right now.";
            Text(Ellipsize(line1,210.0f,9.5f,false),570,443,210,9.5f,false,RGBF(0xC5D1E0));
            if(!line2.empty())Text(Ellipsize(line2,210.0f,9.5f,false),570,457,210,9.5f,false,RGBF(0x8FA4C2));
            else if(selected_entry&&!selected_entry->last_when.empty())Text(Ellipsize(L"Last active: "+selected_entry->last_when,210.0f,9.5f,false),570,457,210,9.5f,false,RGBF(0x8DC8FF));
        }

        // Bottom utility strip (no clipped text!)
        Card(206,488,592,36);
        const std::wstring footer_note = [&]() -> std::wstring {
            if (report.status == ReportStatus::Loading) return L"Querying Windows SRUM per-app & service usage...";
            if (report.status == ReportStatus::Failed) return L"Windows SRUM query unavailable \u2022 Check sensor status or permissions";
            if (report.status == ReportStatus::Stale) return L"Retaining prior report \u2022 Background update failed";
            if (report.status == ReportStatus::Partial) return L"Windows SRUM partial data \u2022 Some time slices unavailable";
            return L"Windows SRUM 24/7  \u2022  Hover or click chart bars to inspect";
        }();
        Text(Ellipsize(footer_note,286.0f,10.5f,false),220,498,288,10.5f,false,RGBF(0x91A0B9));
        PillButton(D2D1::RectF(514,493,588,519),L"Refresh",[this](){m_usage_force_refresh=true;},false,true,11.0f);
        PillButton(D2D1::RectF(594,493,692,519),L"Export CSV",[this](){
            if(!m_last_report.ready || m_last_report.status == ReportStatus::Loading || m_last_report.status == ReportStatus::Failed) {
                MessageBoxW(m_hwnd, L"Detailed network usage report is loading or unavailable. Please wait until data is ready before exporting.", L"Export CSV", MB_OK | MB_ICONINFORMATION);
                return;
            }
            wchar_t path[32768]=L"TrafficMonitor-app-usage.csv";
            OPENFILENAMEW dialog{sizeof(dialog)};dialog.hwndOwner=m_hwnd;
            dialog.lpstrFilter=L"CSV files\0*.csv\0\0";dialog.lpstrFile=path;
            dialog.nMaxFile=32768;dialog.lpstrDefExt=L"csv";
            dialog.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
            if(GetSaveFileNameW(&dialog)){
                const bool ok = on_export_detailed_usage ? on_export_detailed_usage(path, m_last_report) : false;
                if(!ok)MessageBoxW(m_hwnd,L"Could not export usage report. Check the selected folder.",L"Export",MB_OK|MB_ICONERROR);
                else m_status=L"Exported CSV report.";
            }
        },false,true,11.0f);
        PillButton(D2D1::RectF(698,493,788,519),L"Open folder",[this](){if(on_open_history)on_open_history();},false,true,11.0f);
    }else if(m_page==5){
        Section(110,186,L"Sign-in startup");
        Toggle(156,L"Start with Windows",m_startup_ready?L"Elevated sign-in startup is configured.":L"Sensor access may require administrator privileges.",m_draft.auto_start,226,550);
        Text(L"If the widget does not start at sign-in, repair its startup registration.",226,222,550,11,false,RGBF(0x91A0B9));
        Button(D2D1::RectF(226,250,410,282),L"Repair startup",[this](){if(on_repair_startup)on_repair_startup();m_startup_ready=startup_status&&startup_status();});
        Section(310,112,L"Application updates");
        Toggle(355,L"Check for updates",L"Check for a new version when the app starts.",m_draft.check_updates,226,550);
        Text(L"Sensor refresh frequency is in Performance & sensors.",226,451,550,11,false,RGBF(0x91A0B9));
    }else if(m_page==7){
        Section(110,108,L"Notifications");
        Toggle(154,L"Usage alerts",L"Notify when CPU, memory or temperature stays high.",m_draft.notifications_enabled,226,550);
        Section(232,202,L"Alert thresholds",L"CPU, RAM and temperature limits also set the red value colour.");
        Stepper(226,312,L"CPU usage %",m_draft.cpu_alert_threshold,1,100,5,258);
        Stepper(518,312,L"RAM usage %",m_draft.ram_alert_threshold,1,100,5,258);
        Stepper(226,370,L"Temperature °C",m_draft.temp_alert_threshold,1,150,5,258);
        Text(m_draft.notifications_enabled?L"Notifications are enabled. Apply & save to keep your changes.":L"Notifications are off. Your thresholds are retained for when you enable them.",226,460,550,11,false,RGBF(0x91A0B9));
        Button(D2D1::RectF(226,486,410,518),L"Value colours  >",[this](){Page(6);});
    }else if(m_page==8){
        Section(110,164,L"Refresh interval",L"Shorter intervals feel more responsive; longer intervals reduce background work.");
        const wchar_t* names[]={L"0.5 sec",L"0.7 sec",L"1 sec",L"2 sec",L"5 sec"};int intervals[]={500,700,1000,2000,5000};
        for(int i=0;i<5;++i)PillButton(D2D1::RectF(226.0f+i*112,184,330.0f+i*112,220),names[i],[this,i](){int values[]={500,700,1000,2000,5000};m_draft.poll_interval_ms=values[i];},m_draft.poll_interval_ms==intervals[i]);
        Text(L"Hardware sensor reads are throttled independently to limit overhead.",226,240,550,11,false,RGBF(0x91A0B9));
        Section(288,158,L"Sensor diagnostics",L"Inspect detected hardware and troubleshoot unavailable readings.");
        Button(D2D1::RectF(226,361,436,397),L"Open sensor status",[this](){if(on_sensor_status)on_sensor_status();});
        Text(L"Startup registration and repair are in Startup & updates.",226,415,550,11,false,RGBF(0x91A0B9));
    }else if(m_page==6){
        Section(110,126,L"Colour behaviour");
        Toggle(149,L"Automatic value colours",L"Gradually white to yellow, orange, then red. Labels remain white.",m_draft.thermal_colors,226,550);
        Button(D2D1::RectF(226,198,490,226),L"Choose fixed value colour",[this](){static COLORREF palette[16]{};CHOOSECOLORW c{sizeof(c)};c.hwndOwner=m_hwnd;c.lpCustColors=palette;c.rgbResult=RGB(255,255,255);c.Flags=CC_RGBINIT|CC_FULLOPEN;if(ChooseColorW(&c)){for(uint32_t i=0;i<DISPLAY_ITEM_COUNT;++i)m_draft.custom_value_colors[static_cast<DisplayItem>(i)]=c.rgbResult;m_draft.thermal_colors=false;}});
        Button(D2D1::RectF(504,198,776,226),L"Restore automatic colours",[this](){m_draft.custom_value_colors.clear();m_draft.thermal_colors=true;});
        Section(250,264,L"Red colour thresholds",L"CPU, RAM and temperature share the notification limits in Alerts.");
        Stepper(226,321,L"CPU %",m_draft.cpu_alert_threshold,1,100,5,258);
        Stepper(518,321,L"GPU %",m_draft.gpu_color_threshold,1,100,5,258);
        Stepper(226,365,L"RAM %",m_draft.ram_alert_threshold,1,100,5,258);
        Stepper(518,365,L"Temperature °C",m_draft.temp_alert_threshold,1,150,5,258);
        Stepper(226,409,L"Download MB/s",m_draft.download_color_threshold_mb,1,10000,5,258);
        Stepper(518,409,L"Upload MB/s",m_draft.upload_color_threshold_mb,1,10000,5,258);
        Stepper(226,453,L"Battery watts",m_draft.battery_color_threshold_w,1,500,5,258);
        Stepper(518,453,L"CPU clock GHz",m_draft.frequency_color_threshold_ghz,1,10,1,258);
        Text(std::format(L"Network: yellow 150 KB/s; orange download {:.1f} / upload {:.1f} MB/s.",m_draft.download_color_threshold_mb*.20,m_draft.upload_color_threshold_mb*.25),226,490,550,10.5f,false,RGBF(0x91A0B9));
    }
    m_renderer->DrawLine(D2D1::Point2F(206,534),D2D1::Point2F(798,534),RGBF(0x263349),1);
    Button(D2D1::RectF(206,550,338,581),L"Reset defaults",[this](){m_draft=Settings::GetDefaults();m_reset=true;RefreshOrder();SelectMetric(m_metric);PositionSliders();if(IsVisible()&&on_layout_preview)on_layout_preview(m_draft);});
    if(!m_status.empty())Text(Ellipsize(m_status,400,10,false),206,525,400,10,false,RGBF(0x74BDFF));
    if(HasUnsavedChanges()){Button(D2D1::RectF(438,550,541,581),L"Discard",[this](){Discard();});}
    Button(D2D1::RectF(555,550,658,581),L"Close",[this](){Hide();});Button(D2D1::RectF(672,548,798,583),L"Apply & save",[this](){Save();},true);
    if(auto* t=m_renderer->GetTarget())t->SetTransform(D2D1::Matrix3x2F::Identity());
    m_renderer->EndDraw();
    // Keep native controls above the Direct2D surface after a caption repaint
    // while a slider is being dragged.
    for(auto slider:m_layout_sliders)if(slider&&IsWindowVisible(slider))RedrawWindow(slider,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
}
}
