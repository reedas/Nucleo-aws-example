/* mbed TM1638 Library, for TM1638 LED controller
 * Copyright (c) 2015, v01: WH, Initial version
 *               2016, v02: WH, refactored display and keyboard defines 
 *               2016, v03: WH, Added QYF-TM1638 and LKM1638, refactoring of writeData() 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TM1638_H
#define TM1638_H
#include "mbed.h"

// Select one of the testboards for TM1638 LED controller
#include "TM1638_Config.h"

/** An interface for driving TM1638 LED controller
 *
 * @code
 * #include "mbed.h"
 * #include "TM1638.h" 
 * 
 * DisplayData_t size is 16 bytes (8 grids @ 10 segments)
 * TM1638::DisplayData_t all_str  = {0xFF,0x3F, 0xFF,0x3F, 0xFF,0x3F, 0xFF,0x3F, 0xFF,0x3F, 0xFF,0x3F, 0xFF,0x3F, 0xFF,0x3F};  
 *
 * // KeyData_t size is 4 bytes  
 * TM1638::KeyData_t keydata; 
 *
 * // TM1638 declaration
 * TM1638 TM1638(p5,p6,p7, p8);
 *
 * int main() {
 *   TM1638.cls(); 
 *   TM1638.writeData(all_str);
 *   wait(1);
 *   TM1638.setBrightness(TM1638_BRT0);
 *   wait(1);
 *   TM1638.setBrightness(TM1638_BRT3);
 *
 *   while (1) {
 *    // Check and read keydata
 *    if (TM1638.getKeys(&keydata)) {
 *      pc.printf("Keydata 0..3 = 0x%02x 0x%02x 0x%02x 0x%02x\r\n", keydata[0], keydata[1], keydata[2], keydata[3]);
 *
 *      if (keydata[0] == 0x01) { //sw1   
 *        TM1638.cls(); 
 *        TM1638.writeData(all_str);
 *      }  
 *    } 
 *   }   
 * }
 * @endcode
 */


//TM1638 Display and Keymatrix data
#define TM1638_MAX_NR_GRIDS    8
#define TM1638_BYTES_PER_GRID  2
//Significant bits Keymatrix data
#define TM1638_KEY_MSK      0x77 

//Memory size in bytes for Display and Keymatrix
#define TM1638_DISPLAY_MEM  (TM1638_MAX_NR_GRIDS * TM1638_BYTES_PER_GRID)
#define TM1638_KEY_MEM         4


//Reserved bits for commands
#define TM1638_CMD_MSK      0xC0

//Data setting commands
#define TM1638_DATA_SET_CMD 0x40
#define TM1638_DATA_WR      0x00
#define TM1638_KEY_RD       0x02
#define TM1638_ADDR_INC     0x00
#define TM1638_ADDR_FIXED   0x04
#define TM1638_MODE_NORM    0x00
#define TM1638_MODE_TEST    0x08

//Address setting commands
#define TM1638_ADDR_SET_CMD 0xC0
#define TM1638_ADDR_MSK     0x0F

//Display control commands
#define TM1638_DSP_CTRL_CMD 0x80
#define TM1638_BRT_MSK      0x07
#define TM1638_BRT0         0x00 //Pulsewidth 1/16
#define TM1638_BRT1         0x01
#define TM1638_BRT2         0x02
#define TM1638_BRT3         0x03
#define TM1638_BRT4         0x04
#define TM1638_BRT5         0x05
#define TM1638_BRT6         0x06
#define TM1638_BRT7         0x07 //Pulsewidth 14/16

#define TM1638_BRT_DEF      TM1638_BRT3

#define TM1638_DSP_OFF      0x00
#define TM1638_DSP_ON       0x08


/** A class for driving TM1638 LED controller
 *
 * @brief Supports 8 Grids @ 10 Segments. 
 *        Also supports a scanned keyboard of upto 24 keys.
 *        SPI bus interface device. 
 */
class TM1638 {
 public:
  /** Datatype for display data */
  typedef char DisplayData_t[TM1638_DISPLAY_MEM];
  
  /** Datatypes for keymatrix data */
  typedef char KeyData_t[TM1638_KEY_MEM];
    
