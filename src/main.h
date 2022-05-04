#ifndef __EVE812_H
#define __EVE812_H

// Memory base addresses
#define RAM_REG 0x302000 // Offset
#define RAM_G 0x0
#define RAM_G_WORKING 0x0FF000 // This address may be used as the start of a 4K block to be used for copying data
#define RAM_DL 0x300000
#define RAM_CMD 0x308000
#define RAM_ERR_REPORT 0x309800 // max 128 bytes null terminated string
#define RAM_FLASH 0x800000
#define RAM_FLASH_POSTBLOB 0x801000

// Graphics Engine Registers - FT81x Series Programmers Guide Section 3.1
// Addresses defined as offsets from the base address called RAM_REG and located at 0x302000
// Discussion: Defining this way leads to an additional add operation in code that can be avoided by defining
// these addresses as 32 bit values, but this is easily paid for in clarity and coorelation to documentation.
// Further, you can add defines together in code and allow the precompiler to do the add operation (as here).
#define REG_CSPREAD 0x68
#define REG_DITHER 0x60
#define REG_DLSWAP 0x54
#define REG_HCYCLE 0x2C
#define REG_HOFFSET 0x30
#define REG_HSIZE 0x34
#define REG_HSYNC0 0x38
#define REG_HSYNC1 0x3C
#define REG_OUTBITS 0x5C
#define REG_PCLK 0x70
#define REG_PCLK_POL 0x6C
#define REG_PLAY 0x8C
#define REG_PLAYBACK_FORMAT 0xC4
#define REG_PLAYBACK_FREQ 0xC0
#define REG_PLAYBACK_LENGTH 0xB8
#define REG_PLAYBACK_LOOP 0xC8
#define REG_PLAYBACK_PLAY 0xCC
#define REG_PLAYBACK_READPTR 0xBC
#define REG_PLAYBACK_START 0xB4
#define REG_PWM_DUTY 0xD4
#define REG_ROTATE 0x58
#define REG_SOUND 0x88
#define REG_SWIZZLE 0x64
#define REG_TAG 0x7C
#define REG_TAG_X 0x74
#define REG_TAG_Y 0x78
#define REG_VCYCLE 0x40
#define REG_VOFFSET 0x44
#define REG_VOL_SOUND 0x84
#define REG_VOL_PB 0x80
#define REG_VSYNC0 0x4C
#define REG_VSYNC1 0x50
#define REG_VSIZE 0x48

#define CLEAR(c1, s1, t1) ((38UL << 24) | (((c1)&1UL) << 2) | (((s1)&1UL) << 1) | (((t1)&1UL) << 0))                                                                                                         // CLEAR - FT-PG Section 4.21
#define CLEAR_COLOR_RGB(red1, green1, blue1) ((2UL << 24) | (((red1)&255UL) << 16) | (((green1)&255UL) << 8) | (((blue1)&255UL) << 0))                                                                       // CLEAR_COLOR_RGB - FT-PG Section 4.23
#define COLOR_RGB(red1, green1, blue1) ((4UL << 24) | (((red1)&255UL) << 16) | (((green1)&255UL) << 8) | (((blue1)&255UL) << 0))                                                                             // COLOR_RGB - FT-PG Section 4.28
#define VERTEX2II(x1, y1, handle1, cell1) ((2UL << 30) | (((x1)&511UL) << 21) | (((y1)&511UL) << 12) | (((handle1)&31UL) << 7) | (((cell1)&127UL) << 0))                                                     // VERTEX2II - FT-PG Section 4.48
#define VERTEX2F(x1, y1) ((1UL << 30) | (((x1)&32767UL) << 15) | (((y1)&32767UL) << 0))                                                                                                                      // VERTEX2F - FT-PG Section 4.47
#define CELL(cell1) ((6UL << 24) | (((cell1)&127UL) << 0))                                                                                                                                                   // CELL - FT-PG Section 4.20
#define BITMAP_HANDLE(handle1) ((5UL << 24) | (((handle1)&31UL) << 0))                                                                                                                                       // BITMAP_HANDLE - FT-PG Section 4.06
#define BITMAP_SOURCE(addr1) ((1UL << 24) | (((addr1)&1048575UL) << 0))                                                                                                                                      // BITMAP_SOURCE - FT-PG Section 4.11
#define BITMAP_LAYOUT(format1, linestride1, height1) ((7UL << 24) | (((format1)&31UL) << 19) | (((linestride1)&1023UL) << 9) | (((height1)&511UL) << 0))                                                     // BITMAP_LAYOUT - FT-PG Section 4.07
#define BITMAP_SIZE(filter1, wrapx1, wrapy1, width1, height1) ((8UL << 24) | (((filter1)&1UL) << 20) | (((wrapx1)&1UL) << 19) | (((wrapy1)&1UL) << 18) | (((width1)&511UL) << 9) | (((height1)&511UL) << 0)) // BITMAP_SIZE - FT-PG Section 4.09
#define TAG(s1) ((3UL << 24) | (((s1)&255UL) << 0))                                                                                                                                                          // TAG - FT-PG Section 4.43
#define POINT_SIZE(sighs1) ((13UL << 24) | (((sighs1)&8191UL) << 0))                                                                                                                                         // POINT_SIZE - FT-PG Section 4.36
#define BEGIN(PrimitiveTypeRef1) ((31UL << 24) | (((PrimitiveTypeRef1)&15UL) << 0))                                                                                                                          // BEGIN - FT-PG Section 4.05
#define END() ((33UL << 24))                                                                                                                                                                                 // END - FT-PG Section 4.30
#define DISPLAY() ((0UL << 24))

