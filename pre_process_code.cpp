#include <algorithm>
#include <iostream>  // std::endl, std::cerr
#include <vector>

#include "helper.h"
#include "pre_process_code.h"

const int32_t START = ((int32_t)(1 << 28));

extern std::vector<std::string> code;

std::vector<std::string> codeinit;
std::vector<lab> labels;
std::vector<seg> datalabel;

/* To process Load Word (lw) pseudo instruction.
 *
 * "lw t0, SYMBOL" -> "lui t0, SYMBOL[31:12]" + "lw t0, SYMBOL[11:0](t0)"
 */
static void __process_lw(const std::string type, const std::string &line,
                         const int32_t pos) {
  std::string rd, labeladd;
  int32_t currentpc, temp;
  size_t i;

  std::cout << __func__ << std::endl;
  std::cout << "line: " << line << std::endl;

  i = line.find('x') + 1;
  rd = line.substr(i, line.find_first_of(' ', i) - i);

  code.push_back("auipc x" + rd + " 65536");

  currentpc = pos - (code.size() * 4 - 4 + START);
  temp = abs(currentpc);

  while (temp != 0) {
    labeladd += (temp % 10) + '0';
    temp /= 10;
  }

  if (currentpc < 0) std::cout << "!!!!" << std::endl;
  labeladd += (currentpc < 0 ? '-' : '0');
  reverse(labeladd.begin(), labeladd.end());

  std::cout << type + " x" + rd + " " + labeladd + "(x" + rd + ")" << std::endl;
  code.push_back(type + " x" + rd + " " + labeladd + "(x" + rd + ")");
}

static int32_t __label_position(const std::string &line) {
  std::string label;
  size_t j;

  label = line.substr(line.find_last_of(' ') + 1);

  for (j = 0; j < datalabel.size(); j++) {
    if (label.compare(datalabel[j].name) == 0) {
      return datalabel[j].position;
    }
  }

  return -1;
}

/* To process Load Address(la) pseudo instruction.
 *
 * "la t0, SYMBOL" -> "lui t0, SYMBOL[31:12]" -> "addi t0, t0, SYMBOL[11:0]"
 */
static void __process_la(const std::string &line) {
  std::string rd, labeltype, labeladd;
  int32_t labeladdress;
  size_t i = 0;

  i = line.find('x') + 1;
  rd = line.substr(i, line.find_first_of(' ', i) - i);

  code.push_back("auipc x" + rd + " 65536");

  if ((labeladdress = __label_position(line)) == -1) {
    std::cerr << "ERROR: Label in \"" << line
              << "\" doesn't exists anywhere in the file" << std::endl;
    exit(1);
  }

  labeladd = std::to_string(labeladdress - ((code.size() * 4 - 4) + START));

  code.push_back("addi x" + rd + " x" + rd + " " + labeladd);
  // std::cout << "addi x" + rd + " x" + rd + " " + labeladd << std::endl;
}

static inline int __is_load_instr(const std::string &instr) {
  return (instr == "lw" || instr == "lb" || instr == "lhw");
}

/* Convert registers from their ABI Name to their register number. For example:
 * Register | ABI Name
 * x2       | sp
 * x12      | a2
 */
static std::string __change_reg_names(std::string line) {
  const static struct {
    const std::string reg_name;
    const std::string ABI_name;
  } conversion[] = {
      {"x0", "zero"}, {"x1", "ra"},  {"x2", "sp"},  {"x3", "gp"},
      {"x4", "tp"},   {"x5", "t0"},  {"x6", "t1"},  {"x7", "t2"},
      {"x8", "fp"},   {"x8", "s0"},  {"x9", "s1"},  {"x10", "a0"},
      {"x11", "a1"},  {"x12", "a2"}, {"x13", "a3"}, {"x14", "a4"},
      {"x15", "a5"},  {"x16", "a6"}, {"x17", "a7"}, {"x18", "s2"},
      {"x19", "s3"},  {"x20", "s4"}, {"x21", "s5"}, {"x22", "s6"},
      {"x23", "s7"},  {"x24", "s8"}, {"x25", "s9"}, {"x26", "s10"},
      {"x27", "s11"}, {"x28", "t3"}, {"x29", "t4"}, {"x30", "t5"},
      {"x31", "t6"}};

  size_t pos;

  for (size_t i = 1; i < sizeof(conversion) / sizeof(conversion[0]); i++) {
    pos = line.find(' ');

    pos = line.find(conversion[i].ABI_name, pos);

    while (pos != std::string::npos) {
      line.replace(pos, conversion[i].ABI_name.length(),
                   conversion[i].reg_name);
      pos += conversion[i].reg_name.length();
      pos = line.find(conversion[i].ABI_name, pos);
    }
  }

  return line;
}

void pre_process_code(void) {
  size_t n_code_lines = codeinit.size();
  int32_t count = 0;

  for (size_t i = 0; i < n_code_lines; i++) {
    const std::string &line = codeinit[i];
    std::string instr;
    size_t j = 0;
    size_t start = 0;

    // If we have a label we push it to labels
    if ((j = line.find(':')) != std::string::npos) {
      labels.push_back((lab){line.substr(0, j), count});
      j++;  // Jump ':' character
      // If there is no instruction after label, go to next line
      if (line.size() == j) continue;
      // Otherwise, get where instruction starts
      start = j = line.find_first_not_of(' ', j);
    } else {
      j = 0;
    }

    instr = line.substr(j, line.find_first_of(' ', j) - j);

    if (instr == "la") {
      __process_la(line);
      count += 2;
      continue;
    } else if (__is_load_instr(instr)) {
      int32_t pos;

      if ((pos = __label_position(line)) != -1) {
        __process_lw(instr, line, pos);
        count += 2;
        continue;
      }
    }

    std::string format = __get_instr_format(instr);

    if (!format.empty()) {
      code.push_back(__change_reg_names(line).substr(start));
      count++;
    }

    instr.clear();
  }
}
