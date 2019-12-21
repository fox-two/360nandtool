#pragma once

#include <gpiod.h>
#include "SPI_Interface_base.h"

class XSPI_Interface_software : public SPI_Base
{
public:
    XSPI_Interface_software(const char* gpiochip_name, int pin_EJ, int pin_XX, int pin_SS, int pin_SCK, int pin_MOSI, int pin_MISO);
    ~XSPI_Interface_software();

    void enterFlashMode();
    void leaveFlashMode();

    uint8_t readByte(uint8_t reg);
    //this api reads and writes in blocks of 4 bytes
    void read(uint8_t reg, uint8_t* out);
    
    void write(uint8_t reg, const uint8_t* data);
    void write(uint8_t reg); //assumes data is 4 zeros
    void write(uint8_t reg, uint32_t data);
    void write(uint8_t reg, uint8_t data);
    

    void testPins();
private:
    gpiod_chip* gpio_chip;
    gpiod_line* gpio_line_EJ;
    gpiod_line* gpio_line_XX;
    gpiod_line* gpio_line_SS;
    gpiod_line* gpio_line_SCK;
    gpiod_line* gpio_line_MOSI;
    gpiod_line* gpio_line_MISO;

    void write_byte(uint8_t b);
    uint8_t read_byte();
    void clock_pulse();
};