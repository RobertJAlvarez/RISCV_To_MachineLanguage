#include <bits/stdc++.h>

#include <fstream>   // std::ifstream std::ofstream
#include <iostream>  // std::endl
#include <sstream>   // std::stringstream
#include <string>    // std::string

#define START 268435456
#define DATA_MEMO_SIZE (200)
#define ARCH_SIZE (32)
#define MC_FILE ("MCode.mc")

static std::vector<std::string> codeinit;
static std::vector<std::string> code;
static std::vector<std::string> Format;

// Initial Size of Data Memory
std::string datamemory[DATA_MEMO_SIZE];

int32_t pc = 0;
int binary[ARCH_SIZE];

typedef struct {
  std::string s;
  int index;
} lab;

std::vector<lab> Label;

struct seg {
  std::string name;
  int32_t position;
};

std::vector<seg> datalabel;

// If positive, take twos complement of the first n_bits. Else, turn to 0 all
// bits higher than n_bits
static uint32_t __get_inver(int32_t num, const int n_bits) {
  // Create a mask with the first n bits set to 1
  int32_t mask = (n_bits >= 32 ? 0 : 1 << n_bits) - 1;

  // Take the twos complement of the first n_bits
  if (num >= 0) num = (num ^ mask) + 1;

  // If num is negative we truncate it to the first n_bits
  num &= mask;

  return (uint32_t)num;
}

static char __int_to_hex(const int32_t num) {
  if (num <= 9) return static_cast<char>('0' + num);
  return static_cast<char>('A' + (num - 10));
}

// To convert a number in std::string format to its Hexadecimal
static std::string __convert(const std::string s, const int len) {
  std::stringstream ss;
  std::string ans;
  uint32_t num = 0;
  int is_neg = 0;

  // Convert input s into a number
  if (s.find_first_of("xX") == std::string::npos) {
    // If input is not in hex
    if (s[0] == '-') {
      is_neg = 1;
      num = std::atoi(s.substr(1).c_str());
    } else {
      num = std::atoi(s.c_str());
    }
  } else {
    ss << std::hex << s;
    ss >> num;
    is_neg = num < 0;
  }

  if (is_neg) num = __get_inver(num, len * 4);

  // Convert number into hexadecimal number using 8 character.
  // For example, 20 = 00000014
  for (int i = 0; i < len; i++) {
    ans += __int_to_hex(num % 0x10);
    num /= 16;
  }
  reverse(ans.begin(), ans.end());

  return ans;
}

typedef struct {
  std::string name;
  std::string type;
  std::vector<std::string> value;
} datafile;

static int __read_data(void) {
  std::ifstream file;
  std::string word;
  std::vector<datafile> stored;

  int flag;
  int start = 0;

  file.open("test.asm");

  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return 1;
  }

  while (!file.eof()) {
    file >> word;
    if (start == 0) {
      if (word == ".data")  // data part starts
        start = 1;
    } else if (start > 1) {
      continue;
    } else if (start == 1) {
      if (word == ".text")  // data part ends
        start = 2;
      else {
        flag = 0;
        int index;

        for (size_t i = 0; i < word.size() - 1; i++) {
          if (word[i] == ':' && word[i + 1] == '.') {
            flag = 1;
            index = i;
          }
        }

        datafile temp;

        if (flag == 1) {
          std::string nameT = "\0";
          std::string typeT = "\0";
          for (int i = 0; i < index; i++) nameT += word[i];
          for (size_t i = index + 2; i < word.size(); i++) typeT += word[i];
          temp.name = nameT;
          temp.type = typeT;
        } else {
          word.erase(word.end() - 1);
          temp.name = word;
          file >> word;
          word.erase(word.begin());
          temp.type = word;
        }

        getline(file, word);
        std::stringstream ss(word);
        while (ss >> word) temp.value.push_back(word);
        stored.push_back(temp);
      }
    }
  }

  file.close();

  int pos = 0;
  std::string s;

  for (size_t i = 0; i < stored.size(); i++) {
    datalabel.push_back((seg){stored[i].name, pos + START});

    for (size_t j = 0; j < stored[i].value.size(); j++) {
      if (stored[i].type == "byte") {
        datamemory[pos++] = __convert(stored[i].value[j], 2);
      } else if (stored[i].type == "word") {
        // std::cout << "227" << std::endl;
        s = __convert(stored[i].value[j], 8);
        // std::cout << s << std::endl;
        for (int k = 6; k >= 0; k -= 2) {
          datamemory[pos++] = s.substr(k, 2);
        }
      } else if (stored[i].type == "halfword") {
        s = __convert(stored[i].value[j], 4);
        for (int k = 4; k >= 0; k -= 2) {
          datamemory[pos++] = s.substr(k, 2);
        }
      }
    }
  }

  return 0;
}

