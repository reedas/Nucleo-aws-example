/*
* Project Name: LCD_16x02_I2C
* File: lcd.c
* Description: HD44780-based character LCD 16x02 I2C library main file
* Author: Gavin Lyons.
* Complier: xc8 v2.00 compiler
* PIC: PIC16F1619 
* IDE:  MPLAB X v5.00
* Development board: Microchip Curiosity Board, PIC16F1619
* Created March 2019
* Description: See URL for full details.
* URL: https://github.com/gavinlyonsrepo/pic_16F1619_projects
*/
/*
* Modified and extended for ARM mbed 6 by Andrew Reed
* 
* Inspiration and code for extra functions from
* https://github.com/ARMmbed/TextLCD
*/

#include "i2cLCD.h"         
#include "mbed.h"
//
//extern I2C i2c;
I2C i2c(I2C_SDA, I2C_SCL);// Send data to LCD



    uint8_t _Addr;
    uint8_t _displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
    uint8_t _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    uint8_t _displaymode;
    uint8_t _numlines = 2;
    uint8_t _cols = 16;
    uint8_t _rows = 2;
    uint8_t _backlightval = LCD_BACKLIGHT;

i2cLCD::i2cLCD(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
}


void i2cLCD::send_data (unsigned char data)
{
	unsigned char data_l, data_u;
	char data_I2C[4];

	data_l = (data<<4)&0xf0;  //select lower nibble by moving it to the upper nibble position
	data_u = data&0xf0;  //select upper nibble
    data_I2C[0] = data_u|DATA_BYTE_ON|_backlightval; //enable=1 and rs =1 1101  YYYY-BL-en-X-rs
    data_I2C[1] = data_u|DATA_BYTE_OFF|_backlightval; //enable=0 and rs =1 1001 YYYY-BL-en-X-rs
    data_I2C[2] = data_l|DATA_BYTE_ON|_backlightval; //enable=1 and rs =1 1101  YYYY-BL-en-X-rs
    data_I2C[3] = data_l|DATA_BYTE_OFF|_backlightval; //enable=0 and rs =1 1001 YYYY-BL-en-X-rs

    //i2c_open(SLAVE_ADDRESS_LCD);
    //i2c.write (SLAVE_ADDRESS_LCD, data_I2C, 4);
    for (int i = 0; i < 4; i++ ){
        i2c.write (SLAVE_ADDRESS_LCD, data_I2C + i , 1);  
        wait_us(40);
    }

    //i2c_close(); 
}

// send command to LCD
void i2cLCD::send_cmd (unsigned char data)
{
	unsigned char data_l, data_u;
	char data_I2C[4];

	data_l = (data<<4)&0xf0;  //select lower nibble by moving it to the upper nibble position
	data_u = data&0xf0;  //select upper nibble
    data_I2C[0] = data_u|CMD_BYTE_ON|_backlightval; // 1100 YYYY-BL-en-X-rs enable=1 and rs =0
    data_I2C[1] = data_u|CMD_BYTE_OFF|_backlightval; // 1000 YYYY-BL-en-X-rs enable=0 and rs =0
    data_I2C[2] = data_l|CMD_BYTE_ON|_backlightval; // 1100 YYYY-BL-en-X-rs enable=1 and rs =0
    data_I2C[3] = data_l|CMD_BYTE_OFF|_backlightval; // 1000 YYYY-BL-en-X-rs enable=0 and rs =0
    char sending[1];	
   ///i2c.open (SLAVE_ADDRESS_LCD);
    for (int i = 0; i < 4; i++ ){

        sending[0] = data_I2C[i] | _backlightval;
        i2c.write (SLAVE_ADDRESS_LCD, sending, 1);  
        wait_us(40);
    }
    //i2c_close();
}

//Clear LCD
void i2cLCD::clear (void)
{
    i2cLCD::send_cmd(CLRSCR);
    ThisThread::sleep_for(2ms);
}

