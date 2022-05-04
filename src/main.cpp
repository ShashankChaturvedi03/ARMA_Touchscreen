#include <mbed.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <main.h>

void screen_init();
void clear_screen();

// SPI spi(PE_14,PE_13,PE_12); //mosi,miso,sclk
SPI spi(PE_6, PE_5, PE_2); // mosi,miso,sclk

DigitalOut cs(PC_14); // Chip select
DigitalOut pd(PC_15);
DigitalOut led(LED4);

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

  uint8_t dummy = spi.write(0x00); // dummy

  buf = spi.write(0x00);

  cs = 1;

  return (buf);
}
uint16_t rd16(uint32_t address)
{
  uint8_t buf[2] = {0, 0};

  cs = 0;

  spi.write((address >> 16) & 0x3F);
  spi.write((address >> 8) & 0xff);
  spi.write(address & 0xff);

  uint8_t dummy = spi.write(0x00); // dummy

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

  uint8_t dummy = spi.write(0x00); // dummy

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

  spi.write((uint8_t)(parameter & 0xff)); // Little endian (yes, it is most significant bit first and least significant byte first)
  spi.write((uint8_t)(parameter >> 8));

  cs = 1;
}
void wr32(uint32_t address, uint32_t parameter)
{
  cs = 0;

  spi.write((uint8_t)((address >> 16) | 0x80)); // RAM_REG = 0x302000 and high bit is set - result always 0xB0
  spi.write((uint8_t)(address >> 8));           // Next byte of the register address
  spi.write((uint8_t)address);                  // Low byte of register address - usually just the 1 byte offset

  spi.write((uint8_t)(parameter & 0xff)); // Little endian (yes, it is most significant bit first and least significant byte first)
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
  wr32(FifoWriteLocation + RAM_CMD, data); // write the command at the globally tracked "write pointer" for the FIFO

  FifoWriteLocation += FT_CMD_SIZE;      // Increment the Write Address by the size of a command - which we just sent
  FifoWriteLocation %= FT_CMD_FIFO_SIZE; // Wrap the address to the FIFO space
}

void Cmd_Text(uint16_t x, uint16_t y, uint16_t font, uint16_t options, const char *str)
{
  uint16_t DataPtr, LoopCount, StrPtr;

  uint16_t length = (uint16_t)strlen(str);
  if (!length)
    return;

  uint32_t *data = (uint32_t *)calloc((length / 4) + 1, sizeof(uint32_t)); // Allocate memory for the string expansion

  StrPtr = 0;
  for (DataPtr = 0; DataPtr < (length / 4); ++DataPtr, StrPtr = StrPtr + 4)
    data[DataPtr] = (uint32_t)str[StrPtr + 3] << 24 | (uint32_t)str[StrPtr + 2] << 16 | (uint32_t)str[StrPtr + 1] << 8 | (uint32_t)str[StrPtr];

  for (LoopCount = 0; LoopCount < (length % 4); ++LoopCount, ++StrPtr)
    data[DataPtr] |= (uint32_t)str[StrPtr] << (LoopCount * 8);

  // Set up the command
  Send_CMD(CMD_TEXT);
  Send_CMD(((uint32_t)y << 16) | x);
  Send_CMD(((uint32_t)options << 16) | font);

  // Send out the text
  for (LoopCount = 0; LoopCount <= length / 4; LoopCount++)
    Send_CMD(data[LoopCount]); // These text bytes get sucked up 4 at a time and fired at the FIFO

  free(data);
}

void chip_wake()
{

  cs = 0;
  spi.write(0x00);
  spi.write(0x00);
  spi.write(0x00);
  wait_us(3000);
  cs = 1;
  return;
}

void Cmd_Track(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t tag)
{
  Send_CMD(CMD_TRACK);
  Send_CMD(((uint32_t)y << 16) | x);
  Send_CMD(((uint32_t)h << 16) | w);
  Send_CMD((uint32_t)tag);
}

void UpdateFIFO(void)
{
  wr16(REG_CMD_WRITE + RAM_REG, FifoWriteLocation); // We manually update the write position pointer
}

