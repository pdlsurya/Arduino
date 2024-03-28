#ifndef SOFTWARE_I2C_H
#define SOFTWARE_I2C_H



void i2c_init(uint8_t sda, uint8_t scl); //initialize i2c bus
void i2c_start(uint8_t address); //Start I2C transmission

bool i2c_write(uint8_t data);
void i2c_stop();


#endif // SOFTWARE_I2C_H

