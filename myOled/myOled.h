#ifndef __MYOLED_H
#define __MYOLED_H

#define PLATFORM_ESP8266

#define HARDWARE_I2C

void initOled();

void initOled(uint8_t sda, uint8_t scl);

void clearDisplay();

void clearPart(uint8_t page, uint8_t start_pos, uint8_t end_pos);

void setCursorPos(uint8_t x, uint8_t y);

void displayBmp(const byte binArray[]);

void printChar(char C, uint8_t xpos, uint8_t ypos, uint8_t font_size, bool Highlight);

void printString(const char* str, uint8_t xpos , uint8_t ypos, uint8_t font_size,bool Highlight);

void setBattery(uint8_t percentage);

void setSignal(uint8_t level);

void setBar(uint8_t level, uint8_t page);

void display();

void setCursor(uint8_t x, uint8_t y);

void print7Seg_number(const char *str, uint8_t xpos, uint8_t ypos);

void writeByte(uint8_t byte);

void setPixel(uint8_t x, uint8_t y, bool set);

void drawLine(float x1, float y1, float x2, float y2, bool set);

void drawCircle(float Cx, float Cy, float radius, bool set);

void setDisplayStart(uint8_t Start);

void setLineAddress( uint8_t address);

void printLog(char* log_msg);

void drawSine(float frequency, uint8_t shift, bool set);

void plot(float x, float y);

void setContrast(uint8_t level);

void resetLog();

#endif