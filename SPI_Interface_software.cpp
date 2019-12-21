#include "SPI_Interface_software.h"
#include <cstdio>
#include <string>

#include <unistd.h>
#include <stdexcept>

constexpr const char* CONSUMER_NAME = "x360nanddump";

timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

void clock_wait()
{
    //usleep(1 * 1000);
    /*timespec time_start, now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);
    //printf("start\n");
    while(1)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);

        //printf("%i\n", diff(time_start, now).tv_nsec);
        if(diff(time_start, now).tv_nsec >= 4000)
            break;
    }*/
    //usleep(100ls);

}

static gpiod_line* get_pin(gpiod_chip* gpio_chip, int pin_number, bool output=true)
{
    gpiod_line* line = gpiod_chip_get_line(gpio_chip, pin_number);
    if(line == nullptr)
        throw std::runtime_error("Could not get pin " + std::to_string(pin_number));

    if(output)
    {
        if (gpiod_line_request_output(line, CONSUMER_NAME, 0) < 0) 
            throw std::runtime_error("Failed to set pin" + std::to_string(pin_number) +" as output.");
    }
    else
    {
        if (gpiod_line_request_input(line, CONSUMER_NAME) < 0) 
            throw std::runtime_error("Failed to set pin" + std::to_string(pin_number) +" as output.");
    }
    return line;
}
XSPI_Interface_software::XSPI_Interface_software(const char* gpiochip_name, int pin_EJ, int pin_XX, int pin_SS, int pin_SCK, int pin_MOSI, int pin_MISO)
{
    gpio_chip = gpiod_chip_open_by_name(gpiochip_name);
    if(gpio_chip == nullptr)
        throw std::runtime_error("Could not open gpio chip.");

    gpio_line_EJ = get_pin(gpio_chip, pin_EJ);
    gpio_line_XX = get_pin(gpio_chip, pin_XX);
    gpio_line_SS = get_pin(gpio_chip, pin_SS);
    gpio_line_SCK = get_pin(gpio_chip, pin_SCK);
    gpio_line_MOSI = get_pin(gpio_chip, pin_MOSI);
    gpio_line_MISO = get_pin(gpio_chip, pin_MISO, false);

    gpiod_line_set_value(gpio_line_XX, 1);
    gpiod_line_set_value(gpio_line_EJ, 1);
    gpiod_line_set_value(gpio_line_SS, 1);
    gpiod_line_set_value(gpio_line_SCK, 1);
    gpiod_line_set_value(gpio_line_MOSI, 0);
}

XSPI_Interface_software::~XSPI_Interface_software()
{
    gpiod_line_release(gpio_line_XX);
    gpiod_line_release(gpio_line_EJ);
    gpiod_line_release(gpio_line_SS);
    gpiod_line_release(gpio_line_SCK);
    gpiod_line_release(gpio_line_MOSI);
    gpiod_line_release(gpio_line_MISO);
	gpiod_chip_close(gpio_chip);
}

void XSPI_Interface_software::enterFlashMode()
{
    gpiod_line_set_value(gpio_line_XX, 0);
    
    usleep(100 * 1000);

    gpiod_line_set_value(gpio_line_SS, 0);
    gpiod_line_set_value(gpio_line_EJ, 0);

    usleep(100 * 1000);

    gpiod_line_set_value(gpio_line_XX, 1);
    gpiod_line_set_value(gpio_line_EJ, 1);

    usleep(1000 * 1000);
}

void XSPI_Interface_software::leaveFlashMode()
{
    gpiod_line_set_value(gpio_line_SS, 1);
    gpiod_line_set_value(gpio_line_EJ, 0);

    usleep(100 * 1000);

    gpiod_line_set_value(gpio_line_XX, 0);
    gpiod_line_set_value(gpio_line_EJ, 1);
}

void XSPI_Interface_software::write_byte(uint8_t b)
{
    for(int i=0; i < 8; i++)
    {
        gpiod_line_set_value(gpio_line_MOSI, ((b & 0x01) == 1) );

        gpiod_line_set_value(gpio_line_SCK, 1);
        clock_wait();
        gpiod_line_set_value(gpio_line_SCK, 0);
        clock_wait();
        b >>= 1;
    }
}

uint8_t XSPI_Interface_software::read_byte()
{
    uint8_t return_value = 0;
    gpiod_line_set_value(gpio_line_MOSI, 0);

    for(int i=0; i < 8; i++)
    {
        gpiod_line_set_value(gpio_line_SCK, 1);
        clock_wait();
        
        int read = gpiod_line_get_value(gpio_line_MISO);
        if(read == -1)
            throw std::runtime_error("Error reading input.");
        
        read = read == 1;

        gpiod_line_set_value(gpio_line_SCK, 0);
        clock_wait();

        return_value |= (read << i);
    }
    return return_value;
}

uint8_t XSPI_Interface_software::readByte(uint8_t reg)
{
    gpiod_line_set_value(gpio_line_SS, 0);

    write_byte((uint8_t)((reg << 2) | 1));
    write_byte(0xff);

    uint8_t ret = read_byte();
    
    gpiod_line_set_value(gpio_line_SS, 1);    
    return ret;
}

void XSPI_Interface_software::read(uint8_t reg, uint8_t* out)
{
    gpiod_line_set_value(gpio_line_SS, 0);

    write_byte((uint8_t)((reg << 2) | 1));
    write_byte(0xff);

    out[0] = read_byte();
    out[1] = read_byte();
    out[2] = read_byte();
    out[3] = read_byte();
    
    gpiod_line_set_value(gpio_line_SS, 1);
}

void XSPI_Interface_software::write(uint8_t reg, const uint8_t* data)
{
    gpiod_line_set_value(gpio_line_SS, 0);

    write_byte((uint8_t)((reg << 2) | 2));

    write_byte(data[0]);
    write_byte(data[1]);
    write_byte(data[2]);
    write_byte(data[3]);
    
    gpiod_line_set_value(gpio_line_SS, 1);
}

const uint8_t zeros[4] = {0};
void XSPI_Interface_software::write(uint8_t reg)
{
    write(reg, zeros);
}

void XSPI_Interface_software::write(uint8_t reg, uint32_t data)
{
    uint8_t data_converted[] = {
        (uint8_t)(data & 0xff), 
        (uint8_t)((data >> 8) & 0xff),
        (uint8_t)((data >> 16) & 0xff),
        (uint8_t)((data >> 24) & 0xff)
    };
    write(reg, data_converted);
}

void XSPI_Interface_software::write(uint8_t reg, uint8_t data)
{
    uint8_t data_converted[] = {data, 0, 0, 0};
    write(reg, data_converted);
}


void XSPI_Interface_software::testPins()
{
    gpiod_line_set_value(gpio_line_XX, 0);
    gpiod_line_set_value(gpio_line_SS, 0);
    usleep(2000 * 1000);

    gpiod_line_set_value(gpio_line_XX, 0);
    gpiod_line_set_value(gpio_line_SS, 1);
    usleep(2000 * 1000);

    gpiod_line_set_value(gpio_line_XX, 1);
    gpiod_line_set_value(gpio_line_SS, 0);
    usleep(2000 * 1000);

    gpiod_line_set_value(gpio_line_XX, 1);
    gpiod_line_set_value(gpio_line_SS, 1);
    usleep(2000 * 1000);
}