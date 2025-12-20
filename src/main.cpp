#include <iostream>
#include <arpa/inet.h>
#include <vector>
#include <sstream>
#include <fstream>
#include "serializeSBS.h"
#include <filesystem>
#include <chrono>

#define SERVER_IP "10.182.69.106"
#define SERVER_PORT 30003

void rtrim(std::string &s) {
    // Find last character that is NOT whitespace or newline
    size_t end = s.find_last_not_of(" \t\r\n");
    if (end != std::string::npos) {
        s.erase(end + 1);
    } else {
        s.clear(); // string is all whitespace
    }
}

std::string getCurrentDateFormat()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t); 
    char dateBuff[80];
    std::strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &tm);
    std::string date_str(dateBuff);
    return date_str;
}

std::string currentDate=getCurrentDateFormat();

std::string findFileName()
{
    std::string date_str = getCurrentDateFormat();
    
    std::string filePath;
    int fileIndex = 0;
    while (true)
    {
        std::stringstream pathStream;
        pathStream<<"data"<<date_str<<"(";
        pathStream<<fileIndex<<").binary";
        std::filesystem::path path(pathStream.str());
        if (!std::filesystem::exists(path))
        {
            filePath = pathStream.str();
            break;
        }
        fileIndex ++;
    }
    return filePath;
}

int start_network(std::ofstream& outputFile, std::fstream& log)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton"); return 1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); return 1;
    }

    struct timeval tv{};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::string buffer;
    char temp[50];

    int lineCount = 0;
    int writtenBytesCount = 0;
    while (true)
    {
        ssize_t bytes = recv(sock, temp, sizeof(temp) - 1, 0);
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                std::cout<<"Empty pipe\n";
                continue;
            } else {
                perror("recv");
                break;
            } 
        } else if (bytes == 0) {
            std::cout << "Connection closed by dump1090.\n";
            break;
        }

        temp[bytes] = '\0';
        buffer += temp;

        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            rtrim(line);
            if (!line.empty()) {
                std::string dateNow = getCurrentDateFormat();
                std::vector<std::string> fields;
                std::stringstream ss(line);
                std::string field;

                while (std::getline(ss, field, ',')) {
                    fields.push_back(field);
                }

                std::vector<uint8_t> bytes;
                serializeLine(fields, bytes);
                uint32_t index = 0;

                std::string result = deserializeLine(bytes, index);
                bool isSame = (result == line );
                outputFile.write((char*)bytes.data(), bytes.size());
                outputFile.flush();

                writtenBytesCount+=bytes.size();
                if(writtenBytesCount>=20000000 || currentDate!=dateNow)
                {
                   outputFile.close();
                   std::string filePath = findFileName();
                   outputFile = std::ofstream(filePath, std::ios::binary);
                   writtenBytesCount = 0;
                   std::cout<<"New File "<<filePath;
                   currentDate = getCurrentDateFormat();
                }

                std::cout<<line<<"\n";
                if (!isSame)
                {
                    std::cout<<"DECODE FAILURE DETECTED\n";
                    std::cout<<line<<"\n";
                    std::cout<<result<< " " << fields.size()<<"\n";

                    log<<"line"<<"\n";
                    log<<result<< " " << fields.size()<<"\n";
                    log.flush();
                }                
            }
        }
    }
    return 0;
}

int main(int, char**)
{
    std::string filePath = findFileName();
    std::cout<<filePath<<"\n";

    std::ofstream outputFile(filePath, std::ios::binary);
    std::fstream outputLog("log.txt", std::ios::app);
    
    outputLog<<"Start!";
    outputLog.flush();
    
    start_network(outputFile, outputLog);

    std::vector<std::string> fields;
    std::stringstream ss("MSG,7,1,1,000000,1,2025/12/14,21:42:51.051,2025/12/14,21:43:43.670,,,,,,,,,,,,-1");
    std::string field;

    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }

    std::vector<uint8_t> bytes;
    serializeLine(fields, bytes);
    uint32_t i=0;
    bool isSame = (deserializeLine(bytes, i) == "MSG,7,1,1,000000,1,2025/12/14,21:42:51.051,2025/12/14,21:43:43.670,,,,,,,,,,,,-1");
    if (!isSame)
    {
        i = 0;
        std::cout<<"MSG,7,1,1,000000,1,2025/12/14,21:42:51.051,2025/12/14,21:43:43.670,,,,,,,,,,,,-1"<<"\n";
        std::cout<<deserializeLine(bytes, i)<< " " << fields.size()<<"\n";
    } 
    else 
    {
        std::cout<<"Okay!\n";
    }

    //return 0;
}
