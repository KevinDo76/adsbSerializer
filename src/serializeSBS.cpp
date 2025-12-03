#include "serializeSBS.h"
#include <iostream>
#include <stdint.h>
#include <chrono>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <exception>

uint64_t getBytes64_t(std::vector<uint8_t>&bytes, int i)
{
    uint64_t returnInt = 0;
    if (i>bytes.size()-8)
    {
        throw std::runtime_error("Data out of bound");
    }
    for (int j=0;j<8;j++)
    {
        returnInt = (returnInt << 8) | bytes[i+j];
    }

    return returnInt;
}

uint32_t getBytes32_t(std::vector<uint8_t>&bytes, int i)
{
    uint32_t returnInt = 0;
    if (i>bytes.size()-4)
    {
        throw std::runtime_error("Data out of bound");
    }
    for (int j=0;j<4;j++)
    {
        returnInt = (returnInt << 8) | bytes[i+j];
    }

    return returnInt;
}

uint16_t getBytes16_t(std::vector<uint8_t>&bytes, int i)
{
    uint32_t returnInt = 0;
    if (i>bytes.size()-2)
    {
        throw std::runtime_error("Data out of bound");
    }
    for (int j=0;j<2;j++)
    {
        returnInt = (returnInt << 8) | bytes[i+j];
    }

    return returnInt;
}

uint8_t getBytes8_t(std::vector<uint8_t>&bytes, int i)
{
    uint8_t returnInt = 0;
    if (i>bytes.size()-1)
    {
        throw std::runtime_error("Data out of bound");
    }
    for (int j=0;j<1;j++)
    {
        returnInt = (returnInt << 8) | bytes[i+j];
    }

    return returnInt;
}

std::string epoch_to_string(int64_t epoch_ms)
{
    // Split into seconds + milliseconds
    time_t seconds = epoch_ms / 1000;
    int ms = epoch_ms % 1000;

    // Convert to UTC calendar time
    std::tm tm{};
    gmtime_r(&seconds, &tm);        // Linux
    // gmtime_s(&tm, &seconds);     // Windows alternative

    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  "%04d/%02d/%02d,%02d:%02d:%02d.%03d",
                  tm.tm_year + 1900,
                  tm.tm_mon + 1,
                  tm.tm_mday,
                  tm.tm_hour,
                  tm.tm_min,
                  tm.tm_sec,
                  ms);

    return buf;
}

void pushBytes16_t(uint32_t num, std::vector<uint8_t>& bytes)
{
    bytes.push_back((num&0xff00)>>8);
    bytes.push_back((num&0xff));
}

void pushBytes8_t(uint8_t num, std::vector<uint8_t>& bytes)
{
    bytes.push_back((num));
}

void pushBytes32_t(uint32_t num, std::vector<uint8_t>& bytes)
{
    bytes.push_back((num&0xff000000)>>24);
    bytes.push_back((num&0xff0000)>>16);
    bytes.push_back((num&0xff00)>>8);
    bytes.push_back((num&0xff));
}

void pushBytes64_t(uint64_t num, std::vector<uint8_t>& bytes)
{
    bytes.push_back((num&0xff00000000000000)>>56);
    bytes.push_back((num&0xff000000000000)>>48);
    bytes.push_back((num&0xff0000000000)>>40);
    bytes.push_back((num&0xff00000000)>>32);
    bytes.push_back((num&0xff000000)>>24);
    bytes.push_back((num&0xff0000)>>16);
    bytes.push_back((num&0xff00)>>8);
    bytes.push_back((num&0xff));
}

uint8_t getIndexMessageType(std::string type)
{
    if (type=="SEL") return 0;
    if (type=="ID") return 1;
    if (type=="AIR") return 2;
    if (type=="STA") return 3;
    if (type=="CLK") return 4;
    if (type=="MSG") return 5;
    return 7;
}

std::string getMessageTypeIndex(uint8_t index)
{
    switch (index)
    {
        case 0:
            return "SEL";
        case 1:
            return "ID";
        case 2:
            return "AIR";
        case 3:
            return "STA";
        case 4:
            return "CLK";
        case 5:
            return "MSG";
        default:
            return "UKN";
    }
}

