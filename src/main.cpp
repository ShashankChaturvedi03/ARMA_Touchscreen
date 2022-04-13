#include <mbed.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

SPI spi(PE_14,PE_13,PE_12); //mosi,miso,sclk  
//SPI spi(PE_6,PE_5,PE_2);
DigitalOut cs(PC_14);       //Chip select
DigitalOut pd(PC_15);

// Memory base addresses
#define RAM_REG                  0x302000   //Offset
#define RAM_G                    0x0
#define RAM_G_WORKING            0x0FF000 // This address may be used as the start of a 4K block to be used for copying data
#define RAM_DL                   0x300000
#define RAM_CMD                  0x308000
#define RAM_ERR_REPORT           0x309800 // max 128 bytes null terminated string
#define RAM_FLASH                0x800000
#define RAM_FLASH_POSTBLOB       0x801000

// Graphics Engine Registers - FT81x Series Programmers Guide Section 3.1
// Addresses defined as offsets from the base address called RAM_REG and located at 0x302000
// Discussion: Defining this way leads to an additional add operation in code that can be avoided by defining
// these addresses as 32 bit values, but this is easily paid for in clarity and coorelation to documentation.
// Further, you can add defines together in code and allow the precompiler to do the add operation (as here).
#define REG_CSPREAD               0x68
#define REG_DITHER                0x60
#define REG_DLSWAP                0x54
#define REG_HCYCLE                0x2C
#define REG_HOFFSET               0x30    
#define REG_HSIZE                 0x34
#define REG_HSYNC0                0x38
#define REG_HSYNC1                0x3C
#define REG_OUTBITS               0x5C
#define REG_PCLK                  0x70
#define REG_PCLK_POL              0x6C
#define REG_PLAY                  0x8C
#define REG_PLAYBACK_FORMAT       0xC4
#define REG_PLAYBACK_FREQ         0xC0
#define REG_PLAYBACK_LENGTH       0xB8
#define REG_PLAYBACK_LOOP         0xC8
#define REG_PLAYBACK_PLAY         0xCC
#define REG_PLAYBACK_READPTR      0xBC
#define REG_PLAYBACK_START        0xB4
#define REG_PWM_DUTY              0xD4
#define REG_ROTATE                0x58
#define REG_SOUND                 0x88
#define REG_SWIZZLE               0x64
#define REG_TAG                   0x7C
#define REG_TAG_X                 0x74
#define REG_TAG_Y                 0x78
#define REG_VCYCLE                0x40
#define REG_VOFFSET               0x44
#define REG_VOL_SOUND             0x84
#define REG_VOL_PB                0x80
#define REG_VSYNC0                0x4C
#define REG_VSYNC1                0x50
#define REG_VSIZE                 0x48 

#define CLEAR(c1,s1,t1) ((38UL<<24)|(((c1)&1UL)<<2)|(((s1)&1UL)<<1)|(((t1)&1UL)<<0))                                                                                           // CLEAR - FT-PG Section 4.21
#define CLEAR_COLOR_RGB(red1,green1,blue1) ((2UL<<24)|(((red1)&255UL)<<16)|(((green1)&255UL)<<8)|(((blue1)&255UL)<<0))                                                         // CLEAR_COLOR_RGB - FT-PG Section 4.23
#define COLOR_RGB(red1,green1,blue1)       ((4UL<<24)|(((red1)&255UL)<<16)|(((green1)&255UL)<<8)|(((blue1)&255UL)<<0))                                                               // COLOR_RGB - FT-PG Section 4.28
#define VERTEX2II(x1,y1,handle1,cell1) ((2UL<<30)|(((x1)&511UL)<<21)|(((y1)&511UL)<<12)|(((handle1)&31UL)<<7)|(((cell1)&127UL)<<0))                                              // VERTEX2II - FT-PG Section 4.48
#define VERTEX2F(x1,y1) ((1UL<<30)|(((x1)&32767UL)<<15)|(((y1)&32767UL)<<0))                                                                                                 // VERTEX2F - FT-PG Section 4.47
#define CELL(cell1) ((6UL<<24)|(((cell1)&127UL)<<0))                                                                                                                       // CELL - FT-PG Section 4.20
#define BITMAP_HANDLE(handle1) ((5UL<<24) | (((handle1) & 31UL) << 0))                                                                                                     // BITMAP_HANDLE - FT-PG Section 4.06
#define BITMAP_SOURCE(addr1) ((1UL<<24)|(((addr1)&1048575UL)<<0))                                                                                                          // BITMAP_SOURCE - FT-PG Section 4.11
#define BITMAP_LAYOUT(format1,linestride1,height1) ((7UL<<24)|(((format1)&31UL)<<19)|(((linestride1)&1023UL)<<9)|(((height1)&511UL)<<0))                                       // BITMAP_LAYOUT - FT-PG Section 4.07
#define BITMAP_SIZE(filter1,wrapx1,wrapy1,width1,height1) ((8UL<<24)|(((filter1)&1UL)<<20)|(((wrapx1)&1UL)<<19)|(((wrapy1)&1UL)<<18)|(((width1)&511UL)<<9)|(((height1)&511UL)<<0)) // BITMAP_SIZE - FT-PG Section 4.09
#define TAG(s1) ((3UL<<24)|(((s1)&255UL)<<0))                                                                                                                              // TAG - FT-PG Section 4.43
#define POINT_SIZE(sighs1) ((13UL<<24)|(((sighs1)&8191UL)<<0))                                                                                                             // POINT_SIZE - FT-PG Section 4.36
#define BEGIN(PrimitiveTypeRef1) ((31UL<<24)|(((PrimitiveTypeRef1)&15UL)<<0))                                                                                              // BEGIN - FT-PG Section 4.05
#define END() ((33UL<<24))                                                                                                                                               // END - FT-PG Section 4.30
#define DISPLAY() ((0UL<<24))        

