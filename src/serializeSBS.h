#pragma once
#include <string>
#include <vector>
#include <stdint.h>


void serializeLine(std::vector<std::string> fields, std::vector<uint8_t>& bytes);
std::string deserializeLine(std::vector<uint8_t>&bytes, uint32_t& i);
void readwholeFile(std::vector<uint8_t>&bytes);