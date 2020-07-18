/* mbed TM1638 Library, for TM1638 LED controller
 * Copyright (c) 2015, v01: WH, Initial version
 *               2016, v02: WH, Added ASCII alphabet display selector, refactored display and keyboard defines 
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
#include "mbed.h" 
#include "TM1638.h"
#include "mbed_trace.h"
#include "mbedtls/debug.h"


/** Constructor for class for driving TM1638 LED controller with SPI bus interface device. 
 *  @brief Supports 8 digits @ 10 segments. 
 *         Also supports a scanned keyboard of upto 24 keys.
 *   
 *  @param  PinName mosi, miso, sclk, cs SPI bus pins
*/
TM1638::TM1638(PinName mosi, PinName miso, PinName sclk, PinName cs) : _spi(mosi,miso,sclk), _cs(cs) {

  _init();
}

/** Init the SPI interface and the controller
  * @param  none
  * @return none
  */ 
void TM1638::_init(){
  
//init SPI
  _cs=1;
  _spi.format(8,3); //TM1638 uses mode 3 (Clock High on Idle, Data latched on second (=rising) edge)
  _spi.frequency(500000);   

//init controller  
  _display = TM1638_DSP_ON;
  _bright  = TM1638_BRT_DEF; 
  _writeCmd(TM1638_DSP_CTRL_CMD, _display | _bright );                                 // Display control cmd, display on/off, brightness   
  
  _writeCmd(TM1638_DATA_SET_CMD, TM1638_DATA_WR | TM1638_ADDR_INC | TM1638_MODE_NORM); // Data set cmd, normal mode, auto incr, write data  
}   


/** Clear the screen and locate to 0
 */  
void TM1638::cls() {
  
  _cs=0;
  wait_us(1);    
  _spi.write(_flip(TM1638_ADDR_SET_CMD | 0x00)); // Address set cmd, 0
      
  for (int cnt=0; cnt<TM1638_DISPLAY_MEM; cnt++) {
    _spi.write(0x00); // data 
  }
  
  wait_us(1);
  _cs=1;      
}  

/** Set Brightness
  *
  * @param  char brightness (3 significant bits, valid range 0..7 (1/16 .. 14/14 dutycycle)  
  * @return none
  */
void TM1638::setBrightness(char brightness){

  _bright = brightness & TM1638_BRT_MSK; // mask invalid bits
  
  _writeCmd(TM1638_DSP_CTRL_CMD, _display | _bright );  // Display control cmd, display on/off, brightness  
}

/** Set the Display mode On/off
  *
  * @param bool display mode
  */
void TM1638::setDisplay(bool on) {
  
  if (on) {
    _display = TM1638_DSP_ON;
  }
  else {
    _display = TM1638_DSP_OFF;
  }
  
  _writeCmd(TM1638_DSP_CTRL_CMD, _display | _bright );  // Display control cmd, display on/off, brightness   
}


/** Write databyte to TM1638
  *  @param  char data byte written at given address
  *  @param  int address display memory location to write byte
  *  @return none
  */ 
void TM1638::writeData(char data, int address) {
  _cs=0;
  wait_us(1);    
  _spi.write(_flip(TM1638_ADDR_SET_CMD | (address & TM1638_ADDR_MSK))); // Set Address cmd
      
  _spi.write(_flip(data)); // data 
  
  wait_us(1);
  _cs=1;             
}


/** Write Display datablock to TM1638
  *  @param  DisplayData_t data Array of TM1638_DISPLAY_MEM (=16) bytes for displaydata
  *  @param  length number bytes to write (valid range 0..TM1638_DISPLAY_MEM (=16), when starting at address 0)
  *  @param  int address display memory location to write bytes (default = 0)   
  *  @return none
  */ 
void TM1638::writeData(DisplayData_t data, int length, int address) {
  _cs=0;
  wait_us(1);    
       
// sanity check
  address &= TM1638_ADDR_MSK;
  if (length < 0) {length = 0;}
  if ((length + address) > TM1638_DISPLAY_MEM) {length = (TM1638_DISPLAY_MEM - address);}

  // _spi.write(_flip(TM1638_ADDR_SET_CMD | 0x00)); // Set Address at 0
  _spi.write(_flip(TM1638_ADDR_SET_CMD | address)); // Set Address

  for (int idx=0; idx<length; idx++) {    
    //_spi.write(_flip(data[idx])); // data 
    _spi.write(_flip(data[address + idx])); // data 
  }
  
  wait_us(1);
  _cs=1;             
}