// initialise LCD
void i2cLCD::init (void)
{
    ThisThread::sleep_for(50ms);
    i2cLCD::send_cmd (LCD_HOME);
    ThisThread::sleep_for(5ms);
    i2cLCD::send_cmd (MODE_4BIT);
    ThisThread::sleep_for(5ms);
    i2cLCD::send_cmd (DISPLAY_ON);
    i2cLCD::send_cmd (CURSOR_BLINK); 
    i2cLCD::send_cmd (CLRSCR);
}

// send string of data bytes to LCD
void i2cLCD::printString (char *str)
{
	while (*str) i2cLCD::send_data (*str++);
}
void i2cLCD::printChar (char c)
{
	i2cLCD::send_data (c);
}

void i2cLCD::locate(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
	i2cLCD::send_cmd(SETDDRAMADDR | (col + row_offsets[row]));
}
void i2cLCD::noDisplay(void){ 
    _displaycontrol &= ~LCD_DISPLAYON;
	i2cLCD::send_cmd(DISPLAYCONTROL | _displaycontrol);
}
void i2cLCD::display(void){ 
    _displaycontrol |= LCD_DISPLAYON;
	i2cLCD::send_cmd(DISPLAYCONTROL | _displaycontrol);
}
void i2cLCD::noBlink(void){ 
    _displaycontrol &= ~LCD_BLINKON;
	i2cLCD::send_cmd(DISPLAYCONTROL | _displaycontrol);
}
void i2cLCD::blink(void){
    _displaycontrol |= LCD_BLINKON;
	i2cLCD::send_cmd(DISPLAYCONTROL | _displaycontrol);

}
void i2cLCD::noCursor(void){
    _displaycontrol &= ~LCD_CURSORON;
	i2cLCD::send_cmd(DISPLAYCONTROL | _displaycontrol);
}
void i2cLCD::cursor(void){
    _displaycontrol |= LCD_CURSORON;
	i2cLCD::send_cmd(DISPLAYCONTROL | _displaycontrol);
}
void i2cLCD::scrollDisplayLeft(void){
    i2cLCD::send_cmd(CURSORSHIFT | DISPLAYMOVE | MOVELEFT);
}
void i2cLCD::scrollDisplayRight(void){
    i2cLCD::send_cmd(CURSORSHIFT | DISPLAYMOVE | MOVERIGHT);
}
void i2cLCD::leftToRight(void){
    _displaymode |= LCD_ENTRYLEFT;
	i2cLCD::send_cmd(ENTRYMODESET | _displaymode);
}
void i2cLCD::rightToLeft(void){
    _displaymode &= ~LCD_ENTRYLEFT;
	i2cLCD::send_cmd(ENTRYMODESET | _displaymode);
}
void i2cLCD::noBacklight(void){
    _backlightval = LCD_NOBACKLIGHT;
    i2cLCD::send_cmd(0);
}
void i2cLCD::backlight(void){
    _backlightval = LCD_BACKLIGHT;
    i2cLCD::send_cmd(0);
}
void i2cLCD::autoscroll(void){
    _displaymode |= LCD_ENTRYSHIFTINCREMENT;
	i2cLCD::send_cmd(ENTRYMODESET | _displaymode);
} 
void i2cLCD::noAutoscroll(void){ 
    _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	i2cLCD::send_cmd(ENTRYMODESET | _displaymode);
} 
void i2cLCD::createChar(char location, char *charmap){
    location &= 0x7; // we only have 8 locations 0-7
	i2cLCD::send_cmd(SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		i2cLCD::send_data(charmap[i]);
	}
}
void i2cLCD::set_ncol_nrow(uint8_t cols, uint8_t rows) {
    _cols = cols;
    _rows = rows;
}
void i2cLCD::begin(uint8_t cols, uint8_t  rows) {
    _cols = cols;
    _rows = rows;
    i2cLCD::init();
}