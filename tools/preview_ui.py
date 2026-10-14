"""Render the actual C UI on Windows with FreeType; no game files are opened.

Requires MSVC, a devkitPro include tree, and freetype-py==2.5.1 for its host DLL.
Fixture data and host fonts differ from Switch data/fonts. This checks layout
and controller routing, not libnx or game runtime behavior.
"""
import argparse,ctypes,hashlib,json,os,re,shutil,struct,subprocess,zlib
from pathlib import Path
import freetype

def without_comments(text):
    pattern=r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*'
    return re.sub(pattern,lambda m:m[0] if m[0].startswith(('"',"'")) else '\n'*m[0].count('\n'),text,flags=re.S)

def end_brace(s,start):
    depth=0;quoted=None;i=start
    while i<len(s):
        c=s[i]
        if quoted:
            if c=='\\':i+=2;continue
            if c==quoted:quoted=None
        elif c in ['"',"'"]:quoted=c
        elif c=='{':depth+=1
        elif c=='}':
            depth-=1
            if depth==0:return i+1
        i+=1
    raise ValueError('Unbalanced source braces')

def split_source(source):
    source=without_comments(source);functions={};spans=[]
    pattern=r'(?m)^[A-Za-z_][\w* \t]*?\b(\w+)\s*\([^;{}]*?\)\s*\{'
    for match in re.finditer(pattern,source):
        if spans and match.start()<spans[-1][1]:continue
        end=end_brace(source,source.index('{',match.start(),match.end()))
        functions[match[1]]=source[match.start():end];spans.append((match.start(),end))
    parts=[];cursor=0
    for start,end in spans:parts.append(source[cursor:start]);cursor=end
    parts.append(source[cursor:]);return ''.join(parts),functions

FIXTURES=r'''
static int fixture_gfx_steps,fixture_perf_steps;
static int fixture_touch_down,fixture_touch_reads=1;
static int hidGetTouchScreenStates(HidTouchScreenState *state,int count) {
    (void)count;state->count=fixture_touch_down;state->touches[0].x=300;state->touches[0].y=490;return fixture_touch_reads;
}
static void gfx_step(int direction) {fixture_gfx_steps+=direction;}
static void gc_knob_step(int direction) {fixture_perf_steps+=direction;}
static const char *gc_script_stock_name(int index) {
    const char *names[]={"achievement_controller.nsc","error_listener.nsc","shop_controller.nsc"};return names[index%3];
}
static int gc_script_stock_count(void) {return 3;}
static int gc_script_builtin_count(void) {return 4;}
static const GcBuiltinMod *gc_script_builtin(int index) {
    static GcBuiltinMod mods[4];
    for(int i=0;i<4;++i){mods[i].title="\xE5\x86\x85\xE7\xBD\xAE\xE6\xA8\xA1\xE7\xBB\x84";mods[i].title_en="Bundled Gameplay Mod";mods[i].author="Original author";mods[i].note="";mods[i].note_en="Restore the original scripts to remove this mod.";mods[i].n_files=2;}
    return &mods[index%4];
}
static int gc_get_edited(const char *key) {(void)key;return INT32_MIN;}
static int gc_xml_get_int(const char *xml,size_t length,size_t start,size_t end,const char *key,int *out) {
    (void)xml;(void)length;(void)start;(void)end;(void)key;*out=42;return 0;
}
static const char *gc_rpf_last_error(void) {return "Archive not found. Use Check Paths to locate update.rpf.";}
'''