/** Read keydata block from TM1638
  *  @param  *keydata Ptr to Array of TM1638_KEY_MEM (=4) bytes for keydata
  *  @return bool keypress True when at least one key was pressed
  *
  * Note: Due to the hardware configuration the TM1638 key matrix scanner will detect multiple keys pressed at same time,
  *       but this may also result in some spurious keys being set in keypress data array.
  *       It may be best to ignore all keys in those situations. That option is implemented in this method depending on #define setting.  
  */ 
bool TM1638::getKeys(KeyData_t *keydata) {
  int keypress = 0;
  char data;

  // Read keys
  _cs=0;
  wait_us(1);    
  
  // Enable Key Read mode
  _spi.write(_flip(TM1638_DATA_SET_CMD | TM1638_KEY_RD | TM1638_ADDR_INC | TM1638_MODE_NORM)); // Data set cmd, normal mode, auto incr, read data

  for (int idx=0; idx < TM1638_KEY_MEM; idx++) {
    data = _flip(_spi.write(0xFF));    // read keys and correct bitorder

    data = data & TM1638_KEY_MSK; // Mask valid bits
    if (data != 0) {  // Check for any pressed key
      for (int bit=0; bit < 8; bit++) {
        if (data & (1 << bit)) {keypress++;} // Test all significant bits
      }
    }  

    (*keydata)[idx] = data;            // Store keydata after correcting bitorder
  }

  wait_us(1);
  _cs=1;    

  // Restore Data Write mode
  _writeCmd(TM1638_DATA_SET_CMD, TM1638_DATA_WR | TM1638_ADDR_INC | TM1638_MODE_NORM); // Data set cmd, normal mode, auto incr, write data  
      
#if(1)
// Dismiss multiple keypresses at same time
  return (keypress == 1);    
#else
// Allow multiple keypress and accept possible spurious keys
  return (keypress > 0);
#endif  
}
    

/** Helper to reverse all command or databits. The TM1638 expects LSB first, whereas SPI is MSB first
  *  @param  char data
  *  @return bitreversed data
  */ 
#if(1)
char TM1638::_flip(char data) {
 char value=0;
  
 if (data & 0x01) {value |= 0x80;} ;  
 if (data & 0x02) {value |= 0x40;} ;
 if (data & 0x04) {value |= 0x20;} ;
 if (data & 0x08) {value |= 0x10;} ;
 if (data & 0x10) {value |= 0x08;} ;
 if (data & 0x20) {value |= 0x04;} ;
 if (data & 0x40) {value |= 0x02;} ;
 if (data & 0x80) {value |= 0x01;} ;
 return value;       
}
#else
char TM1638::_flip(char data) {

 data   = (((data & 0xAA) >> 1) | ((data & 0x55) << 1));
 data   = (((data & 0xCC) >> 2) | ((data & 0x33) << 2));
 return   (((data & 0xF0) >> 4) | ((data & 0x0F) << 4));
}
#endif

/** Write command and parameter to TM1638
  *  @param  int cmd Command byte
  *  &Param  int data Parameters for command
  *  @return none
  */  
void TM1638::_writeCmd(int cmd, int data){
    
  _cs=0;
  wait_us(1);    
//  _spi.write(_flip( (cmd & 0xF0) | (data & 0x0F)));  
  _spi.write(_flip( (cmd & TM1638_CMD_MSK) | (data & ~TM1638_CMD_MSK)));   
 
  wait_us(1);
  _cs=1;          
}  


#if (LEDKEY8_TEST == 1) 
// Derived class for TM1638 used in LED&KEY display unit
//

/** Constructor for class for driving TM1638 LED controller as used in LEDKEY8
  *
  *  @brief Supports 8 Digits of 7 Segments + DP + LED Icons. Also supports a scanned keyboard of 8.
  *   
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
TM1638_LEDKEY8::TM1638_LEDKEY8(PinName mosi, PinName miso, PinName sclk, PinName cs) : TM1638(mosi, miso, sclk, cs) {
  _column  = 0;
  _columns = LEDKEY8_NR_DIGITS;    
}  

#if(0)
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
#endif

/* Display a tring in ascii to the 8 digit seven segment display 
 * TODO start the string at the locate column
 *
 */
 void TM1638_LEDKEY8::displayStringAt(char *inString, int column) {
    char outString[22];
    for (int i = 0; i < (strlen( inString )) && i < 9; i++) {
        outString[2*i] = FONT_7S[0x5f&inString[i]];
        outString[2*i+1]= 0;
    }
    locate(column);
    writeData(outString);
  
 }