#define DLSWAP_FRAME 2UL

// Definitions for FT8xx co processor command buffer
#define FT_DL_SIZE (8 * 1024)       // 8KB Display List buffer size
#define FT_CMD_FIFO_SIZE (4 * 1024) // 4KB coprocessor Fifo size
#define FT_CMD_SIZE (4)             // 4 byte per coprocessor command of EVE

uint16_t FifoWriteLocation = 0;

#define HCMD_ACTIVE 0x00
#define HCMD_STANDBY 0x41
#define HCMD_SLEEP 0x42
#define HCMD_PWRDOWN 0x50
#define HCMD_CLKINT 0x48
#define HCMD_CLKEXT 0x44
#define HCMD_CLK48M 0x62
#define HCMD_CLK36M 0x61
#define HCMD_CORERESET 0x68

#define CMD_APPEND 0xFFFFFF1E
#define CMD_BGCOLOR 0xFFFFFF09
#define CMD_BUTTON 0xFFFFFF0D
#define CMD_CALIBRATE 0xFFFFFF15 // 4294967061UL
#define CMD_CLOCK 0xFFFFFF14
#define CMD_COLDSTART 0xFFFFFF32
#define CMD_CRC 0xFFFFFF18
#define CMD_DIAL 0xFFFFFF2D
#define CMD_DLSTART 0xFFFFFF00
#define CMD_FGCOLOR 0xFFFFFF0A
#define CMD_GAUGE 0xFFFFFF13
#define CMD_GETMATRIX 0xFFFFFF33
#define CMD_GETPROPS 0xFFFFFF25
#define CMD_GETPTR 0xFFFFFF23
#define CMD_GRADCOLOR 0xFFFFFF34
#define CMD_GRADIENT 0xFFFFFF0B
#define CMD_INFLATE 0xFFFFFF22
#define CMD_INFLATE2 0xFFFFFF50
#define CMD_INTERRUPT 0xFFFFFF02
#define CMD_KEYS 0xFFFFFF0E
#define CMD_LOADIDENTITY 0xFFFFFF26
#define CMD_LOADIMAGE 0xFFFFFF24
#define CMD_LOGO 0xFFFFFF31
#define CMD_MEDIAFIFO 0xFFFFFF39
#define CMD_MEMCPY 0xFFFFFF1D
#define CMD_MEMCRC 0xFFFFFF18
#define CMD_MEMSET 0xFFFFFF1B
#define CMD_MEMWRITE 0xFFFFFF1A
#define CMD_MEMZERO 0xFFFFFF1C
#define CMD_NUMBER 0xFFFFFF38
#define CMD_PLAYVIDEO 0xFFFFFF3A
#define CMD_PROGRESS 0xFFFFFF0F
#define CMD_REGREAD 0xFFFFFF19
#define CMD_ROTATE 0xFFFFFF29
#define CMD_SCALE 0xFFFFFF28
#define CMD_SCREENSAVER 0xFFFFFF2F
#define CMD_SCROLLBAR 0xFFFFFF11
#define CMD_SETBITMAP 0xFFFFFF43
#define CMD_SETFONT 0xFFFFFF2B
#define CMD_SETMATRIX 0xFFFFFF2A
#define CMD_SETROTATE 0xFFFFFF36
#define CMD_SKETCH 0xFFFFFF30
#define CMD_SLIDER 0xFFFFFF10
#define CMD_SNAPSHOT 0xFFFFFF1F
#define CMD_SPINNER 0xFFFFFF16
#define CMD_STOP 0xFFFFFF17
#define CMD_SWAP 0xFFFFFF01
#define CMD_TEXT 0xFFFFFF0C
#define CMD_TOGGLE 0xFFFFFF12
#define CMD_TRACK 0xFFFFFF2C
#define CMD_TRANSLATE 0xFFFFFF27
#define CMD_VIDEOFRAME 0xFFFFFF41
#define CMD_VIDEOSTART 0xFFFFFF40

// BT81X COMMANDS
#define CMD_FLASHERASE 0xFFFFFF44
#define CMD_FLASHWRITE 0xFFFFFF45
#define CMD_FLASHREAD 0xFFFFFF46
#define CMD_FLASHUPDATE 0xFFFFFF47
#define CMD_FLASHDETACH 0xFFFFFF48
#define CMD_FLASHATTACH 0xFFFFFF49
#define CMD_FLASHFAST 0xFFFFFF4A
#define CMD_FLASHSPIDESEL 0xFFFFFF4B
#define CMD_FLASHSPITX 0xFFFFFF4C
#define CMD_FLASHSPIRX 0xFFFFFF4D
#define CMD_FLASHSOURCE 0xFFFFFF4E
#define CMD_CLEARCACHE 0xFFFFFF4F
#define CMD_FLASHAPPENDF 0xFFFFFF59
#define CMD_VIDEOSTARTF 0xFFFFFF5F