SEED=r'''
static void seed(int scene,int english) {
    g_lang_en=english;g_modal_mode=0;g_gc_confirm=g_dlc_confirm=g_gc_selftest_show=0;
    g_script_dirinfo_on=0;g_ui_actions_focus=0;g_ui_context=-1;g_ui_confirm_choice=0;
    memset(g_ui_action_index,0,sizeof(g_ui_action_index));
    fixture_gfx_steps=fixture_perf_steps=0;
    g_toast_tick=0;g_ui_tool_index=6;g_stride_words=FB_WIDTH;
    strcpy(g_save_path,"sdmc:/switch/gta5save/SGTA50000");g_num_slots=g_num_backups=12;g_active_slot_idx=0;
    for(int i=0;i<12;++i){snprintf(g_slots[i].filename,sizeof(g_slots[i].filename),"SGTA5%04d",i);snprintf(g_slots[i].title,sizeof(g_slots[i].title),"Story chapter %d",i+1);g_slots[i].size=512000;snprintf(g_backups[i].title,sizeof(g_backups[i].title),"Snapshot %d - a long story title",i+1);snprintf(g_backups[i].dir_name,sizeof(g_backups[i].dir_name),"2026-10-14-%02d",i);strcpy(g_backups[i].note,"Before installing a mod");g_backups[i].size=512000;}
    g_dlc_xml_loaded=1;g_dlc_count=g_dlc_src_count=24;g_dlc_view=0;g_dlc_sel=g_dlc_src_sel=7;g_dlc_scr=g_dlc_src_scr=0;
    for(int i=0;i<24;++i){DlcItem *d=&g_dlc_items[i];memset(d,0,sizeof(*d));snprintf(d->name,sizeof(d->name),"vehicle_pack_%02d_with_a_long_folder_name",i);snprintf(d->cn,sizeof(d->cn),"Custom vehicle collection %d",i+1);d->rpf_size=64000000;d->n_model=4;d->n_tex=8;d->full=i%3==0;d->registered=i%3==1;strcpy(d->spawn,"sportscar");g_dlc_src[i]=*d;}
    g_script_count=32;g_script_sel=8;g_script_scr=0;g_script_rpf_n=1027;
    for(int i=0;i<32;++i){ScriptItem *it=&g_script_items[i];snprintf(it->name,sizeof(it->name),"script_%02d_with_a_long_compiled_name.nsc",i);it->size=420000;it->installed=i%2;}
    g_gfx_count=40;g_gfx_cur_idx=9;g_gfx_scroll=0;g_gfx_dirty=1;
    for(int i=0;i<40;++i){GfxItem *it=&g_gfx_items[i];snprintf(it->cn,sizeof(it->cn),"Graphics setting %02d with a descriptive label",i+1);strcpy(it->value,i%2?"1.000000":"1920");}
    g_gc_opened=1;g_gc_xml=(u8 *)"<fixture/>";g_gc_xml_len=10;g_gc_dirty=1;g_gc_knob=8;g_gc_knob_scr=0;
    g_gfx_switch_count=14;g_check_paths_count=14;
    for(int i=0;i<14;++i){snprintf(g_gfx_switch_files[i],sizeof(g_gfx_switch_files[i]),"graphics_profile_%02d.xml",i+1);snprintf(g_check_paths[i],sizeof(g_check_paths[i]),"sdmc:/atmosphere/contents/0100b00b51230000/romfs/update/configuration_%02d/settings.xml : missing (creatable)",i+1);}
    g_package_kind="selfradio";g_package_count=12;
    for(int i=0;i<12;++i){snprintf(g_packages[i].title,sizeof(g_packages[i].title),"Self Radio NX v1.1 - package %d",i+1);strcpy(g_packages[i].title_zh,"\xE8\x87\xAA\xE5\xAE\x9A\xE4\xB9\x89\xE7\x94\xB5\xE5\x8F\xB0 NX v1.1");g_packages[i].count=18;}
    g_modal_sel=6;g_modal_scr=0;g_tab=scene<5?scene:TAB_TOOL;
    if(scene==5){g_modal_mode=MODE_SCRIPT_MGR;}
    if(scene==6){g_modal_mode=MODE_SCRIPT_CONFIRM;g_mod_pending_kind=1;g_mod_pending_idx=0;}
    if(scene>=7 && scene<=12)g_modal_mode=scene-6;
    if(scene==13)g_modal_mode=MODE_PACKAGES;
    if(scene==14){g_modal_mode=MODE_PACKAGE_CONFIRM;g_package_restore=0;}
    if(scene==15){g_tab=TAB_DLC;g_dlc_view=1;}
    if(scene==16){g_tab=TAB_DLC;g_dlc_xml_loaded=0;}
    if(scene==17){g_tab=TAB_SCRIPT;g_script_count=0;}
    if(scene==18){g_tab=TAB_GFX;g_gfx_count=0;}
    if(scene==19){g_tab=TAB_PERF;g_gc_opened=0;}
    if(scene==20){g_tab=TAB_PERF;g_gc_selftest_show=1;strcpy(g_gc_selftest,"Path check\n\nCandidate archive:\nsdmc:/atmosphere/contents/0100b00b51230000/romfs/update/update.rpf\n\nMissing archive\nUse your existing port archive, then reopen.");}
    if(scene==21){g_tab=TAB_DLC;g_dlc_confirm=1;}
    if(scene==22){g_tab=TAB_PERF;g_gc_confirm=1;}
    ui_context_sync();
    if(scene==23){g_tab=TAB_DLC;ui_context_sync();g_ui_actions_focus=1;g_ui_action_index[TAB_DLC]=7;}
}
static struct {uint32_t left[16],pixels[FB_WIDTH*FB_HEIGHT],right[16];} frame;
__declspec(dllexport) int preview_init(const char *font) {return FT_Init_FreeType(&g_ft) || FT_New_Face(g_ft,font,0,&g_face);}
__declspec(dllexport) const uint32_t *preview_render(int scene,int english) {
    seed(scene,english);
    for(int i=0;i<16;++i)frame.left[i]=frame.right[i]=0xabcdef01;
    ui_draw_app(frame.pixels);
    for(int i=0;i<16;++i)if(frame.left[i]!=0xabcdef01 || frame.right[i]!=0xabcdef01)return NULL;
    return frame.pixels;
}
__declspec(dllexport) int preview_hot_count(void) {return g_hot_count;}
__declspec(dllexport) const HotZone *preview_hotzones(void) {return g_hot;}
__declspec(dllexport) int preview_controller_checks(void) {
    int checks=0,action=ACT_NONE,param=0;u32 key;
#define CHECK(expr) do {if(!(expr))return -checks-1;++checks;} while(0)
    seed(4,1);key=HidNpadButton_A;ui_controller(&key,&action,&param);CHECK(action==ACT_SELF_RADIO && !key);
    seed(4,1);action=ACT_NONE;key=HidNpadButton_X;ui_controller(&key,&action,&param);CHECK(action==ACT_SELF_RADIO && !key && !g_modal_mode);
    seed(4,1);action=ACT_NONE;key=HidNpadButton_Y;ui_controller(&key,&action,&param);CHECK(action==ACT_RADIO_IMPORTS && !key);
    seed(4,1);action=ACT_NONE;key=HidNpadButton_Right;ui_controller(&key,&action,&param);CHECK(g_ui_tool_index==7 && !key && action==ACT_NONE);
    seed(3,1);action=ACT_NONE;key=HidNpadButton_Minus;ui_controller(&key,&action,&param);CHECK(g_ui_actions_focus && !key);
    key=HidNpadButton_Right;ui_controller(&key,&action,&param);CHECK(g_ui_action_index[3]==1 && !key);
    key=HidNpadButton_A;ui_controller(&key,&action,&param);CHECK(action==ACT_GC_GROUP_PREV && !key);
    action=ACT_NONE;key=HidNpadButton_B;ui_controller(&key,&action,&param);CHECK(!g_ui_actions_focus && !key && g_tab==TAB_PERF);
    seed(3,1);g_ui_actions_focus=1;key=HidNpadButton_R;ui_controller(&key,&action,&param);CHECK(key==HidNpadButton_R && g_ui_actions_focus);
    key=HidNpadButton_Plus;ui_controller(&key,&action,&param);CHECK(key==HidNpadButton_Plus);
    key=HidNpadButton_A|HidNpadButton_B;action=ACT_NONE;ui_controller(&key,&action,&param);CHECK(!key && action==ACT_NONE && !g_ui_actions_focus);
    seed(4,1);key=HidNpadButton_A|HidNpadButton_B;action=ACT_NONE;ui_controller(&key,&action,&param);CHECK(key==HidNpadButton_B && action==ACT_NONE);
    seed(13,1);key=HidNpadButton_A|HidNpadButton_B;action=ACT_NONE;ui_controller(&key,&action,&param);CHECK(key==HidNpadButton_B);
    seed(4,1);key=HidNpadButton_A;action=ACT_GFX_PRESET;ui_controller(&key,&action,&param);CHECK(!key && action==ACT_GFX_PRESET);
    seed(14,1);action=ACT_NONE;key=HidNpadButton_Right;ui_controller(&key,&action,&param);CHECK(g_ui_confirm_choice==1);
    key=HidNpadButton_A;ui_controller(&key,&action,&param);CHECK(!(key&HidNpadButton_A) && (key&HidNpadButton_B));
    seed(14,1);key=HidNpadButton_A|HidNpadButton_B;ui_controller(&key,&action,&param);CHECK(!(key&HidNpadButton_A));
    seed(20,1);key=HidNpadButton_Down;ui_controller(&key,&action,&param);CHECK(g_ui_report_scroll==1 && !key && g_gc_selftest_show);
    key=HidNpadButton_B;ui_controller(&key,&action,&param);CHECK(!key && !g_gc_selftest_show);
    seed(20,1);action=ACT_BACK;key=0;ui_controller(&key,&action,&param);CHECK(action==ACT_NONE && !g_gc_selftest_show && g_tab==TAB_PERF);
    seed(2,1);action=ACT_UI_PAGE_DOWN;ui_touch(&action);CHECK(g_gfx_cur_idx==17 && action==ACT_NONE);
    action=ACT_UI_PAGE_UP;ui_touch(&action);CHECK(g_gfx_cur_idx==9);
    action=ACT_UI_INCREASE;ui_touch(&action);CHECK(fixture_gfx_steps==1 && action==ACT_NONE);
    seed(3,1);action=ACT_UI_DECREASE;ui_touch(&action);CHECK(fixture_perf_steps==-1);
    seed(14,1);action=ACT_UI_INCREASE;ui_touch(&action);CHECK(!fixture_gfx_steps && !fixture_perf_steps);
    seed(13,1);action=ACT_UI_PAGE_DOWN;ui_touch(&action);CHECK(g_modal_sel==11 && action==ACT_NONE);
    seed(0,1);action=ACT_UI_ACTIONS_NEXT;ui_touch(&action);CHECK(g_ui_action_index[0]==6 && g_ui_actions_focus && action==ACT_NONE);
    action=ACT_UI_ACTIONS_NEXT;ui_touch(&action);CHECK(g_ui_action_index[0]==7);
    action=ACT_UI_ACTIONS_PREV;ui_touch(&action);CHECK(g_ui_action_index[0]==1);
    fixture_touch_reads=1;fixture_touch_down=0;touch_poll();CHECK(!g_touch_tapped);
    fixture_touch_down=1;touch_poll();CHECK(g_touch_tapped);
    fixture_tick+=10000000;touch_poll();CHECK(!g_touch_tapped);
    fixture_touch_reads=0;touch_poll();CHECK(!g_touch_tapped);
    fixture_touch_reads=1;touch_poll();CHECK(!g_touch_tapped);
    fixture_touch_down=0;touch_poll();fixture_touch_down=1;touch_poll();CHECK(g_touch_tapped);
    fixture_touch_down=0;touch_poll();
    return checks;
#undef CHECK
}
'''