void Cmd_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t font, uint16_t options, const char *str)
{
  uint16_t DataPtr, LoopCount, StrPtr;

  uint16_t length = (uint16_t)strlen(str);
  if (!length)
    return;

  uint32_t *data = (uint32_t *)calloc((length / 4) + 1, sizeof(uint32_t));

  StrPtr = 0;
  for (DataPtr = 0; DataPtr < (length / 4); DataPtr++, StrPtr += 4)
    data[DataPtr] = (uint32_t)str[StrPtr + 3] << 24 | (uint32_t)str[StrPtr + 2] << 16 | (uint32_t)str[StrPtr + 1] << 8 | (uint32_t)str[StrPtr];

  for (LoopCount = 0; LoopCount < (length % 4); LoopCount++, StrPtr++)
    data[DataPtr] |= (uint32_t)str[StrPtr] << (LoopCount * 8);

  Send_CMD(CMD_BUTTON);
  Send_CMD(((uint32_t)y << 16) | x); // Put two 16 bit values together into one 32 bit value - do it little endian
  Send_CMD(((uint32_t)h << 16) | w);
  Send_CMD(((uint32_t)options << 16) | font);

  for (LoopCount = 0; LoopCount <= length / 4; LoopCount++)
    Send_CMD(data[LoopCount]);

  free(data);
}

void Cmd_Calibrate(uint32_t result)
{
  Send_CMD(CMD_CALIBRATE);
  Send_CMD(result);
}

// Sit and wait until the CoPro FIFO is empty
// Detect operational errors and print the error and stop.
void Wait4CoProFIFOEmpty(void)
{
  uint16_t ReadReg;
  uint8_t ErrChar;
  uint8_t buffy[2];
  do
  {
    ReadReg = rd16(REG_CMD_READ + RAM_REG);
    if (ReadReg == 0xFFF)
    {
      // this is a error which would require sophistication to fix and continue but we fake it somewhat unsuccessfully
      uint8_t Offset = 0;
      do
      {
        // Get the error character and display it
        ErrChar = rd8(RAM_ERR_REPORT + Offset);
        Offset++;
      } while ((ErrChar != 0) && (Offset < 128)); // when the last stuffed character was null, we are done

      // Eve is unhappy - needs a paddling.
      uint32_t Patch_Add = rd32(REG_COPRO_PATCH_PTR + RAM_REG);
      wr8(REG_CPU_RESET + RAM_REG, 1);
      wr8(REG_CMD_READ + RAM_REG, 0);
      wr8(REG_CMD_WRITE + RAM_REG, 0);
      wr8(REG_CMD_DL + RAM_REG, 0);
      wr8(REG_CPU_RESET + RAM_REG, 0);
      wr32(REG_COPRO_PATCH_PTR + RAM_REG, Patch_Add);
      wait_us(250000); // we already saw one error message and we don't need to see then 1000 times a second
    }
  } while (ReadReg != rd16(REG_CMD_WRITE + RAM_REG));
}

// A calibration screen for the touch digitizer
void MakeScreen_Calibrate(void)
{
  Send_CMD(CMD_DLSTART);
  Send_CMD(CLEAR_COLOR_RGB(0, 0, 0));
  Send_CMD(CLEAR(1, 1, 1));
  Cmd_Text(100, 10, 27, OPT_CENTER, "Tap on the dots");
  Cmd_Calibrate(0); // This widget generates a blocking screen that doesn't unblock until 3 dots have been touched
  Send_CMD(DISPLAY());
  Send_CMD(CMD_SWAP);
  UpdateFIFO(); // Trigger the CoProcessor to start processing commands out of the FIFO

  Wait4CoProFIFOEmpty(); // wait here until the coprocessor has read and executed every pending command.
}

void set_clockext()
{

  cs = 0;

  spi.write(0x44);
  spi.write(0x00);
  spi.write(0x00);

  wait_us(30000);

  cs = 1;
}

void clear_screen()
{
  cs = 0;
  wr32(RAM_DL + 0, CLEAR(1, 1, 1)); // clear screen
  wr32(RAM_DL + 4, CLEAR_COLOR_RGB(5, 45, 110));
  wr32(RAM_DL + 8, COLOR_RGB(255, 168, 64));
  wr32(RAM_DL + 12, CLEAR(1, 1, 1));
  cs = 1;
}