static void __formats(const std::string filename) {
  std::ifstream myFile;
  std::string line;

  myFile.open(filename);

  while (getline(myFile, line)) Format.push_back(line);

  myFile.close();
}

// Return a numerical value of whose digits are stored in std::vector
static int32_t __get_num(const std::vector<int> temp, const int32_t giv) {
  int32_t num = 1;
  int32_t ans = 0;

  for (int32_t i = temp.size() - 1; i >= 0; i--) {
    ans += num * temp[i];
    num *= giv;
  }

  return ans;
}

// To get numerical value of hexadecimal Format
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

static void __hexa(void) {
  std::ofstream file;
  file.open(MC_FILE, std::ios_base::app);
  file << "0x";
  std::string s;
  int32_t temppc = pc;

  if (temppc == 0) s += '0';

  while (temppc != 0) {
    s += __int_to_hex(temppc % 16);
    temppc /= 16;
  }

  reverse(s.begin(), s.end());
  file << s << " 0x";

  for (int i = 0; i < ARCH_SIZE; i++) {
    std::vector<int> t;
    for (int j = 0; j < 4; j++) t.push_back(binary[i++]);
    i--;

    file << __int_to_hex(__get_num(t, 2));
  }

  file << '\n';
  pc += 4;

  file.close();
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

  __hexa();
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

  __hexa();
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

  __hexa();
}

// To get aint32_t the Labels used in Code
static int __get_label(const std::string label, const int ind) {
  int sizelabel = Label.size();

  for (int i = 0; i < sizelabel; i++) {
    if (label.compare(Label[i].s) == 0) {
      return (Label[i].index - ind) * 2;
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

  __hexa();
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

  __hexa();
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

  __hexa();
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

    const size_t n_instructions = Format.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = Format[k];
      std::string type;

      type = format_line.substr(0, format_line.find(' '));

      if (instr.compare(type) == 0) {
        __type_number(format_line.substr(format_line.find_last_of(' ') + 1), i,
                      Format[k]);
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

  std::cout << '\n';

  for (int i = 0; i < n_code_lines; i++) {
    size_t j;
    std::string &line = codeinit[i];
    for (j = 0; j < line.size(); j++) {
      if (line[j] == '\t') line[j] = ' ';
    }

    std::string instr;
    int start;

    start = j = line.find_first_not_of(' ', 0);

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

    const size_t n_instructions = Format.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = Format[k];
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
      Label.push_back((lab){instr.substr(0, instr.size() - 1), count + 1});
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

    const size_t n_instructions = Format.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = Format[k];
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
int main(void) {
  const std::string s =
      "-------------------------------------------------------";
  std::ifstream myFile;
  std::string line;
  std::ofstream files;
  std::ofstream file;

  for (int i = 0; i < DATA_MEMO_SIZE; i++) datamemory[i] = "00";

  if (__read_data()) return 1;

  files.open(MC_FILE);
  files.close();

  __formats("Format.txt");

  myFile.open("test.asm");
  int flag = 0;

  while (getline(myFile, line)) {
    if (line == ".data") flag = 1;
    if (line == ".text") {
      flag = 0;
      continue;
    }
    if (flag != 1) codeinit.push_back(line);
  }

  __shift();
  __setlabel();

  __preprocess();
  __process();

  myFile.close();
  file.open(MC_FILE, std::ios_base::app);

  file << s << std::endl;

  // Print the Data Memory Part in Increasing Address Order
  for (int i = 0; i < DATA_MEMO_SIZE; i++) {
    file << datamemory[i] << " ";
    if ((i + 1) % 4 == 0) file << std::endl;
  }

  file << s << std::endl;
  file.close();
}