/** Locate cursor to a screen column
  *
  * @param column  The horizontal position from the left, indexed from 0
  */
void TM1638_LEDKEY8::locate(int column) {
  //sanity check
  if (column < 0) {column = 0;}
  if (column > (_columns - 1)) {column = _columns - 1;}  
  
  _column = column;       
}


/** Number of screen columns
  *
  * @param none
  * @return columns
  */
int TM1638_LEDKEY8::columns() {
    return _columns;
}

    
/** Clear the screen and locate to 0
  * @param bool clrAll Clear Icons also (default = false)
  */ 
void TM1638_LEDKEY8::cls(bool clrAll) {  

  if (clrAll) {
    //clear local buffer (including Icons)
    for (int idx=0; idx < (LEDKEY8_NR_GRIDS << 1); idx++) {
      _displaybuffer[idx] = 0x00;  
    }
  }  
  else {
    //clear local buffer (preserving Icons)
    for (int idx=0; idx < LEDKEY8_NR_GRIDS; idx++) {
      _displaybuffer[(idx<<1)]     = _displaybuffer[(idx<<1)]     & MASK_ICON_GRID[idx][0];  
      _displaybuffer[(idx<<1) + 1] = _displaybuffer[(idx<<1) + 1] & MASK_ICON_GRID[idx][1];
    }  
  }

  writeData(_displaybuffer, (LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID));

  _column = 0;   
}     

/** Set Icon
  *
  * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  * @return none
  */
void TM1638_LEDKEY8::setIcon(Icon icon) {
  int addr, icn;

   icn =        icon  & 0xFFFF;
  addr = (icon >> 24) & 0xFF; 
  addr = (addr - 1) << 1;   // * TM1638_BYTES_PER_GRID
    
  //Save char...and set bits for icon to write
  _displaybuffer[addr]   = _displaybuffer[addr]   | LO(icn);      
  _displaybuffer[addr+1] = _displaybuffer[addr+1] | HI(icn);      
//  writeData(_displaybuffer, (LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID));
  writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);      
}

/** Clr Icon
  *
  * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  * @return none
  */
void TM1638_LEDKEY8::clrIcon(Icon icon) {
  int addr, icn;

   icn =        icon  & 0xFFFF;
  addr = (icon >> 24) & 0xFF; 
  addr = (addr - 1) << 1;   // * TM1638_BYTES_PER_GRID
    
  //Save char...and clr bits for icon to write
  _displaybuffer[addr]   = _displaybuffer[addr]   & ~LO(icn);      
  _displaybuffer[addr+1] = _displaybuffer[addr+1] & ~HI(icn);      
//  writeData(_displaybuffer, (LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID));
  writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);      
}


/** Set User Defined Characters (UDC)
  *
  * @param unsigned char udc_idx  The Index of the UDC (0..7)
  * @param int udc_data           The bitpattern for the UDC (8 bits)       
  */
void TM1638_LEDKEY8::setUDC(unsigned char udc_idx, int udc_data) {

  //Sanity check
  if (udc_idx > (LEDKEY8_NR_UDC-1)) {
    return;
  }
  // Mask out Icon bits?

  _UDC_7S[udc_idx] = LO(udc_data);
}


/** Write a single character (Stream implementation)
  */
int TM1638_LEDKEY8::_putc(int value) {
    int addr;
    bool validChar = false;
    char pattern   = 0x00;
    
    if ((value == '\n') || (value == '\r')) {
      //No character to write
      validChar = false;
      
      //Update Cursor      
      _column = 0;
    }
    else if ((value == '.') || (value == ',')) {
      //No character to write
      validChar = false;
      pattern = S7_DP; // placeholder for all DPs
      
      // Check to see that DP can be shown for current column
      if (_column > 0) {
        //Translate between _column and displaybuffer entries
        //Add DP to bitpattern of digit left of current column.
        addr = (_column - 1) << 1; // * TM1638_BYTES_PER_GRID
      
        //Save icons...and set bits for decimal point to write
        _displaybuffer[addr]   = _displaybuffer[addr] | pattern;
//        _displaybuffer[addr+1] = _displaybuffer[addr+1] | pattern;

//        writeData(_displaybuffer, (LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID));
        writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);    
          
        //No Cursor Update
      }
    }
    else if ((value >= 0) && (value < LEDKEY8_NR_UDC)) {
      //Character to write
      validChar = true;
      pattern = _UDC_7S[value];
    }  
    
