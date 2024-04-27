#include <iostream>  // std::endl, std::cerr

#include "helper.h"
#include "process_files.h"

#define ARCH_SIZE (32)

static std::vector<std::string> code;
std::vector<std::string> codeinit;
std::vector<std::string> formats;
std::vector<seg> datalabel;

const int32_t START = ((int32_t)(1 << 28));

static int32_t pc = 0;
static int32_t binary[ARCH_SIZE];

typedef struct {
  std::string s;
  int index;
} lab;

static std::vector<lab> labels;

// To get numerical value of hexadecimal formats
static int32_t __get_hex(std::vector<int> temp) {
  int32_t num = 1;
  int32_t ans = 0;

  for (int32_t i = temp.size() - 1; i > 1; i--) {
    if (temp[i] < 16) {
      ans += temp[i] * num;
    } else if (temp[i] <= 42) {
      ans += (temp[i] - 7) * num;
    } else {
      temp[i] -= 32;
      ans += (temp[i] - 7) * num;
    }
    num *= 16;
  }

  return ans;
}

static void __get_reg_num(const std::string &line, size_t &i, int32_t &reg) {
  std::vector<int> temp;

  i = line.find('x', i) + 1;

  while (line[i] != ' ' && line[i] != ',') {
    temp.push_back(line[i] - '0');
    i++;
  }

  reg = __get_num(temp, 10);
  temp.clear();
}

static void __get_dst_src(const std::string &line, int32_t &r1, int32_t &imm,
                          int32_t &r2) {
  std::vector<int> temp;
  size_t i = 0;

  __get_reg_num(line, i, r1);

  while (!isdigit(line[i])) i++;

  int is_neg = (i - 1 >= 0 && line[i - 1] == '-' ? 1 : 0);
  int flag = 0;

  while (line[i] != '(') {
    temp.push_back(line[i] - '0');
    if (line[i] == 'x') flag = 1;
    i++;
  }

  imm = (flag == 0 ? __get_num(temp, 10) : __get_hex(temp));
  temp.clear();

  if (is_neg) imm = __get_inver(imm, 12);

  i = line.find('x', i) + 1;

  while (i < line.size() && line[i] != ')') {
    temp.push_back(line[i] - '0');
    i++;
  }

  r2 = __get_num(temp, 10);
  temp.clear();
}

static void __get_last_num(const std::string &line, size_t i, int32_t &imm,
                           const int n_bits) {
  std::vector<int> temp;
  int flag = 0;
  int is_neg = 0;

  i = line.find_first_not_of(" ,", i);

  if (line[i] == '-') {
    is_neg = 1;
    i++;
  }

  while (i < line.size() && line[i] != ' ' && line[i] != '#') {
    temp.push_back(line[i] - '0');
    if (line[i] == 'x') flag = 1;
    i++;
  }

  imm = (flag == 0 ? __get_num(temp, 10) : __get_hex(temp));
  if (is_neg) imm = __get_inver(imm, n_bits);

  temp.clear();
}

static void __fill_bin(const std::string &format_line, size_t &i,
                       const int start, const int end) {
  for (int j = start; j < end; j++) {
    binary[j] = format_line[i++] - '0';
  }
  i++;
}

static void __fill_bin(int32_t &num, const int start, const int end) {
  for (int j = start; j > end; j--) {
    binary[j] = num & 1;
    num /= 2;
  }
}

static void __i_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rd, rs1, imm;
  size_t size11 = line.size();
  int open_bracket = 0;
  size_t i;

  for (i = 0; i < size11; i++) {
    if (line[i] == '(') {
      open_bracket = 1;
      break;
    }
    if (line[i] == '#') break;
  }

  if (!open_bracket) {
    i = 1;
    __get_reg_num(line, i, rd);
    __get_reg_num(line, i, rs1);
    __get_last_num(line, i, imm, 12);
  } else {
    __get_dst_src(line, rd, imm, rs1);
  }

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(format_line, i, 17, 20);

  __fill_bin(rd, 24, 19);
  __fill_bin(rs1, 16, 11);
  __fill_bin(imm, 11, -1);

  __write_mc(binary, pc);
}

static void __s_type(const int index, const std::string &format_line) {
  int32_t rs1, rs2, imm;
  size_t i;
  std::vector<int> temp;

  __get_dst_src(code[index], rs2, imm, rs1);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(format_line, i, 17, 20);

  __fill_bin(imm, 24, 19);
  __fill_bin(rs1, 16, 11);
  __fill_bin(rs2, 11, 6);
  __fill_bin(imm, 6, -1);

  __write_mc(binary, pc);
}