#define DLSWAP_FRAME         2UL

// Definitions for FT8xx co processor command buffer
#define FT_DL_SIZE           (8*1024)  // 8KB Display List buffer size
#define FT_CMD_FIFO_SIZE     (4*1024)  // 4KB coprocessor Fifo size
#define FT_CMD_SIZE          (4)       // 4 byte per coprocessor command of EVE

uint16_t FifoWriteLocation = 0;

#define HCMD_ACTIVE      0x00
#define HCMD_STANDBY     0x41
#define HCMD_SLEEP       0x42
#define HCMD_PWRDOWN     0x50
#define HCMD_CLKINT      0x48
#define HCMD_CLKEXT      0x44
#define HCMD_CLK48M      0x62
#define HCMD_CLK36M      0x61
#define HCMD_CORERESET   0x68

#define CMD_APPEND           0xFFFFFF1E
#define CMD_BGCOLOR          0xFFFFFF09
#define CMD_BUTTON           0xFFFFFF0D
#define CMD_CALIBRATE        0xFFFFFF15 // 4294967061UL
#define CMD_CLOCK            0xFFFFFF14
#define CMD_COLDSTART        0xFFFFFF32
#define CMD_CRC              0xFFFFFF18
#define CMD_DIAL             0xFFFFFF2D
#define CMD_DLSTART          0xFFFFFF00
#define CMD_FGCOLOR          0xFFFFFF0A
#define CMD_GAUGE            0xFFFFFF13
#define CMD_GETMATRIX        0xFFFFFF33
#define CMD_GETPROPS         0xFFFFFF25
#define CMD_GETPTR           0xFFFFFF23
#define CMD_GRADCOLOR        0xFFFFFF34
#define CMD_GRADIENT         0xFFFFFF0B
#define CMD_INFLATE          0xFFFFFF22
#define CMD_INFLATE2         0xFFFFFF50
#define CMD_INTERRUPT        0xFFFFFF02
#define CMD_KEYS             0xFFFFFF0E
#define CMD_LOADIDENTITY     0xFFFFFF26
#define CMD_LOADIMAGE        0xFFFFFF24
#define CMD_LOGO             0xFFFFFF31
#define CMD_MEDIAFIFO        0xFFFFFF39
#define CMD_MEMCPY           0xFFFFFF1D
#define CMD_MEMCRC           0xFFFFFF18
#define CMD_MEMSET           0xFFFFFF1B
#define CMD_MEMWRITE         0xFFFFFF1A
#define CMD_MEMZERO          0xFFFFFF1C
#define CMD_NUMBER           0xFFFFFF38
#define CMD_PLAYVIDEO        0xFFFFFF3A
#define CMD_PROGRESS         0xFFFFFF0F
#define CMD_REGREAD          0xFFFFFF19
#define CMD_ROTATE           0xFFFFFF29
#define CMD_SCALE            0xFFFFFF28
#define CMD_SCREENSAVER      0xFFFFFF2F
#define CMD_SCROLLBAR        0xFFFFFF11
#define CMD_SETBITMAP        0xFFFFFF43
#define CMD_SETFONT          0xFFFFFF2B
#define CMD_SETMATRIX        0xFFFFFF2A
#define CMD_SETROTATE        0xFFFFFF36
#define CMD_SKETCH           0xFFFFFF30
#define CMD_SLIDER           0xFFFFFF10
#define CMD_SNAPSHOT         0xFFFFFF1F
#define CMD_SPINNER          0xFFFFFF16
#define CMD_STOP             0xFFFFFF17
#define CMD_SWAP             0xFFFFFF01
#define CMD_TEXT             0xFFFFFF0C
#define CMD_TOGGLE           0xFFFFFF12
#define CMD_TRACK            0xFFFFFF2C
#define CMD_TRANSLATE        0xFFFFFF27
#define CMD_VIDEOFRAME       0xFFFFFF41
#define CMD_VIDEOSTART       0xFFFFFF40

