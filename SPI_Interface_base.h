#pragma once
#include <cstdint>

class SPI_Base
{
public:   
    virtual void enterFlashMode()=0;
    virtual void leaveFlashMode()=0;

    virtual uint8_t readByte(uint8_t reg)=0;
    //this api reads and writes in blocks of 4 bytes
    virtual void read(uint8_t reg, uint8_t* out)=0;
    
    virtual void write(uint8_t reg, const uint8_t* data)=0;
    virtual void write(uint8_t reg)=0; //assumes data is 4 zeros
    virtual void write(uint8_t reg, uint32_t data)=0;
    virtual void write(uint8_t reg, uint8_t data)=0;

};