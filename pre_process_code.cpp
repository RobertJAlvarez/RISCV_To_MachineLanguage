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

  s = line.substr(i, line.find(" ", i) - i);

  currentpc = pos - (code.size() * 4 + START);
  code.push_back("auipc x" + s + " 65536");
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

  s = line.substr(i, line.find_first_of(" ", i) - i);

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

static int __load_label(const std::string &line) {
  std::string lab;
  size_t j;

  j = line.find_last_not_of(' ');
  lab = line.substr(line.find_last_of(" ", j) + 1);

  for (j = 0; j < datalabel.size(); j++) {
    if (lab.compare(datalabel[j].name) == 0) {
      return datalabel[j].position;
    }
  }

  return -1;
}

static inline int is_load_instr(const std::string &instr) {
  return (instr == "lw" || instr == "lb" || instr == "lhw");
}

/* To convert Stack Pointer(sp) to x2. */
static std::string __change_sp(const std::string &org_line) {
  std::string line = org_line;
  size_t pos = 0;

  while ((pos = line.find("sp", pos)) != std::string::npos) {
    line.replace(pos, 2, "x2");
    pos += 2;
  }

  return line;
}

static std::string __get_token(const std::string &line, size_t &j) {
  std::string instr;

  while (j < line.size() && line[j] != ' ') {
    instr += line[j++];
    if (j < line.size() && line[j] == ':') {
      instr += line[j++];
      break;
    }
  }

  return instr;
}

/* Expand psudo-instruction and extract all the labels. */
void pre_process_code(void) {
  size_t n_code_lines = codeinit.size();
  int count = -1;

  for (size_t i = 0; i < n_code_lines; i++) {
    const std::string &line = codeinit[i];
    std::string instr;
    size_t j;
    int start;

    start = j = line.find_first_not_of(' ');
    instr = __get_token(line, j);

    j = line.find_first_not_of(',', j);

    if (__is_label(instr)) {
      labels.push_back((lab){instr.substr(0, instr.size() - 1), count + 1});
      if (line.size() > instr.size() + 1) {
        start = j = line.find_first_not_of(' ', j);

        instr.clear();
        instr = line.substr(j, line.find(' ', j) - j);
      }
    }

    if (instr == "la") {
      __process_la(i);
      count += 2;
      continue;
    } else if (is_load_instr(instr)) {
      int32_t pos;

      if ((pos = __load_label(line)) != -1) {
        __process_lw(instr, line, pos);
        count += 2;
        continue;
      }
    }

    std::string format = __get_instr_format(instr);

    if (!format.empty()) {
      code.push_back(__change_sp(line).substr(start));
      count++;
    }

    instr.clear();
  }
}
