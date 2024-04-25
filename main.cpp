#include <bits/stdc++.h>

#include <fstream>   // std::ifstream std::ofstream
#include <iostream>  // std::endl
#include <sstream>   // std::stringstream
#include <string>    // std::string

#define ll_t long long int
#define START 268435456
#define DATA_MEMO_SIZE (4000)
#define ARCH_SIZE (32)
#define MC_FILE ("MCode.mc")

static std::vector<std::string> codeinit;
static std::vector<std::string> code;
static std::vector<std::string> Format;

// Initial Size of Data Memory
std::string datamemory[DATA_MEMO_SIZE];

ll_t pc = 0;
size_t size;
int binary[ARCH_SIZE];

typedef struct {
  std::string s;
  int index;
} lab;

std::vector<lab> Label;

struct seg {
  std::string name;
  ll_t position;
};

std::vector<seg> datalabel;

// To get 2's Complement Representation for Immediate Values
static ll_t __get_inver(ll_t imme, const int bit) {
  std::vector<int> bb;
  imme = -imme;
  int i, j;

  for (i = 0; i < bit; i++) {
    bb.push_back(imme & 1);
    imme /= 2;
  }

  reverse(bb.begin(), bb.end());
  for (i = 0; i < bit; i++) bb[i] = !bb[i];

  j = bit - 1;

  while (bb[j] != 0) {
    bb[j] = 0;
    j--;
  }
  bb[j] = 1;

  ll_t num = 0;
  ll_t mul = 1;
  for (i = bit - 1; i >= 0; i--) {
    num += bb[i] * mul;
    mul *= 2;
  }

  return num;
}