uint64_t to_epoch_ms(const std::string date, const std::string time)
{
    int year, month, day, hour, minute, second, ms;
    //std::cout<<date<<" | "<<time<<"\n";
    // Parse full timestamp including milliseconds
    if (sscanf(date.c_str(), "%d/%d/%d",
               &year, &month, &day) != 3)
    {
        return -1; // invalid format
    }

    if (sscanf(time.c_str(), "%d:%d:%d.%d",
               &hour, &minute, &second, &ms) != 4)
    {
        return -1; // invalid format
    }


    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon  = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = second;

    // Convert struct tm → epoch seconds (UTC)
    time_t sec_epoch = timegm(&tm);  // requires Linux; for Windows use _mkgmtime

    if (sec_epoch == -1)
        return -1;

    //std::cout<<uint64_t(sec_epoch) * 1000 + ms<<" real\n";
    return uint64_t(sec_epoch) * 1000 + ms;

}

uint8_t convertMsgAndTranmissionType(std::string msgType, std::string TranType)
{
    uint8_t mType = getIndexMessageType(msgType);
    uint8_t tType = 0;
    if (!TranType.empty())
    {
        tType = std::stoi(TranType) - 1;
        if (tType < 0 || tType > 7)
        {
            tType = 0;
        }
    }
    uint8_t returnType = 0;
    returnType = returnType | ((mType & 7) << 3);
    returnType = returnType | (tType & 7);
    return returnType;
}

void serializeLine(std::vector<std::string> fields, std::vector<uint8_t>& bytes)
{
    uint32_t keyMap = 0;
    for (int i=0;i<22;i++)
    {
        if (i<fields.size())
        {
            keyMap = (keyMap<<1) | (!fields[i].empty());
        } else 
        {
            keyMap = keyMap << 1;
        }
    }
    bytes.push_back((keyMap&0xff0000)>>16);
    bytes.push_back((keyMap&0xff00)>>8);
    bytes.push_back((keyMap&0xff));

    uint8_t msgType = 0;

    bytes.push_back(convertMsgAndTranmissionType(fields[0], fields[1]));
    pushBytes32_t(std::stoi(fields[2]), bytes);
    pushBytes32_t(std::stoi(fields[3]), bytes);
    pushBytes32_t(std::stoi(fields[4], nullptr, 16), bytes);
    pushBytes32_t(std::stoi(fields[5]), bytes);
    pushBytes64_t(to_epoch_ms(fields[6], fields[7]), bytes);
    pushBytes64_t(to_epoch_ms(fields[8], fields[9]), bytes);
    if (!fields[10].empty()) //callsign
    {
        for (int i=0;i<fields[10].length();i++)
        {
            //std::cout<<(int)fields[10][i]<<" "<<fields[10][i]<<"\n";
            bytes.push_back(fields[10][i]);
        }
        bytes.push_back(0);
    }
    if (!fields[11].empty()) //altitude
    {
        pushBytes32_t(std::stoi(fields[11]), bytes);
    }
    if (!fields[12].empty())
    {
        pushBytes16_t(std::stoi(fields[12]), bytes);
    }
    if (!fields[13].empty())
    {
        pushBytes16_t(std::stoi(fields[13]), bytes);
    }

    if (!fields[14].empty())
    {
        float lat = std::stof(fields[14]);
        int* latP = (int*)(&lat);
        pushBytes32_t(*latP, bytes);
    }
    if (!fields[15].empty())
    {
        float lon = std::stof(fields[15]);
        int* lonP = (int*)(&lon);
        pushBytes32_t(*lonP, bytes);
    }

    if (!fields[16].empty())
    {
        pushBytes16_t(std::stoi(fields[16]), bytes);
    }

    if (!fields[17].empty())
    {
        pushBytes16_t(std::stoi(fields[17]), bytes);
    }

    if (!fields[18].empty())
    {
        pushBytes8_t(std::stoi(fields[18]), bytes);
    }
    if (!fields[19].empty())
    {
        pushBytes8_t(std::stoi(fields[19]), bytes);
    }
    if (!fields[20].empty())
    {
        pushBytes8_t(std::stoi(fields[20]), bytes);
    }
    if (fields.size()==22 && !fields[21].empty())
    {
        pushBytes8_t(std::stoi(fields[21]), bytes);
    }

    //std::cout<<keyMap<<" "<<fields.size()<<"\n";
}