 /** Constructor for class for driving TM1638 LED controller
  *
  * @brief Supports 8 Grids @ 10 segments. 
  *        Also supports a scanned keyboard of upto 24 keys.
  *        SPI bus interface device. 
  *
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
  TM1638(PinName mosi, PinName miso, PinName sclk, PinName cs);
 
  /** Clear the screen and locate to 0
   */ 
  void cls();  

  /** Write databyte to TM1638
   *  @param  char data byte written at given address
   *  @param  int address display memory location to write byte
   *  @return none
   */ 
   void writeData(char data, int address); 

   /** Write Display datablock to TM1638
    *  @param  DisplayData_t data Array of TM1638_DISPLAY_MEM (=16) bytes for displaydata
    *  @param  length number bytes to write (valid range 0..(TM1638_MAX_NR_GRIDS * TM1638_BYTES_PER_GRID) (=16), when starting at address 0)  
    *  @param  int address display memory location to write bytes (default = 0) 
    *  @return none
    */ 
    void writeData(DisplayData_t data, int length = (TM1638_MAX_NR_GRIDS * TM1638_BYTES_PER_GRID), int address = 0);
 
  /** Read keydata block from TM1638
   *  @param  *keydata Ptr to Array of TM1638_KEY_MEM (=4) bytes for keydata
   *  @return bool keypress True when at least one key was pressed
   *
   * Note: Due to the hardware configuration the TM1638 key matrix scanner will detect multiple keys pressed at same time,
   *       but this may result in some spurious keys also being set in keypress data array.
   *       It may be best to ignore all keys in those situations. That option is implemented in this method depending on #define setting.
   */   
  bool getKeys(KeyData_t *keydata);

  /** Set Brightness
    *
    * @param  char brightness (3 significant bits, valid range 0..7 (1/16 .. 14/16 dutycycle)  
    * @return none
    */
  void setBrightness(char brightness = TM1638_BRT_DEF);
  
  /** Set the Display mode On/off
    *
    * @param bool display mode
    */
  void setDisplay(bool on);
  
 private:  
  SPI _spi;
  DigitalOut _cs;
  char _display;
  char _bright; 
  
  /** Init the SPI interface and the controller
    * @param  none
    * @return none
    */ 
  void _init();

  /** Helper to reverse all command or databits. The TM1638 expects LSB first, whereas SPI is MSB first
    *  @param  char data
    *  @return bitreversed data
    */ 
  char _flip(char data);

  /** Write command and parameter to TM1638
    *  @param  int cmd Command byte
    *  &Param  int data Parameters for command
    *  @return none
    */ 
  void _writeCmd(int cmd, int data);  
};


#if (LEDKEY8_TEST == 1) 
// Derived class for TM1638 used in LED&KEY display unit
//

#include "Font_7Seg.h"

#define LEDKEY8_NR_GRIDS  8
#define LEDKEY8_NR_DIGITS 8
#define LEDKEY8_NR_UDC    8

//Access to 8 Switches
#define LEDKEY8_SW1_IDX   0
#define LEDKEY8_SW1_BIT   0x01
#define LEDKEY8_SW2_IDX   1
#define LEDKEY8_SW2_BIT   0x01
#define LEDKEY8_SW3_IDX   2
#define LEDKEY8_SW3_BIT   0x01
#define LEDKEY8_SW4_IDX   3
#define LEDKEY8_SW4_BIT   0x01

#define LEDKEY8_SW5_IDX   0
#define LEDKEY8_SW5_BIT   0x10
#define LEDKEY8_SW6_IDX   1
#define LEDKEY8_SW6_BIT   0x10
#define LEDKEY8_SW7_IDX   2
#define LEDKEY8_SW7_BIT   0x10
#define LEDKEY8_SW8_IDX   3
#define LEDKEY8_SW8_BIT   0x10

