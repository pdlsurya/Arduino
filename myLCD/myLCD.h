#ifndef MYLCD_H
#define MYLCD_H

#define SOFTWARE_I2C

#define FUNC_SET 0X2C
#define CMD_FUNC_DOT_LINE 0X28
#define CMD_CLEAR_DISP 0X01
#define CMD_RETURN_HOME 0X02
#define CMD_ENTRY_MODE 0X06
#define CMD_DISP_ON_OFF 0X0C
#define CMD_AUTO_SCROLL_RIGHT 0X1C
#define CMD_AUTO_SCROLL_LEFT 0X18

class myLCD { //Class declaration
public:
void backlight_on();
void backlight_off();
void autoscroll_right();
void autoscroll_left();
void init(uint8_t lcd_address);
void init(uint8_t lcd_address, uint8_t sda, uint8_t scl);
void write_string(uint8_t* chr);
void clear_display();
void set_cursor(uint8_t row, uint8_t column);
void write_custom_char(uint8_t pos, uint8_t chr[]);
void write_char();
void write_char(uint8_t data);

private:
uint8_t backlight_mask;
uint8_t i2c_address;
void I2C_write(uint8_t data);
void send_command(uint8_t command);
void function_set(uint8_t func_cmd);

};

#endif //MYLCD_H
//end-of-file