// BT81X COMMANDS 
#define CMD_FLASHERASE       0xFFFFFF44
#define CMD_FLASHWRITE       0xFFFFFF45
#define CMD_FLASHREAD        0xFFFFFF46
#define CMD_FLASHUPDATE      0xFFFFFF47
#define CMD_FLASHDETACH      0xFFFFFF48
#define CMD_FLASHATTACH      0xFFFFFF49
#define CMD_FLASHFAST        0xFFFFFF4A
#define CMD_FLASHSPIDESEL    0xFFFFFF4B
#define CMD_FLASHSPITX       0xFFFFFF4C
#define CMD_FLASHSPIRX       0xFFFFFF4D
#define CMD_FLASHSOURCE      0xFFFFFF4E
#define CMD_CLEARCACHE       0xFFFFFF4F
#define CMD_FLASHAPPENDF     0xFFFFFF59
#define CMD_VIDEOSTARTF      0xFFFFFF5F

#define CMD_ROMFONT          0xFFFFFF3F

#define DLSWAP_FRAME         2UL

#define OPT_CENTER           1536UL
#define OPT_CENTERX          512UL
#define OPT_CENTERY          1024UL
#define OPT_FLASH            64UL
#define OPT_FLAT             256UL
#define OPT_FULLSCREEN       8UL
#define OPT_MEDIAFIFO        16UL
#define OPT_MONO             1UL
#define OPT_NOBACK           4096UL
#define OPT_NODL             2UL
#define OPT_NOHANDS          49152UL
#define OPT_NOHM             16384UL
#define OPT_NOPOINTER        16384UL
#define OPT_NOSECS           32768UL
#define OPT_NOTEAR           4UL
#define OPT_NOTICKS          8192UL
#define OPT_RGB565           0UL
#define OPT_RIGHTX           2048UL
#define OPT_SIGNED           256UL
#define OPT_SOUND            32UL

// Definitions for FT8xx co processor command buffer
#define FT_DL_SIZE           (8*1024)  // 8KB Display List buffer size
#define FT_CMD_FIFO_SIZE     (4*1024)  // 4KB coprocessor Fifo size
#define FT_CMD_SIZE          (4)       // 4 byte per coprocessor command of EVE

// Primitive Type Reference Definitions - FT81x Series Programmers Guide Section 4.5 - Table 6
#define BITMAPS                    1
#define POINTS                     2
#define LINES                      3
#define LINE_STRIP                 4
#define EDGE_STRIP_R               5
#define EDGE_STRIP_L               6
#define EDGE_STRIP_A               7
#define EDGE_STRIP_B               8
#define RECTS                      9

// Co-processor Engine Registers - FT81x Series Programmers Guide Section 3.4
// Addresses defined as offsets from the base address called RAM_REG and located at 0x302000
#define REG_CMD_DL                0x100
#define REG_CMD_READ              0xF8
#define REG_CMD_WRITE             0xFC
#define REG_CMDB_SPACE            0x574
#define REG_CMDB_WRITE            0x578
#define REG_COPRO_PATCH_PTR       0x7162

// Miscellaneous Registers - FT81x Series Programmers Guide Section 3.6 - Document inspecific about base address
// Addresses assumed to be defined as offsets from the base address called RAM_REG and located at 0x302000
#define REG_CPU_RESET             0x20
#define REG_PWM_DUTY              0xD4
#define REG_PWM_HZ                0xD0
#define REG_INT_MASK              0xB0
#define REG_INT_EN                0xAC
#define REG_INT_FLAGS             0xA8
#define REG_GPIO                  0x94
#define REG_GPIO_DIR              0x90
#define REG_GPIOX                 0x9C
#define REG_GPIOX_DIR             0x98
#define REG_FREQUENCY             0x0C
#define REG_CLOCK                 0x08
#define REG_FRAMES                0x04
#define REG_ID                    0x00
#define REG_TRIM                  0x10256C
#define REG_SPI_WIDTH             0x180
#define REG_CHIP_ID               0xC0000   // Temporary Chip ID location in RAMG