/** Constructor for class for driving TM1638 controller as used in LEDKEY8
  *
  *  @brief Supports 8 Digits of 7 Segments + DP + LED Icons, Also supports a scanned keyboard of 8 keys.
  *  
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
class TM1638_LEDKEY8 : public TM1638 {
 public:

  /** Enums for Icons */
  //  Grid encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  enum Icon {
    LD1  = (1<<24) | S7_LD1, /**<  LED1 */
    LD2  = (2<<24) | S7_LD2, /**<  LED2 */
    LD3  = (3<<24) | S7_LD3, /**<  LED3 */
    LD4  = (4<<24) | S7_LD4, /**<  LED4 */
    LD5  = (5<<24) | S7_LD5, /**<  LED5 */
    LD6  = (6<<24) | S7_LD6, /**<  LED6 */
    LD7  = (7<<24) | S7_LD7, /**<  LED7 */
    LD8  = (8<<24) | S7_LD8, /**<  LED8 */
                                
    DP1  = (1<<24) | S7_DP1, /**<  Decimal Point 1 */
    DP2  = (2<<24) | S7_DP2, /**<  Decimal Point 2 */
    DP3  = (3<<24) | S7_DP3, /**<  Decimal Point 3 */
    DP4  = (4<<24) | S7_DP4, /**<  Decimal Point 4 */
    DP5  = (5<<24) | S7_DP5, /**<  Decimal Point 5 */
    DP6  = (6<<24) | S7_DP6, /**<  Decimal Point 6 */
    DP7  = (7<<24) | S7_DP7, /**<  Decimal Point 7 */
    DP8  = (8<<24) | S7_DP8  /**<  Decimal Point 8 */  
  };
  
  typedef char UDCData_t[LEDKEY8_NR_UDC];
  
 /** Constructor for class for driving TM1638 LED controller as used in LEDKEY8
   *
   * @brief Supports 8 Digits of 7 Segments + DP + LED Icons. Also supports a scanned keyboard of 8 keys.
   *  
   * @param  PinName mosi, miso, sclk, cs SPI bus pins
   */
  TM1638_LEDKEY8(PinName mosi, PinName miso, PinName sclk, PinName cs);

#if DOXYGEN_ONLY
    /** Write a character to the Display
     *
     * @param c The character to write to the display
     */
    int putc(int c);

    /** Write a formatted string to the Display
     *
     * @param format A printf-style format string, followed by the
     *               variables to use in formatting the string.
     */
    int printf(const char* format, ...);   
#endif

/* Display a tring in ascii to the 8 digit seven segment display 
 * TODO start the string at the locate column
 *
 */
 void displayStringAt(char *inString, int column) ;

     /** Locate cursor to a screen column
     *
     * @param column  The horizontal position from the left, indexed from 0
     */
    void locate(int column);
    
    /** Clear the screen and locate to 0
     * @param bool clrAll Clear Icons also (default = false)
     */
    void cls(bool clrAll = false);

    /** Set Icon
     *
     * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
     * @return none
     */
    void setIcon(Icon icon);

    /** Clr Icon
     *
     * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
     * @return none
     */
    void clrIcon(Icon icon);

   /** Set User Defined Characters (UDC)
     *
     * @param unsigned char udc_idx   The Index of the UDC (0..7)
     * @param int udc_data            The bitpattern for the UDC (16 bits)       
     */
    void setUDC(unsigned char udc_idx, int udc_data);


   /** Number of screen columns
    *
    * @param none
    * @return columns
    */
    int columns();   

   /** Write databyte to TM1638
     *  @param  char data byte written at given address
     *  @param  int address display memory location to write byte
     *  @return none
     */ 
    void writeData(char data, int address){
      TM1638::writeData(data, address);
    }        

   /** Write Display datablock to TM1638
    *  @param  DisplayData_t data Array of TM1638_DISPLAY_MEM (=16) bytes for displaydata
    *  @param  length number bytes to write (valid range 0..(LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID) (=16), when starting at address 0)  
    *  @param  int address display memory location to write bytes (default = 0) 
    *  @return none
    */   
    void writeData(DisplayData_t data, int length = (LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID), int address = 0) {
      TM1638::writeData(data, length, address);
    }  

protected:  
    // Stream implementation functions
    virtual int _putc(int value);
    virtual int _getc();

