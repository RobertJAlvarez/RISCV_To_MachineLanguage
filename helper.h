#ifndef __HELPER_H__
#define __HELPER_H__

#include <stdint.h>
#include <string>
#include <vector>

/* If positive, take twos complement of the first n_bits. Else, turn to 0 all
 * bits higher than n_bits.
 */
int32_t __get_inver(int32_t num, const int n_bits);

char __int_to_hex(const int num);

/* Return a numerical value of whose digits are stored in std::vector */
int32_t __get_num(const std::vector<int> temp, const int32_t giv);

std::string __get_instr_format(const std::string &instr);

#endif
