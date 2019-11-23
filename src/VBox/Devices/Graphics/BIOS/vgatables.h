
#ifndef VBOX_INCLUDED_SRC_Graphics_BIOS_vgatables_h
#define VBOX_INCLUDED_SRC_Graphics_BIOS_vgatables_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Video memory */
#define VGAMEM_GRAPH 0xA000
#define VGAMEM_CTEXT 0xB800
#define VGAMEM_MTEXT 0xB000

/*
 *
 * Tables of default values for each mode
 *
 */
#define MODE_MAX   15
#define TEXT       0x00
#define GRAPH      0x01

#define CTEXT      0x00
#define MTEXT      0x01
#define CGA        0x02
#define PLANAR1    0x03
#define PLANAR4    0x04
#define LINEAR8    0x05

// for SVGA
#define LINEAR15   0x10
#define LINEAR16   0x11
#define LINEAR24   0x12
#define LINEAR32   0x13

typedef struct
{uint8_t    svgamode;
 uint8_t    class;    /* TEXT, GRAPH */
 uint8_t    memmodel; /* CTEXT,MTEXT,CGA,PL1,PL2,PL4,P8,P15,P16,P24,P32 */
 uint8_t    pixbits;
 uint16_t   sstart;
 uint8_t    pelmask;
 uint8_t    dacmodel; /* 0 1 2 3 */
} VGAMODES;

static VGAMODES vga_modes[MODE_MAX+1]=
{//mode  class  model bits sstart  pelm  dac
 {0x00, TEXT,  CTEXT,   4, 0xB800, 0xFF, 0x02},
 {0x01, TEXT,  CTEXT,   4, 0xB800, 0xFF, 0x02},
 {0x02, TEXT,  CTEXT,   4, 0xB800, 0xFF, 0x02},
 {0x03, TEXT,  CTEXT,   4, 0xB800, 0xFF, 0x02},
 {0x04, GRAPH, CGA,     2, 0xB800, 0xFF, 0x01},
 {0x05, GRAPH, CGA,     2, 0xB800, 0xFF, 0x01},
 {0x06, GRAPH, CGA,     1, 0xB800, 0xFF, 0x01},
 {0x07, TEXT,  MTEXT,   4, 0xB000, 0xFF, 0x00},
 {0x0D, GRAPH, PLANAR4, 4, 0xA000, 0xFF, 0x01},
 {0x0E, GRAPH, PLANAR4, 4, 0xA000, 0xFF, 0x01},
 {0x0F, GRAPH, PLANAR1, 1, 0xA000, 0xFF, 0x00},
 {0x10, GRAPH, PLANAR4, 4, 0xA000, 0xFF, 0x02},
 {0x11, GRAPH, PLANAR1, 1, 0xA000, 0xFF, 0x02},
 {0x12, GRAPH, PLANAR4, 4, 0xA000, 0xFF, 0x02},
 {0x13, GRAPH, LINEAR8, 8, 0xA000, 0xFF, 0x03},
 {0x6A, GRAPH, PLANAR4, 4, 0xA000, 0xFF, 0x02}
};

/* convert index in vga_modes[] to index in video_param_table[] */
static uint8_t line_to_vpti[MODE_MAX+1]={
    0x17, 0x17, 0x18, 0x18, 0x04, 0x05, 0x06, 0x07,
    0x0d, 0x0e, 0x11, 0x12, 0x1a, 0x1b, 0x1c, 0x1d,
};

/* Default Palette */
#define DAC_MAX_MODEL 3

static uint8_t dac_regs[DAC_MAX_MODEL+1]=
{0x3f,0x3f,0x3f,0xff};

/* standard BIOS Video Parameter Table */
typedef struct {
    uint8_t     twidth;
    uint8_t     theightm1;
    uint8_t     cheight;
    uint8_t     slength_l;
    uint8_t     slength_h;
    uint8_t     sequ_regs[4];
    uint8_t     miscreg;
    uint8_t     crtc_regs[25];
    uint8_t     actl_regs[20];
    uint8_t     grdc_regs[9];
} VideoParamTableEntry;

