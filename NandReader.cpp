#include "NandReader.h"
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <cstdio>

constexpr int READ_RETRIES = 0x1000;
constexpr int WORDS_PER_BLOCK = 0x84;
constexpr int BLOCKSIZE = WORDS_PER_BLOCK*4;

NandReader::NandReader(SPI_Base& interface) :
    spi_interface(interface)
{
    spi_interface.enterFlashMode();

    union
    {
        uint8_t buffer[4];
        uint32_t integer;
    } flash_config;
    
    spi_interface.read(0, flash_config.buffer);
    spi_interface.read(0, flash_config.buffer);

    printf("flash config: 0x%X\n", flash_config.integer);
}

NandReader::~NandReader()
{
    spi_interface.leaveFlashMode();
}

#include <cstdlib>
void NandReader::dumpNand(int block_count, ReadCallbackPtr callback, bool ignore_ecc_errors)
{
    ECC_retry_errors = 0;
    ECC_errors_ignored = 0;
    uint8_t buffer[BLOCKSIZE];
    int max_retries = 10;

    if(ignore_ecc_errors)
        max_retries = 1;
    for(int i=0; i < block_count; i++)
    {
        bool ecc_ok = false;
        for(int r=0; r < max_retries; r++)
        {
            readBlock(i, buffer);
            ecc_ok = checkBlockECC(buffer);
            if(ecc_ok)
                break;
            ECC_retry_errors++;
        }

        if(!ecc_ok)
        {
            if(!ignore_ecc_errors)
                throw std::runtime_error("Error reading block " + std::to_string(i) + " (bad ECC).");
            else
                ECC_errors_ignored++;
        }
        callback(buffer, BLOCKSIZE);
    }
}

void NandReader::writeNand(int block_count, WriteCallbackPtr callback)
{
    uint8_t buffer[BLOCKSIZE];
    for(int i=0; i < block_count; i++)
    {
        callback(buffer);
        if((i % 256) == 0)
            eraseBlockGroup(i);
        writeBlock(i, buffer);
    }
}


void NandReader::readBlock(uint16_t block_number, uint8_t* buffer)
{
    clearStatus();

    //select the block
    spi_interface.write(0x0c, (uint32_t)(block_number << 9));
    spi_interface.write(0x08, (uint8_t)0x03);

    //wait until nand is ready
    if(!waitReady())
        throw std::runtime_error("Error while waiting nand (timeout).");

    int res = getStatus();
    spi_interface.write(0x0c);

    //now begin reading
    for(int i=0; i < WORDS_PER_BLOCK; i++)
    {
        spi_interface.write(0x08);
        spi_interface.read(0x10, buffer + 4*i);
    }
}


void NandReader::writeBlock(uint16_t block_number, const uint8_t* buffer)
{
    //start write block operation
    clearStatus();
    spi_interface.write(0x0c);

    //write block data
    for(int i=0; i < WORDS_PER_BLOCK; i++)
    {
        spi_interface.write(0x10, buffer + 4*i);
        spi_interface.write(0x08, (uint8_t)0x01);
    }

    //commit the write
    spi_interface.write(0x0c, (uint32_t)block_number << 9);

    spi_interface.write(0x08, (uint8_t)0x55);
    spi_interface.write(0x08, (uint8_t)0xAA);
    spi_interface.write(0x08, (uint8_t)0x4);

    if(!waitReady())
        throw std::runtime_error("Error while waiting nand (timeout).");
    getStatus();

}

uint32_t NandReader::getStatus()
{
    uint8_t tmp[4];
    spi_interface.read(4, tmp);
    return tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
}

bool NandReader::waitReady()
{
    int retries = READ_RETRIES;
    do
    {
        if(!(spi_interface.readByte(0x04) & 0x01))
            return true;
    } while (retries > 0);
    return false;
}

void NandReader::clearStatus()
{
    uint8_t tmp[4];
    spi_interface.read(4, tmp);
    spi_interface.write(4, tmp);
}

void NandReader::eraseBlockGroup(uint16_t block_number)
{
    uint32_t number_ = (uint32_t)block_number << 9;
    
    clearStatus();

    uint8_t tmp[4];
    spi_interface.read(0, tmp);
    tmp[0] |= 0x08;
    spi_interface.write(0, tmp);

    spi_interface.write(0x0c, number_);

    if(!waitReady())
        throw std::runtime_error("Error while waiting nand (timeout).");
    
    spi_interface.write(0x08, (uint8_t)0xAA);
    spi_interface.write(0x08, (uint8_t)0x55);

    if(!waitReady())
        throw std::runtime_error("Error while waiting nand (timeout).");

    spi_interface.write(0x08, (uint8_t)0x5);

    if(!waitReady())
        throw std::runtime_error("Error while waiting nand (timeout).");
}


//code found on https://free60project.github.io/wiki/NAND_File_System.html
bool NandReader::checkBlockECC(uint8_t* datc)
{
    uint32_t i=0, val=0; 
    uint8_t edc[4] = {0,0,0,0}; 
    uint8_t * data = datc;
    uint8_t* spare = (datc + 0x200);
    uint32_t v=0; 

    for (i = 0; i < 0x1066; i++) 
    {    
        if (!(i & 31))    
        {
            if (i == 0x1000)
                data = spare;
            v = ~(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
            data += 4;
        }
        val ^= v & 1;
        v>>=1;
        if (val & 1)
            val ^= 0x6954559;    
        val >>= 1; 
    }
    val = ~val;

    edc[0] = (val << 6) & 0xC0;
    edc[1] = (val >> 2) & 0xFF;
    edc[2] = (val >> 10) & 0xFF;
    edc[3] = (val >> 18) & 0xFF;

    if(((spare[0xC] & 0xC0) != edc[0])||(spare[0xD] != edc[1])||(spare[0xE] != edc[2])||(spare[0xF] != edc[3]))
        return false;

    return true; 
}

