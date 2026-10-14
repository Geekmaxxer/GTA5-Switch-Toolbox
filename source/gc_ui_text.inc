/* Shared FreeType metrics and a bounded glyph cache: drawing and measuring agree. */
#define UI_GLYPHS 512
#define UI_GLYPH_SIDE 48
typedef struct {
    uint32_t code,used;int size,advance,left,top,width,height;
    unsigned char pixels[UI_GLYPH_SIDE*UI_GLYPH_SIDE];
} UiGlyph;
static UiGlyph g_ui_glyphs[UI_GLYPHS];
static uint32_t g_ui_glyph_clock;
static FT_Face g_ui_cached_face;
static int g_ui_font_size;
static int g_ui_text_left,g_ui_text_top,g_ui_text_right=FB_WIDTH,g_ui_text_bottom=FB_HEIGHT;
static uint32_t utf8_next(const char **s) {return ui_utf8_next(s);}
static UiGlyph *ui_glyph(uint32_t cp,int size) {
    if(!g_face)return NULL;
    if(g_face!=g_ui_cached_face){memset(g_ui_glyphs,0,sizeof(g_ui_glyphs));g_ui_cached_face=g_face;g_ui_font_size=0;}
    unsigned index=(cp*2654435761u+(unsigned)size*97u)&(UI_GLYPHS-1),victim=index;
    for(unsigned i=0;i<8;++i) {
        unsigned at=(index+i)&(UI_GLYPHS-1);UiGlyph *g=&g_ui_glyphs[at];
        if(g->size==size && g->code==cp){g->used=++g_ui_glyph_clock;return g;}
        if(!g->size){victim=at;break;}
        if(g->used<g_ui_glyphs[victim].used)victim=at;
    }
    if(size!=g_ui_font_size){FT_Set_Pixel_Sizes(g_face,0,size);g_ui_font_size=size;}
    if(FT_Load_Char(g_face,cp,FT_LOAD_RENDER))return NULL;
    FT_GlyphSlot slot=g_face->glyph;FT_Bitmap *bmp=&slot->bitmap;UiGlyph *g=&g_ui_glyphs[victim];
    g->code=cp;g->size=size;g->used=++g_ui_glyph_clock;g->advance=(int)(slot->advance.x>>6);
    g->left=slot->bitmap_left;g->top=slot->bitmap_top;
    g->width=bmp->width<UI_GLYPH_SIDE?(int)bmp->width:UI_GLYPH_SIDE;
    g->height=bmp->rows<UI_GLYPH_SIDE?(int)bmp->rows:UI_GLYPH_SIDE;
    for(int r=0;r<g->height;++r) {
        int row=bmp->pitch>=0?r:g->height-1-r;
        memcpy(g->pixels+r*UI_GLYPH_SIDE,bmp->buffer+row*abs(bmp->pitch),g->width);
    }
    return g;
}
static void ui_text_bounds(int x,int y,int w,int h) {
    g_ui_text_left=x<0?0:x;g_ui_text_top=y<0?0:y;
    g_ui_text_right=x+w>FB_WIDTH?FB_WIDTH:x+w;
    g_ui_text_bottom=y+h>FB_HEIGHT?FB_HEIGHT:y+h;
}
static void ui_text_reset(void) {ui_text_bounds(0,0,FB_WIDTH,FB_HEIGHT);}
static int draw_text(uint32_t *fb,int x,int y,int size,const char *str,uint32_t color) {
    if(!str)return x;
    int pen=x;
    for(const char *s=str;*s;) {
        uint32_t cp=utf8_next(&s);if(!cp || cp=='\n')break;
        UiGlyph *g=ui_glyph(cp,size);if(!g)continue;
        int gx=pen+g->left,gy=y+size-g->top;
        unsigned cr=color&255,cg=color>>8&255,cb=color>>16&255;
        for(int r=0;r<g->height;++r) {
            int py=gy+r;if(py<g_ui_text_top || py>=g_ui_text_bottom)continue;
            for(int c=0;c<g->width;++c) {
                int px=gx+c;if(px<g_ui_text_left || px>=g_ui_text_right)continue;
                unsigned alpha=g->pixels[r*UI_GLYPH_SIDE+c];if(!alpha)continue;
                uint32_t *dst=fb+py*g_stride_words+px,old=*dst;
                unsigned red=(cr*alpha+(old&255)*(255-alpha))/255;
                unsigned green=(cg*alpha+(old>>8&255)*(255-alpha))/255;
                unsigned blue=(cb*alpha+(old>>16&255)*(255-alpha))/255;
                *dst=RGB_A(red,green,blue);
            }
        }
        pen+=g->advance;
    }
    return pen;
}
static int text_width(const char *text,int size) {
    int width=0;if(!text)return 0;
    for(const char *s=text;*s;) {uint32_t cp=utf8_next(&s);if(!cp || cp=='\n')break;UiGlyph *g=ui_glyph(cp,size);if(g)width+=g->advance;}
    return width;
}
static int draw_text_fit(uint32_t *fb,int x,int y,int size,int width,const char *text,uint32_t color) {
    if(!text || width<1)return x;
    if(text_width(text,size)<=width)return draw_text(fb,x,y,size,text,color);
    char short_text[768];const char *s=text;size_t length=0;int used=0,reserve=text_width("...",size);
    while(*s && *s!='\n') {
        const char *old=s;uint32_t cp=utf8_next(&s);UiGlyph *g=ui_glyph(cp,size);int advance=g?g->advance:0;
        if(used+advance+reserve>width || length+(size_t)(s-old)+4>=sizeof(short_text))break;
        memcpy(short_text+length,old,(size_t)(s-old));length+=(size_t)(s-old);used+=advance;
    }
    if(reserve<=width)memcpy(short_text+length,"...",4);else short_text[length]=0;
    return draw_text(fb,x,y,size,short_text,color);
}
static int draw_text_c(uint32_t *fb,int cx,int y,int size,const char *text,uint32_t color) {
    int x=cx-text_width(text,size)/2;draw_text(fb,x,y,size,text,color);return x;
}
static int draw_text_r(uint32_t *fb,int right,int y,int size,const char *text,uint32_t color) {
    int x=right-text_width(text,size);draw_text(fb,x,y,size,text,color);return x;
}
static int draw_text_wrap(uint32_t *fb,int x,int y,int width,int size,int lines,const char *text,uint32_t color) {
    if(!text || width<1 || lines<1)return 0;
    const char *s=text;int row=0;
    while(*s && row<lines) {
        char line[768];size_t length=0,word=0;int used=0;const char *start=s,*word_end=NULL;
        while(*s && *s!='\n') {
            const char *old=s;uint32_t cp=utf8_next(&s);UiGlyph *g=ui_glyph(cp,size);int advance=g?g->advance:0;
            if(used+advance>width || length+(size_t)(s-old)+1>=sizeof(line)) {
                s=old;if(word_end){length=word;s=word_end;}
                break;
            }
            if(cp==' '){word=length;word_end=s;}
            memcpy(line+length,old,(size_t)(s-old));length+=(size_t)(s-old);used+=advance;
        }
        if(s==start && *s){const char *old=s;utf8_next(&s);length=(size_t)(s-old);memcpy(line,old,length);}
        line[length]=0;
        if(row==lines-1 && *s && length+4<sizeof(line)) {
            memcpy(line+length,"...",4);draw_text_fit(fb,x,y+row*(size+7),size,width,line,color);
        }
        else draw_text(fb,x,y+row*(size+7),size,line,color);
        if(*s=='\n')++s;
        while(*s==' ')++s;
        ++row;
    }
    return row*(size+7);
}
