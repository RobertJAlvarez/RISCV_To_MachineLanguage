#include <algorithm>
#include <vector>

#include "helper.h"
#include "pre_process_code.h"

const int32_t START = ((int32_t)(1 << 28));

extern std::vector<std::string> code;

std::vector<std::string> codeinit;
std::vector<lab> labels;
std::vector<seg> datalabel;

/* To process Load Word (lw) pseudo instruction. */
static void __process_lw(const std::string type, const std::string &line,
                         const int32_t pos) {
  std::string s, labeladd;
  int32_t currentpc, temp1;
  int i;

  i = line.find('x') + 1;

  s = line.substr(i, line.find_first_of(' ', i) - i);
  code.push_back("auipc x" + s + " 65536");

  currentpc = pos - (code.size() * 4 - 4 + START);
  temp1 = abs(currentpc);

  while (temp1 != 0) {
    labeladd += (temp1 % 10) + '0';
    temp1 /= 10;
  }

  labeladd += (currentpc < 0 ? '-' : '0');
  reverse(labeladd.begin(), labeladd.end());

  code.push_back(type + " x" + s + " " + labeladd + "(x" + s + ")");
}

/* To process Load Address(la) pseudo instruction. */
static void __process_la(const int index) {
  const std::string &line = codeinit[index];
  std::string s, labeltype, labeladd;
  int32_t currentpc, labeladdress;
  size_t i = 0;

  i = line.find('x') + 1;

  s = line.substr(i, line.find_first_of(' ', i) - i);
  code.push_back("auipc x" + s + " 65536");

  currentpc = (code.size() * 4 - 4) + START;

  i = line.find(' ', i);

  labeltype = line.substr(i, line.find(' ', i) - i);
  labeladdress = 0;

  for (size_t j = 0; j < datalabel.size(); j++) {
    if (labeltype.compare(datalabel[j].name) == 0) {
      labeladdress = datalabel[j].position;
      break;
    }
  }

  int32_t temp1 = abs(labeladdress - currentpc);

  while (temp1 != 0) {
    labeladd += (temp1 % 10) + '0';
    temp1 /= 10;
  }

  labeladd += (labeladdress < 0 ? '-' : '0');
  reverse(labeladd.begin(), labeladd.end());

  code.push_back("addi x" + s + " x" + s + " " + labeladd);
}

static int __label_position(const std::string &line) {
  std::string lab;
  size_t j;

  lab = line.substr(line.find_last_of(' ') + 1);

  for (j = 0; j < datalabel.size(); j++) {
    if (lab.compare(datalabel[j].name) == 0) {
      return datalabel[j].position;
    }
  }

  return -1;
}

static inline int __is_load_instr(const std::string &instr) {
  return (instr == "lw" || instr == "lb" || instr == "lhw") ||
         (instr == "LW" || instr == "LB" || instr == "LHW");
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
      {"x0", "zero"}, {"x1", "RA"},  {"x2", "SP"},  {"x3", "GP"},
      {"x4", "TP"},   {"x5", "T0"},  {"x6", "T1"},  {"x7", "T2"},
      {"x8", "FP"},   {"x8", "S0"},  {"x9", "S1"},  {"x10", "A0"},
      {"x11", "A1"},  {"x12", "A2"}, {"x13", "A3"}, {"x14", "A4"},
      {"x15", "A5"},  {"x16", "A6"}, {"x17", "A7"}, {"x18", "S2"},
      {"x19", "S3"},  {"x20", "S4"}, {"x21", "S5"}, {"x22", "S6"},
      {"x23", "S7"},  {"x24", "S8"}, {"x25", "S9"}, {"x26", "S10"},
      {"x27", "S11"}, {"x28", "T3"}, {"x29", "T4"}, {"x30", "T5"},
      {"x31", "T6"}};

  std::string low_case;
  size_t pos;

  for (size_t i = 1; i < sizeof(conversion) / sizeof(conversion[0]); i++) {
    pos = ((size_t)0);
    low_case = conversion[i].ABI_name;
    std::transform(low_case.begin(), low_case.end(), low_case.begin(),
                   ::tolower);

    pos = std::min((size_t)line.find(conversion[i].ABI_name, pos),
                   (size_t)line.find(low_case, pos));

    while (pos != std::string::npos) {
      line.replace(pos, conversion[i].ABI_name.length(),
                   conversion[i].reg_name);
      pos += conversion[i].reg_name.length();
      pos = std::min((size_t)line.find(conversion[i].ABI_name, pos),
                     (size_t)line.find(low_case, pos));
    }
  }

  return line;
}

void pre_process_code(void) {
  size_t n_code_lines = codeinit.size();
  int count = -1;

  for (size_t i = 0; i < n_code_lines; i++) {
    const std::string &line = codeinit[i];
    std::string instr;
    size_t j = 0;
    int start = 0;

    // If we have a label we push it to labels
    if ((j = line.find(':')) != std::string::npos) {
      labels.push_back((lab){line.substr(0, j), count + 1});
      j++;  // Jump ':' character
      // If there is no instruction after label, go to next line
      if (line.size() == j) continue;
      // Otherwise, get where instruction starts
      start = j = line.find_first_not_of(' ', j);
    } else {
      j = 0;
    }

    instr = line.substr(j, line.find_first_of(' ', j) - j);

    if ((instr == "la") || (instr == "LA")) {
      __process_la(i);
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
