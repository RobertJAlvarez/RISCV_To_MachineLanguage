#include "helper.h"

uint32_t __get_inver(int32_t num, const int n_bits) {
  // Create a mask with the first n bits set to 1
  int32_t mask = (n_bits >= 32 ? 0 : 1 << n_bits) - 1;

  // Take the twos complement of the first n_bits
  if (num >= 0) num = (num ^ mask) + 1;

  // If num is negative we truncate it to the first n_bits
  num &= mask;

  return (uint32_t)num;
}

char __int_to_hex(const int32_t num) {
  if (num <= 9) return static_cast<char>('0' + num);
  return static_cast<char>('A' + (num - 10));
}

int32_t __get_num(const std::vector<int> temp, const int32_t giv) {
  int32_t num = 1;
  int32_t ans = 0;

  for (int32_t i = temp.size() - 1; i >= 0; i--) {
    ans += num * temp[i];
    num *= giv;
  }

  return ans;
}
