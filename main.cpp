#include <iostream>  // std::endl, std::cerr
#include <sstream>   // std::stringstream
#include <string>    // std::stoi()

#include "helper.h"
#include "pre_process_code.h"
#include "process_files.h"

std::vector<std::string> code;
std::vector<std::string> formats;

const int32_t ARCH_SIZE = 32;

static uint32_t pc = 0;
static uint32_t binary = 0;

extern std::vector<lab> labels;

static int32_t __get_num(const std::string &line, size_t &i,
                         const size_t start) {
  i = line.find_first_not_of("1234567890", start);
  return std::stoi(line.substr(start, i - start));
}

static int32_t __get_reg_num(const std::string &line, size_t &i) {
  size_t start = line.find('x', i) + 1;

  return __get_num(line, i, start);
}

/* If positive, take twos complement of the first n_bits. Else, turn to 0 all
 * bits higher than n_bits.
 */
static int32_t __get_inver(int32_t num, const int n_bits) {
  // Create a mask with the first n bits set to 1
  int32_t mask = (n_bits >= 32 ? 0 : 1 << n_bits) - 1;

  // Take the twos complement of the first n_bits
  if (num >= 0) num = (num ^ mask) + 1;

  // If num is negative we truncate it to the first n_bits
  num &= mask;

  return num;
}

static void __get_dst_src(const std::string &line, int32_t &r1, int32_t &imm,
                          int32_t &r2) {
  std::vector<int> temp;
  size_t i = 0;
  int is_neg;

  r1 = __get_reg_num(line, i);

  i = line.find_first_of("1234567890", i);

  is_neg = line[i - 1] == '-';

  imm = __get_num(line, i, i);

  if (is_neg) imm = __get_inver(imm, 12);

  r2 = __get_reg_num(line, i);
}

static int32_t __get_last_num(const std::string &line, size_t i,
                              const int n_bits) {
  std::vector<int> temp;
  int32_t imm;
  int base;
  int is_neg = 0;

  i = line.find_first_not_of(' ', i);

  if (line[i] == '-') {
    is_neg = 1;
    i++;
  }

  base = (line.find("x", i) == std::string::npos ? 10 : 16);
  imm = std::stoi(line.substr(i), 0, base);

  if (is_neg) imm = __get_inver(imm, n_bits);

  temp.clear();

  return imm;
}

static inline void __set_k_bit(uint32_t &n, int k, uint32_t val) {
  n |= (val << k);
}

static void __fill_bin(const std::string &format, const int start,
                       const int end) {
  size_t i = 0;
  for (int j = start; j >= end; j--) {
    __set_k_bit(binary, j, static_cast<uint32_t>(format[i++] - '0'));
  }
  i++;
}

static void __fill_bin(int32_t &num, const int start, const int end) {
  for (int j = start; j < end; j++) {
    __set_k_bit(binary, j, num & 1);
    num /= 2;
  }
}

static inline void __fill_bin_rd(int32_t rd) { __fill_bin(rd, 7, 12); }

static inline void __fill_bin_rs1(int32_t rs1) { __fill_bin(rs1, 15, 20); }

static inline void __fill_bin_rs2(int32_t rs2) { __fill_bin(rs2, 20, 25); }

static void __write_i_instr(int32_t rd, int32_t rs1, int32_t imm) {
  __fill_bin_rd(rd);
  __fill_bin_rs1(rs1);
  __fill_bin(imm, 20, 32);

  __write_mc(binary, pc);
}

static void __i_type(const size_t index) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rd, rs1, imm;
  size_t i;

  if (line.find_first_of('(') != std::string::npos) {
    __get_dst_src(line, rd, imm, rs1);
  } else {
    i = line.find(' ');
    rd = __get_reg_num(line, i);
    rs1 = __get_reg_num(line, i);
    imm = __get_last_num(line, i, 12);
  }

  __write_i_instr(rd, rs1, imm);
}

static void __write_s_instr(int32_t rs1, int32_t rs2, int32_t imm) {
  __fill_bin_rs1(rs1);
  __fill_bin_rs2(rs2);
  __fill_bin(imm, 7, 12);
  __fill_bin(imm, 25, 32);

  __write_mc(binary, pc);
}

static void __s_type(const size_t index) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rs1, rs2, imm;
  size_t i;

  if (line.find_first_of('(') != std::string::npos) {
    __get_dst_src(line, rs2, imm, rs1);
  } else {
    i = line.find(' ');
    rs1 = __get_reg_num(line, i);
    rs2 = __get_reg_num(line, i);
    imm = __get_last_num(line, i, 12);
  }

  __write_s_instr(rs1, rs2, imm);
}

static void __write_r_instr(int32_t rd, int32_t rs1, int32_t rs2) {
  __fill_bin_rd(rd);
  __fill_bin_rs1(rs1);
  __fill_bin_rs2(rs2);

  __write_mc(binary, pc);
}

