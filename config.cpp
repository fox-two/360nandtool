#include "config.h"
#include <regex>
#include <fstream>

std::regex ini_name("[A-Za-z_][A-Za-z0-9_]*");
std::regex ini_number("[0-9]*");
std::string parse_config_name(const char*& text)
{
    std::cmatch m;
        
    if(!std::regex_search(text, m, ini_name, std::regex_constants::match_continuous))
        throw std::runtime_error("Error parsing ini file.");
    
    text += m[0].length();

    return m[0].str();
}

int parse_number(const char*& text)
{
    std::cmatch m;
        
    if(!std::regex_search(text, m, ini_number, std::regex_constants::match_continuous))
        throw std::runtime_error("Error parsing ini file.");
    
    text += m[0].length();

    return std::stoi(m[0].str());
}

std::pair<std::string, int> parse_config_line(const char*& text)
{
    std::pair<std::string, int> return_val;
    return_val.first = parse_config_name(text);

    if(*text != '=')
        throw std::runtime_error("Error parsing ini file." + std::string(text));
    
    text++;

    return_val.second = parse_number(text);

    if(*text != '\n' && *text != '\0')
        throw std::runtime_error("Error parsing ini file.");
    
    text++;
    return return_val;
}


std::map<std::string, int> load_pin_config()
{
    std::map<std::string, int> pin_config = {
        {"pin_EJ", 24},
        {"pin_XX", 25},
        {"pin_SS", 8},
        {"pin_SCK", 10},
        {"pin_MOSI", 11},
        {"pin_MISO", 9},
    };
    std::fstream config_file("config.ini", std::fstream::in);
    if(!config_file.is_open())
        return pin_config;
    
    std::string str((std::istreambuf_iterator<char>(config_file)),
                     std::istreambuf_iterator<char>());
    
    for(auto it=str.begin(); it != str.end(); )
    {
        if(*it == ' ')
            it = str.erase(it);
        else
            it++;
    }
    const char* config_file_str = str.c_str();
    while(*config_file_str != '\0')
    {
        if(*config_file_str == '\n')
        {
            config_file_str++;
            continue;
        }
        if(*config_file_str == '#')
        {
            while(*config_file_str != '\n')
                config_file_str++;
            continue;
        }
        std::pair<std::string, int> config_line = parse_config_line(config_file_str);
        if(pin_config.count(config_line.first) == 0)
            continue;
        
        pin_config[config_line.first] = config_line.second;
    }
    return pin_config;
}