// To convert a number in std::string format to its Hexadecimal
static std::string __convert(const std::string s, const int len) {
  size_t length = s.size();
  int flag = 1;

  for (size_t i = 0; i < length; i++) {
    if (s[i] == 'x' || s[i] == 'X') {
      flag = 0;
      break;
    }
  }

  std::string ans;
  ll_t num = 0;

  if (flag) {
    ll_t mul = 1;
    int last = 0;
    int is_neg = 0;

    if (s[0] == '-') {
      is_neg = 1;
      last = 1;
    }

    for (int i = length - 1; i >= last; i--) {
      num += (s[i] - '0') * mul;
      mul *= 10;
    }

    if (is_neg) num = __get_inver(num, len * 4);
  } else {
    ll_t x;
    std::stringstream ss;
    ss << std::hex << s;
    ss >> x;
    num = x;
    if (num < 0) num = __get_inver(num, len * 4);
  }

  for (int i = 0; i < len; i++) {
    ll_t rem = num % 16;
    ans += (rem <= 9 ? (rem + '0') : (rem - 10 + 65));
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

static void __read_data(void) {
  std::ifstream file;
  std::string word;
  std::vector<datafile> stored;

  int flag;
  int start = 0;

  file.open("test.asm");

  if (file.is_open()) {
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
  }

  file.close();

  int pos = 0;
  std::string s;

  for (size_t i = 0; i < stored.size(); i++) {
    seg t = {stored[i].name, pos + START};
    datalabel.push_back(std::move(t));

    for (size_t j = 0; j < stored[i].value.size(); j++) {
      if (stored[i].type == "byte") {
        datamemory[pos++] = __convert(stored[i].value[j], 2);
      } else if (stored[i].type == "word") {
        s = __convert(stored[i].value[j], 8);
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
}

static void __formats(const std::string filename) {
  std::ifstream myFile;
  std::string line;

  myFile.open(filename);

  while (getline(myFile, line)) Format.push_back(line);

  myFile.close();
}

static char h(const ll_t ind) {
  if (ind <= 9) return ind + '0';
  return ind - 10 + 65;
}

// Return a numerical value of whose digits are stored in std::vector
static ll_t __get_num(const std::vector<int> temp, const ll_t giv) {
  ll_t num = 1;
  ll_t ans = 0;

  for (ll_t i = temp.size() - 1; i >= 0; i--) {
    ans += num * temp[i];
    num *= giv;
  }

  return ans;
}

// To get numerical value of hexadecimal Format
static ll_t __get_hex(std::vector<int> temp) {
  ll_t num = 1;
  ll_t ans = 0;

  for (ll_t i = temp.size() - 1; i > 1; i--) {
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

static void __hexa(void) {
  std::ofstream file;
  file.open(MC_FILE, std::ios_base::app);
  file << "0x";
  std::string s;
  ll_t temppc = pc;

  if (temppc == 0) s += '0';

  while (temppc != 0) {
    char e = h(temppc % 16);
    temppc /= 16;
    s += e;
  }

  reverse(s.begin(), s.end());
  file << s << " ";
  file << "0x";

  for (int i = 0; i < ARCH_SIZE; i++) {
    std::vector<int> t;
    for (int j = 0; j < 4; j++) t.push_back(binary[i++]);

    file << h(__get_num(t, 2));
    i--;
  }

  file << '\n';
  pc += 4;
}

static void __get_reg_num(const std::string &line, size_t &i, ll_t &reg) {
  std::vector<int> temp;

  i = line.find('x', i) + 1;

  while (line[i] != ' ' && line[i] != ',') {
    temp.push_back(line[i] - '0');
    i++;
  }
  reg = __get_num(temp, 10);
  temp.clear();
}

static void __get_dst_src(const std::string &line, ll_t &r1, ll_t &imm,
                          ll_t &r2) {
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

static void __get_last_num(const std::string &line, size_t i, ll_t &imm) {
  std::vector<int> temp;
  int flag = 0;
  int is_neg = 0;

  while (line[i] == ' ' || line[i] == ',') i++;

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
  if (is_neg) imm = __get_inver(imm, 12);

  temp.clear();
}

static void __fill_bin(const std::string &format_line, size_t &i, const int start, const int end) {
  for (int j = start; j < end; j++) {
    binary[j] = format_line[i++] - '0';
  }
  i++;
}

static void __fill_bin(ll_t reg, const int start, const int end) {
  for (int j = start; j > end; j--) {
    binary[j] = reg & 1;
    reg /= 2;
  }
}

static void __i_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  ll_t rd, rs1, imme;
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
    __get_last_num(line, i, imme);
  } else {
    __get_dst_src(line, rd, imme, rs1);
  }

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(rd,24,19);
  __fill_bin(format_line, i, 17, 20);
  __fill_bin(rs1,16,11);
  __fill_bin(imme, 11, -1);

  __hexa();
}

static void __s_type(const int index, const std::string &format_line) {
  ll_t rs1, rs2, imme;
  size_t i;
  std::vector<int> temp;

  __get_dst_src(code[index], rs2, imme, rs1);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(imme, 24, 19);

  int j;

  // __fill_bin(imme, 24, 19);

  for (j = 24; j > 19; j--) {
    binary[j] = imme & 1;
    imme /= 2;
  }

  __fill_bin(format_line, i, 17, 20);
  __fill_bin(rs1,16,11);
  __fill_bin(rs2, 11, 6);
  __fill_bin(imme, 6, -1);

  __hexa();
}

static void __r_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  ll_t rd, rs1, rs2;
  size_t i;

  // Start at 1 to get rid of x from xor instruction
  i = 1;
  __get_reg_num(line, i, rd);
  __get_reg_num(line, i, rs1);

  i = line.find('x', i) + 1;

  while (i < line.size() && line[i] != ' ' && line[i] != '#') {
    temp.push_back(line[i] - '0');
    i++;
  }
  rs2 = __get_num(temp, 10);
  temp.clear();

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(rd,24,19);
  __fill_bin(format_line, i, 17, 20);
  __fill_bin(rs1,16,11);
  __fill_bin(rs2, 11, 6);
  __fill_bin(format_line, i, 0, 7);

  __hexa();
}

// To get all_t the Labels used in Code
static int __get_label(const std::string label, const int ind) {
  int sizelabel = Label.size();

  for (int i = 0; i < sizelabel; i++) {
    if (label.compare(Label[i].s) == 0) {
      return (Label[i].index - ind) * 2;
    }
  }

  return -1;
}

static void __get_label_imm(const int index, size_t &i, ll_t &imme,
                            const int n_bits) {
  const std::string &line = code[index];
  std::string label;

  while (line[i] == ' ' || line[i] == ',') i++;

  while (i < line.size() && line[i] != ' ' && line[i] != '#') {
    label += line[i];
    i++;
  }
  imme = __get_label(label, index);
  if (imme < 0) imme = __get_inver(imme, n_bits);
}

static void __uj_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  ll_t rd, imme;
  size_t i, j;

  i = 0;
  __get_reg_num(line, i, rd);
  __get_label_imm(index, i, imme, 20);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(rd,24,19);

  j = 10;
  for (int k = 0; k < 20; k++) {
    if (k <= 9) {
      binary[j--] = imme & 1;
      imme /= 2;
    } else if (k == 10) {
      binary[11] = imme & 1;
      imme /= 2;
      j = 19;
    } else if (k != 19) {
      binary[j--] = imme & 1;
      imme /= 2;
    } else {
      binary[0] = imme & 1;
      imme /= 2;
    }
  }

  __hexa();
}

static void __u_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  std::string label;
  ll_t rd, imme;
  size_t i = 0;

  __get_reg_num(line, i, rd);

  while (line[i] == ' ' || line[i] == ',') i++;

  int flag = 0;
  int is_neg = 0;

  if (line[i] == '-') {
    is_neg = 1;
    i++;
  }

  while (i < line.size() && line[i] != ' ' && line[i] != '#') {
    temp.push_back(line[i] - '0');
    if (line[i] == 'x') flag = 1;
    i++;
  }

  imme = (flag == 0 ? __get_num(temp, 10) : __get_hex(temp));
  temp.clear();
  if (is_neg) imme = __get_inver(imme, 20);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(rd,24,19);
  __fill_bin(imme, 19, -1);

  __hexa();
}

static void __sb_type(const int index, const std::string &format_line) {
  const std::string &line = code[index];
  std::vector<int> temp;
  std::string label;
  ll_t rs1, rs2, imme;
  size_t i;

  i = 0;
  __get_reg_num(line, i, rs1);
  __get_reg_num(line, i, rs2);
  __get_label_imm(index, i, imme, 12);

  i = format_line.find(' ') + 1;

  __fill_bin(format_line, i, 25, ARCH_SIZE);
  __fill_bin(format_line, i, 17, 20);
  __fill_bin(rs1,16,11);
  __fill_bin(rs2, 11, 6);

  for (int j = 23; j > 19; j--) {
    binary[j] = imme & 1;
    imme /= 2;
  }

  for (int j = 6; j >= 0; j--) {
    binary[j] = imme & 1;
    imme /= 2;
  }

  binary[24] = imme & 1;
  imme /= 2;

  binary[0] = imme & 1;

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
  size = code.size();
  for (size_t i = 0; i < size; i++) {
    const std::string &line = code[i];
    std::string ins;
    size_t j = 0;

    while (j < line.size() && line[j] != ' ') {
      ins += line[j];
      j++;
    }

    size_t sins = ins.size();
    if (ins[sins - 1] == ':' && line.size() > sins) {
      ins.clear();
      while (j < line.size() && line[j] == ' ') j++;
      while (j < line.size() && line[j] != ' ') ins += line[j++];
    }

    const size_t n_instructions = Format.size();
    for (size_t k = 0; k < n_instructions; k++) {
      std::string type;
      const std::string &format_line = Format[k];
      int k1 = 0;

      while (format_line[k1] != ' ') {
        type += format_line[k1];
        k1++;
      }

      if (ins.compare(type) == 0) {
        k1 = format_line.size() - 1;
        std::string type1;
        while (format_line[k1] != ' ') {
          type1 += (format_line[k1]);
          k1--;
        }
        reverse(type1.begin(), type1.end());
        __type_number(type1, i, Format[k]);
        break;
      }
    }
  }
}

// To convert Stack Pointer(sp) to x2
static void __preprocess(void) {
  for (size_t i = 0; i < code.size(); i++) {
    std::string &line = code[i];
    size_t inssize = line.size();

    for (size_t j = 1; j < inssize; j++) {
      if (line[j - 1] == ' ' && line[j] == 's' && j + 1 < inssize &&
          line[j + 1] == 'p' && j + 2 < inssize && line[j + 2] == ' ') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
      if (line[j - 1] == '(' && line[j] == 's' && j + 1 < inssize &&
          line[j + 1] == 'p' && j + 2 < inssize && line[j + 2] == ')') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
      if (line[j - 1] == ' ' && line[j] == 's' && j + 1 < inssize &&
          line[j + 1] == 'p' && j + 2 < inssize && line[j + 2] == ',') {
        line[j] = 'x';
        line[j + 1] = '2';
      }
      if (line[j - 1] == ',' && line[j] == 's' && j + 1 < inssize &&
          line[j + 1] == 'p' && j + 2 < inssize && line[j + 2] == ',') {
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
  ll_t currentpc, labeladdress;
  size_t i = 0;

  i = line.find('x') + 1;

  while (line[i] != ' ') s += line[i++];

  code.push_back("auipc x" + s + " 65536");
  currentpc = (code.size() * 4 - 4) + START;

  i = line.find(' ', i);

  while (i < line.size() && line[i] != ' ') {
    labeltype += line[i++];
  }
  labeladdress = 0;

  for (size_t j = 0; j < datalabel.size(); j++) {
    if (labeltype.compare(datalabel[j].name) == 0) {
      labeladdress = datalabel[j].position;
      break;
    }
  }

  ll_t temp1 = abs(labeladdress - currentpc);

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
                        const ll_t pos) {
  const std::string &line = codeinit[index];
  std::string s, ins, labeladd;
  ll_t currentpc;
  int i;

  i = line.find('x') + 1;

  while (line[i] != ' ') s += line[i++];

  currentpc = pos - (code.size() * 4 + START);
  code.push_back("auipc x" + s + " 65536");
  ll_t temp1 = abs(currentpc);

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

    std::string ins;
    j = 0;
    int start = -1;

    while (j < line.size() && line[j] == ' ') j++;

    start = j;
    while (j < line.size() && line[j] != ' ') {
      ins += line[j];
      j++;
      if (j < line.size() && line[j] == ':') {
        ins += line[j];
        j++;
        break;
      }
    }

    while (j < line.size() && line[j] == ' ') j++;

    size_t sins = ins.size();

    if (ins[sins - 1] == ':' && line.size() > sins) {
      ins.clear();
      while (j < line.size() && line[j] == ' ') j++;
      start = j;
      while (line[j] != ' ') ins += line[j++];
    }

    if (ins == "la") {
      __processla(i);
      continue;
    } else if (ins == "lw" || ins == "lb" || ins == "lhw") {
      std::string lab;

      j = line.find_last_not_of(' ', line.size() - 1);

      while (line[j] != ' ') {
        lab += line[j];
        j--;
      }

      reverse(lab.begin(), lab.end());

      int flag = 1;
      for (j = 0; j < datalabel.size(); j++) {
        if (lab.compare(datalabel[j].name) == 0) {
          flag = 0;
          __processlw(ins, i, datalabel[j].position);
          break;
        }
      }

      if (!flag) continue;
    }

    const size_t n_instructions = Format.size();
    for (size_t k = 0; k < n_instructions; k++) {
      const std::string &format_line = Format[k];
      std::string type;
      int k1 = 0;

      while (format_line[k1] != ' ') {
        type += format_line[k1];
        k1++;
      }

      if (ins.compare(type) == 0) {
        std::string add;
        for (size_t l = start; l < line.size(); l++) add += line[l];

        code.push_back(add);
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
    std::string ins;
    size_t j;

    for (j = 0; j < line.size() && line[j] == ' '; j++)
      ;

    while (j < line.size() && line[j] != ' ') {
      ins += line[j];
      j++;
      if (j < line.size() && line[j] == ':') {
        ins += line[j];
        j++;
        break;
      }
    }

    size_t sins = ins.size();
    if (ins[sins - 1] == ':') {
      std::string ins1;
      for (size_t k = 0; k < sins - 1; k++) ins1 += ins[k];
      lab temp;
      temp.s = ins1;
      temp.index = count + 1;
      Label.push_back(temp);
    }

    if (ins[sins - 1] == ':' && sins < line.size()) {
      while (j < line.size() && line[j] == ' ') j++;

      ins.clear();
      while (j < line.size() && line[j] != ' ') {
        ins += line[j];
        j++;
      }
    }

    if (ins == "la") {
      count += 2;
      continue;
    } else if (ins == "lw" || ins == "lb" || ins == "lhw") {
      std::string lab;

      j = line.find_last_not_of(' ', line.size() - 1);

      while (line[j] != ' ') {
        lab += line[j];
        j--;
      }
      reverse(lab.begin(), lab.end());
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
      int k1 = 0;

      while (format_line[k1] != ' ') {
        type += format_line[k1];
        k1++;
      }

      if (ins.compare(type) == 0) {
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
  __read_data();

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
  for (int i = 0; i < 400; i++) {
    file << datamemory[i] << " ";
    if ((i + 1) % 4 == 0) file << std::endl;
  }

  file << s << std::endl;
  file.close();
}