static void __r_type(const size_t index) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rd, rs1, rs2;
  size_t i;

  i = line.find(' ');
  rd = __get_reg_num(line, i);
  rs1 = __get_reg_num(line, i);
  rs2 = __get_reg_num(line, i);

  __write_r_instr(rd, rs1, rs2);
}

static int32_t __get_label(const std::string label, const int32_t ind) {
  size_t sizelabel = labels.size();

  for (size_t i = 0; i < sizelabel; i++) {
    if (label.compare(labels[i].s) == 0) {
      return (labels[i].index - ind) << 1;
    }
  }

  return -1;
}

static int32_t __get_label_imm(const size_t index, size_t &i,
                               const int n_bits) {
  const std::string &line = code[index];
  std::string label;
  int32_t imm;

  i = line.find_first_not_of(' ', i);

  label = line.substr(i, line.find(' ', i));
  imm = __get_label(label, (int32_t)index);

  if (imm < 0) imm = (int32_t)__get_inver(imm, n_bits);

  return imm;
}

static void __write_uj_instr(int32_t rd, int32_t imm) {
  __fill_bin_rd(rd);
  __fill_bin(imm, 21, 32);
  __fill_bin(imm, 20, 21);
  __fill_bin(imm, 12, 20);
  __fill_bin(imm, 31, 32);

  __write_mc(binary, pc);
}

static void __uj_type(const size_t index) {
  const std::string &line = code[index];
  std::vector<int> temp;
  int32_t rd, imm;
  size_t i;

  i = 0;
  rd = __get_reg_num(line, i);
  imm = __get_label_imm(index, i, 20);

  __write_uj_instr(rd, imm);
}

static void __write_u_instr(int32_t rd, int32_t imm) {
  __fill_bin_rd(rd);
  // imm >>= 12;
  __fill_bin(imm, 12, 32);

  __write_mc(binary, pc);
}

static void __u_type(const size_t index) {
  const std::string &line = code[index];
  std::vector<int> temp;
  std::string label;
  int32_t rd, imm;
  size_t i = 0;

  rd = __get_reg_num(line, i);
  imm = __get_last_num(line, i, 20);

  __write_u_instr(rd, imm);
}

static void __write_sb_instr(int32_t rs1, int32_t rs2, int32_t imm) {
  __fill_bin_rs1(rs1);
  __fill_bin_rs2(rs2);
  __fill_bin(imm, 8, 12);
  __fill_bin(imm, 25, 31);
  __fill_bin(imm, 7, 8);
  __fill_bin(imm, 31, 32);

  __write_mc(binary, pc);
}

static void __sb_type(const size_t index) {
  const std::string &line = code[index];
  std::vector<int> temp;
  std::string label;
  int32_t rs1, rs2, imm;
  size_t i;

  i = 0;
  rs1 = __get_reg_num(line, i);
  rs2 = __get_reg_num(line, i);
  imm = __get_label_imm(index, i, 12);

  __write_sb_instr(rs1, rs2, imm);
}

static void __process_instr(const std::string ins, const size_t index) {
  if (ins == "I")
    __i_type(index);
  else if (ins == "R")
    __r_type(index);
  else if (ins == "S")
    __s_type(index);
  else if (ins == "UJ")
    __uj_type(index);
  else if (ins == "U")
    __u_type(index);
  else if (ins == "SB")
    __sb_type(index);
}

static std::vector<std::string> __tokenize(const std::string &str) {
  std::vector<std::string> tokens;
  std::istringstream iss(str);
  std::string token;

  while (iss >> token) {
    tokens.push_back(token);
  }

  return tokens;
}

static void __set_format_bin(const std::vector<std::string> &tokens) {
  const static struct {
    const int start;
    const int end;
  } ranges[] = {{6, 0}, {14, 12}, {31, 25}};

  for (size_t i = 1; i < tokens.size() - 1; i++)
    __fill_bin(tokens[i], ranges[i - 1].start, ranges[i - 1].end);
}

/* To extract instruction type and process them independently. */
static void __process_code(void) {
  size_t size = code.size();
  std::string instr;

  for (size_t i = 0; i < size; i++) {
    const std::string &line = code[i];
    size_t j = 0;

    j = line.find(' ');
    instr = line.substr(0, j);

    std::string format = __get_instr_format(instr);

    if (!format.empty()) {
      const std::vector<std::string> tokens = __tokenize(format);
      binary = ((uint32_t)0);  // Restart binary code for next instruction
      __set_format_bin(tokens);
      __process_instr(tokens.back(), i);
    }

    instr.clear();
  }
}

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

  pre_process_code();

  __process_code();

  save_mc();

  return 0;
}