#if (SHOW_ASCII == 1)
    //display all ASCII characters
    else if ((value >= FONT_7S_START) && (value <= FONT_7S_END)) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[value - FONT_7S_START];
    } // else
#else    
    //display only digits and hex characters      
    else if (value == '-') {
      //Character to write
      validChar = true;
      pattern = C7_MIN;         
    }
    else if ((value >= (int)'0') && (value <= (int) '9')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[value - (int) '0'];
    }
    else if ((value >= (int) 'A') && (value <= (int) 'F')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[10 + value - (int) 'A'];
    }
    else if ((value >= (int) 'a') && (value <= (int) 'f')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[10 + value - (int) 'a'];
    } //else
#endif

    if (validChar) {
      //Character to write
 
      //Translate between _column and displaybuffer entries
      addr = _column << 1; // * TM1638_BYTES_PER_GRID

      //Save icons...and set bits for character to write
      _displaybuffer[addr]   = (_displaybuffer[addr]   & MASK_ICON_GRID[_column][0]) | pattern;
//      _displaybuffer[addr+1] = (_displaybuffer[addr+1] & MASK_ICON_GRID[_column][0]) | pattern;

//      writeData(_displaybuffer, (LEDKEY8_NR_GRIDS * TM1638_BYTES_PER_GRID));
      writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);
      
      //Update Cursor
      _column++;
      if (_column > (LEDKEY8_NR_DIGITS - 1)) {
        _column = 0;
      }

    } // if validChar           

    return value;
}


// get a single character (Stream implementation)
int TM1638_LEDKEY8::_getc() {
    return -1;
}

#endif


#if (QYF_TEST == 1) 
// Derived class for TM1638 used in QYF-TM1638 display unit
//

/** Constructor for class for driving TM1638 LED controller as used in QYF
  *
  *  @brief Supports 8 Digits of 7 Segments + DP. Also supports a scanned keyboard of 16 keys.
  *   
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
TM1638_QYF::TM1638_QYF(PinName mosi, PinName miso, PinName sclk, PinName cs) : TM1638(mosi, miso, sclk, cs) {
  _column  = 0;
  _columns = QYF_NR_DIGITS;    
}  

#if(0)
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
#endif

/** Locate cursor to a screen column
  *
  * @param column  The horizontal position from the left, indexed from 0
  */
void TM1638_QYF::locate(int column) {
  //sanity check
  if (column < 0) {column = 0;}
  if (column > (_columns - 1)) {column = _columns - 1;}  
  
  _column = column;       
}


/** Number of screen columns
  *
  * @param none
  * @return columns
  */
int TM1638_QYF::columns() {
    return _columns;
}

    
/** Clear the screen and locate to 0
  * @param bool clrAll Clear Icons also (default = false)
  */ 
void TM1638_QYF::cls(bool clrAll) {  

  if (clrAll) {
    //clear local buffer (including Icons)
    for (int idx=0; idx < (QYF_NR_GRIDS << 1); idx++) {
      _displaybuffer[idx] = 0x00;  
    }
  }  
  else {
    //clear local buffer (preserving Icons)
    for (int idx=0; idx < QYF_NR_GRIDS; idx++) {
      _displaybuffer[(idx<<1)]     = _displaybuffer[(idx<<1)]     & MASK_ICON_GRID[idx][0];  
      _displaybuffer[(idx<<1) + 1] = _displaybuffer[(idx<<1) + 1] & MASK_ICON_GRID[idx][1];
    }  
  }

  writeData(_displaybuffer, (QYF_NR_GRIDS * TM1638_BYTES_PER_GRID));

  _column = 0;   
}     

/** Set Icon
  *
  * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  * @return none
  */
void TM1638_QYF::setIcon(Icon icon) {
  int addr, icn;

   icn =        icon  & 0xFFFF;
  addr = (icon >> 24) & 0xFF; 
  addr = (addr - 1) << 1;   // * TM1638_BYTES_PER_GRID
    
  //Save char...and set bits for icon to write
  _displaybuffer[addr]   = _displaybuffer[addr]   | LO(icn);      
  _displaybuffer[addr+1] = _displaybuffer[addr+1] | HI(icn);      
//  writeData(_displaybuffer, (QYF_NR_GRIDS * TM1638_BYTES_PER_GRID));
  writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);    
}

