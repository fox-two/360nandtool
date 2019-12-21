#pragma once

#include "SPI_Interface_base.h"
#include <cctype>

typedef void (*ReadCallbackPtr)(const uint8_t* data, int size);
typedef void (*WriteCallbackPtr)(uint8_t* data);
class NandReader
{
public:
    NandReader(SPI_Base& interface);
    ~NandReader();
    void dumpNand(int block_count, ReadCallbackPtr callback, bool ignore_ecc_errors=false);
    void writeNand(int block_count, WriteCallbackPtr callback);
    uint32_t getStatus();

    //check ECC data for a block
    //expects a pointer to 528 bytes of data
    static bool checkBlockECC(uint8_t* data);


    int ECC_retry_errors;
    int ECC_errors_ignored;

    void writeBlock(uint16_t block_number, const uint8_t* buffer);
    void readBlock(uint16_t block_number, uint8_t* buffer);

    void eraseBlockGroup(uint16_t block_group_number);
private:
    void clearStatus();
    bool waitReady();
    
    SPI_Base& spi_interface;
};