void blue_dot()
{

  wr32(RAM_DL + 44, COLOR_RGB(160, 22, 22));    // change colour to red
  wr32(RAM_DL + 48, POINT_SIZE(320));           // set point size to 20 pixels in radius
  wr32(RAM_DL + 52, BEGIN(POINTS));             // start drawing points
  wr32(RAM_DL + 56, VERTEX2II(192, 133, 0, 0)); // red point
  wr32(RAM_DL + 60, END());

  wr32(RAM_DL + 84, DISPLAY());            // display the image
  wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME); // swap display lists
  wr8(REG_PCLK + RAM_REG, 2);              // after this display is visible on the LCD
  wait_us(5000);
}

void red_dot(int dot)
{

  cs = 0;

  wr32(RAM_DL + 0, CLEAR(1, 1, 1)); // clear screen

  wr32(RAM_DL + 4, CLEAR_COLOR_RGB(5, 45, 110));
  wr32(RAM_DL + 8, COLOR_RGB(255, 168, 64));
  wr32(RAM_DL + 12, CLEAR(1, 1, 1));
  wr32(RAM_DL + 16, TAG(1));
  wr32(RAM_DL + 20, BEGIN(BITMAPS));            // start drawing bitmaps
  wr32(RAM_DL + 24, COLOR_RGB(160, 22, 22));    // change colour to red
  wr32(RAM_DL + 28, POINT_SIZE(dot));           // set point size to 20 pixels in radius
  wr32(RAM_DL + 32, BEGIN(POINTS));             // start drawing points
  wr32(RAM_DL + 36, VERTEX2II(350, 160, 0, 0)); // red point
  wr32(RAM_DL + 40, END());

  wr32(RAM_DL + 44, DISPLAY()); // display the image

  wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME); // swap display lists
  wr8(REG_PCLK + RAM_REG, 2);
  wr16(REG_CMD_WRITE + RAM_REG, FifoWriteLocation);

  cs = 1;
}

void write_ARMA()
{

  cs = 0;

  wr32(RAM_DL + 0, CLEAR(1, 1, 1));
  wr32(RAM_DL + 4, COLOR_RGB(120, 55, 55));
  wr32(RAM_DL + 8, DISPLAY());
  wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME);

  wr32(RAM_DL + 0, CLEAR(1, 1, 1)); // clear screen

  wr32(RAM_DL + 4, CLEAR_COLOR_RGB(55, 45, 40));
  wr32(RAM_DL + 8, COLOR_RGB(255, 24, 24));
  wr32(RAM_DL + 12, CLEAR(1, 1, 1));

  wr32(RAM_DL + 16, BEGIN(BITMAPS));               // start drawing bitmaps
  // wr32(RAM_DL + 20, VERTEX2II(150, 200, 31, 'W'));
  // wr32(RAM_DL + 24, VERTEX2II(178, 200, 31, 'E'));
  // wr32(RAM_DL + 28, VERTEX2II(202, 200, 31, 'L'));
  // wr32(RAM_DL + 32, VERTEX2II(228, 200, 31, 'C'));
  // wr32(RAM_DL + 36, VERTEX2II(253, 200, 31, 'O'));
  // wr32(RAM_DL + 40, VERTEX2II(277, 200, 31, 'M'));
  // wr32(RAM_DL + 44, VERTEX2II(302, 200, 31, 'E'));
  // wr32(RAM_DL + 48, VERTEX2II(351, 200, 31, 'T'));
  // wr32(RAM_DL + 52, VERTEX2II(377, 200, 31, 'O'));
  wr32(RAM_DL + 56, VERTEX2II(380, 200, 31, 'A')); // ascii F in font 31
  wr32(RAM_DL + 60, VERTEX2II(420, 200, 31, 'R')); // ascii T
  wr32(RAM_DL + 64, VERTEX2II(460, 200, 31, 'M')); // ascii D
  wr32(RAM_DL + 68, VERTEX2II(500, 200, 31, 'A')); // ascii I

  wr32(RAM_DL + 72, END());

  wr32(RAM_DL + 76, DISPLAY()); // display the image

  wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME); // swap display lists
  wr16(REG_CMD_WRITE + RAM_REG, FifoWriteLocation);
  wr8(REG_PCLK + RAM_REG, 2);

  cs = 1;
}

