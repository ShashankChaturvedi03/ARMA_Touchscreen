
#include <mbed.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

  //SPI spi(PE_14,PE_13,PE_12); //mosi,miso,sclk  
  SPI spi(PE_6,PE_5,PE_2);
  DigitalOut cs(PC_14);       //Chip select

#define RAM_REG                  0x302000   //Offset
#define RAM_G                    0x0
#define RAM_G_WORKING            0x0FF000 // This address may be used as the start of a 4K block to be used for copying data
#define RAM_DL                   0x300000
#define RAM_CMD                  0x308000
#define RAM_ERR_REPORT           0x309800 // max 128 bytes null terminated string
#define RAM_FLASH                0x800000
#define RAM_FLASH_POSTBLOB       0x801000

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


void chip_wake(){

    cs = 0;
    spi.write(0x00);
    spi.write(0x00);
    spi.write(0x00);
    wait_us(3000);
    cs = 1;
    return ;

  }
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
		PCLK = 2;
		SWIZZLE = 0;
		PCLK_POL = 1;
		HSIZE = 800;
		VSIZE = 480;
		CSPREAD = 0;
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
    wr8(REG_SWIZZLE + RAM_REG, SWIZZLE);        // Set SWIZZLE to 0
    wr8(REG_PCLK_POL + RAM_REG, PCLK_POL);      // Set PCLK_POL to 1
    wr16(REG_HSIZE + RAM_REG, HSIZE);           // Set H_SIZE to 480
    wr16(REG_VSIZE + RAM_REG, VSIZE);           // Set V_SIZE to 272
    wr8(REG_CSPREAD + RAM_REG, CSPREAD);        // Set CSPREAD to 1    (32 bit register - write only 8 bits)
    wr8(REG_DITHER + RAM_REG, DITHER);          // Set DITHER to 1     (32 bit register - write only 8 bits)

    wr32(RAM_DL+0, CLEAR_COLOR_RGB(1,1,1));
    wr32(RAM_DL+4, CLEAR(1,1,1));
    wr32(RAM_DL+8, DISPLAY());
    wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME);          // swap display lists
    wr8(REG_PCLK + RAM_REG, PCLK);                       // after this display is visible on the LCD

}




int main() {

 spi.format(8,0);
 spi.frequency(10000000);

  chip_wake();
cs = 1;

  while(1) {
 
    volatile uint8_t check_id = rd8(0x302000);
    if(check_id == 0x7C){
      init_screen();
    }
        wait_us(5000);
  }
}