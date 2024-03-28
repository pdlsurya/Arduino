#include <Arduino.h>
#include "myLCD.h"
#if defined(HARDWARE_I2C)
#include<Wire.h>
#elif defined(SOFTWARE_I2C)
#include "software_I2C.h"
#endif


void myLCD::I2C_write(uint8_t data) //Write to I2C bus
{
  #if defined(HARDWARE_I2C) //using hardware I2C
  Wire.beginTransmission(i2c_address);
  Wire.write(data & backlight_mask);
  Wire.endTransmission();

  #elif defined(SOFTWARE_I2C)  //Using software I2C
   i2c_start(i2c_address);
   i2c_write(data & backlight_mask);
   i2c_stop();  
  #endif
}

void myLCD::send_command(uint8_t command)
{
  
  I2C_write(0x0C|(command& 0xF0));
  delayMicroseconds(37);
  I2C_write(0xF8);
  delayMicroseconds(37);
  I2C_write(0x0C|((command&0x0F)<<4));
  delayMicroseconds(37);
  I2C_write(0xF8);
}

void myLCD::write_char(uint8_t data) //Print ASCII character
{
  I2C_write(0x0D|(data & 0xF0));
  delayMicroseconds(37);
  I2C_write(0xF9);
  delayMicroseconds(37);
  I2C_write(0x0D |((data & 0X0F)<<4));
  delayMicroseconds(37); 
  I2C_write(0xF9);
  
}

void myLCD::write_string(uint8_t* chr)
{
  for(uint8_t i=0;i<strlen(chr);i++)
      write_char(chr[i]);
}

void myLCD::set_cursor(uint8_t row, uint8_t column) //set Cursor position
{
  send_command((row*0x40+column)|0x80);
}

void myLCD::backlight_on() 
{
   backlight_mask=0xFF;
}


void myLCD::backlight_off()
{
  backlight_mask=0xF7;
}

void myLCD::autoscroll_right()
 {
  send_command(CMD_AUTO_SCROLL_RIGHT);
 }


 void myLCD::autoscroll_left()
 {
  send_command(CMD_AUTO_SCROLL_LEFT);
 }
 
 void myLCD::clear_display()
{
  send_command(CMD_CLEAR_DISP);
}

 void myLCD::function_set(uint8_t func_cmd)
{ 
  I2C_write(func_cmd);
  delayMicroseconds(37);
  I2C_write(func_cmd & 0xF8);//SET EN low
}

void myLCD::write_custom_char(uint8_t pos, uint8_t chr[]) //Display custom characters
{
   send_command((pos<<3)|0x40);
   delayMicroseconds(37);
  for(int i=0;i<8;i++)
  {
    
      write_char(chr[i]);
  }
}


#if defined(HARDWARE_I2C)
void myLCD::init(uint8_t lcd_address)
#elif defined(SOFTWARE_I2C)
void myLCD::init(uint8_t lcd_address, uint8_t sda, uint8_t scl)
#endif
{ 
  #if defined(HARDWARE_I2C) 
  Wire.begin();
  #elif defined(SOFTWARE_I2C)
  i2c_init(sda, scl);
  #endif
  backlight_mask=0xFF;
  i2c_address= lcd_address;

   function_set(0x3C);
   delayMicroseconds(4200);
   function_set(0x3C);
   delayMicroseconds(150);
   function_set(0x3C);
   delayMicroseconds(37);
   function_set(FUNC_SET);
 
  send_command(CMD_FUNC_DOT_LINE); //5X8 dots, 2 line display
  
  send_command(CMD_DISP_ON_OFF);  //DISPLAY_ON 
  
  clear_display();
 
  send_command(CMD_ENTRY_MODE);//left to right
 
}
//end-of-file