private:
    int _column;
    int _columns;   
    
    DisplayData_t _displaybuffer;
    UDCData_t _UDC_7S; 
};
#endif


#if (QYF_TEST == 1) 
// Derived class for TM1638 used in QYF-TM1638 display unit
//

#include "Font_7Seg.h"

#define QYF_NR_GRIDS  8
#define QYF_NR_DIGITS 8
#define QYF_NR_UDC    8

//Access to 16 Switches
#define QYF_SW1_IDX   0
#define QYF_SW1_BIT   0x04
#define QYF_SW2_IDX   0
#define QYF_SW2_BIT   0x40
#define QYF_SW3_IDX   1
#define QYF_SW3_BIT   0x04
#define QYF_SW4_IDX   1
#define QYF_SW4_BIT   0x40

#define QYF_SW5_IDX   2
#define QYF_SW5_BIT   0x04
#define QYF_SW6_IDX   2
#define QYF_SW6_BIT   0x40
#define QYF_SW7_IDX   3
#define QYF_SW7_BIT   0x04
#define QYF_SW8_IDX   3
#define QYF_SW8_BIT   0x40

#define QYF_SW9_IDX    0
#define QYF_SW9_BIT    0x02
#define QYF_SW10_IDX   0
#define QYF_SW10_BIT   0x20
#define QYF_SW11_IDX   1
#define QYF_SW11_BIT   0x02
#define QYF_SW12_IDX   1
#define QYF_SW12_BIT   0x20

#define QYF_SW13_IDX   2
#define QYF_SW13_BIT   0x02
#define QYF_SW14_IDX   2
#define QYF_SW14_BIT   0x20
#define QYF_SW15_IDX   3
#define QYF_SW15_BIT   0x02
#define QYF_SW16_IDX   3
#define QYF_SW16_BIT   0x20

/** Constructor for class for driving TM1638 controller as used in QYF
  *
  *  @brief Supports 8 Digits of 7 Segments + DP, Also supports a scanned keyboard of 16 keys.
  *  
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
class TM1638_QYF : public TM1638, public Stream {
 public:

  /** Enums for Icons */
  //  Grid encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  enum Icon {
    DP1  = (8<<24) | S7_DP1, /**<  Decimal Point 1 */
    DP2  = (8<<24) | S7_DP2, /**<  Decimal Point 2 */
    DP3  = (8<<24) | S7_DP3, /**<  Decimal Point 3 */
    DP4  = (8<<24) | S7_DP4, /**<  Decimal Point 4 */
    DP5  = (8<<24) | S7_DP5, /**<  Decimal Point 5 */
    DP6  = (8<<24) | S7_DP6, /**<  Decimal Point 6 */
    DP7  = (8<<24) | S7_DP7, /**<  Decimal Point 7 */
    DP8  = (8<<24) | S7_DP8  /**<  Decimal Point 8 */  
  };
  
  typedef char UDCData_t[QYF_NR_UDC];
  
 /** Constructor for class for driving TM1638 LED controller as used in QYF
   *
   * @brief Supports 8 Digits of 7 Segments + DP Icons. Also supports a scanned keyboard of 16 keys.
   *  
   * @param  PinName mosi, miso, sclk, cs SPI bus pins
   */
  TM1638_QYF(PinName mosi, PinName miso, PinName sclk, PinName cs);

#if DOXYGEN_ONLY
    /** Write a character to the Display
     *
     * @param c The character to write to the display
     */
    int putc(int c);

    /** Write a formatted string to the Display
     *
     * @param format A printf-style format string, followed by the
     *               variables to use in formatting the string.
     */
    int printf(const char* format, ...);   
