#include "helper.h"

extern std::vector<std::string> formats;

int32_t __get_inver(int32_t num, const int n_bits) {
  // Create a mask with the first n bits set to 1
  int32_t mask = (n_bits >= 32 ? 0 : 1 << n_bits) - 1;

  // Take the twos complement of the first n_bits
  if (num >= 0) num = (num ^ mask) + 1;

  // If num is negative we truncate it to the first n_bits
  num &= mask;

  return num;
}

char __int_to_hex(const int num) {
  if (num <= 9) return static_cast<char>('0' + num);
  return static_cast<char>('A' + (num - 10));
}

int32_t __get_num(const std::vector<int> temp, const int32_t base) {
  int32_t num = 1;
  int32_t ans = 0;

  for (auto digit = temp.rbegin(); digit != temp.rend(); ++digit) {
    ans += num * *digit;
    num *= base;
  }

  return ans;
}

std::string __get_instr_format(const std::string &instr) {
  const size_t n_instructions = formats.size();
  std::string type;

  for (size_t k = 0; k < n_instructions; k++) {
    const std::string &format_line = formats[k];

    type = format_line.substr(0, format_line.find(' '));

    if (instr.compare(type) == 0) return format_line;
  }

  return std::string();
}