static VideoParamTableEntry video_param_table[30] = {
{
 /* index=0x00 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x01 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x02 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x03 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x04 vga mode 0x04 */
 40, 24, 8, 0x00, 0x08, /* tw, th-1, ch, slength */
 0x09, 0x03, 0x00, 0x02, /* sequ_regs */
 0x63, /* miscreg */
 0x2d, 0x27, 0x28, 0x90, 0x2b, 0x80, 0xbf, 0x1f,
 0x00, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x14, 0x00, 0x96, 0xb9, 0xa2,
 0xff, /* crtc_regs */
 0x00, 0x13, 0x15, 0x17, 0x02, 0x04, 0x06, 0x07,
 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
 0x01, 0x00, 0x03, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x0f, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x05 vga mode 0x05 */
 40, 24, 8, 0x00, 0x08, /* tw, th-1, ch, slength */
 0x09, 0x03, 0x00, 0x02, /* sequ_regs */
 0x63, /* miscreg */
 0x2d, 0x27, 0x28, 0x90, 0x2b, 0x80, 0xbf, 0x1f,
 0x00, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x14, 0x00, 0x96, 0xb9, 0xa2,
 0xff, /* crtc_regs */
 0x00, 0x13, 0x15, 0x17, 0x02, 0x04, 0x06, 0x07,
 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
 0x01, 0x00, 0x03, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x0f, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x06 vga mode 0x06 */
 80, 24, 8, 0x00, 0x10, /* tw, th-1, ch, slength */
 0x01, 0x01, 0x00, 0x06, /* sequ_regs */
 0x63, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
 0x00, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x28, 0x00, 0x96, 0xb9, 0xc2,
 0xff, /* crtc_regs */
 0x00, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
 0x01, 0x00, 0x01, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x07 vga mode 0x07 */
 80, 24, 16, 0x00, 0x10, /* tw, th-1, ch, slength */
 0x00, 0x03, 0x00, 0x02, /* sequ_regs */
 0x66, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
 0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x28, 0x0f, 0x96, 0xb9, 0xa3,
 0xff, /* crtc_regs */
 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
 0x10, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
 0x0e, 0x00, 0x0f, 0x08, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0a, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x08 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x09 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x0a no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x0b no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x0c no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x0d vga mode 0x0d */
 40, 24, 8, 0x00, 0x20, /* tw, th-1, ch, slength */
 0x09, 0x0f, 0x00, 0x06, /* sequ_regs */
 0x63, /* miscreg */
 0x2d, 0x27, 0x28, 0x90, 0x2b, 0x80, 0xbf, 0x1f,
 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x14, 0x00, 0x96, 0xb9, 0xe3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x0e vga mode 0x0e */
 80, 24, 8, 0x00, 0x40, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
 0x63, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x28, 0x00, 0x96, 0xb9, 0xe3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x0f no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x10 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x11 vga mode 0x0f */
 80, 24, 14, 0x00, 0x80, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
 0xa3, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x83, 0x85, 0x5d, 0x28, 0x0f, 0x63, 0xba, 0xe3,
 0xff, /* crtc_regs */
 0x00, 0x08, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
 0x00, 0x08, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
 0x01, 0x00, 0x01, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x12 vga mode 0x10 */
 80, 24, 14, 0x00, 0x80, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
 0xa3, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x83, 0x85, 0x5d, 0x28, 0x0f, 0x63, 0xba, 0xe3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x13 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x14 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
 /* index=0x15 no mode defined */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
},
{
    /* index=0x16 ega mode 0x03 */
    80, 24, 14, 0x00, 0x10, /* tw, th-1, ch, slength */
    0x00, 0x03, 0x00, 0x02, /* sequ_regs */
    0x67, /* miscreg */
    0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
    0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
    0x9c, 0x8e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
    0xff, /* crtc_regs */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x0c, 0x00, 0x0f, 0x08, /* actl_regs */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x17 vga mode 0x01 */
 40, 24, 16, 0x00, 0x08, /* tw, th-1, ch, slength */
 0x08, 0x03, 0x00, 0x02, /* sequ_regs */
 0x67, /* miscreg */
 0x2d, 0x27, 0x28, 0x90, 0x2b, 0xa0, 0xbf, 0x1f,
 0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x14, 0x1f, 0x96, 0xb9, 0xa3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x0c, 0x00, 0x0f, 0x08, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x18 vga mode 0x03 */
 80, 24, 16, 0x00, 0x10, /* tw, th-1, ch, slength */
 0x00, 0x03, 0x00, 0x02, /* sequ_regs */
 0x67, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
 0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x0c, 0x00, 0x0f, 0x08, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0e, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x19 vga mode 0x07 */
 80, 24, 16, 0x00, 0x10, /* tw, th-1, ch, slength */
 0x00, 0x03, 0x00, 0x02, /* sequ_regs */
 0x66, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
 0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x28, 0x0f, 0x96, 0xb9, 0xa3,
 0xff, /* crtc_regs */
 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
 0x10, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
 0x0e, 0x00, 0x0f, 0x08, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0a, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x1a vga mode 0x11 */
 80, 29, 16, 0x00, 0x00, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
 0xe3, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xc3,
 0xff, /* crtc_regs */
 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f,
 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f,
 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x1b vga mode 0x12 */
 80, 29, 16, 0x00, 0x00, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
 0xe3, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x1c vga mode 0x13 */
 40, 24, 8, 0x00, 0x00, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x0e, /* sequ_regs */
 0x63, /* miscreg */
 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f,
 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x9c, 0x8e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
 0x41, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff, /* grdc_regs */
},
{
 /* index=0x1d vga mode 0x6a */
 100, 36, 16, 0x00, 0x00, /* tw, th-1, ch, slength */
 0x01, 0x0f, 0x00, 0x06, /* sequ_regs */
 0xe3, /* miscreg */
 0x7f, 0x63, 0x63, 0x83, 0x6b, 0x1b, 0x72, 0xf0,
 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x59, 0x8d, 0x57, 0x32, 0x00, 0x57, 0x73, 0xe3,
 0xff, /* crtc_regs */
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
 0x01, 0x00, 0x0f, 0x00, /* actl_regs */
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f, 0xff, /* grdc_regs */
},
};