#endif

     /** Locate cursor to a screen column
     *
     * @param column  The horizontal position from the left, indexed from 0
     */
    void locate(int column);
    
    /** Clear the screen and locate to 0
     * @param bool clrAll Clear Icons also (default = false)
     */
    void cls(bool clrAll = false);

    /** Set Icon
     *
     * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
     * @return none
     */
    void setIcon(Icon icon);

    /** Clr Icon
     *
     * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
     * @return none
     */
    void clrIcon(Icon icon);

   /** Set User Defined Characters (UDC)
     *
     * @param unsigned char udc_idx   The Index of the UDC (0..7)
     * @param int udc_data            The bitpattern for the UDC (16 bits)       
     */
    void setUDC(unsigned char udc_idx, int udc_data);


   /** Number of screen columns
    *
    * @param none
    * @return columns
    */
    int columns();   

   /** Write databyte to TM1638
     *  @param  char data byte written at given address   
     *  @param  int address display memory location to write byte
     *  @return none
     */ 
    void writeData(char data, int address){
      TM1638::writeData(data, address);
    }        

   /** Write Display datablock to TM1638
    *  @param  DisplayData_t data Array of TM1638_DISPLAY_MEM (=16) bytes for displaydata
    *  @param  length number bytes to write (valid range 0..(QYF_NR_GRIDS * TM1638_BYTES_PER_GRID) (=16), when starting at address 0)  
    *  @param  int address display memory location to write bytes (default = 0) 
    *  @return none
    */   
    void writeData(DisplayData_t data, int length = (QYF_NR_GRIDS * TM1638_BYTES_PER_GRID), int address = 0) {
      TM1638::writeData(data, length, address);
    }  

protected:  
    // Stream implementation functions
    virtual int _putc(int value);
    virtual int _getc();

private:
    int _column;
    int _columns;   
    
    DisplayData_t _displaybuffer;
    UDCData_t _UDC_7S; 
};
#endif

#if (LKM1638_TEST == 1) 
// Derived class for TM1638 used in LKM1638 TM1638 display unit
//

#include "Font_7Seg.h"

#define LKM1638_NR_GRIDS  8
#define LKM1638_NR_DIGITS 8
#define LKM1638_NR_UDC    8

//Access to 8 Switches
#define LKM1638_SW1_IDX   0
#define LKM1638_SW1_BIT   0x01
#define LKM1638_SW2_IDX   1
#define LKM1638_SW2_BIT   0x01
#define LKM1638_SW3_IDX   2
#define LKM1638_SW3_BIT   0x01
#define LKM1638_SW4_IDX   3
#define LKM1638_SW4_BIT   0x01

#define LKM1638_SW5_IDX   0
#define LKM1638_SW5_BIT   0x10
#define LKM1638_SW6_IDX   1
#define LKM1638_SW6_BIT   0x10
#define LKM1638_SW7_IDX   2
#define LKM1638_SW7_BIT   0x10
#define LKM1638_SW8_IDX   3
#define LKM1638_SW8_BIT   0x10

