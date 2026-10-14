/* Presentation and controller focus only. File operations remain in main.c/backends. */
#define UI_LIST_ROW 64
#define UI_LIST_VIS 6
#define UI_VALUE_ROW 48
#define UI_VALUE_VIS 8
#define UI_MODAL_ROW 80
#define UI_MODAL_VIS 5
typedef struct { const char *label;int action,enabled; } UiButton;
typedef struct { const char *title,*note;int action; } UiTool;
static int g_ui_actions_focus,g_ui_action_index[NUM_TABS],g_ui_tool_index;
static int g_ui_context=-1,g_ui_confirm_choice,g_ui_report_scroll;
static UiRepeat g_ui_repeat;
static UiTool ui_tool(int i) {
    UiTool tools[]={
        {TR("剧情存档","Story Saves"),TR("选择内置剧情进度","Choose a story checkpoint"),ACT_LOAD_PRESET},
        {TR("存档备份","Save Backups"),TR("浏览和恢复历史快照","Browse saved snapshots"),ACT_LOAD_BACKUP},
        {TR("存档槽位","Save Slots"),TR("选择要使用的存档","Choose your active save"),ACT_PICK_SLOT},
        {TR("画质预设","Graphics Presets"),TR("应用预设画质配置","Choose a graphics preset"),ACT_GFX_PRESET},
        {TR("导入画质","Import Graphics"),TR("从 SD 卡读取配置","Use an XML from your SD"),ACT_GFX_IMPORT},
        {TR("检查路径","Check Paths"),TR("查看配置文件位置","Check configuration paths"),ACT_GFX_CHECK},
        {TR("自定义电台 NX","Self Radio NX"),TR("安装或还原电台","Install or restore Self Radio"),ACT_SELF_RADIO},
        {TR("电台导入","Radio Imports"),TR("安装配套的音频和时间数据","Install paired radio outputs"),ACT_RADIO_IMPORTS},
        {TR("保存修改","Save Changes"),TR("写入当前存档修改","Write your current save edits"),ACT_SAVE},
        {TR("保存并退出","Save & Exit"),TR("保存后关闭工具箱","Save edits and close toolbox"),ACT_QUIT}
    };
    return tools[i>=0 && i<10?i:0];
}
static int ui_buttons(UiButton *b) {
    int n=0;
#define UI_ADD(zh,en,act,ready) do {b[n++]=(UiButton){TR(zh,en),act,ready};} while(0)
    if(g_tab==TAB_DLC) {
        if(!g_dlc_xml_loaded){UI_ADD("载入并扫描","Load & Scan",ACT_DLC_LOAD,1);UI_ADD("待导入","Pending",ACT_DLC_VIEW,1);}
        else {
            int selected=g_dlc_view?g_dlc_src_count>0:g_dlc_count>0;
            UI_ADD("重新扫描","Rescan",ACT_DLC_SCAN_SRC,1);
            if(g_dlc_view){UI_ADD("导入","Import",ACT_DLC_IMPORT,selected);}
            else {UI_ADD("注册状态","Registration",ACT_DLC_TOGGLE,selected);}
            UI_ADD("切换列表","Switch List",ACT_DLC_VIEW,1);
            UI_ADD("完整性检查","Check Files",ACT_DLC_VERIFY,selected && !g_dlc_view);
            UI_ADD("转换格式","Convert",ACT_DLC_CONVERT,selected && !g_dlc_view);
            UI_ADD("隐藏高清贴图","Hide HD",ACT_DLC_HIDEHD,selected && !g_dlc_view);
            UI_ADD("删除","Delete",ACT_DLC_DELETE,selected && !g_dlc_view);
            UI_ADD("模型目录","Model Folder",ACT_OPEN_MODELDIR,1);
        }
    } else if(g_tab==TAB_SCRIPT) {
        UI_ADD("重新扫描","Rescan",ACT_SCRIPT_SCAN,1);
        UI_ADD("安装 / 替换","Install / Replace",ACT_SCRIPT_INSTALL,g_script_count>0);
        UI_ADD("删除","Delete",ACT_SCRIPT_UNINSTALL,g_script_count>0);
        UI_ADD("目录信息","Directory",ACT_SCRIPT_DIR,1);
        UI_ADD("模组管理","Mod Manager",ACT_SCRIPT_MGR,1);
    } else if(g_tab==TAB_GFX) {
        UI_ADD("画质预设","Presets",ACT_GFX_PRESET,1);
        UI_ADD("导入配置","Import XML",ACT_GFX_IMPORT,1);
        UI_ADD("检查路径","Check Paths",ACT_GFX_CHECK,1);
        UI_ADD("第一项","First Item",ACT_SCROLL_TOP,g_gfx_count>0);
        UI_ADD("最后一项","Last Item",ACT_SCROLL_BOTTOM,g_gfx_count>0);
        UI_ADD("保存","Save",ACT_SAVE,g_gfx_dirty);
    } else if(g_tab==TAB_PERF) {
        UI_ADD("载入","Load",ACT_GC_LOAD,1);
        UI_ADD("上一组","Prev Group",ACT_GC_GROUP_PREV,g_gc_opened);
        UI_ADD("下一组","Next Group",ACT_GC_GROUP_NEXT,g_gc_opened);
        UI_ADD("恢复原值","Restore",ACT_GC_RESTORE,g_gc_opened);
        UI_ADD("校验","Verify",ACT_GC_VERIFY,g_gc_opened);
        UI_ADD("写入 RPF","Write RPF",ACT_GC_WRITE,g_gc_opened && g_gc_dirty);
        UI_ADD("检查路径","Check Paths",ACT_GC_SELFTEST,1);
    }
#undef UI_ADD
    return n;
}
static int ui_report_active(void) {return g_gc_selftest_show || (g_tab==TAB_SCRIPT && g_script_dirinfo_on);}
static int ui_context_id(void) {
    if(g_modal_mode)return 100+g_modal_mode;
    if(g_gc_confirm)return 200+g_gc_confirm;
    if(g_dlc_confirm)return 300;
    if(ui_report_active())return 400+g_tab;
    return g_tab;
}
static void ui_context_sync(void) {
    int context=ui_context_id();
    if(context!=g_ui_context){g_ui_context=context;g_ui_actions_focus=0;g_ui_confirm_choice=0;g_ui_report_scroll=0;g_ui_repeat.held=0;}
}
static unsigned ui_direction(u32 keys) {
    if(keys&HidNpadButton_Up)return UI_UP;
    if(keys&HidNpadButton_Down)return UI_DOWN;
    if(keys&HidNpadButton_Left)return UI_LEFT;
    if(keys&HidNpadButton_Right)return UI_RIGHT;
    return 0;
}
static void ui_touch(int *action) {
    if(*action==ACT_UI_ACTIONS_PREV || *action==ACT_UI_ACTIONS_NEXT) {
        if(!g_modal_mode && !g_gc_confirm && !g_dlc_confirm && !ui_report_active()) {
            UiButton buttons[10];int count=ui_buttons(buttons),*index=&g_ui_action_index[g_tab];
            *index+=*action==ACT_UI_ACTIONS_NEXT?6:-6;
            if(*index<0)*index=0;
            if(*index>=count)*index=count?count-1:0;
            g_ui_actions_focus=count>0;
        }
        *action=ACT_NONE;
    }
    int direction=*action==ACT_UI_PAGE_UP?-1:*action==ACT_UI_PAGE_DOWN?1:0;
    if(direction) {
        int count=0,visible=UI_MODAL_VIS,*selected=&g_modal_sel;
        if(ui_report_active()){g_ui_report_scroll+=direction*18;if(g_ui_report_scroll<0)g_ui_report_scroll=0;}
        else {
            if(g_modal_mode==1)count=NUM_PRESETS;
            else if(g_modal_mode==2)count=g_num_backups;
            else if(g_modal_mode==3)count=g_num_slots;
            else if(g_modal_mode==4)count=NUM_GFX_PRESETS+1;
            else if(g_modal_mode==5)count=g_gfx_switch_count;
            else if(g_modal_mode==6)count=g_check_paths_count;
            else if(g_modal_mode==MODE_SCRIPT_MGR)count=script_mgr_total();
            else if(g_modal_mode==MODE_PACKAGES)count=g_package_count;
            else if(!g_modal_mode && !g_gc_confirm && !g_dlc_confirm) {
                if(g_tab==TAB_DLC){count=g_dlc_view?g_dlc_src_count:g_dlc_count;selected=g_dlc_view?&g_dlc_src_sel:&g_dlc_sel;visible=UI_LIST_VIS;}
                if(g_tab==TAB_SCRIPT){count=g_script_count;selected=&g_script_sel;visible=UI_LIST_VIS;}
                if(g_tab==TAB_PERF){count=GC_NUM_KNOB_META;selected=&g_gc_knob;visible=UI_VALUE_VIS;}
                if(g_tab==TAB_GFX){int item=g_gfx_cur_idx+direction*UI_VALUE_VIS;if(item<0)item=0;if(item>=g_gfx_count)item=g_gfx_count?g_gfx_count-1:0;g_gfx_cur_idx=(char)item;*action=ACT_NONE;return;}
            }
            if(count){*selected+=direction*visible;if(*selected<0)*selected=0;if(*selected>=count)*selected=count-1;}
        }
        *action=ACT_NONE;
    }
    if(*action==ACT_UI_DECREASE || *action==ACT_UI_INCREASE) {
        if(!g_modal_mode && !g_gc_confirm && !g_dlc_confirm && !ui_report_active()) {
            int change=*action==ACT_UI_INCREASE?1:-1;
            if(g_tab==TAB_GFX && g_gfx_count)gfx_step(change);
            if(g_tab==TAB_PERF && g_gc_opened)gc_knob_step(change);
            g_ui_actions_focus=0;
        }
        *action=ACT_NONE;
    }
}
/* Route controller activation through the same UiAction switch as touch. */
static void ui_controller(u32 *keys,int *action,int *param) {
    ui_context_sync();
    if(*action!=ACT_NONE) {
        *keys=0; /* A touch action owns this frame; do not also activate a pad action. */
        if(g_tab==TAB_TOOL && !g_modal_mode)for(int i=0;i<10;++i)if(ui_tool(i).action==*action)g_ui_tool_index=i;
        if(*action==ACT_BACK && ui_report_active()){g_gc_selftest_show=g_script_dirinfo_on=0;*action=ACT_NONE;*keys=0;}
        return;
    }
    if(ui_report_active()) {
        if(*keys&HidNpadButton_Up){if(g_ui_report_scroll>0)--g_ui_report_scroll;}
        if(*keys&HidNpadButton_Down)++g_ui_report_scroll;
        if(*keys&(HidNpadButton_B|HidNpadButton_A)){g_gc_selftest_show=g_script_dirinfo_on=0;}
        *keys=0;return;
    }
    if(g_modal_mode==MODE_SCRIPT_CONFIRM || g_modal_mode==MODE_PACKAGE_CONFIRM || g_gc_confirm || g_dlc_confirm) {
        if(*keys&(HidNpadButton_Left|HidNpadButton_Right))g_ui_confirm_choice=1-g_ui_confirm_choice;
        if((*keys&HidNpadButton_A) && g_ui_confirm_choice){*keys&=~HidNpadButton_A;*keys|=HidNpadButton_B;}
        if(*keys&HidNpadButton_B)*keys&=~HidNpadButton_A;
        return;
    }
    if(g_modal_mode){if(*keys&HidNpadButton_B)*keys&=~(HidNpadButton_A|HidNpadButton_X|HidNpadButton_Y);return;}
    UiButton buttons[10];int count=ui_buttons(buttons);
    if((*keys&HidNpadButton_Minus) && count){g_ui_actions_focus=!g_ui_actions_focus;*keys=0;return;}
    if(g_ui_actions_focus) {
        u32 navigation=*keys&(HidNpadButton_L|HidNpadButton_R|HidNpadButton_Plus);
        int *index=&g_ui_action_index[g_tab];if(*index>=count)*index=0;
        if(*keys&HidNpadButton_B){g_ui_actions_focus=0;*keys=0;return;}
        if(*keys&HidNpadButton_Left)*index=(*index+count-1)%count;
        if(*keys&HidNpadButton_Right)*index=(*index+1)%count;
        if((*keys&HidNpadButton_A) && buttons[*index].enabled){*action=buttons[*index].action;*param=0;}
        *keys=navigation;return;
    }
    if(*keys&HidNpadButton_B){*keys=HidNpadButton_B;return;}
    if(g_tab==TAB_TOOL) {
        g_ui_tool_index=ui_grid_move(g_ui_tool_index,10,4,ui_direction(*keys));
        if(*keys&HidNpadButton_A)*action=ui_tool(g_ui_tool_index).action;
        if(*keys&HidNpadButton_X)*action=ACT_SELF_RADIO;
        if(*keys&HidNpadButton_Y)*action=ACT_RADIO_IMPORTS;
        if(*action!=ACT_NONE){*param=0;*keys=0;}
        else *keys&=~(HidNpadButton_Up|HidNpadButton_Down|HidNpadButton_Left|HidNpadButton_Right);
    }
}
static void ui_panel(uint32_t *fb,int x,int y,int w,int h) {gfx_card(fb,x,y,w,h,C_CARD,8);}
static void ui_button(uint32_t *fb,int x,int y,int w,int h,const char *label,int action,int focused,int enabled) {
    gfx_card(fb,x,y,w,h,focused?C_ACCENT:C_CARD,8);
    if(focused)gfx_card(fb,x+2,y+2,w-4,h-4,C_CARD_SEL,6);
    int size=17;while(size>13 && text_width(label,size)>w-24)--size;
    ui_text_bounds(x+10,y+6,w-20,h-12);
    uint32_t label_color=!enabled?C_TEXT_MUTED:(action==ACT_DLC_DELETE || action==ACT_SCRIPT_UNINSTALL)?C_RED:focused?C_ACCENT:C_TEXT;
    draw_text_c(fb,x+w/2,y+(h-size)/2-2,size,label,label_color);
    ui_text_reset();if(enabled)hot_add(x,y,w,h,action,0);
}
static void ui_scrollbar(uint32_t *fb,int x,int y,int h,int count,int visible,int scroll) {
    if(count<=visible)return;
    int thumb=h*visible/count;if(thumb<24)thumb=24;
    int top=y+(h-thumb)*scroll/(count-visible);
    gfx_card(fb,x,y,6,h,C_CARD,3);gfx_card(fb,x,top,6,thumb,C_ACCENT,3);
    hot_add(x-14,y,44,h/2,ACT_UI_PAGE_UP,0);
    hot_add(x-14,y+h/2,44,h-h/2,ACT_UI_PAGE_DOWN,0);
}
static void ui_value_buttons(uint32_t *fb,int y) {
    int x=LY_CONTENT_R-104;
    gfx_card(fb,x,y+2,40,39,C_HEADER,6);gfx_minus_sign(fb,x+20,y+21,7,2,C_ACCENT);
    hot_add(x,y+2,40,39,ACT_UI_DECREASE,0);
    gfx_card(fb,x+48,y+2,40,39,C_HEADER,6);
    gfx_plus_sign(fb,x+68,y+21,7,2,C_ACCENT);hot_add(x+48,y+2,40,39,ACT_UI_INCREASE,0);
}
static void ui_value_text(uint32_t *fb,int y,const char *text,int selected) {
    int width=text_width(text,17);if(width>180)width=180;
    draw_text_fit(fb,LY_CONTENT_R-132-width,y+10,17,180,text,selected?C_ACCENT:C_TEXT_MUTED);
}
static void ui_row(uint32_t *fb,int x,int y,int w,int h,const char *title,const char *detail,const char *badge,uint32_t status,int selected,int action,int param) {
    gfx_card(fb,x,y,w,h,selected?C_CARD_SEL:C_CARD,8);
    if(selected)gfx_accent_bar(fb,x+5,y+9,h-18,C_ACCENT);
    int reserve=badge && *badge?text_width(badge,14)+40:20;if(reserve>280)reserve=280;
    ui_text_bounds(x+18,y+6,w-36,h-12);
    draw_text_fit(fb,x+20,y+8,19,w-40-reserve,title,C_TEXT);
    if(detail)draw_text_fit(fb,x+20,y+35,14,w-40,detail,C_TEXT_MUTED);
    if(badge && *badge)draw_text_r(fb,x+w-20,y+11,14,badge,status);
    ui_text_reset();if(action!=ACT_NONE)hot_add(x,y,w,h,action,param);
}
static void ui_empty(uint32_t *fb,const char *title,const char *description,const char *path) {
    ui_panel(fb,LY_PAD,LY_BODY_Y,FB_WIDTH-2*LY_PAD,240);
    draw_text_fit(fb,LY_PAD+24,LY_BODY_Y+24,24,FB_WIDTH-2*LY_PAD-48,title,C_TEXT);
    draw_text_wrap(fb,LY_PAD+24,LY_BODY_Y+70,FB_WIDTH-2*LY_PAD-48,18,3,description,C_TEXT_MUTED);
    if(path)draw_text_wrap(fb,LY_PAD+24,LY_BODY_Y+167,FB_WIDTH-2*LY_PAD-48,15,2,path,C_ACCENT);
}
static void ui_header(uint32_t *fb,const char *title,const char *subtitle,int selected,int count) {
    gfx_rect(fb,0,0,FB_WIDTH,108,C_HEADER);
    draw_text(fb,LY_PAD,12,25,"GTA V Switch Toolbox",C_TEXT);
    draw_text_r(fb,FB_WIDTH-LY_PAD,18,14,TR("界面测试版","UI Test"),C_ACCENT);
    int width=(FB_WIDTH-2*LY_PAD-32)/NUM_TABS;
    for(int i=0;i<NUM_TABS;++i) {
        int x=LY_PAD+i*(width+8),active=g_tab==TABS[i].id;
        gfx_card(fb,x,58,width,42,active?C_CARD_SEL:C_HEADER,8);
        draw_text_c(fb,x+width/2,67,18,TABS[i].name,active?C_ACCENT:C_TEXT_MUTED);
        if(active)gfx_rect(fb,x+20,97,width-40,3,C_ACCENT);
        hot_add(x,58,width,42,TABS[i].action,0);
    }
    draw_text_fit(fb,LY_PAD,117,22,FB_WIDTH-2*LY_PAD-200,title,C_TEXT);
    draw_text_fit(fb,LY_PAD,148,14,FB_WIDTH-2*LY_PAD,subtitle,C_TEXT_MUTED);
    if(count>0){char position[64];snprintf(position,sizeof(position),"%d / %d",selected+1,count);draw_text_r(fb,FB_WIDTH-LY_PAD,126,14,position,C_TEXT_MUTED);}
}
static void ui_actions(uint32_t *fb) {
    UiButton b[10];int count=ui_buttons(b);if(!count)return;
    int index=g_ui_action_index[g_tab];if(index>=count)index=0;
    int visible=count<6?count:6,first=index>=visible?index-visible+1:0;
    int arrows=count>visible?44:0;
    int width=(FB_WIDTH-2*LY_PAD-2*arrows-(visible-1)*10)/visible;
    for(int i=0;i<visible;++i)ui_button(fb,LY_PAD+arrows+i*(width+10),LY_BTN_Y,width,LY_BTN_H,b[first+i].label,b[first+i].action,g_ui_actions_focus && index==first+i,b[first+i].enabled);
    if(arrows) {
        gfx_card(fb,LY_PAD,LY_BTN_Y,36,LY_BTN_H,C_CARD,8);
        gfx_tri(fb,LY_PAD+18,LY_BTN_Y+25,7,2,first?C_ACCENT:C_TEXT_MUTED);
        if(first)hot_add(LY_PAD,LY_BTN_Y,44,LY_BTN_H,ACT_UI_ACTIONS_PREV,0);
        int x=FB_WIDTH-LY_PAD-36;gfx_card(fb,x,LY_BTN_Y,36,LY_BTN_H,C_CARD,8);
        gfx_tri(fb,x+18,LY_BTN_Y+25,7,3,first+visible<count?C_ACCENT:C_TEXT_MUTED);
        if(first+visible<count)hot_add(x-8,LY_BTN_Y,44,LY_BTN_H,ACT_UI_ACTIONS_NEXT,0);
    }
}
static void ui_footer(uint32_t *fb) {
    const char *message=g_status_msg;uint32_t color=g_status_color;int dirty=g_gfx_dirty || g_dirty;
    if(g_tab==TAB_PERF){message=g_gc_msg;color=g_gc_msg_color;dirty=g_gc_dirty;}
    else if(g_tab==TAB_DLC || g_tab==TAB_SCRIPT || g_tab==TAB_TOOL){if(*g_dlc_msg){message=g_dlc_msg;color=g_dlc_msg_ok?C_GREEN:C_RED;}dirty=g_tab==TAB_DLC?g_dlc_dirty:0;}
    if(!*message)message=TR("选择一项开始操作","Select an item to begin");
    int x=LY_PAD;
    if(dirty)x=draw_text(fb,x,LY_HINT_Y,15,TR("未保存  ","Unsaved  "),C_GOLD);
    draw_text_fit(fb,x,LY_HINT_Y,15,FB_WIDTH-LY_PAD-x,message,color);
    const char *hint=g_ui_actions_focus?TR("左右 选择按钮   A 执行   B 返回列表   L/R 换页","Left/Right choose action   A run   B return to list   L/R tabs"):
        g_tab==TAB_TOOL?TR("方向键 / 摇杆 选择   A 打开   B 返回   X 自定义电台   Y 电台导入   L/R 换页","D-pad / Stick navigate   A open   B back   X Self Radio   Y Radio Imports   L/R tabs"):
        (g_tab==TAB_GFX || g_tab==TAB_PERF)?TR("上下 选择   左右 / ZL/ZR 调值   - 操作按钮   B 返回   L/R 换页","Up/Down select   Left/Right or ZL/ZR adjust   - actions   B back   L/R tabs"):
        TR("上下 选择   A 执行   - 操作按钮   B 返回   L/R 换页","Up/Down select   A run   - actions   B back   L/R tabs");
    draw_text_fit(fb,LY_PAD,LY_HINT_Y+28,14,FB_WIDTH-2*LY_PAD,hint,C_TEXT_MUTED);
}
static void ui_draw_tools(uint32_t *fb) {
    char note[180];snprintf(note,sizeof(note),TR("%d 个存档槽位  /  %d 个备份","%d save slots  /  %d backups"),g_num_slots,g_num_backups);
    ui_header(fb,TR("工具","Tools"),note,0,0);
    int width=(FB_WIDTH-2*LY_PAD-36)/4;
    for(int i=0;i<10;++i) {
        UiTool tool=ui_tool(i);int x=LY_PAD+(i%4)*(width+12),y=LY_BODY_Y+(i/4)*112,selected=i==g_ui_tool_index;
        gfx_card(fb,x,y,width,100,selected?C_ACCENT:C_CARD,8);
        if(selected){gfx_card(fb,x+2,y+2,width-4,96,C_CARD_SEL,6);gfx_accent_bar(fb,x+6,y+14,72,C_ACCENT);}
        draw_text_fit(fb,x+20,y+19,19,width-40,tool.title,selected?C_ACCENT:C_TEXT);
        draw_text_wrap(fb,x+20,y+52,width-40,14,2,tool.note,C_TEXT_MUTED);
        hot_add(x,y,width,100,tool.action,0);
    }
    draw_text_fit(fb,LY_PAD,550,14,FB_WIDTH-2*LY_PAD,g_save_path,C_TEXT_MUTED);
}
static void ui_draw_dlc(uint32_t *fb) {
    int pending=g_dlc_view,count=pending?g_dlc_src_count:g_dlc_count,*selected=pending?&g_dlc_src_sel:&g_dlc_sel,*scroll=pending?&g_dlc_src_scr:&g_dlc_scr;
    *selected=ui_viewport(count,*selected,scroll,UI_LIST_VIS);
    ui_header(fb,pending?TR("待导入 DLC","Pending DLC"):TR("已安装 DLC","Installed DLC"),
        pending?TR("从 SD 卡安装包目录中选择要导入的内容","Choose a package folder from your SD card"):TR("管理游戏中的 DLC 安装与注册状态","Manage installed packages and registration"),*selected,count);
    if(!g_dlc_xml_loaded){ui_empty(fb,TR("载入 DLC 列表","Load your DLC library"),TR("按 A 载入并扫描。将待安装的 dlcpack 文件夹（含 dlc.rpf）放到下方目录。","Press A to load and scan. Put new dlcpack folders containing dlc.rpf in the directory below."),"sdmc:/switch/gta5save/dlc/");return;}
    if(!count){ui_empty(fb,TR("此列表为空","Nothing here yet"),pending?TR("复制 dlcpack 文件夹后，使用重新扫描。","Copy a dlcpack folder, then use Rescan."):TR("可以切换到待导入列表添加 DLC。","Switch to the pending list to import a DLC."),pending?"sdmc:/switch/gta5save/dlc/":DLC_DLCPACKS);return;}
    for(int row=0;row<UI_LIST_VIS && *scroll+row<count;++row) {
        int i=*scroll+row;DlcItem *item=pending?&g_dlc_src[i]:&g_dlc_items[i];char detail[260],size[32];fmt_size(size,sizeof(size),item->rpf_size);
        snprintf(detail,sizeof(detail),TR("%s  /  %s  /  模型 %d  地图 %d  贴图 %d%s%s","%s  /  %s  /  %d models  %d maps  %d textures%s%s"),
            item->name,size,item->n_model,item->n_map,item->n_tex,*item->spawn?"  /  ":"",item->spawn);
        const char *badge=pending?TR("可导入","Pending"):item->full?TR("已注册","Registered"):(item->registered || item->mounted)?TR("部分注册","Partial"):TR("未注册","Not registered");
        uint32_t status=pending?C_ACCENT:item->full?C_GREEN:(item->registered || item->mounted)?C_GOLD:C_TEXT_MUTED;
        if(item->map_st==2){badge=TR("地图数据缺失","Map metadata missing");status=C_RED;}
        ui_row(fb,LY_PAD,LY_BODY_Y+row*UI_LIST_ROW,LY_CONTENT_R-LY_PAD,UI_LIST_ROW-6,*item->cn?item->cn:item->name,detail,badge,status,i==*selected,ACT_DLC_PICK,i);
    }
    ui_scrollbar(fb,LY_CONTENT_R+8,LY_BODY_Y,LY_BODY_H-6,count,UI_LIST_VIS,*scroll);
}
static void ui_draw_scripts(uint32_t *fb) {
    g_script_sel=ui_viewport(g_script_count,g_script_sel,&g_script_scr,UI_LIST_VIS);
    ui_header(fb,TR("脚本","Scripts"),g_script_rpf_n<0?TR("无法读取游戏档案，已安装状态暂不可用","Game archive unavailable; installed status cannot be checked"):TR("选择脚本安装，或从模组管理打开内置模组","Install a script or open Mod Manager for bundled mods"),g_script_sel,g_script_count);
    if(!g_script_count){ui_empty(fb,TR("还没有脚本","No scripts yet"),TR("将 .nsc 文件复制到下方目录后重新扫描。使用模组管理可安装工具内置的模组。","Copy .nsc files to the directory below, then Rescan. Mod Manager also includes bundled mods."),SCRIPT_DIR);return;}
    for(int row=0;row<UI_LIST_VIS && g_script_scr+row<g_script_count;++row) {
        int i=g_script_scr+row;ScriptItem *item=&g_script_items[i];char detail[120],size[32];fmt_size(size,sizeof(size),item->size);
        snprintf(detail,sizeof(detail),"%s%s",size,item->from_alt?TR("  /  兼容目录","  /  alternate folder"):"");
        const char *badge=!script_file_usable(item->size)?TR("文件无效","Invalid file"):g_script_rpf_n<0?TR("状态未知","Unknown"):item->installed?TR("已安装","Installed"):TR("未安装","Not installed");
        uint32_t status=!script_file_usable(item->size) || g_script_rpf_n<0?C_RED:item->installed?C_GREEN:C_TEXT_MUTED;
        ui_row(fb,LY_PAD,LY_BODY_Y+row*UI_LIST_ROW,LY_CONTENT_R-LY_PAD,UI_LIST_ROW-6,item->name,detail,badge,status,i==g_script_sel,ACT_SCRIPT_INSTALL,1000+i);
    }
    ui_scrollbar(fb,LY_CONTENT_R+8,LY_BODY_Y,LY_BODY_H-6,g_script_count,UI_LIST_VIS,g_script_scr);
}
static void ui_draw_gfx(uint32_t *fb) {
    int selected=ui_viewport(g_gfx_count,g_gfx_cur_idx,&g_gfx_scroll,UI_VALUE_VIS);g_gfx_cur_idx=(char)selected;
    ui_header(fb,TR("画质设置","Graphics"),TR("上下选择设置，左右调整数值。修改后使用保存。","Choose a setting with Up/Down; adjust with Left/Right, then Save."),selected,g_gfx_count);
    if(!g_gfx_count){ui_empty(fb,TR("尚未载入配置","No configuration loaded"),TR("选择预设，或导入 settings.xml 来开始编辑。","Choose a preset or import settings.xml to start editing."),g_gfx_path);return;}
    for(int row=0;row<UI_VALUE_VIS && g_gfx_scroll+row<g_gfx_count;++row) {
        int i=g_gfx_scroll+row,y=LY_BODY_Y+row*UI_VALUE_ROW,chosen=i==selected;GfxItem *item=&g_gfx_items[i];
        gfx_card(fb,LY_PAD,y,LY_CONTENT_R-LY_PAD,UI_VALUE_ROW-5,chosen?C_CARD_SEL:C_CARD,8);
        if(chosen)gfx_accent_bar(fb,LY_PAD+5,y+8,UI_VALUE_ROW-21,C_ACCENT);
        draw_text_fit(fb,LY_PAD+20,y+9,18,LY_CONTENT_R-LY_PAD-360,item->cn,C_TEXT);
        ui_value_text(fb,y,item->value,chosen);
        hot_add(LY_PAD,y,LY_CONTENT_R-LY_PAD,UI_VALUE_ROW-5,ACT_PICK_ITEM,100000+i);
        if(chosen)ui_value_buttons(fb,y);
    }
    ui_scrollbar(fb,LY_CONTENT_R+8,LY_BODY_Y,LY_BODY_H-5,g_gfx_count,UI_VALUE_VIS,g_gfx_scroll);
}
static int ui_perf_value(int i) {
    static const u8 *source;static size_t length;static int values[GC_NUM_KNOB_META],was_dirty;
    if(source!=g_gc_xml || length!=g_gc_xml_len || (was_dirty && !g_gc_dirty)) {
        source=g_gc_xml;length=g_gc_xml_len;
        for(int k=0;k<GC_NUM_KNOB_META;++k) {
            int value=0;const char *key=GC_KNOB_META[k].key;
            if(gc_xml_get_int((char *)g_gc_xml,g_gc_xml_len,g_gc_sw_a,g_gc_sw_b,key,&value)!=GC_OK)
                gc_xml_get_int((char *)g_gc_xml,g_gc_xml_len,g_gc_any_a,g_gc_any_b,key,&value);
            values[k]=value;
        }
    }
    was_dirty=g_gc_dirty;int edit=gc_get_edited(GC_KNOB_META[i].key);return edit==INT32_MIN?values[i]:edit;
}
static void ui_draw_perf(uint32_t *fb) {
    g_gc_knob=ui_viewport(GC_NUM_KNOB_META,g_gc_knob,&g_gc_knob_scr,UI_VALUE_VIS);
    ui_header(fb,TR("性能配置","Performance Configuration"),TR("保留移植作者的配置基准；只有点击写入才保存修改。","Keep the porter's baseline in mind. Changes are saved only by Write RPF."),g_gc_opened?g_gc_knob:0,g_gc_opened?GC_NUM_KNOB_META:0);
    if(!g_gc_opened){ui_empty(fb,TR("载入 gameconfig.xml","Load gameconfig.xml"),g_gc_err?gc_rpf_last_error():TR("按 A 从现有 update.rpf 载入。检查路径可帮助定位档案。","Press A to load your existing update.rpf. Check Paths helps locate the archive."),DLC_UPDATERPF);return;}
    for(int row=0;row<UI_VALUE_VIS && g_gc_knob_scr+row<GC_NUM_KNOB_META;++row) {
        int i=g_gc_knob_scr+row,y=LY_BODY_Y+row*UI_VALUE_ROW,selected=i==g_gc_knob;const GcKnobMeta *item=&GC_KNOB_META[i];
        gfx_card(fb,LY_PAD,y,LY_CONTENT_R-LY_PAD,UI_VALUE_ROW-5,selected?C_CARD_SEL:C_CARD,8);
        if(selected)gfx_accent_bar(fb,LY_PAD+5,y+8,UI_VALUE_ROW-21,C_ACCENT);
        const char *label=g_lang_en && item->cn_en?item->cn_en:item->cn;
        char group[8];snprintf(group,sizeof(group),"[%s]",item->weight==1?"S":item->weight==2?"A":item->weight==3?"B":"C");
        uint32_t group_color=item->weight==1?C_GOLD:item->weight==2?C_ACCENT:item->weight==3?C_TEXT_MUTED:C_RED;
        draw_text(fb,LY_PAD+20,y+12,14,group,group_color);
        draw_text_fit(fb,LY_PAD+58,y+10,18,LY_CONTENT_R-LY_PAD-420,label,C_TEXT);
        char value[64],range[64];snprintf(value,sizeof(value),"%d",ui_perf_value(i));snprintf(range,sizeof(range),"%d - %d",item->lo,item->hi);
        draw_text_r(fb,LY_CONTENT_R-132,y+10,18,value,gc_get_edited(item->key)==INT32_MIN?C_TEXT:C_GOLD);
        int range_width=text_width(range,14);if(range_width>100)range_width=100;
        draw_text_fit(fb,LY_CONTENT_R-244-range_width,y+12,14,100,range,C_TEXT_MUTED);
        hot_add(LY_PAD,y,LY_CONTENT_R-LY_PAD,UI_VALUE_ROW-5,ACT_GC_PICK_KNOB,i);
        if(selected)ui_value_buttons(fb,y);
    }
    ui_scrollbar(fb,LY_CONTENT_R+8,LY_BODY_Y,LY_BODY_H-5,GC_NUM_KNOB_META,UI_VALUE_VIS,g_gc_knob_scr);
}
static void ui_dialog(uint32_t *fb,const char *title,const char *hint) {
    gfx_rect(fb,0,0,FB_WIDTH,FB_HEIGHT,C_BG);hot_reset();ui_text_reset();
    gfx_card(fb,64,32,FB_WIDTH-128,FB_HEIGHT-64,C_MODAL_BG,12);
    draw_text_fit(fb,92,54,25,FB_WIDTH-184,title,C_TEXT);
    draw_text_fit(fb,92,93,15,FB_WIDTH-184,hint,C_TEXT_MUTED);
}
static void ui_modal_footer(uint32_t *fb,int selected,int count,const char *action) {
    char count_text[80];snprintf(count_text,sizeof(count_text),"%d / %d",count?selected+1:0,count);
    draw_text(fb,92,584,14,count_text,C_TEXT_MUTED);
    ui_button(fb,92,622,220,44,action,ACT_CONFIRM,1,count>0);
    ui_button(fb,330,622,220,44,TR("B 返回","B Back"),ACT_BACK,0,1);
}
static void ui_confirm(uint32_t *fb,const char *title,const char *subject,const char *description,int confirm_action) {
    gfx_rect(fb,0,0,FB_WIDTH,FB_HEIGHT,C_BG);hot_reset();ui_context_sync();
    int x=248,w=784;gfx_card(fb,x,170,w,380,C_MODAL_BG,12);
    draw_text_fit(fb,x+32,197,25,w-64,title,C_TEXT);
    draw_text_wrap(fb,x+32,249,w-64,19,2,subject,C_ACCENT);
    draw_text_wrap(fb,x+32,320,w-64,17,3,description,C_TEXT_MUTED);
    ui_button(fb,x+32,464,340,50,TR("A 确认","A Confirm"),confirm_action,!g_ui_confirm_choice,1);
    ui_button(fb,x+392,464,360,50,g_ui_confirm_choice?TR("A 取消 / B 返回","A Cancel / B Back"):TR("B 取消","B Cancel"),ACT_BACK,g_ui_confirm_choice,1);
}
static void ui_draw_mod_manager(uint32_t *fb) {
    int stock=gc_script_stock_count(),count=stock+gc_script_builtin_count();
    ui_dialog(fb,TR("模组管理","Mod Manager"),TR("上下选择，A 安装或还原，B 返回。所有写入都需要确认。","Up/Down choose; A install or restore; B back. Writes require confirmation."));
    g_modal_sel=ui_viewport(count,g_modal_sel,&g_modal_scr,UI_MODAL_VIS);
    for(int row=0;row<UI_MODAL_VIS && g_modal_scr+row<count;++row) {
        int i=g_modal_scr+row,y=138+row*UI_MODAL_ROW;char detail[256];const char *title,*badge,*note=NULL;
        if(i<stock){title=gc_script_stock_name(i);badge=TR("还原官方","Restore Stock");snprintf(detail,sizeof(detail),"%s",TR("用内置原版替换此脚本","Replace this entry with the bundled original"));}
        else {const GcBuiltinMod *m=gc_script_builtin(i-stock);title=MOD_TITLE(m);badge=TR("安装模组","Install Mod");note=MOD_NOTE(m);snprintf(detail,sizeof(detail),TR("作者 %s  /  %d 个文件","By %s  /  %d files"),m->author,m->n_files);}
        ui_row(fb,92,y,FB_WIDTH-200,UI_MODAL_ROW-8,title,detail,badge,i<stock?C_ACCENT:C_GREEN,i==g_modal_sel,ACT_SCRIPT_MGR,i);
        if(note)draw_text_fit(fb,112,y+55,13,FB_WIDTH-240,note,C_TEXT_MUTED);
    }
    ui_scrollbar(fb,FB_WIDTH-98,138,392,count,UI_MODAL_VIS,g_modal_scr);
    if(*g_dlc_msg)draw_text_fit(fb,92,551,15,FB_WIDTH-184,g_dlc_msg,g_dlc_msg_ok?C_GREEN:C_RED);
    ui_modal_footer(fb,g_modal_sel,count,TR("A 选择","A Select"));
}
static void ui_draw_save_dialog(uint32_t *fb) {
    int mode=g_modal_mode,count=mode==1?(int)NUM_PRESETS:mode==2?g_num_backups:g_num_slots;
    ui_dialog(fb,mode==1?TR("剧情存档","Story Saves"):mode==2?TR("存档备份","Save Backups"):TR("存档槽位","Save Slots"),TR("选择要载入的存档。载入后可在工具页保存。","Choose a save to load. Save it from Tools when you are ready."));
    g_modal_sel=ui_viewport(count,g_modal_sel,&g_modal_scr,UI_MODAL_VIS);
    for(int row=0;row<UI_MODAL_VIS && g_modal_scr+row<count;++row) {
        int i=g_modal_scr+row;char title[256],detail[256];const char *badge=NULL;
        if(mode==1){snprintf(title,sizeof(title),"%s",PRESET_SAVE(i).name);snprintf(detail,sizeof(detail),"%s",PRESET_SAVE(i).desc);}
        else if(mode==2){snprintf(title,sizeof(title),"%s",*g_backups[i].title?g_backups[i].title:g_backups[i].dir_name);snprintf(detail,sizeof(detail),"%s  /  %zu KB  /  %s",g_backups[i].dir_name,g_backups[i].size/1024,g_backups[i].note);}
        else {snprintf(title,sizeof(title),"%s",*g_slots[i].title?g_slots[i].title:g_slots[i].filename);snprintf(detail,sizeof(detail),"%s  /  %zu KB",g_slots[i].filename,g_slots[i].size/1024);if(i==g_active_slot_idx)badge=TR("当前槽位","Active");}
        ui_row(fb,92,138+row*UI_MODAL_ROW,FB_WIDTH-200,UI_MODAL_ROW-8,title,detail,badge,C_GREEN,i==g_modal_sel,ACT_PICK_ITEM,i);
    }
    if(!count)draw_text_wrap(fb,112,178,FB_WIDTH-224,19,3,TR("没有可用的存档或备份。","No saves or backups are available."),C_TEXT_MUTED);
    ui_scrollbar(fb,FB_WIDTH-98,138,392,count,UI_MODAL_VIS,g_modal_scr);ui_modal_footer(fb,g_modal_sel,count,TR("A 载入","A Load"));
}
static void ui_draw_gfx_dialog(uint32_t *fb) {
    int mode=g_modal_mode,count=mode==4?(int)NUM_GFX_PRESETS+1:mode==5?g_gfx_switch_count:g_check_paths_count;
    ui_dialog(fb,mode==4?TR("画质预设","Graphics Presets"):mode==5?TR("导入画质配置","Import Graphics"):TR("配置路径","Configuration Paths"),mode==6?TR("上下浏览所有位置，B 返回。","Up/Down browse all locations; B back."):TR("上下选择，A 应用，B 返回。","Up/Down choose; A apply; B back."));
    g_modal_sel=ui_viewport(count,g_modal_sel,&g_modal_scr,UI_MODAL_VIS);
    for(int row=0;row<UI_MODAL_VIS && g_modal_scr+row<count;++row) {
        int i=g_modal_scr+row,y=138+row*UI_MODAL_ROW;const char *title,*detail=NULL;
        if(mode==4){title=i?GFX_PRESETS[i-1].name:TR("导入 XML 文件","Import an XML file");detail=i?GFX_PRESETS[i-1].desc:TR("从 /switch/gta5save/ 中选择文件","Choose a file from /switch/gta5save/");}
        else if(mode==5){title=g_gfx_switch_files[i];detail="sdmc:/switch/gta5save/";}
        else {title=TR("配置文件位置","Configuration location");detail=g_check_paths[i];}
        ui_row(fb,92,y,FB_WIDTH-200,UI_MODAL_ROW-8,title,mode==6?NULL:detail,NULL,C_TEXT,i==g_modal_sel,mode==6?ACT_PICK_ITEM:ACT_PICK_ITEM,i);
        if(mode==6)draw_text_wrap(fb,112,y+34,FB_WIDTH-240,14,2,detail,C_TEXT_MUTED);
    }
    if(!count)draw_text_wrap(fb,112,178,FB_WIDTH-224,19,3,TR("没有找到配置文件。将 XML 放入 /switch/gta5save/ 后重试。","No configuration files found. Copy an XML to /switch/gta5save/ and try again."),C_TEXT_MUTED);
    ui_scrollbar(fb,FB_WIDTH-98,138,392,count,UI_MODAL_VIS,g_modal_scr);
    if(mode==6)ui_button(fb,92,622,220,44,TR("B 返回","B Back"),ACT_BACK,1,1);
    else ui_modal_footer(fb,g_modal_sel,count,TR("A 应用","A Apply"));
}
static void ui_draw_packages(uint32_t *fb) {
    ui_dialog(fb,!strcmp(g_package_kind,"selfradio")?TR("自定义电台 NX","Self Radio NX"):TR("电台导入","Radio Imports"),TR("安装或还原前请关闭 GTA。原文件备份会保留。","Close GTA before installing or restoring. Original-file backups are retained."));
    g_modal_sel=ui_viewport(g_package_count,g_modal_sel,&g_modal_scr,UI_MODAL_VIS);
    for(int row=0;row<UI_MODAL_VIS && g_modal_scr+row<g_package_count;++row) {
        int i=g_modal_scr+row;char detail[80];snprintf(detail,sizeof(detail),TR("%d 个文件","%d files"),g_packages[i].count);
        ui_row(fb,92,138+row*UI_MODAL_ROW,FB_WIDTH-200,UI_MODAL_ROW-8,package_title(&g_packages[i]),detail,NULL,C_ACCENT,i==g_modal_sel,ACT_PACKAGE_SELECT,i);
    }
    if(!g_package_count)draw_text_wrap(fb,112,178,FB_WIDTH-224,19,4,TR("没有找到安装包。将准备好的文件夹复制到下方目录，再重新打开此页面。","No packages found. Copy a prepared folder to the directory below, then reopen this page."),C_TEXT_MUTED);
    if(*g_dlc_msg)draw_text_fit(fb,92,551,15,FB_WIDTH-184,g_dlc_msg,g_dlc_msg_ok?C_GREEN:C_RED);
    ui_scrollbar(fb,FB_WIDTH-98,138,392,g_package_count,UI_MODAL_VIS,g_modal_scr);
    draw_text_fit(fb,92,584,14,FB_WIDTH-320,GC_PACKAGE_HOME,C_TEXT_MUTED);
    char position[64];snprintf(position,sizeof(position),"%d / %d",g_package_count?g_modal_sel+1:0,g_package_count);
    draw_text_r(fb,FB_WIDTH-108,584,14,position,C_TEXT_MUTED);
    ui_button(fb,92,622,244,44,TR("A 安装","A Install"),ACT_PACKAGE_INSTALL,1,g_package_count>0);
    ui_button(fb,350,622,244,44,TR("X 还原","X Restore"),ACT_PACKAGE_RESTORE,0,g_package_count>0);
    ui_button(fb,608,622,244,44,TR("Y 恢复更新","Y Recover"),ACT_PACKAGE_RECOVER,0,1);
    ui_button(fb,866,622,244,44,TR("B 返回","B Back"),ACT_BACK,0,1);
}
static void ui_draw_report(uint32_t *fb) {
    const char *source=g_gc_selftest_show?g_gc_selftest:g_script_dirinfo;
    ui_dialog(fb,g_gc_selftest_show?TR("档案路径检查","Archive Path Check"):TR("脚本目录信息","Script Directory"),TR("上下滚动查看，A 或 B 返回。","Up/Down scroll; A or B back."));
    const char *s=source;int row=0,drawn=0,visible=18;
    int total=1;for(const char *p=source;*p;++p)if(*p=='\n')++total;
    int maximum=total>visible?total-visible:0;
    if(g_ui_report_scroll>maximum)g_ui_report_scroll=maximum;
    while(*s) {
        char line[768];const char *end=strchr(s,'\n');size_t length=end?(size_t)(end-s):strlen(s);
        if(length>=sizeof(line))length=sizeof(line)-1;
        memcpy(line,s,length);line[length]=0;
        if(row>=g_ui_report_scroll && drawn<visible){draw_text_fit(fb,92,136+drawn*24,16,FB_WIDTH-184,line,C_TEXT);++drawn;}
        ++row;if(!end)break;s=end+1;
    }
    ui_scrollbar(fb,FB_WIDTH-94,136,visible*24,total,visible,g_ui_report_scroll);
    ui_button(fb,92,622,220,44,TR("B 返回","B Back"),ACT_BACK,1,1);
}
static void ui_draw_app(uint32_t *fb) {
    ui_context_sync();ui_text_reset();hot_reset();gfx_rect(fb,0,0,FB_WIDTH,FB_HEIGHT,C_BG);
    if(g_modal_mode==MODE_SCRIPT_CONFIRM) {
        const GcBuiltinMod *m=g_mod_pending_kind?gc_script_builtin(g_mod_pending_idx):NULL;
        ui_confirm(fb,g_mod_pending_kind?TR("安装内置模组？","Install bundled mod?"):TR("还原官方脚本？","Restore stock script?"),g_mod_pending_kind?MOD_TITLE(m):gc_script_stock_name(g_mod_pending_idx),
            TR("此操作会替换对应的游戏脚本。安装完成后重新启动游戏。","This replaces the corresponding game scripts. Restart GTA after installation."),ACT_SCRIPT_CONFIRM_OK);return;
    }
    if(g_modal_mode==MODE_PACKAGE_CONFIRM) {
        ui_confirm(fb,g_package_restore==2?TR("恢复中断的更新？","Recover interrupted update?"):g_package_restore?TR("还原此安装包？","Restore this package?"):TR("安装此安装包？","Install this package?"),
            g_package_restore==2?TR("保留的恢复记录","Saved recovery record"):package_title(&g_packages[g_modal_sel]),
            TR("关闭 GTA 后继续。安装会保留原文件备份；SD 卡需要足够的临时空间。","Close GTA before continuing. Installation retains original-file backups and needs room for staged files."),ACT_CONFIRM);return;
    }
    if(g_gc_confirm){ui_confirm(fb,g_gc_confirm==1?TR("写入配置？","Write configuration?"):TR("恢复原配置？","Restore original configuration?"),"gameconfig.xml",TR("确认后执行所选操作。写入会更改 update.rpf 中的配置。","Confirm to perform the selected action. Writing changes the configuration inside update.rpf."),ACT_CONFIRM);return;}
    if(g_dlc_confirm){ui_confirm(fb,TR("删除 DLC？","Delete DLC?"),g_dlc_count?g_dlc_items[g_dlc_sel].name:"",TR("此操作会注销此 DLC 并移除它的文件夹。","This unregisters the DLC and removes its folder."),ACT_CONFIRM);return;}
    if(ui_report_active()){ui_draw_report(fb);return;}
    if(g_modal_mode>=1 && g_modal_mode<=3){ui_draw_save_dialog(fb);return;}
    if(g_modal_mode>=4 && g_modal_mode<=6){ui_draw_gfx_dialog(fb);return;}
    if(g_modal_mode==MODE_SCRIPT_MGR){ui_draw_mod_manager(fb);return;}
    if(g_modal_mode==MODE_PACKAGES){ui_draw_packages(fb);return;}
    if(g_tab==TAB_DLC)ui_draw_dlc(fb);
    else if(g_tab==TAB_SCRIPT)ui_draw_scripts(fb);
    else if(g_tab==TAB_GFX)ui_draw_gfx(fb);
    else if(g_tab==TAB_PERF)ui_draw_perf(fb);
    else ui_draw_tools(fb);
    ui_actions(fb);ui_footer(fb);
}