/* Mono */
static uint8_t palette0[63+1][3]=
{
  0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,
  0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a,
  0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a,
  0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f,
  0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,
  0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a,
  0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a, 0x2a,0x2a,0x2a,
  0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f, 0x3f,0x3f,0x3f
};

static uint8_t palette1[63+1][3]=
{
  0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
  0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
  0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
  0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
  0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
  0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
  0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
  0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f
};

static uint8_t palette2[63+1][3]=
{
  0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x2a,0x00, 0x2a,0x2a,0x2a,
  0x00,0x00,0x15, 0x00,0x00,0x3f, 0x00,0x2a,0x15, 0x00,0x2a,0x3f, 0x2a,0x00,0x15, 0x2a,0x00,0x3f, 0x2a,0x2a,0x15, 0x2a,0x2a,0x3f,
  0x00,0x15,0x00, 0x00,0x15,0x2a, 0x00,0x3f,0x00, 0x00,0x3f,0x2a, 0x2a,0x15,0x00, 0x2a,0x15,0x2a, 0x2a,0x3f,0x00, 0x2a,0x3f,0x2a,
  0x00,0x15,0x15, 0x00,0x15,0x3f, 0x00,0x3f,0x15, 0x00,0x3f,0x3f, 0x2a,0x15,0x15, 0x2a,0x15,0x3f, 0x2a,0x3f,0x15, 0x2a,0x3f,0x3f,
  0x15,0x00,0x00, 0x15,0x00,0x2a, 0x15,0x2a,0x00, 0x15,0x2a,0x2a, 0x3f,0x00,0x00, 0x3f,0x00,0x2a, 0x3f,0x2a,0x00, 0x3f,0x2a,0x2a,
  0x15,0x00,0x15, 0x15,0x00,0x3f, 0x15,0x2a,0x15, 0x15,0x2a,0x3f, 0x3f,0x00,0x15, 0x3f,0x00,0x3f, 0x3f,0x2a,0x15, 0x3f,0x2a,0x3f,
  0x15,0x15,0x00, 0x15,0x15,0x2a, 0x15,0x3f,0x00, 0x15,0x3f,0x2a, 0x3f,0x15,0x00, 0x3f,0x15,0x2a, 0x3f,0x3f,0x00, 0x3f,0x3f,0x2a,
  0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f
};