/** Clr Icon
  *
  * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  * @return none
  */
void TM1638_QYF::clrIcon(Icon icon) {
  int addr, icn;

   icn =        icon  & 0xFFFF;
  addr = (icon >> 24) & 0xFF; 
  addr = (addr - 1) << 1;   // * TM1638_BYTES_PER_GRID
    
  //Save char...and clr bits for icon to write
  _displaybuffer[addr]   = _displaybuffer[addr]   & ~LO(icn);      
  _displaybuffer[addr+1] = _displaybuffer[addr+1] & ~HI(icn);      
//  writeData(_displaybuffer, (QYF_NR_GRIDS * TM1638_BYTES_PER_GRID));
  writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);  
}


/** Set User Defined Characters (UDC)
  *
  * @param unsigned char udc_idx  The Index of the UDC (0..7)
  * @param int udc_data           The bitpattern for the UDC (8 bits)       
  */
void TM1638_QYF::setUDC(unsigned char udc_idx, int udc_data) {

  //Sanity check
  if (udc_idx > (QYF_NR_UDC-1)) {
    return;
  }
  // Mask out Icon bits?

  _UDC_7S[udc_idx] = LO(udc_data);
}


/** Write a single character (Stream implementation)
  */
int TM1638_QYF::_putc(int value) {
    bool validChar = false;
    char pattern   = 0x00;
    char bit       = 0x00;
        
    if ((value == '\n') || (value == '\r')) {
      //No character to write
      validChar = false;
      
      //Update Cursor      
      _column = 0;
    }
    else if ((value == '.') || (value == ',')) {
      //No character to write
      validChar = false;
      pattern = S7_DP; // placeholder for all DPs
      
      // Check to see that DP can be shown for current column
      if (_column > 0) {
        //Add DP to bitpattern of digit left of current column.
        bit = 1 << (8 - _column); // bitposition for the previous _column

        _displaybuffer[14] = (_displaybuffer[14] | bit); // set bit

        writeData(_displaybuffer, (QYF_NR_GRIDS * TM1638_BYTES_PER_GRID));
        
        //No Cursor Update
      }
    }
    else if ((value >= 0) && (value < QYF_NR_UDC)) {
      //Character to write
      validChar = true;
      pattern = _UDC_7S[value];
    }  
    
#if (SHOW_ASCII == 1)
    //display all ASCII characters
    else if ((value >= FONT_7S_START) && (value <= FONT_7S_END)) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[value - FONT_7S_START];
    } // else
#else    
    //display only digits and hex characters      
    else if (value == '-') {
      //Character to write
      validChar = true;
      pattern = C7_MIN;         
    }
    else if ((value >= (int)'0') && (value <= (int) '9')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[value - (int) '0'];
    }
    else if ((value >= (int) 'A') && (value <= (int) 'F')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[10 + value - (int) 'A'];
    }
    else if ((value >= (int) 'a') && (value <= (int) 'f')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[10 + value - (int) 'a'];
    } //else