static void __r_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rd, rs1, rs2;
  size_t i;

  // Start at 1 to get rid of x from xor instruction
  i = 1;
  __get_reg_num(line, i, rd);
  __get_reg_num(line, i, rs1);

  i = line.find('x', i) + 1;

  while (i < line.size() && line[i] != ' ') {
    temp.push_back(line[i] - '0');
    i++;
  }
  rs2 = __get_num(temp, 10);
  temp.clear();

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(format_line, i, 17, 20);
  __fill_bin(format_line, i, 0, 7);

  __fill_bin(rd, 24, 19);
  __fill_bin(rs1, 16, 11);
  __fill_bin(rs2, 11, 6);

  __write_mc(binary, pc);
}

// To get aint32_t the labelss used in Code
static int __get_label(const std::string label, const int ind) {
  int sizelabel = labels.size();

  for (int i = 0; i < sizelabel; i++) {
    if (label.compare(labels[i].s) == 0) {
      return (labels[i].index - ind) * 2;
    }
  }

  return -1;
}

static void __get_label_imm(const int index, size_t &i, int32_t &imm,
                            const int n_bits) {
  const std::string &line = code[index];
  std::string label;

  i = line.find_first_not_of(" ,", i);

  label = line.substr(i, line.find(' ', i));
  imm = __get_label(label, index);

  if (imm < 0) imm = __get_inver(imm, n_bits);
}

static void __uj_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rd, imm;
  size_t i, j;

  i = 0;
  __get_reg_num(line, i, rd);
  __get_label_imm(index, i, imm, 20);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);

  __fill_bin(rd, 24, 19);

  j = 10;
  for (int k = 0; k < 20; k++) {
    if (k <= 9) {
      binary[j--] = imm & 1;
      imm /= 2;
    } else if (k == 10) {
      binary[11] = imm & 1;
      imm /= 2;
      j = 19;
    } else if (k != 19) {
      binary[j--] = imm & 1;
      imm /= 2;
    } else {
      binary[0] = imm & 1;
      imm /= 2;
    }
  }

  __write_mc(binary, pc);
}

static void __u_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  std::string label;
  int32_t rd, imm;
  size_t i = 0;

  __get_reg_num(line, i, rd);
  __get_last_num(line, i, imm, 20);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);

  __fill_bin(rd, 24, 19);
  __fill_bin(imm, 19, -1);

  __write_mc(binary, pc);
}

static void __sb_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  std::string label;
  int32_t rs1, rs2, imm;
  size_t i;

  i = 0;
  __get_reg_num(line, i, rs1);
  __get_reg_num(line, i, rs2);
  __get_label_imm(index, i, imm, 12);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(format_line, i, 17, 20);

  __fill_bin(rs1, 16, 11);
  __fill_bin(rs2, 11, 6);
  __fill_bin(imm, 23, 19);
  __fill_bin(imm, 6, -1);

  binary[24] = imm & 1;
  imm /= 2;

  binary[0] = imm & 1;

  __write_mc(binary, pc);
}

static void __type_number(const std::string ins, const int index,
                          const std::string &format_line) {
  if (ins == "I") __i_type(index, format_line);
  if (ins == "R") __r_type(index, format_line);
  if (ins == "S") __s_type(index, format_line);
  if (ins == "UJ") __uj_type(index, format_line);
  if (ins == "U") __u_type(index, format_line);
  if (ins == "SB") __sb_type(index, format_line);
}

// To extract instruction type and process them independently
static void __process(void) {
  size_t size = code.size();
  std::string instr;

  for (size_t i = 0; i < size; i++) {
    const std::string &line = code[i];
    size_t j = 0;

    j = line.find(' ');
    instr = line.substr(0, j);

    size_t instr_size = instr.size();

    if (instr[instr_size - 1] == ':' && line.size() > instr_size) {
      j = line.find_first_not_of(' ', j);

      instr.clear();
      instr = line.substr(j, line.find(' ', j) - j);
    }

    const size_t n_instructions = formats.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = formats[k];
      std::string type;

      type = format_line.substr(0, format_line.find(' '));

      if (instr.compare(type) == 0) {
        __type_number(format_line.substr(format_line.find_last_of(' ') + 1), i,
                      formats[k]);
        break;
      }
    }
    instr.clear();
  }
}