/** Constructor for class for driving TM1638 controller as used in LKM1638
  *
  *  @brief Supports 8 Digits of 7 Segments + DP, Also supports 8 Bi-color LEDs and a scanned keyboard of 8 keys.
  *  
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
class TM1638_LKM1638 : public TM1638, public Stream {
 public:

  /** Enums for Icons */
  //  Grid encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  enum Icon {
    DP1  = (1<<24) | S7_DP1, /**<  Decimal Point 1 */
    DP2  = (2<<24) | S7_DP2, /**<  Decimal Point 2 */
    DP3  = (3<<24) | S7_DP3, /**<  Decimal Point 3 */
    DP4  = (4<<24) | S7_DP4, /**<  Decimal Point 4 */
    DP5  = (5<<24) | S7_DP5, /**<  Decimal Point 5 */
    DP6  = (6<<24) | S7_DP6, /**<  Decimal Point 6 */
    DP7  = (7<<24) | S7_DP7, /**<  Decimal Point 7 */
    DP8  = (8<<24) | S7_DP8, /**<  Decimal Point 8 */  
    
    GR1  = (1<<24) | S7_GR1, /**<  Green LED 1 */
    GR2  = (2<<24) | S7_GR2, /**<  Green LED 2 */
    GR3  = (3<<24) | S7_GR3, /**<  Green LED 3 */
    GR4  = (4<<24) | S7_GR4, /**<  Green LED 4 */
    GR5  = (5<<24) | S7_GR5, /**<  Green LED 5 */
    GR6  = (6<<24) | S7_GR6, /**<  Green LED 6 */
    GR7  = (7<<24) | S7_GR7, /**<  Green LED 7 */
    GR8  = (8<<24) | S7_GR8, /**<  Green LED 8 */  

    RD1  = (1<<24) | S7_RD1, /**<  Red LED 1 */
    RD2  = (2<<24) | S7_RD2, /**<  Red LED 2 */
    RD3  = (3<<24) | S7_RD3, /**<  Red LED 3 */
    RD4  = (4<<24) | S7_RD4, /**<  Red LED 4 */
    RD5  = (5<<24) | S7_RD5, /**<  Red LED 5 */
    RD6  = (6<<24) | S7_RD6, /**<  Red LED 6 */
    RD7  = (7<<24) | S7_RD7, /**<  Red LED 7 */
    RD8  = (8<<24) | S7_RD8, /**<  Red LED 8 */  
    
    YL1  = (1<<24) | S7_YL1, /**<  Yellow LED 1 */
    YL2  = (2<<24) | S7_YL2, /**<  Yellow LED 2 */
    YL3  = (3<<24) | S7_YL3, /**<  Yellow LED 3 */
    YL4  = (4<<24) | S7_YL4, /**<  Yellow LED 4 */
    YL5  = (5<<24) | S7_YL5, /**<  Yellow LED 5 */
    YL6  = (6<<24) | S7_YL6, /**<  Yellow LED 6 */
    YL7  = (7<<24) | S7_YL7, /**<  Yellow LED 7 */
    YL8  = (8<<24) | S7_YL8  /**<  Yellow LED 8 */  
  };
  
  typedef char UDCData_t[LKM1638_NR_UDC];
  
 /** Constructor for class for driving TM1638 LED controller as used in LKM1638
   *
   * @brief Supports 8 Digits of 7 Segments + DP Icons. Also supports 8 Bi-Color LEDs and a scanned keyboard of 8 keys.
   *  
   * @param  PinName mosi, miso, sclk, cs SPI bus pins
   */
  TM1638_LKM1638(PinName mosi, PinName miso, PinName sclk, PinName cs);

#if DOXYGEN_ONLY
    /** Write a character to the Display
     *
     * @param c The character to write to the display
     */
    int putc(int c);

    /** Write a formatted string to the Display
     *
     * @param format A printf-style format string, followed by the
     *               variables to use in formatting the string.
     */
    int printf(const char* format, ...);   
#endif

     /** Locate cursor to a screen column
     *
     * @param column  The horizontal position from the left, indexed from 0
     */
    void locate(int column);
    
    /** Clear the screen and locate to 0
     * @param bool clrAll Clear Icons also (default = false)
     */
    void cls(bool clrAll = false);

    /** Set Icon
     *
     * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
     * @return none
     */
    void setIcon(Icon icon);

    /** Clr Icon
     *
     * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
     * @return none
     */
    void clrIcon(Icon icon);

   /** Set User Defined Characters (UDC)
     *
     * @param unsigned char udc_idx   The Index of the UDC (0..7)
     * @param int udc_data            The bitpattern for the UDC (16 bits)       
     */
    void setUDC(unsigned char udc_idx, int udc_data);


   /** Number of screen columns
    *
    * @param none
    * @return columns
    */
    int columns();   

   /** Write databyte to TM1638
     *  @param  char data byte written at given address   
     *  @param  int address display memory location to write byte
     *  @return none
     */ 
    void writeData(char data, int address){
      TM1638::writeData(data, address);
    }        

   /** Write Display datablock to TM1638
    *  @param  DisplayData_t data Array of TM1638_DISPLAY_MEM (=16) bytes for displaydata
    *  @param  length number bytes to write (valid range 0..(LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID) (=16), when starting at address 0)  
    *  @param  int address display memory location to write bytes (default = 0) 
    *  @return none
    */   
    void writeData(DisplayData_t data, int length = (LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID), int address = 0) {
      TM1638::writeData(data, length, address);
    }  

protected:  
    // Stream implementation functions
    virtual int _putc(int value);
    virtual int _getc();

private:
    int _column;
    int _columns;   
    
    DisplayData_t _displaybuffer;
    UDCData_t _UDC_7S; 
};
#endif

#endif