#define CMD_ROMFONT 0xFFFFFF3F

#define DLSWAP_FRAME 2UL

#define OPT_CENTER 1536UL
#define OPT_CENTERX 512UL
#define OPT_CENTERY 1024UL
#define OPT_FLASH 64UL
#define OPT_FLAT 256UL
#define OPT_FULLSCREEN 8UL
#define OPT_MEDIAFIFO 16UL
#define OPT_MONO 1UL
#define OPT_NOBACK 4096UL
#define OPT_NODL 2UL
#define OPT_NOHANDS 49152UL
#define OPT_NOHM 16384UL
#define OPT_NOPOINTER 16384UL
#define OPT_NOSECS 32768UL
#define OPT_NOTEAR 4UL
#define OPT_NOTICKS 8192UL
#define OPT_RGB565 0UL
#define OPT_RIGHTX 2048UL
#define OPT_SIGNED 256UL
#define OPT_SOUND 32UL

// Definitions for FT8xx co processor command buffer
#define FT_DL_SIZE (8 * 1024)       // 8KB Display List buffer size
#define FT_CMD_FIFO_SIZE (4 * 1024) // 4KB coprocessor Fifo size
#define FT_CMD_SIZE (4)             // 4 byte per coprocessor command of EVE

// Primitive Type Reference Definitions - FT81x Series Programmers Guide Section 4.5 - Table 6
#define BITMAPS 1
#define POINTS 2
#define LINES 3
#define LINE_STRIP 4
#define EDGE_STRIP_R 5
#define EDGE_STRIP_L 6
#define EDGE_STRIP_A 7
#define EDGE_STRIP_B 8
#define RECTS 9

// Co-processor Engine Registers - FT81x Series Programmers Guide Section 3.4
// Addresses defined as offsets from the base address called RAM_REG and located at 0x302000
#define REG_CMD_DL 0x100
#define REG_CMD_READ 0xF8
#define REG_CMD_WRITE 0xFC
#define REG_CMDB_SPACE 0x574
#define REG_CMDB_WRITE 0x578
#define REG_COPRO_PATCH_PTR 0x7162

// Miscellaneous Registers - FT81x Series Programmers Guide Section 3.6 - Document inspecific about base address
// Addresses assumed to be defined as offsets from the base address called RAM_REG and located at 0x302000
#define REG_CPU_RESET 0x20
#define REG_PWM_DUTY 0xD4
#define REG_PWM_HZ 0xD0
#define REG_INT_MASK 0xB0
#define REG_INT_EN 0xAC
#define REG_INT_FLAGS 0xA8
#define REG_GPIO 0x94
#define REG_GPIO_DIR 0x90
#define REG_GPIOX 0x9C
#define REG_GPIOX_DIR 0x98
#define REG_FREQUENCY 0x0C
#define REG_CLOCK 0x08
#define REG_FRAMES 0x04
#define REG_ID 0x00
#define REG_TRIM 0x10256C
#define REG_SPI_WIDTH 0x180
#define REG_CHIP_ID 0xC0000 // Temporary Chip ID location in RAMG

// Touch Screen Engine Registers - FT81x Series Programmers Guide Section 3.3
// Addresses defined as offsets from the base address called RAM_REG and located at 0x302000
#define REG_TOUCH_CONFIG 0x168
#define REG_TOUCH_TRANSFORM_A 0x150
#define REG_TOUCH_TRANSFORM_B 0x154
#define REG_TOUCH_TRANSFORM_C 0x158
#define REG_TOUCH_TRANSFORM_D 0x15C
#define REG_TOUCH_TRANSFORM_E 0x160
#define REG_TOUCH_TRANSFORM_F 0x164

// Resistive Touch Engine Registers - FT81x Series Programmers Guide Section 3.3.3 - Document confused
// Addresses defined as offsets from the base address called RAM_REG and located at 0x302000
#define REG_TOUCH_ADC_MODE 0x108
#define REG_TOUCH_CHARGE 0x10C
#define REG_TOUCH_DIRECT_XY 0x18C
#define REG_TOUCH_DIRECT_Z1Z2 0x190
#define REG_TOUCH_MODE 0x104
#define REG_TOUCH_OVERSAMPLE 0x114
#define REG_TOUCH_RAW_XY 0x11C
#define REG_TOUCH_RZ 0x120
#define REG_TOUCH_RZTHRESH 0x118
#define REG_TOUCH_SCREEN_XY 0x124
#define REG_TOUCH_SETTLE 0x110
#define REG_TOUCH_TAG 0x12C
#define REG_TOUCH_TAG_XY 0x128


#endif