// To convert Stack Pointer(sp) to x2
static void __preprocess(void) {
  for (size_t i = 0; i < code.size(); i++) {
    std::string &line = code[i];
    size_t instrsize = line.size();

    for (size_t j = 1; j < instrsize; j++) {
      if (line[j - 1] == ' ' && line[j] == 's' && j + 1 < instrsize &&
          line[j + 1] == 'p' && j + 2 < instrsize && line[j + 2] == ' ') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
      if (line[j - 1] == '(' && line[j] == 's' && j + 1 < instrsize &&
          line[j + 1] == 'p' && j + 2 < instrsize && line[j + 2] == ')') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
      if (line[j - 1] == ' ' && line[j] == 's' && j + 1 < instrsize &&
          line[j + 1] == 'p' && j + 2 < instrsize && line[j + 2] == ',') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
      if (line[j - 1] == ',' && line[j] == 's' && j + 1 < instrsize &&
          line[j + 1] == 'p' && j + 2 < instrsize && line[j + 2] == ',') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
    }
  }
}

// To process Load Address(la) psudo command
static void __processla(const int index) {
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

// To process Load Word (lw) psudo command
static void __processlw(const std::string type, const int index,
                        const int32_t pos) {
  const std::string &line = codeinit[index];
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

// To expand all psudo instruction if present
static void __shift(void) {
  int n_code_lines = codeinit.size();

  for (int i = 0; i < n_code_lines; i++) {
    size_t j;
    std::string &line = codeinit[i];
    std::string instr;
    int start;

    std::replace(line.begin(), line.end(), '\t', ' ');

    start = j = line.find_first_not_of(' ');

    while (j < line.size() && line[j] != ' ') {
      instr += line[j++];
      if (j < line.size() && line[j] == ':') {
        instr += line[j++];
        break;
      }
    }

    j = line.find_first_not_of(',', j);

    size_t instr_size = instr.size();

    if (instr[instr_size - 1] == ':' && line.size() > instr_size) {
      instr.clear();

      start = j = line.find_first_not_of(' ', j);
      // instr = line.substr(j, line.find(' ', j) - j);
      while (line[j] != ' ') instr += line[j++];
    }

    if (instr == "la") {
      __processla(i);
      continue;
    } else if (instr == "lw" || instr == "lb" || instr == "lhw") {
      std::string lab;

      j = line.find_last_not_of(' ', line.size() - 1);

      lab = line.substr(line.find_last_of(" ", j) + 1);

      int flag = 1;
      for (j = 0; j < datalabel.size(); j++) {
        if (lab.compare(datalabel[j].name) == 0) {
          flag = 0;
          __processlw(instr, i, datalabel[j].position);
          break;
        }
      }

      if (!flag) continue;
    }

    const size_t n_instructions = formats.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = formats[k];
      std::string type;

      type = format_line.substr(0, format_line.find(' '));

      if (instr.compare(type) == 0) {
        code.push_back(line.substr(start, line.size()));
        break;
      }
    }
  }
}

// To extract all the labels from code
static void __setlabel(void) {
  size_t siz = codeinit.size();
  int count = -1;

  for (size_t i = 0; i < siz; i++) {
    const std::string &line = codeinit[i];
    std::string instr;
    size_t j;

    j = line.find_first_not_of(' ');

    while (j < line.size() && line[j] != ' ') {
      instr += line[j++];

      if (j < line.size() && line[j] == ':') {
        instr += line[j++];
        break;
      }
    }

    size_t instr_size = instr.size();
    if (instr[instr_size - 1] == ':') {
      labels.push_back((lab){instr.substr(0, instr.size() - 1), count + 1});
    }

    if (instr[instr_size - 1] == ':' && instr_size < line.size()) {
      j = line.find_first_not_of(' ', j);
      instr.clear();

      // instr = line.substr(j, line.find(' ', j) - j);
      while (j < line.size() && line[j] != ' ') {
        instr += line[j++];
      }
    }

    if (instr == "la") {
      count += 2;
      continue;
    } else if (instr == "lw" || instr == "lb" || instr == "lhw") {
      std::string lab;

      j = line.find_last_not_of(' ', line.size() - 1);

      lab = line.substr(line.find_last_of(' ', j) + 1, j + 1);

      int flag = 1;

      for (j = 0; j < datalabel.size(); j++) {
        if (lab.compare(datalabel[j].name) == 0) {
          flag = 0;
          count += 2;
          break;
        }
      }

      if (!flag) continue;
    }

    const size_t n_instructions = formats.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = formats[k];
      std::string type;

      type = format_line.substr(0, format_line.find(' '));

      if (instr.compare(type) == 0) {
        count++;
        break;
      }
    }
  }
}

// Driver Code
int main(int argc, char *argv[]) {
  const std::string format_file = "Format.txt";
  const std::string mc_file = "MCode.mc";

  if (argc != 2) {
    std::cerr << "A file Assembly file most be given as the second and only "
                 "parameter."
              << std::endl;
    return 1;
  }

  process_files(argv[1], format_file, mc_file);

  __shift();
  __setlabel();

  __preprocess();
  __process();

  save_mc();
}
