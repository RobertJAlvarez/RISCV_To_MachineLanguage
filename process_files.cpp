#include <fstream>   // std::ifstream std::ofstream
#include <iostream>  // std::endl, std::cerr
#include <sstream>   // std::stringstream

#include "helper.h"
#include "process_files.h"
#include "pre_process_code.h"

// Initial Size of Data Memory
const int32_t DATA_MEMO_SIZE = 200;
std::string datamemory[DATA_MEMO_SIZE];

// From main.cpp
extern std::vector<std::string> formats;

// Global variable to keep mc_file open as long as possible
static std::ofstream mc_file;

void __write_mc(const int32_t binary[], int32_t &pc) {
  std::string s;
  int32_t temppc = pc;

  if (temppc == 0) s += '0';

  while (temppc != 0) {
    s += __int_to_hex(temppc % 16);
    temppc /= 16;
  }
  reverse(s.begin(), s.end());

  mc_file << "0x" << s << " 0x";

  for (int i = 0; i < ARCH_SIZE; i++) {
    std::vector<int> t;
    for (int j = 0; j < 4; j++) t.push_back(binary[i++]);
    i--;

    mc_file << __int_to_hex(__get_num(t, 2));
  }

  mc_file << '\n';
  pc += 4;
}

static int __formats(const std::string &filename) {
  std::ifstream file;
  std::string line;

  file.open(filename);

  while (getline(file, line)) formats.push_back(line);

  file.close();

  return 0;
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

static void __save_data_entry(const std::string name, const std::string type,
                              const std::vector<std::string> value) {
  static int pos = 0;
  std::string s;

  datalabel.push_back((seg){name, pos + START});

  for (size_t j = 0; j < value.size(); j++) {
    if (type == "byte") {
      datamemory[pos++] = __convert(value[j], 2);
    } else if (type == "word") {
      s = __convert(value[j], 8);
      for (int k = 6; k >= 0; k -= 2) {
        datamemory[pos++] = s.substr(k, 2);
      }
    } else if (type == "halfword") {
      s = __convert(value[j], 4);
      for (int k = 4; k >= 0; k -= 2) {
        datamemory[pos++] = s.substr(k, 2);
      }
    }
  }
}

static void __read_data(std::ifstream &file) {
  std::string word;
  std::string name, type;
  std::vector<std::string> value;
  int index;
  int flag;

  while (!file.eof()) {
    file >> word;
    if (word == ".text") break;

    flag = 0;

    for (size_t i = 0; i < word.size() - 1; i++) {
      if (word[i] == ':' && word[i + 1] == '.') {
        flag = 1;
        index = i;
      }
    }

    if (flag == 1) {
      name = "\0";
      type = "\0";
      for (int i = 0; i < index; i++) name += word[i];
      for (size_t i = index + 2; i < word.size(); i++) type += word[i];
    } else {
      word.erase(word.end() - 1);
      name = word;
      file >> word;
      word.erase(word.begin());
      type = word;
    }

    getline(file, word);

    std::stringstream ss(word);
    while (ss >> word) value.push_back(word);

    __save_data_entry(name, type, value);

    value.clear();
  }
}

static void __read_text(std::ifstream &file) {
  std::string line;
  int flag = 0;

  while (getline(file, line)) {
    if (line == ".data") flag = 1;
    if (line == ".text") {
      flag = 0;
      continue;
    }
    if (flag != 1) {
      std::replace(line.begin(), line.end(), '\t', ' ');
      codeinit.push_back(line);
    }
  }
}

static int __read_assembly_file(const std::string &filename) {
  std::ifstream file;
  std::string word;

  file.open(filename);
  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return 1;
  }

  while (!file.eof()) {
    file >> word;

    if (word == ".data") {
      __read_data(file);
      break;
    }
  }

  file.close();

  file.open(filename);
  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return 1;
  }

  __read_text(file);

  file.close();

  return 0;
}

int process_files(const std::string &assembly_file,
                  const std::string &formats_file,
                  const std::string &mc_filename) {
  for (int i = 0; i < DATA_MEMO_SIZE; i++) datamemory[i] = "00";

  if (__read_assembly_file(assembly_file)) return 1;

  // Clear data inside MC_file
  mc_file.open(mc_filename, std::ofstream::out | std::ofstream::trunc);

  __formats(formats_file);

  return 0;
}

int save_mc(void) {
  const std::string s =
      "-------------------------------------------------------";

  // file.open(MC_file, std::ios_base::app);

  mc_file << s << std::endl;

  // Print the Data Memory Part in Increasing Address Order
  for (int i = 0; i < DATA_MEMO_SIZE; i++) {
    mc_file << datamemory[i] << " ";
    if ((i + 1) % 4 == 0) mc_file << std::endl;
  }

  mc_file << s << std::endl;

  mc_file.close();

  return 0;
}