#endif

    if (validChar) {
      //Character to write

      // Very annoying bitmapping :(
      // This display module uses a single byte of each grid to drive a specific segment of all digits.
      // So the bits in byte 0 (Grid 1) drive all A-segments, the bits in byte 2 (Grid 2) drive all B-segments etc.
      // Bit0 is for the segment in Digit 8, Bit1 is for the segment in Digit 7 etc.. This bit manipulation is handled in _putc().
      
      bit = 1 << (7 - _column); // bitposition for the current _column

      if (pattern & S7_A) {_displaybuffer[0] = (_displaybuffer[0] | bit); } // set bit
      else                {_displaybuffer[0] = (_displaybuffer[0] & ~bit);} // clr bit       

      if (pattern & S7_B) {_displaybuffer[2] = (_displaybuffer[2] | bit); } // set bit
      else                {_displaybuffer[2] = (_displaybuffer[2] & ~bit);} // clr bit       

      if (pattern & S7_C) {_displaybuffer[4] = (_displaybuffer[4] | bit); } // set bit
      else                {_displaybuffer[4] = (_displaybuffer[4] & ~bit);} // clr bit       

      if (pattern & S7_D) {_displaybuffer[6] = (_displaybuffer[6] | bit); } // set bit
      else                {_displaybuffer[6] = (_displaybuffer[6] & ~bit);} // clr bit       

      if (pattern & S7_E) {_displaybuffer[8] = (_displaybuffer[8] | bit); } // set bit
      else                {_displaybuffer[8] = (_displaybuffer[8] & ~bit);} // clr bit       

      if (pattern & S7_F) {_displaybuffer[10] = (_displaybuffer[10] | bit); } // set bit
      else                {_displaybuffer[10] = (_displaybuffer[10] & ~bit);} // clr bit       

      if (pattern & S7_G) {_displaybuffer[12] = (_displaybuffer[12] | bit); } // set bit
      else                {_displaybuffer[12] = (_displaybuffer[12] & ~bit);} // clr bit       

      if (pattern & S7_DP) {_displaybuffer[14] = (_displaybuffer[14] | bit); } // set bit
      else                 {_displaybuffer[14] = (_displaybuffer[14] & ~bit);} // clr bit       

      //Save icons...and set bits for character to write
//      _displaybuffer[addr]   = (_displaybuffer[addr]   & MASK_ICON_GRID[_column][0]) | pattern;
//      _displaybuffer[addr+1] = (_displaybuffer[addr+1] & MASK_ICON_GRID[_column][0]) | pattern;

      writeData(_displaybuffer, (QYF_NR_GRIDS * TM1638_BYTES_PER_GRID));
                                
      //Update Cursor
      _column++;
      if (_column > (QYF_NR_DIGITS - 1)) {
        _column = 0;
      }

    } // if validChar           

    return value;
}


// get a single character (Stream implementation)
int TM1638_QYF::_getc() {
    return -1;
}

#endif



#if (LKM1638_TEST == 1) 
// Derived class for TM1638 used in LMK1638 display unit
//

/** Constructor for class for driving TM1638 LED controller as used in LKM1638
  *
  *  @brief Supports 8 Digits of 7 Segments + DP + Bi-Color LED Icons. Also supports a scanned keyboard of 8.
  *   
  *  @param  PinName mosi, miso, sclk, cs SPI bus pins
  */
TM1638_LKM1638::TM1638_LKM1638(PinName mosi, PinName miso, PinName sclk, PinName cs) : TM1638(mosi, miso, sclk, cs) {
  _column  = 0;
  _columns = LKM1638_NR_DIGITS;    
}  

#if(0)
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
#endif

/** Locate cursor to a screen column
  *
  * @param column  The horizontal position from the left, indexed from 0
  */
void TM1638_LKM1638::locate(int column) {
  //sanity check
  if (column < 0) {column = 0;}
  if (column > (_columns - 1)) {column = _columns - 1;}  
  
  _column = column;       
}


/** Number of screen columns
  *
  * @param none
  * @return columns
  */
int TM1638_LKM1638::columns() {
    return _columns;
}

    
/** Clear the screen and locate to 0
  * @param bool clrAll Clear Icons also (default = false)
  */ 
void TM1638_LKM1638::cls(bool clrAll) {  

  if (clrAll) {
    //clear local buffer (including Icons)
    for (int idx=0; idx < (LKM1638_NR_GRIDS << 1); idx++) {
      _displaybuffer[idx] = 0x00;  
    }
  }  
  else {
    //clear local buffer (preserving Icons)
    for (int idx=0; idx < LKM1638_NR_GRIDS; idx++) {
      _displaybuffer[(idx<<1)]     = _displaybuffer[(idx<<1)]     & MASK_ICON_GRID[idx][0];  
      _displaybuffer[(idx<<1) + 1] = _displaybuffer[(idx<<1) + 1] & MASK_ICON_GRID[idx][1];
    }  
  }

  writeData(_displaybuffer, (LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID));

  _column = 0;   
}     

/** Set Icon
  *
  * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  * @return none
  */
void TM1638_LKM1638::setIcon(Icon icon) {
  int addr, icn;

   icn =        icon  & 0xFFFF;
  addr = (icon >> 24) & 0xFF; 
  addr = (addr - 1) << 1;   // * TM1638_BYTES_PER_GRID
    
  //Save char...and set bits for icon to write
  _displaybuffer[addr]   = _displaybuffer[addr]   | LO(icn);      
  _displaybuffer[addr+1] = _displaybuffer[addr+1] | HI(icn);      
//  writeData(_displaybuffer, (LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID));
  writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);      
}