static uint32_t Width;
static uint32_t Height;
static uint32_t HOffset;
static uint32_t VOffset;
static uint8_t Touch;

uint8_t rd8(uint32_t address)
{
  uint8_t buf;
  
 cs = 0;
  
  spi.write((address >> 16) & 0x3F);    
  spi.write((address >> 8) & 0xff);    
  spi.write(address & 0xff);
  
   uint8_t dummy = spi.write(0x00);    //dummy

    buf = spi.write(0x00);
    
  
  cs = 1;
  
  return (buf);  
}
uint16_t rd16(uint32_t address)
{
	uint8_t buf[2] = { 0,0 };
    
  cs = 0;
  
  spi.write((address >> 16) & 0x3F);    
  spi.write((address >> 8) & 0xff);    
  spi.write(address & 0xff);
  
  uint8_t dummy = spi.write(0x00);    //dummy

  buf[0] = spi.write(0x00);
  buf[1] = spi.write(0x00);
  
  uint16_t Data16 = buf[0] + ((uint16_t)buf[1] << 8);
  
  cs = 1;

  return (Data16);  
}
uint32_t rd32(uint32_t address)
{
  uint8_t buf[4];
  uint32_t Data32;
  
  cs = 0;
  
  spi.write((address >> 16) & 0x3F);    
  spi.write((address >> 8) & 0xff);    
  spi.write(address & 0xff);
  
  uint8_t dummy = spi.write(0x00);    //dummy

  buf[0] = spi.write(0x00);
  buf[1] = spi.write(0x00);
   buf[2] = spi.write(0x00);
  buf[3] = spi.write(0x00);

  
  cs = 1;
  
  Data32 = buf[0] + ((uint32_t)buf[1] << 8) + ((uint32_t)buf[2] << 16) + ((uint32_t)buf[3] << 24);
  
  return (Data32);  
}
void wr8(uint32_t address, uint8_t parameter)
{
  cs = 0;
  
  spi.write((uint8_t)((address >> 16) | 0x80)); // RAM_REG = 0x302000 and high bit is set - result always 0xB0
  spi.write((uint8_t)(address >> 8));           // Next byte of the register address   
  spi.write((uint8_t)(address));                // Low byte of register address - usually just the 1 byte offset
  
  spi.write(parameter);             
  
  cs = 1;
}
void wr16(uint32_t address, uint16_t parameter)
{
  cs = 0;
  
  spi.write((uint8_t)((address >> 16) | 0x80)); // RAM_REG = 0x302000 and high bit is set - result always 0xB0
  spi.write((uint8_t)(address >> 8));           // Next byte of the register address   
  spi.write((uint8_t)address);                  // Low byte of register address - usually just the 1 byte offset
  
  spi.write((uint8_t)(parameter & 0xff));       // Little endian (yes, it is most significant bit first and least significant byte first)
  spi.write((uint8_t)(parameter >> 8));
  
  cs = 1;
}
void wr32(uint32_t address, uint32_t parameter)
{
  cs = 0;
  
  spi.write((uint8_t)((address >> 16) | 0x80));   // RAM_REG = 0x302000 and high bit is set - result always 0xB0
  spi.write((uint8_t)(address >> 8));             // Next byte of the register address   
  spi.write((uint8_t)address);                    // Low byte of register address - usually just the 1 byte offset
  
  spi.write((uint8_t)(parameter & 0xff));         // Little endian (yes, it is most significant bit first and least significant byte first)
  spi.write((uint8_t)((parameter >> 8) & 0xff));
  spi.write((uint8_t)((parameter >> 16) & 0xff));
  spi.write((uint8_t)((parameter >> 24) & 0xff));
  
  cs = 1;

}


