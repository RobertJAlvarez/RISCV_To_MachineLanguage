#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdint.h>

#include <string>
#include <vector>

int32_t get_num(const std::string &line, size_t &i, const size_t start);

int32_t get_reg_num(const std::string &line, size_t &i);

std::string __get_instr_format(const std::string &instr);

std::vector<std::string> tokenize(const std::string &str,
                                  const char delims = ' ');

#endif
