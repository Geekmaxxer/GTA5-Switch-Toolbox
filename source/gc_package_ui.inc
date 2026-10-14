/* Included after main.c helpers; reuse the original card/dialog design. */
#define MODE_PACKAGES 9
#define MODE_PACKAGE_CONFIRM 10
static GcPackage g_packages[GC_PACKAGE_MAX];
static int g_package_count,g_package_restore;
static const char *g_package_kind;
static u64 g_package_progress_tick;
static const char *package_title(const GcPackage *p) {return !g_lang_en && *p->title_zh?p->title_zh:p->title;}
static void package_open(const char *kind) {
    g_package_kind=kind;g_package_count=gc_package_scan(g_packages,GC_PACKAGE_MAX,kind);
    g_modal_mode=MODE_PACKAGES;g_modal_sel=g_modal_scr=0;
}
static void package_ask(int restore) {
    if(restore!=2 && (!g_package_count || g_modal_sel<0 || g_modal_sel>=g_package_count))return;
    g_package_restore=restore;g_modal_mode=MODE_PACKAGE_CONFIRM;
}
static void package_progress(unsigned long long done,unsigned long long total,void *user) {
    (void)user;u64 now=svcGetSystemTick();
    if(now-g_package_progress_tick<1920000 && done<total)return;
    g_package_progress_tick=now;
    char detail[128];snprintf(detail,sizeof(detail),TR("当前文件：%.1f / %.1f MB","Current file: %.1f / %.1f MB"),done/1048576.0,total/1048576.0);
    dlc_prog_set(TR("准备并校验文件","Preparing and verifying files"),detail,(int)(total?done*100/total:0));
    dlc_prog_frame();
}
static void package_apply(void) {
    char err[256]={0};int rc;
    dlc_prog_begin(TR("安装包管理","Package manager"),TR("请勿关闭主机或拔出 SD 卡","Keep the console and SD card connected"));g_package_progress_tick=0;
    dlc_prog_frame();
    if(g_package_restore==2)rc=gc_package_recover_progress(package_progress,NULL,err,sizeof(err));
    else if(g_package_restore)rc=gc_package_restore_progress(&g_packages[g_modal_sel],package_progress,NULL,err,sizeof(err));
    else rc=gc_package_install(&g_packages[g_modal_sel],package_progress,NULL,err,sizeof(err));
    dlc_prog_end();g_modal_mode=MODE_PACKAGES;
    if(rc)dlc_err("%s",*err?err:TR("操作失败；保留备份后重试","Operation failed; keep backups and retry"));
    else dlc_msg(TR("操作完成。游戏文件已校验，音乐及存档保留。","Done. Game files verified; music and saves preserved."));
}
static void package_input(u64 keys) {
    if(g_modal_mode==MODE_PACKAGES) {
        if(g_package_count) {
            if(keys & HidNpadButton_Up)g_modal_sel=(g_modal_sel+g_package_count-1)%g_package_count;
            if(keys & HidNpadButton_Down)g_modal_sel=(g_modal_sel+1)%g_package_count;
        }
        if(keys & HidNpadButton_A)package_ask(0);
        else if(keys & HidNpadButton_X)package_ask(1);
        else if(keys & HidNpadButton_Y)package_ask(2);
        else if(keys & HidNpadButton_B)g_modal_mode=0;
    } else {
        if(keys & HidNpadButton_A)package_apply();
        else if(keys & HidNpadButton_B)g_modal_mode=MODE_PACKAGES;
    }
}
