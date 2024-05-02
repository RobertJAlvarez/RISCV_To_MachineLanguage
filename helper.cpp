#include "helper.h"

extern std::vector<std::string> formats;

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