uint32_t Display_Width()
{
	return Width;
}
uint32_t Display_Height()
{
	return Height;
}
uint8_t Display_Touch()
{
	return Touch;
}
uint32_t Display_HOffset()
{
	return HOffset;
}
uint32_t Display_VOffset()
{
	return VOffset;
}
void Send_CMD(uint32_t data)
{
  wr32(FifoWriteLocation + RAM_CMD, data);                         // write the command at the globally tracked "write pointer" for the FIFO

  FifoWriteLocation += FT_CMD_SIZE;                                // Increment the Write Address by the size of a command - which we just sent
  FifoWriteLocation %= FT_CMD_FIFO_SIZE;                           // Wrap the address to the FIFO space
}
void Cmd_Text(uint16_t x, uint16_t y, uint16_t font, uint16_t options, const char* str)
{
  uint16_t DataPtr, LoopCount, StrPtr;
  
  uint16_t length = (uint16_t) strlen(str);
  if(!length) 
    return; 
  
  uint32_t* data = (uint32_t*) calloc((length / 4) + 1, sizeof(uint32_t)); // Allocate memory for the string expansion
  
  StrPtr = 0;
  for(DataPtr=0; DataPtr<(length/4); ++DataPtr, StrPtr=StrPtr+4)
    data[DataPtr] = (uint32_t)str[StrPtr+3]<<24 | (uint32_t)str[StrPtr+2]<<16 | (uint32_t)str[StrPtr+1]<<8 | (uint32_t)str[StrPtr];
  
  for(LoopCount=0; LoopCount<(length%4); ++LoopCount, ++StrPtr)
    data[DataPtr] |= (uint32_t)str[StrPtr] << (LoopCount*8);

  // Set up the command
  Send_CMD(CMD_TEXT);
  Send_CMD( ((uint32_t)y << 16) | x );
  Send_CMD( ((uint32_t)options << 16) | font );

  // Send out the text
  for(LoopCount = 0; LoopCount <= length/4; LoopCount++)
    Send_CMD(data[LoopCount]);  // These text bytes get sucked up 4 at a time and fired at the FIFO

  free(data);
}
void chip_wake(){

    cs = 0;
    spi.write(0x00);
    spi.write(0x00);
    spi.write(0x00);
    wait_us(3000);
    cs = 1;
    return ;

  }