void screen_init()
{

  cs = 0;
  volatile uint8_t check_id = 0;
  check_id = rd8(0x302000);

  while (check_id != 0x7C)
  {
    check_id = rd8(0x302000);
    wait_us(100);
  }

  wr32(0x30200C, 0x3938700); // Set Clock Frequency

  if (rd16(REG_CMD_READ + RAM_REG) == 0xFFF)
  {
    // Eve is unhappy - needs a paddling.
    uint32_t Patch_Add = rd32(REG_COPRO_PATCH_PTR + RAM_REG);
    wr8(REG_CPU_RESET + RAM_REG, 1);
    wr16(REG_CMD_READ + RAM_REG, 0);
    wr16(REG_CMD_WRITE + RAM_REG, 0);
    wr16(REG_CMD_DL + RAM_REG, 0);
    wr8(REG_CPU_RESET + RAM_REG, 0);
    wr32(REG_COPRO_PATCH_PTR + RAM_REG, Patch_Add);
  }

  // turn off screen output during startup
  wr8(REG_GPIOX + RAM_REG, 0); // Set REG_GPIOX to 0 to turn off the LCD DISP signal
  wr8(REG_PCLK + RAM_REG, 0);  // Pixel Clock Output disabled

  wr16(RAM_REG + REG_HCYCLE, 928);
  wr16(RAM_REG + REG_HOFFSET, 43);
  wr16(RAM_REG + REG_HSYNC0, 0);
  wr16(RAM_REG + REG_HSYNC1, 41);
  wr16(RAM_REG + REG_VCYCLE, 525);
  wr16(RAM_REG + REG_VOFFSET, 12);
  wr16(RAM_REG + REG_VSYNC0, 0);
  wr16(RAM_REG + REG_VSYNC1, 10);
  wr8(RAM_REG + REG_SWIZZLE, 0);
  wr8(RAM_REG + REG_PCLK_POL, 1);
  wr8(RAM_REG + REG_CSPREAD, 1);
  wr16(RAM_REG + REG_HSIZE, 840);
  wr16(RAM_REG + REG_VSIZE, 500);
  wr8(REG_CSPREAD + RAM_REG, 1); // Set CSPREAD to 1    (32 bit register - write only 8 bits)
  wr8(REG_DITHER + RAM_REG, 1);

  wr16(REG_TOUCH_CONFIG + RAM_REG, 0x8381);

  // Resistivce touch Initialization

  wr16(REG_TOUCH_RZTHRESH + RAM_REG, 10);  // set touch resistance threshold (changed from 1200)
  wr8(REG_TOUCH_MODE + RAM_REG, 0x02);     // set touch on: continuous - this is default
  wr8(REG_TOUCH_ADC_MODE + RAM_REG, 0x01); // set ADC mode: differential - this is default
  wr8(REG_TOUCH_OVERSAMPLE + RAM_REG, 15); // set touch oversampling to max

  wr16(REG_GPIOX_DIR + RAM_REG, 0x8000); // Set Disp GPIO Direction
  wr16(REG_GPIOX + RAM_REG, 0x8000);     // Enable Disp (if used)

  // Backlight Configuration

  wr16(REG_PWM_HZ + RAM_REG, 0x00FA); // Backlight PWM frequency
  for (uint8_t duty = 0; duty <= 130; duty = duty + 40)
  {
    wr8(REG_PWM_DUTY + RAM_REG, duty); // Backlight PWM duty (on)
    wait_us(1100000);
  }

  // write first display list (which is a clear and blank screen)
  wr32(RAM_DL + 0, CLEAR_COLOR_RGB(0, 0, 0));
  wr32(RAM_DL + 4, CLEAR(1, 1, 1));
  wr32(RAM_DL + 8, DISPLAY());
  wr8(REG_DLSWAP + RAM_REG, DLSWAP_FRAME); // swap display lists
  wr8(REG_PCLK + RAM_REG, 2);              // after this display is visible on the LCD

  cs = 1;
}

int main()
{
  uint8_t Tag = 0;

  spi.format(8, 0);
  spi.frequency(12000000);

  pd = 1;
  chip_wake();
  wait_us(30000);
  set_clockext();
  wait_us(30000);
  screen_init();
  wait_us(30000);
  red_dot(400);
  wait_us(300000);
  MakeScreen_Calibrate();

  int LastTag = 0;

  while (1)
  {

    // wait_us(30000);
    // red_dot(400);
    wait_us(500000);
    // wait_us(5000000);
    // write_ARMA();
    // wait_us(5000000);

    int Tag = rd8(REG_TOUCH_TAG + RAM_REG);

    if (Tag != LastTag)
    {
      clear_screen();
      wait_us(10000);
      write_ARMA();
      wait_us(500000);
      LastTag = Tag;
    }
  }
}