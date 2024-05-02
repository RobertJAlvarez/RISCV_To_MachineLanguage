#include <fstream>     // std::ifstream std::ofstream
#include <functional>  // std::ptr_fun, std::not1
#include <iostream>    // std::endl, std::cerr
#include <sstream>     // std::stringstream

#include "helper.h"
#include "pre_process_code.h"
#include "process_files.h"

// Initial Size of Data Memory
const int32_t DATA_MEMO_SIZE = 200;
static uint8_t datamemory[DATA_MEMO_SIZE];

// From main.cpp
extern std::vector<std::string> formats;

// Global variable to keep mc_file open as long as possible
static std::ofstream mc_file;

static char __int_to_hex(const int num) {
  if (num <= 9) return static_cast<char>('0' + num);
  return static_cast<char>('A' + (num - 10));
}

/* Return a numerical value of whose digits are stored in std::vector */
static int32_t __get_num(const std::vector<int> temp, const int32_t base) {
  int32_t num = 1;
  int32_t ans = 0;

  for (auto digit = temp.rbegin(); digit != temp.rend(); ++digit) {
    ans += num * *digit;
    num *= base;
  }

  return ans;
}

void __write_mc(const int32_t binary[], uint32_t &pc) {
  std::string s;
  uint32_t temppc = pc;

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

static void __save_data_entry(const std::string name, const std::string type,
                              const std::vector<std::string> value) {
  static int pos = 0;
  int32_t num;
  int base, n_bytes;

  datalabel.push_back((seg){name, pos + START});

  for (size_t i = 0; i < value.size(); i++) {
    base = (value[i].find_first_of("x") == std::string::npos ? 10 : 16);
    num = std::stoi(value[i], 0, base);

    if (type == "byte") {
      n_bytes = 1;
    } else if (type == "word") {
      n_bytes = 4;
    } else if (type == "halfword") {
      n_bytes = 2;
    }

    for (int j = 0; j < n_bytes; j++) {
      datamemory[pos++] = (uint8_t) (num & 0xFF);
      num >>= 8;
    }
  }
}

static void __read_data(std::ifstream &file) {
  std::string word;
  std::string name, type;
  std::vector<std::string> value;
  size_t i;

  while (!file.eof()) {
    file >> word;
    if (word == ".text") break;

    // If there is a label
    if ((i = word.find(':')) != std::string::npos) {
      // Get label name
      name = word.substr(0, i);
      // Find if data is .word, .byte, etc.
      if ((i = word.find('.')) != std::string::npos) {
        type = word.substr(i+1);
      } else {
        file >> type;
        type.erase(type.begin());
      }
    }

    // Get data numbers to load
    getline(file, word);
    std::stringstream ss(word);
    while (ss >> word) value.push_back(word);

    // Save data numbers
    __save_data_entry(name, type, value);

    value.clear();
  }
}

// trim from start (in place)
inline void ltrim(std::string &s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
  rtrim(s);
  ltrim(s);
}

static void __read_text(std::ifstream &file) {
  std::string line;
  size_t i;
  int flag = 0;

  while (getline(file, line)) {
    if (line == ".data") flag = 1;
    if (line == ".text") {
      flag = 0;
      continue;
    }

    if (flag != 1) {
      // Replace tabs and comas for spaces
      std::replace(line.begin(), line.end(), '\t', ' ');
      std::replace(line.begin(), line.end(), ',', ' ');

      // Delete everything after a '#'
      if ((i = line.find('#')) != std::string::npos)
        line.erase(line.begin() + (long) i, line.end());

      // Trim lines
      trim(line);

      // Convert to lowercase
      std::transform(line.begin(), line.end(), line.begin(), ::tolower);

      codeinit.push_back(line);
    }
  }
}

static int __read_assembly_file(const std::string &filename) {
  std::ifstream file;
  std::string word;

  // Read data information
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

  // Read text information
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
  for (int i = 0; i < DATA_MEMO_SIZE; i++) datamemory[i] = 0;

  if (__read_assembly_file(assembly_file)) return 1;

  // Clear data inside MC_file
  mc_file.open(mc_filename, std::ofstream::out | std::ofstream::trunc);

  // Read all supported instructions
  __formats(formats_file);

  return 0;
}

int save_mc(void) {
  const std::string s =
      "-------------------------------------------------------";

  mc_file << s << std::endl;

  // Print the Data Memory Part in Increasing Address Order
  for (int i = 0; i < DATA_MEMO_SIZE; i++) {
    mc_file << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(datamemory[i]) << " ";
    if ((i + 1) % 4 == 0) mc_file << std::endl;
  }

  mc_file << s << std::endl;

  mc_file.close();

  return 0;
}