def png(path,data):
    def chunk(kind,payload):return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload))
    rows=b''.join(b'\0'+data[i*1280*4:(i+1)*1280*4] for i in range(720))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',1280,720,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))

def main(a):
    root=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    source=(root/'source/main.c').read_text(encoding='utf-8');globals,functions=split_source(source)
    # Removed save-editor data is not used by any UI screen. Its old initializer
    # relies on GCC's brace flattening and is not accepted by MSVC.
    legacy=globals.find('static const BehaviorField SKILL_BEHAVIORS')
    if legacy>=0:
        end=end_brace(globals,globals.index('{',legacy));globals=globals[:legacy]+globals[end+1:]
    globals=re.sub(r'^#include ["<](?:unistd.h|zlib.h|gc_ui.inc|gc_ui_text.inc|gc_package_ui.inc)[">].*$', '',globals,flags=re.M)
    package_globals,package_functions=split_source((root/'source/gc_package_ui.inc').read_text(encoding='utf-8'))
    helpers=['hot_add','hot_reset','touch_poll','gfx_rect','gfx_rect_outline','sqrt_approx','gfx_card','gfx_accent_bar','gfx_tri','gfx_plus_sign','gfx_minus_sign','gfx_dot','fmt_size','script_file_usable','script_mgr_total']
    # Backend declarations are external; fixture implementations provide display data only.
    fixtures=FIXTURES.replace('static const char *gc_script_stock_name','const char *gc_script_stock_name').replace('static int gc_script_stock_count','int gc_script_stock_count').replace('static int gc_script_builtin_count','int gc_script_builtin_count').replace('static const GcBuiltinMod *gc_script_builtin','const GcBuiltinMod *gc_script_builtin').replace('static int gc_xml_get_int','int gc_xml_get_int').replace('static const char *gc_rpf_last_error','const char *gc_rpf_last_error')
    code=globals+'\n'+package_globals+'\n'+fixtures+'\n'+'\n'.join(functions[n] for n in helpers)+'\n#include "gc_ui_text.inc"\n'+package_functions['package_title']+'\n#include "gc_ui.inc"\n'+SEED
    generated=out/'preview.c';generated.write_text(code,encoding='utf-8')
    sdk=a.devkitpro.resolve();dll=Path(freetype.raw._lib._name);shutil.copy2(dll,out/dll.name)
    definition=out/'freetype.def';definition.write_text('LIBRARY '+dll.name+'\nEXPORTS\nFT_Init_FreeType\nFT_New_Face\nFT_Set_Pixel_Sizes\nFT_Load_Char\n')
    locator=Path(os.environ.get('ProgramFiles(x86)','C:/Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    install=subprocess.check_output([str(locator),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    setup=Path(install)/'VC/Auxiliary/Build/vcvars64.bat';cmd=out/'compile.cmd'
    cmd.write_text(f'@call "{setup}" >nul\n@lib /nologo /DEF:"{definition}" /OUT:"{out}/freetype.lib" /MACHINE:X64\n@cl /nologo /LD /O2 /MD /std:c11 /utf-8 /D_CRT_SECURE_NO_WARNINGS /FI"{root}/tests/support/ui_platform.h" /I"{root}/tests/support" /I"{root}/source" /I"{sdk}/portlibs/switch/include/freetype2" /I"{sdk}/portlibs/switch/include" "{generated}" "{root}/source/gc_ui_core.c" /link "{out}/freetype.lib" /OUT:"{out}/preview.dll"\n')
    run=subprocess.run(['cmd','/c',str(cmd)],cwd=out,capture_output=True,text=True,errors='replace');(out/'compile.log').write_text(run.stdout+run.stderr,encoding='utf-8')
    if run.returncode:raise RuntimeError('Host preview compile failed; see '+str(out/'compile.log'))
    os.add_dll_directory(str(out));library=ctypes.CDLL(str(out/'preview.dll'));library.preview_init.argtypes=[ctypes.c_char_p];assert library.preview_init(str(a.font.resolve()).encode())==0
    library.preview_render.restype=ctypes.c_void_p;assert library.preview_controller_checks()>0
    class Hot(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in ('x','y','w','h','action','param')]
    library.preview_hotzones.restype=ctypes.POINTER(Hot)
    files=[]
    for language in (1,0):
        for scene in range(24):
            pointer=library.preview_render(scene,language);assert pointer,'Framebuffer guard was overwritten'
            for i in range(library.preview_hot_count()):
                zone=library.preview_hotzones()[i];assert zone.x>=0 and zone.y>=0 and zone.x+zone.w<=1280 and zone.y+zone.h<=720
            path=out/f'{"en" if language else "zh"}-{scene:02d}.png';png(path,ctypes.string_at(pointer,1280*720*4));files.append(path.name)
    checks=library.preview_controller_checks();assert checks>0,('Controller check failed',checks)
    report={'screens':len(files),'controller_checks':checks,'framebuffer_guards':True,'touch_bounds':True,'console_verified':False,'host_font':str(a.font),'source_sha256':hashlib.sha256(source.encode()).hexdigest()}
    (out/'ui-preview-tests.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--devkitpro',type=Path,required=True);p.add_argument('--font',type=Path,default=Path('C:/Windows/Fonts/msyh.ttc'));main(p.parse_args())