/** Clr Icon
  *
  * @param Icon icon Enums Icon has Grid position encoded in 8 MSBs, Icon pattern encoded in 16 LSBs
  * @return none
  */
void TM1638_LKM1638::clrIcon(Icon icon) {
  int addr, icn;

   icn =        icon  & 0xFFFF;
  addr = (icon >> 24) & 0xFF; 
  addr = (addr - 1) << 1;   // * TM1638_BYTES_PER_GRID
    
  //Save char...and clr bits for icon to write
  _displaybuffer[addr]   = _displaybuffer[addr]   & ~LO(icn);      
  _displaybuffer[addr+1] = _displaybuffer[addr+1] & ~HI(icn);      
//  writeData(_displaybuffer, (LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID));
  writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);      
}


/** Set User Defined Characters (UDC)
  *
  * @param unsigned char udc_idx  The Index of the UDC (0..7)
  * @param int udc_data           The bitpattern for the UDC (8 bits)       
  */
void TM1638_LKM1638::setUDC(unsigned char udc_idx, int udc_data) {

  //Sanity check
  if (udc_idx > (LKM1638_NR_UDC-1)) {
    return;
  }
  // Mask out Icon bits?

  _UDC_7S[udc_idx] = LO(udc_data);
}


/** Write a single character (Stream implementation)
  */
int TM1638_LKM1638::_putc(int value) {
    int addr;
    bool validChar = false;
    char pattern   = 0x00;
    
    if ((value == '\n') || (value == '\r')) {
      //No character to write
      validChar = false;
      
      //Update Cursor      
      _column = 0;
    }
    else if ((value == '.') || (value == ',')) {
      //No character to write
      validChar = false;
      pattern = S7_DP; // placeholder for all DPs
      
      // Check to see that DP can be shown for current column
      if (_column > 0) {
        //Translate between _column and displaybuffer entries
        //Add DP to bitpattern of digit left of current column.
        addr = (_column - 1) << 1; // * TM1638_BYTES_PER_GRID
      
        //Save icons...and set bits for decimal point to write
        _displaybuffer[addr]   = _displaybuffer[addr] | pattern;
//        _displaybuffer[addr+1] = _displaybuffer[addr+1] | pattern;

//        writeData(_displaybuffer, (LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID));
        writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);    
          
        //No Cursor Update
      }
    }
    else if ((value >= 0) && (value < LKM1638_NR_UDC)) {
      //Character to write
      validChar = true;
      pattern = _UDC_7S[value];
    }  
    
#if (SHOW_ASCII == 1)
    //display all ASCII characters
    else if ((value >= FONT_7S_START) && (value <= FONT_7S_END)) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[value - FONT_7S_START];
    } // else
#else    
    //display only digits and hex characters      
    else if (value == '-') {
      //Character to write
      validChar = true;
      pattern = C7_MIN;         
    }
    else if ((value >= (int)'0') && (value <= (int) '9')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[value - (int) '0'];
    }
    else if ((value >= (int) 'A') && (value <= (int) 'F')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[10 + value - (int) 'A'];
    }
    else if ((value >= (int) 'a') && (value <= (int) 'f')) {   
      //Character to write
      validChar = true;
      pattern = FONT_7S[10 + value - (int) 'a'];
    } //else
#endif

    if (validChar) {
      //Character to write
 
      //Translate between _column and displaybuffer entries
      addr = _column << 1; // * TM1638_BYTES_PER_GRID

      //Save icons...and set bits for character to write
      _displaybuffer[addr]   = (_displaybuffer[addr]   & MASK_ICON_GRID[_column][0]) | pattern;
//      _displaybuffer[addr+1] = (_displaybuffer[addr+1] & MASK_ICON_GRID[_column][0]) | pattern;

//      writeData(_displaybuffer, (LKM1638_NR_GRIDS * TM1638_BYTES_PER_GRID));
      writeData(_displaybuffer, TM1638_BYTES_PER_GRID, addr);
      
      //Update Cursor
      _column++;
      if (_column > (LKM1638_NR_DIGITS - 1)) {
        _column = 0;
      }

    } // if validChar           

    return value;
}


// get a single character (Stream implementation)
int TM1638_LKM1638::_getc() {
    return -1;
}

#endif

