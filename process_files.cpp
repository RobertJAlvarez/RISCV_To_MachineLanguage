#include <fstream>     // std::ifstream std::ofstream
#include <functional>  // std::ptr_fun, std::not1
#include <iostream>    // std::endl, std::cerr
#include <sstream>     // std::stringstream
#include <vector>

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

void __write_mc(uint32_t binary, uint32_t &pc) {
  std::string s;
  uint32_t temppc = pc;

  if (temppc == 0) s += '0';

  while (temppc != 0) {
    s += __int_to_hex(temppc % 16);
    temppc /= 16;
  }
  reverse(s.begin(), s.end());

  mc_file << "0x" << s << " 0x";

  for (int i = 0; i < 8; i++) {
    mc_file << std::hex << std::uppercase
            << static_cast<int>((binary >> (28 - 4 * i)) & 0xF);
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
      datamemory[pos++] = (uint8_t)(num & 0xFF);
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
        type = word.substr(i + 1);
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

/* Convert ["beqz", "rs1", "imm"] to "beq rs1 x0 imm" */
static std::string __beqz_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "beq " << tokens[1] << " x0 " << tokens[2];
  return ss.str();
}

/* Convert ["bnez", "rs1", "imm"] to "bne rs1 x0 imm" */
static std::string __bnez_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "bne " << tokens[1] << " x0 " << tokens[2];
  return ss.str();
}

/* Convert ["j", "imm"] to "jal x0 imm" */
static std::string __j_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "jal x0 " << tokens[1];
  return ss.str();
}

/* Convert ["jr", "imm"] to "jalr x0 imm" */
static std::string __jr_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "jalr x0 " << tokens[1];
  return ss.str();
}

static std::string __la_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  // TODO
  ss << std::endl;
  return ss.str();
}

/* Convert ["li", "rd", "imm"] to "lui rd imm[31:12]" + "addi rd rd imm[11:0]"*/
static std::string __li_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  const int imm = std::stoi(tokens[2]);
  int low, high;

  // Don't use lui if not needed
  if ((imm < 0x1000) && (imm >= -2048)) {
    ss << "addi " << tokens[1] << " x0 " << imm;
    return ss.str();
  }

  /* Positive case *
   * 1000 = 4096  -> lui 1
   * 17FF = 6143  -> lui 1, addi 2047
   * 1800 = 6144  -> lui 2, addi -2048
   * 1801 = 6145  -> lui 2, addi -2047
   * 2000 = 8192  -> lui 2

   * Negative case *
   * F000 = -4096 -> lui -1
   * E801 = -6143 -> lui -1, addi -2047
   * E800 = -6144 -> lui -1, addi -2048
   * E7FF = -6145 -> lui -2, addi 2047
   * E000 = -8192 -> lui -2
   */
  high = imm >> 12;
  low = imm & 0xFFF;

  if (low >= 0x800) {
    high += 1;
    low -= 0x1000;
  }
  ss << "lui " << tokens[1] << " " << std::to_string(high);

  // if addi is needed, separate instruction with ';'
  if (low != 0)
    ss << ";addi " << tokens[1] << " " << tokens[1] << " "
       << std::to_string(low);

  return ss.str();
}

/* Convert ["mv", "rd", "rs"] to "addi rd rs 0" */
static std::string __mv_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "addi " << tokens[1] << " " << tokens[2] << " 0";
  return ss.str();
}

/* Convert ["neg", "rd", "rs"] to "sub rd x0 rs" */
static std::string __neg_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "sub " << tokens[1] << " x0 " << tokens[2];
  return ss.str();
}

/* Convert ["not", "rd", "rs"] to "xori rd rs -1" */
static std::string __not_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "xori " << tokens[1] << " " << tokens[2] << " -1";
  return ss.str();
}

/* Convert ["seqz", "rd", "rs1"] to "sltiu rd rs1 1" */
static std::string __seqz_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "sltiu " << tokens[1] << " " << tokens[2] << " 1";
  return ss.str();
}

/* Convert ["snez", "rd", "rs1"] to "sltu rd x0 rs1" */
static std::string __snez_line(const std::vector<std::string> tokens) {
  std::stringstream ss;
  ss << "sltu " << tokens[1] << " x0 " << tokens[2];
  return ss.str();
}

static std::string __get_non_pseudo_instr(const std::string &line) {
  typedef enum {
    BEQZ,
    BNEZ,
    J,
    JR,
    LA,
    LI,
    MV,
    NEG,
    NOP,
    NOT,
    RET,
    SEQZ,
    SNEZ,
    NONE
  } instr_enum_t;

  const static struct {
    const std::string instr_str;
    const instr_enum_t instr_enum;
  } conversion[] = {
      {"beqz", BEQZ}, {"bnez", BNEZ}, {"j", J},      {"jr", JR},   {"la", LA},
      {"li", LI},     {"mv", MV},     {"neg", NEG},  {"nop", NOP}, {"not", NOT},
      {"ret", RET},   {"seqz", SEQZ}, {"snez", SNEZ}};

  std::vector<std::string> tokens;
  std::string new_line;

  const std::string instr_str = line.substr(0, line.find(" "));
  instr_enum_t instr_enum = NONE;

  // Convert string instruction to enum instruction
  for (size_t i = 0; i < sizeof(conversion) / sizeof(conversion[0]); i++) {
    if (instr_str.compare(conversion[i].instr_str) == 0) {
      instr_enum = conversion[i].instr_enum;
      break;
    }
  }

  if (instr_enum == NONE) return line;

  tokens = tokenize(line);

  switch (instr_enum) {
    case BEQZ:
      new_line = __beqz_line(tokens);
      break;
    case BNEZ:
      new_line = __bnez_line(tokens);
      break;
    case J:
      new_line = __j_line(tokens);
      break;
    case JR:
      new_line = __jr_line(tokens);
      break;
    case LA:
      // TODO: new_line = __la_line(tokens);
      new_line = line;
      break;
    case LI:
      new_line = __li_line(tokens);
      break;
    case MV:
      new_line = __mv_line(tokens);
      break;
    case NEG:
      new_line = __neg_line(tokens);
      break;
    case NOP:
      new_line = "addi x0 x0 0";
      break;
    case NOT:
      new_line = __not_line(tokens);
      break;
    case RET:
      new_line = "jalr x0 x1 0";
      break;
    case SEQZ:
      new_line = __seqz_line(tokens);
      break;
    case SNEZ:
      new_line = __snez_line(tokens);
      break;
    default:
      break;
  }

  return new_line;
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
        line.erase(line.begin() + (long)i, line.end());

      // Trim lines
      trim(line);

      // Convert to lowercase
      std::transform(line.begin(), line.end(), line.begin(), ::tolower);

      // Convert pseudo-instruction to instructions
      line = __get_non_pseudo_instr(line);

      for (const std::string &t : tokenize(line, ';')) codeinit.push_back(t);
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

  // for (auto t : codeinit) std::cout << t << std::endl;

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
    mc_file << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(datamemory[i]) << " ";
    if ((i + 1) % 4 == 0) mc_file << std::endl;
  }

  mc_file << s << std::endl;

  mc_file.close();

  return 0;
}