void init_screen(){
  int DWIDTH;
	int DHEIGHT;
	int PIXVOFFSET;
	int PIXHOFFSET;
	int HCYCLE;
	int HOFFSET;
	int HSYNC0;
	int HSYNC1;
	int VCYCLE;
	int VOFFSET;
	int VSYNC0;
	int VSYNC1;
	int PCLK;
	int SWIZZLE;
	int PCLK_POL;
	int HSIZE;
	int VSIZE;
	int CSPREAD;
	int DITHER;

  	/*
    DWIDTH = 800;
		DHEIGHT = 480;
		PIXVOFFSET = 0;
		PIXHOFFSET = 0;
		HCYCLE = 928;
		HOFFSET = 88;
		HSYNC0 = 0;
		HSYNC1 = 48;
		VCYCLE = 525;
		VOFFSET = 32;
		VSYNC0 = 0;
		VSYNC1 = 3;
		PCLK = 2; //changed from 2
		SWIZZLE = 0;
		PCLK_POL = 1;
		HSIZE = 800;
		VSIZE = 480;
		CSPREAD = 0;
		DITHER = 1;

    */

   	DWIDTH = 480;
		DHEIGHT = 272;
		PIXVOFFSET = 0;
		PIXHOFFSET = 0;
		HCYCLE = 548;
		HOFFSET = 43;
		HSYNC0 = 0;
		HSYNC1 = 41;
		VCYCLE = 292;
		VOFFSET = 12;
		VSYNC0 = 0;
		VSYNC1 = 10;
		PCLK = 2; //changed from 2
		SWIZZLE = 0;
		PCLK_POL = 1;
		HSIZE = 800;
		VSIZE = 480;
		CSPREAD = 1;
		DITHER = 1;

    uint8_t Width = DWIDTH;
    uint8_t Height = DHEIGHT;
    uint8_t HOffset = PIXHOFFSET;
    uint8_t VOffset = PIXVOFFSET;
    

    wr32(0x30200C,0x989680); //Set Clock Frequency


    wr16(REG_HCYCLE + RAM_REG, HCYCLE);         // Set H_Cycle to 548
    wr16(REG_HOFFSET + RAM_REG, HOFFSET);       // Set H_Offset to 43
    wr16(REG_HSYNC0 + RAM_REG, HSYNC0);         // Set H_SYNC_0 to 0
    wr16(REG_HSYNC1 + RAM_REG, HSYNC1);         // Set H_SYNC_1 to 41
    wr16(REG_VCYCLE + RAM_REG, VCYCLE);         // Set V_Cycle to 292
    wr16(REG_VOFFSET + RAM_REG, VOFFSET);       // Set V_OFFSET to 12
    wr16(REG_VSYNC0 + RAM_REG, VSYNC0);         // Set V_SYNC_0 to 0
    wr16(REG_VSYNC1 + RAM_REG, VSYNC1);         // Set V_SYNC_1 to 10
    wr8(REG_SWIZZLE + RAM_REG, 0);        // Set SWIZZLE to 0 change 0 to swizzle
    wr8(REG_PCLK_POL + RAM_REG, PCLK_POL);      // Set PCLK_POL to 1
    wr16(REG_HSIZE + RAM_REG, HSIZE);           // Set H_SIZE to 480
    wr16(REG_VSIZE + RAM_REG, VSIZE);           // Set V_SIZE to 272
    wr8(REG_CSPREAD + RAM_REG, CSPREAD);        // Set CSPREAD to 1    (32 bit register - write only 8 bits)
    //wr8(REG_DITHER + RAM_REG, DITHER);          // Set DITHER to 1     (32 bit register - write only 8 bits)



    wr32(RAM_DL+0, CLEAR_COLOR_RGB(0,0,0));
    wr32(RAM_DL+4, CLEAR(1,1,1));
    wr32(RAM_DL+8, DISPLAY());

    wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME);          // swap display lists
   
       wr16(REG_GPIOX_DIR + RAM_REG, 0x8000);             // Set Disp GPIO Direction 
    wr16(REG_GPIOX + RAM_REG, 0x8000);                 // Enable Disp (if used)
    wr8(REG_PCLK + RAM_REG, 5);                       // after this display is visible on the LCD

}
void UpdateFIFO(void)
{
  wr16(REG_CMD_WRITE + RAM_REG, FifoWriteLocation);               // We manually update the write position pointer
}
void MakeScreen_MatrixOrbital(uint8_t DotSize)
{
	Send_CMD(CMD_DLSTART);                  // Start a new display list
	Send_CMD(CLEAR_COLOR_RGB(0, 0, 0));     // Determine the clear screen color
	Send_CMD(CLEAR(1, 1, 1));	            // Clear the screen and the current display list
	Send_CMD(TAG(1));                       // Tag the blue dot with a touch ID
	Send_CMD(COLOR_RGB(0, 0, 0));           // change colour to black
	Send_CMD(BEGIN(RECTS));                 // start drawing point
	Send_CMD(VERTEX2F(0,0));
	Send_CMD(VERTEX2F(Display_Width()*16,Display_Height()*16));
	Send_CMD(BEGIN(POINTS));                 // start drawing point
	Send_CMD(COLOR_RGB(26, 26, 192));        // change colour to blue
	Send_CMD(POINT_SIZE(DotSize * 16));      // set point size to DotSize pixels. Points = (pixels x 16)
	Send_CMD(VERTEX2II(Display_Width() / 2, Display_Height() / 2, 0, 0));     // place blue point
	Send_CMD(END());                         // end drawing point
	Send_CMD(COLOR_RGB(255, 255, 255));      //Change color to white for text
	Cmd_Text(Display_Width() / 2, Display_Height() / 2, 30, OPT_CENTER, " MATRIX         ORBITAL"); //Write text in the center of the screen
	Send_CMD(DISPLAY());                     //End the display list
	Send_CMD(CMD_SWAP);                      //Swap commands into RAM
	UpdateFIFO();                            // Trigger the CoProcessor to start processing the FIFO
}


void set_clockext(){

  cs = 0;

  spi.write(0x44);
  spi.write(0x00);
  spi.write(0x00);
  
  wait_us(30000);

  cs = 1;


}
int main() {

 spi.format(8,0);
 spi.frequency(10000000);

  chip_wake();
  wait_us(30000);
  set_clockext();
  wait_us(30000);
  cs = 0;
  volatile uint8_t check_id = rd8(0x302000);
    if(check_id == 0x7C){
      pd = 1;
      init_screen();
    }
 //  MakeScreen_MatrixOrbital(20);

cs = 1;

  while(1) {

        Send_CMD(COLOR_RGB(80, 26, 192));   
        wait_us(5000);
        wr32(RAM_REG + RAM_DL,COLOR_RGB(26,54,255));
        //wr32(RAM_REG + RAM_DL, CLEAR(1,1,1));

        wait_us(5000);
  }
}