void readwholeFile(std::vector<uint8_t>& bytes)
{
    std::cout<<"Read out\n";
    uint32_t i = 0;
    while (i<bytes.size())
    {
        std::cout<<deserializeLine(bytes, i)<<"\n";
    }

}

std::string deserializeLine(std::vector<uint8_t>&bytes, uint32_t& i)
{
    uint32_t sizeOfByte = bytes.size();
    std::stringstream line;
    uint32_t keyMap = 0;
    if (i>=bytes.size()-3)throw std::runtime_error("Data out of bound");
    for (int j=0;j<3;j++)
    {
        keyMap = (keyMap<<8) | bytes[i];
        i++;
    }

    if (i>=bytes.size()-1) throw std::runtime_error("Data out of bound");
    line << getMessageTypeIndex(bytes[i]>>3) + ",";
    line << (bytes[i]&7)+1 << ",";
    i++;
    line << getBytes32_t(bytes, i) << ",";
    i+=4;
    line << getBytes32_t(bytes, i) << ",";
    i+=4;
    std::stringstream ICAO;
    ICAO << std::hex << getBytes32_t(bytes, i);
    std::string ICAOUpper = ICAO.str();
    std::transform(ICAOUpper.begin(), ICAOUpper.end(), ICAOUpper.begin(), ::toupper);
    line << ICAOUpper << ",";
    i+=4;
    line << getBytes32_t(bytes, i) << ",";
    i+=4;
    uint64_t timeMS = getBytes64_t(bytes, i);
    line << epoch_to_string(timeMS)<<",";
    i+=8;
    timeMS = getBytes64_t(bytes, i);
    line << epoch_to_string(timeMS)<<",";
    i+=8;
    if (keyMap&0x800)
    {
        while (true)
        {
            if (i<bytes.size() && bytes[i]) 
            {
                if (i>=bytes.size()-1) throw std::runtime_error("Data out of bound");
                line<<bytes[i];
                i++;
            } else {
                i++;
                break;
            }
            
        }    
    }
    line<<",";
    if (keyMap&0x400)
    {
        line << (int32_t)getBytes32_t(bytes, i);
        i+=4;
    }
    line<<",";
    if (keyMap&0x200)
    {
        line << (uint32_t)getBytes16_t(bytes, i);
        i+=2;
    }
    line<<",";
    if (keyMap&0x100)
    {
        line << (uint32_t)getBytes16_t(bytes, i);
        i+=2;
    }
    line<<",";
    if (keyMap&0x80)
    {
        uint32_t coordP = (uint32_t)getBytes32_t(bytes, i);
        float* coord = (float*)(&coordP);
        char buff[64];
        sprintf(buff, "%.5f", *coord);
        line << buff;
        i+=4;
    }
    line<<",";
    if (keyMap&0x40)
    {
        uint32_t coordP = (uint32_t)getBytes32_t(bytes, i);
        float* coord = (float*)(&coordP);
        char buff[64];
        sprintf(buff, "%.5f", *coord);
        line << buff;
        i+=4;
    }
    line<<",";
    if (keyMap&0x20)
    {
        line << (int16_t)getBytes16_t(bytes, i);
        i+=2;
    }
    line<<",";

    if (keyMap&0x10)
    {
        line << (uint16_t)getBytes16_t(bytes, i);
        i+=2;
    }
    line<<",";

    if (keyMap&0x8)
    {
        line << (int32_t)((int8_t)getBytes8_t(bytes, i));
        i++;
    }
    line<<",";

    if (keyMap&0x4)
    {
        line << (int32_t)((int8_t)getBytes8_t(bytes, i));
        i++;
    }
    line<<",";

    if (keyMap&0x2)
    {
        line << (int32_t)((int8_t)getBytes8_t(bytes, i));
        i++;
    }
    line<<",";

    if (keyMap&0x1)
    {
        line << (int32_t)((int8_t)getBytes8_t(bytes, i));
        i++;
    }


    //std::cout<<keyMap<<" decoded\n";
    return line.str();
}
