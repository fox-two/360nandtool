#include <iostream>
#include <stdexcept>
#include "NandReader.h"
#include <vector>
#include <cstdio>
#include <chrono>
#include <fstream>
#include <cstring>
#include "SPI_Interface_software.h"
#include "config.h"

std::chrono::steady_clock::time_point start_time;
std::fstream output_file;
std::fstream input_file;
int current_block = 0;
int blocks_to_process;

void show_progress()
{
    constexpr int progress_bar_max_width = 80;

    std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
    float elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count()/1000.0;

    int ETA = -1;

    if(elapsed_time > 0)
    {
        int blocks_per_sec = current_block / (elapsed_time);
        ETA = (blocks_to_process - current_block) / blocks_per_sec;
    }

    float progress = (current_block/(float)blocks_to_process);

    int progress_bar_width = progress*80;
    printf("\r[");
    for(int i=0; i < progress_bar_max_width; i++)
    {
        if(i <= progress_bar_width)
            putchar('-');
        else 
            putchar(' ');
    }
    if(ETA != -1)
        printf("] %1.2f%% (ETA: %ds)", 100*progress, ETA);
    else
        printf("] %1.2f%%", 100*progress);
}


void show_usage(const char* command_name)
{
    printf("Usage: %s [-s number_of_mb] [-o output_file] [-i input_file] [-f]\n", command_name);
    printf("[-s]: size of nand file.\n");
    printf("[-o]: file to dump NAND data.\n");
    printf("[-f]: ignores ECC errors.\n");
    printf("[-i]: writes the file to the NAND.");
    printf("\nExamples:\n");
    printf("To read 16mb to dump.bin: %s -s 16 -o dump.bin\n", command_name);
    printf("Write dump file back to NAND: %s -i dump.bin\n", command_name);
    printf("Write only the first 3mb: %s -i dump.bin -s 3\n", command_name);
}

int main(int argc, char* argv[])
{ 
    auto pin_config = load_pin_config();

    for(auto& a : pin_config)
    {
        printf("%s: %i\n", a.first.c_str(), a.second);
    }
    int size_in_megabytes = 0;
    bool ignore_ecc_errors = false;
    const char* output_filename = nullptr;
    const char* input_filename = nullptr;

    for(int i=1; i < argc; i++)
    {
        if(strcmp(argv[i], "-s") == 0)
        {
            if((i+1) >= argc)
            {
                show_usage(argv[0]);
                return -1;
            }
            size_in_megabytes = std::atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-f") == 0)
        {
            ignore_ecc_errors = true;
        }
        else if(strcmp(argv[i], "-o") == 0)
        {
            if((i+1) >= argc)
            {
                show_usage(argv[0]);
                return -1;
            }
            output_filename = argv[++i];
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            if((i+1) >= argc)
            {
                show_usage(argv[0]);
                return -1;
            }
            input_filename = argv[++i];
        }
        else if(strcmp(argv[i], "-h") == 0)
        {
            show_usage(argv[0]);
            return -1;
        }
        else
        {
            show_usage(argv[0]);
            return -1;
        }
    }

    if( ((output_filename != nullptr) && (input_filename != nullptr)) || 
        ((output_filename == nullptr) && (input_filename == nullptr)) )
    {
        show_usage(argv[0]);
        return -1;
    }
    else if(input_filename != nullptr)
    {
        XSPI_Interface_software spi_interface("gpiochip0", 
                                              pin_config["pin_EJ"], 
                                              pin_config["pin_XX"], 
                                              pin_config["pin_SS"], 
                                              pin_config["pin_SCK"],  
                                              pin_config["pin_MOSI"], 
                                              pin_config["pin_MISO"]);
        NandReader reader(spi_interface);
        try
        {
            printf("Writing \"%s\" to NAND.\n", input_filename);
            input_file.open(input_filename, std::fstream::in | std::fstream::binary);

            if(size_in_megabytes == 0)
            {
                input_file.seekg(0, std::fstream::end);
                blocks_to_process = input_file.tellg()/528;
                input_file.seekg(0, std::fstream::beg);
            }
            else
                blocks_to_process = 2048*size_in_megabytes;
            start_time = std::chrono::steady_clock::now();

            printf("%d blocks to write.\n", blocks_to_process);
            reader.writeNand(blocks_to_process, [](uint8_t* buffer) {
                current_block += 1;
                show_progress();
                input_file.read((char*)buffer, 528);
            });
            printf("\n");
        }
        catch(std::runtime_error& ex)
        {
            printf("\nError: %s\n", ex.what());
        }
    }
    else if(output_filename != nullptr)
    {
        if(size_in_megabytes == 0)
        {
            show_usage(argv[0]);
            return -1;
        }
        try
        {
            printf("Reading first %dmb from NAND.\n", size_in_megabytes);
            if(ignore_ecc_errors)
                printf("Ignoring ECC errors.\n");
            printf("Writing to file \"%s\"\n", output_filename);
            output_file.open(output_filename, std::fstream::out | std::fstream::binary);

            XSPI_Interface_software spi_interface("gpiochip0", 24, 25, 8, 11, 10, 9);
            NandReader reader(spi_interface);
            
            printf("Status started: 0x%Xz\n", reader.getStatus());
            blocks_to_process = (size_in_megabytes*2048);
            
            start_time = std::chrono::steady_clock::now();
            
            reader.dumpNand(blocks_to_process, [](const uint8_t* data, int size) {
                current_block += 1;
                show_progress();
                output_file.write((const char*)(data), size);
            }, ignore_ecc_errors);
            printf("\nRead retries: %d\n", reader.ECC_retry_errors);
            printf("Blocks with invalid ECC: %d\n", reader.ECC_errors_ignored);

        }
        catch(std::runtime_error& ex)
        {
            printf("\nError: %s\n", ex.what());
        }
    }

    
    return 0;
}