#include "gc_ui_core.h"
unsigned ui_repeat(UiRepeat *s,unsigned held,uint64_t now) {
    if(!held){s->held=0;s->next=0;return 0;}
    if(held!=s->held){s->held=held;s->next=now+320;return held;}
    if(now<s->next)return 0;
    s->next=now+75;return held;
}
int ui_viewport(int count,int selected,int *scroll,int visible) {
    if(count<1){*scroll=0;return 0;}
    if(visible<1)visible=1;
    if(selected<0)selected=0;
    if(selected>=count)selected=count-1;
    int maximum=count>visible?count-visible:0;
    if(*scroll<0)*scroll=0;
    if(*scroll>maximum)*scroll=maximum;
    if(selected<*scroll)*scroll=selected;
    if(selected>=*scroll+visible)*scroll=selected-visible+1;
    return selected;
}
int ui_grid_move(int item,int count,int columns,unsigned direction) {
    if(count<1 || columns<1)return 0;
    if(item<0)item=0;
    if(item>=count)item=count-1;
    if(direction==UI_LEFT && item%columns>0)--item;
    if(direction==UI_RIGHT && item%columns<columns-1 && item+1<count)++item;
    if(direction==UI_UP && item>=columns)item-=columns;
    if(direction==UI_DOWN && item+columns<count)item+=columns;
    else if(direction==UI_DOWN && item/columns<(count-1)/columns)item=count-1;
    return item;
}
uint32_t ui_utf8_next(const char **cursor) {
    const unsigned char *p=(const unsigned char *)*cursor;
    if(!*p)return 0;
    unsigned n;uint32_t cp;
    if(*p<0x80){++*cursor;return *p;}
    if(*p>=0xc2 && *p<=0xdf){n=2;cp=*p&31;}
    else if(*p>=0xe0 && *p<=0xef){n=3;cp=*p&15;}
    else if(*p>=0xf0 && *p<=0xf4){n=4;cp=*p&7;}
    else {++*cursor;return 0xfffd;}
    for(unsigned i=1;i<n;++i) {
        if(!p[i] || (p[i]&0xc0)!=0x80){++*cursor;return 0xfffd;}
        cp=(cp<<6)|(p[i]&63);
    }
    if((n==3 && cp<0x800) || (n==4 && cp<0x10000) || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) {
        ++*cursor;return 0xfffd;
    }
    *cursor+=n;return cp;
}