static uint8_t palette3[256][3]=
{
  0x00,0x00,0x00, 0x00,0x00,0x2a, 0x00,0x2a,0x00, 0x00,0x2a,0x2a, 0x2a,0x00,0x00, 0x2a,0x00,0x2a, 0x2a,0x15,0x00, 0x2a,0x2a,0x2a,
  0x15,0x15,0x15, 0x15,0x15,0x3f, 0x15,0x3f,0x15, 0x15,0x3f,0x3f, 0x3f,0x15,0x15, 0x3f,0x15,0x3f, 0x3f,0x3f,0x15, 0x3f,0x3f,0x3f,
  0x00,0x00,0x00, 0x05,0x05,0x05, 0x08,0x08,0x08, 0x0b,0x0b,0x0b, 0x0e,0x0e,0x0e, 0x11,0x11,0x11, 0x14,0x14,0x14, 0x18,0x18,0x18,
  0x1c,0x1c,0x1c, 0x20,0x20,0x20, 0x24,0x24,0x24, 0x28,0x28,0x28, 0x2d,0x2d,0x2d, 0x32,0x32,0x32, 0x38,0x38,0x38, 0x3f,0x3f,0x3f,
  0x00,0x00,0x3f, 0x10,0x00,0x3f, 0x1f,0x00,0x3f, 0x2f,0x00,0x3f, 0x3f,0x00,0x3f, 0x3f,0x00,0x2f, 0x3f,0x00,0x1f, 0x3f,0x00,0x10,
  0x3f,0x00,0x00, 0x3f,0x10,0x00, 0x3f,0x1f,0x00, 0x3f,0x2f,0x00, 0x3f,0x3f,0x00, 0x2f,0x3f,0x00, 0x1f,0x3f,0x00, 0x10,0x3f,0x00,
  0x00,0x3f,0x00, 0x00,0x3f,0x10, 0x00,0x3f,0x1f, 0x00,0x3f,0x2f, 0x00,0x3f,0x3f, 0x00,0x2f,0x3f, 0x00,0x1f,0x3f, 0x00,0x10,0x3f,
  0x1f,0x1f,0x3f, 0x27,0x1f,0x3f, 0x2f,0x1f,0x3f, 0x37,0x1f,0x3f, 0x3f,0x1f,0x3f, 0x3f,0x1f,0x37, 0x3f,0x1f,0x2f, 0x3f,0x1f,0x27,

  0x3f,0x1f,0x1f, 0x3f,0x27,0x1f, 0x3f,0x2f,0x1f, 0x3f,0x37,0x1f, 0x3f,0x3f,0x1f, 0x37,0x3f,0x1f, 0x2f,0x3f,0x1f, 0x27,0x3f,0x1f,
  0x1f,0x3f,0x1f, 0x1f,0x3f,0x27, 0x1f,0x3f,0x2f, 0x1f,0x3f,0x37, 0x1f,0x3f,0x3f, 0x1f,0x37,0x3f, 0x1f,0x2f,0x3f, 0x1f,0x27,0x3f,
  0x2d,0x2d,0x3f, 0x31,0x2d,0x3f, 0x36,0x2d,0x3f, 0x3a,0x2d,0x3f, 0x3f,0x2d,0x3f, 0x3f,0x2d,0x3a, 0x3f,0x2d,0x36, 0x3f,0x2d,0x31,
  0x3f,0x2d,0x2d, 0x3f,0x31,0x2d, 0x3f,0x36,0x2d, 0x3f,0x3a,0x2d, 0x3f,0x3f,0x2d, 0x3a,0x3f,0x2d, 0x36,0x3f,0x2d, 0x31,0x3f,0x2d,
  0x2d,0x3f,0x2d, 0x2d,0x3f,0x31, 0x2d,0x3f,0x36, 0x2d,0x3f,0x3a, 0x2d,0x3f,0x3f, 0x2d,0x3a,0x3f, 0x2d,0x36,0x3f, 0x2d,0x31,0x3f,
  0x00,0x00,0x1c, 0x07,0x00,0x1c, 0x0e,0x00,0x1c, 0x15,0x00,0x1c, 0x1c,0x00,0x1c, 0x1c,0x00,0x15, 0x1c,0x00,0x0e, 0x1c,0x00,0x07,
  0x1c,0x00,0x00, 0x1c,0x07,0x00, 0x1c,0x0e,0x00, 0x1c,0x15,0x00, 0x1c,0x1c,0x00, 0x15,0x1c,0x00, 0x0e,0x1c,0x00, 0x07,0x1c,0x00,
  0x00,0x1c,0x00, 0x00,0x1c,0x07, 0x00,0x1c,0x0e, 0x00,0x1c,0x15, 0x00,0x1c,0x1c, 0x00,0x15,0x1c, 0x00,0x0e,0x1c, 0x00,0x07,0x1c,

  0x0e,0x0e,0x1c, 0x11,0x0e,0x1c, 0x15,0x0e,0x1c, 0x18,0x0e,0x1c, 0x1c,0x0e,0x1c, 0x1c,0x0e,0x18, 0x1c,0x0e,0x15, 0x1c,0x0e,0x11,
  0x1c,0x0e,0x0e, 0x1c,0x11,0x0e, 0x1c,0x15,0x0e, 0x1c,0x18,0x0e, 0x1c,0x1c,0x0e, 0x18,0x1c,0x0e, 0x15,0x1c,0x0e, 0x11,0x1c,0x0e,
  0x0e,0x1c,0x0e, 0x0e,0x1c,0x11, 0x0e,0x1c,0x15, 0x0e,0x1c,0x18, 0x0e,0x1c,0x1c, 0x0e,0x18,0x1c, 0x0e,0x15,0x1c, 0x0e,0x11,0x1c,
  0x14,0x14,0x1c, 0x16,0x14,0x1c, 0x18,0x14,0x1c, 0x1a,0x14,0x1c, 0x1c,0x14,0x1c, 0x1c,0x14,0x1a, 0x1c,0x14,0x18, 0x1c,0x14,0x16,
  0x1c,0x14,0x14, 0x1c,0x16,0x14, 0x1c,0x18,0x14, 0x1c,0x1a,0x14, 0x1c,0x1c,0x14, 0x1a,0x1c,0x14, 0x18,0x1c,0x14, 0x16,0x1c,0x14,
  0x14,0x1c,0x14, 0x14,0x1c,0x16, 0x14,0x1c,0x18, 0x14,0x1c,0x1a, 0x14,0x1c,0x1c, 0x14,0x1a,0x1c, 0x14,0x18,0x1c, 0x14,0x16,0x1c,
  0x00,0x00,0x10, 0x04,0x00,0x10, 0x08,0x00,0x10, 0x0c,0x00,0x10, 0x10,0x00,0x10, 0x10,0x00,0x0c, 0x10,0x00,0x08, 0x10,0x00,0x04,
  0x10,0x00,0x00, 0x10,0x04,0x00, 0x10,0x08,0x00, 0x10,0x0c,0x00, 0x10,0x10,0x00, 0x0c,0x10,0x00, 0x08,0x10,0x00, 0x04,0x10,0x00,

  0x00,0x10,0x00, 0x00,0x10,0x04, 0x00,0x10,0x08, 0x00,0x10,0x0c, 0x00,0x10,0x10, 0x00,0x0c,0x10, 0x00,0x08,0x10, 0x00,0x04,0x10,
  0x08,0x08,0x10, 0x0a,0x08,0x10, 0x0c,0x08,0x10, 0x0e,0x08,0x10, 0x10,0x08,0x10, 0x10,0x08,0x0e, 0x10,0x08,0x0c, 0x10,0x08,0x0a,
  0x10,0x08,0x08, 0x10,0x0a,0x08, 0x10,0x0c,0x08, 0x10,0x0e,0x08, 0x10,0x10,0x08, 0x0e,0x10,0x08, 0x0c,0x10,0x08, 0x0a,0x10,0x08,
  0x08,0x10,0x08, 0x08,0x10,0x0a, 0x08,0x10,0x0c, 0x08,0x10,0x0e, 0x08,0x10,0x10, 0x08,0x0e,0x10, 0x08,0x0c,0x10, 0x08,0x0a,0x10,
  0x0b,0x0b,0x10, 0x0c,0x0b,0x10, 0x0d,0x0b,0x10, 0x0f,0x0b,0x10, 0x10,0x0b,0x10, 0x10,0x0b,0x0f, 0x10,0x0b,0x0d, 0x10,0x0b,0x0c,
  0x10,0x0b,0x0b, 0x10,0x0c,0x0b, 0x10,0x0d,0x0b, 0x10,0x0f,0x0b, 0x10,0x10,0x0b, 0x0f,0x10,0x0b, 0x0d,0x10,0x0b, 0x0c,0x10,0x0b,
  0x0b,0x10,0x0b, 0x0b,0x10,0x0c, 0x0b,0x10,0x0d, 0x0b,0x10,0x0f, 0x0b,0x10,0x10, 0x0b,0x0f,0x10, 0x0b,0x0d,0x10, 0x0b,0x0c,0x10,
  0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00
};

static uint8_t static_functionality[0x10]=
{
 /* 0 */ 0xff,  // All modes supported #1
 /* 1 */ 0xe0,  // All modes supported #2
 /* 2 */ 0x0f,  // All modes supported #3
 /* 3 */ 0x00, 0x00, 0x00, 0x00,  // reserved
 /* 7 */ 0x07,  // 200, 350, 400 scan lines
 /* 8 */ 0x02,  // maximum number of visible charsets in text mode
 /* 9 */ 0x08,  // total number of charset blocks in text mode
 /* a */ 0xe7,  // Change to add new functions
 /* b */ 0x0c,  // Change to add new functions
 /* c */ 0x00,  // reserved
 /* d */ 0x00,  // reserved
 /* e */ 0x00,  // Change to add new functions
 /* f */ 0x00   // reserved
};

#endif /* !VBOX_INCLUDED_SRC_Graphics_BIOS_vgatables_h */

