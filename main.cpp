#include <bits/stdc++.h>

#include <fstream>   // std::ifstream std::ofstream
#include <iostream>  // std::endl
#include <sstream>   // std::stringstream
#include <string>    // std::string

#define ll long long int
#define START 268435456

std::vector<std::string> codeinit;
std::vector<std::string> code;
std::vector<std::string> Format;
size_t sizeI, size;
int binary[32];
ll pccount = 0;

typedef struct {
  std::string s;
  int index;
} lab;

std::vector<lab> Label;

// Initial Size of Data Memory is Fixed 4000 Bytes
std::string datamemory[4000];

struct datafile {
  std::string name;
  std::string type;
  std::vector<std::string> value;
};

struct seg {
  std::string name;
  ll position;
};

// To get 2's Complement Representation for Immediate Values
ll getinver(ll imme, const int bit) {
  std::vector<int> bb;
  imme = -imme;
  int i, j;

  for (i = 0; i < bit; i++) {
    bb.push_back(imme % 2);
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

  ll num = 0;
  ll mul = 1;
  for (i = bit - 1; i >= 0; i--) {
    num += bb[i] * mul;
    mul *= 2;
  }

  return num;
}

std::vector<seg> datalabel;

// To convert a number in std::string format to its Hexadecimal
std::string convert(const std::string s, const int len) {
  size_t length = s.size();
  int flag = 1;

  for (size_t i = 0; i < length; i++) {
    if (s[i] == 'x' || s[i] == 'X') {
      flag = 0;
      break;
    }
  }

  std::string ans;
  ll num = 0;

  if (flag) {
    ll mul = 1;
    int last = 0;
    int flag1 = 0;

    if (s[0] == '-') flag1 = 1;
    if (flag1) last = 1;
    for (int i = length - 1; i >= last; i--) {
      num += (s[i] - 48) * mul;
      mul *= 10;
    }

    if (flag1) num = getinver(num, len * 4);
  } else {
    ll x;
    std::stringstream ss;
    ss << std::hex << s;
    ss >> x;
    num = x;
    if (num < 0) num = getinver(num, len * 4);
  }

  for (int i = 0; i < len; i++) {
    ll rem = num % 16;
    ans += (rem <= 9 ? (rem + 48) : (rem - 10 + 65));
    num /= 16;
  }
  reverse(ans.begin(), ans.end());

  return ans;
}

void read_data(void) {
  std::ifstream file;
  std::string word;
  std::vector<datafile> stored;
  file.open("test.asm");
  int flag;
  int start = 0;

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
          datafile temp;
          flag = 0;
          int index;

          for (size_t i = 0; i < word.size() - 1; i++) {
            if (word[i] == ':' && word[i + 1] == '.') {
              flag = 1;
              index = i;
            }
          }

          if (flag == 1) {
            std::string nameT = "\0", typeT = "\0";
            for (int i = 0; i < index; i++) nameT += word[i];
            for (size_t i = index + 2; i < word.size(); i++) typeT += word[i];
            temp.name = nameT;
            temp.type = typeT;
            getline(file, word);
            std::stringstream ss(word);
            while (ss >> word) {
              temp.value.push_back(word);
            }
          } else {
            word.erase(word.end() - 1);
            temp.name = word;
            file >> word;
            word.erase(word.begin());
            temp.type = word;
            getline(file, word);
            std::stringstream ss(word);
            while (ss >> word) {
              temp.value.push_back(word);
            }
          }
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
        datamemory[pos++] = convert(stored[i].value[j], 2);
      } else if (stored[i].type == "word") {
        s = convert(stored[i].value[j], 8);
        for (int k = 6; k >= 0; k -= 2) {
          datamemory[pos++] = s.substr(k, 2);
        }
      } else if (stored[i].type == "halfword") {
        s = convert(stored[i].value[j], 4);
        for (int k = 4; k >= 0; k -= 2) {
          datamemory[pos++] = s.substr(k, 2);
        }
      }
    }
  }
}

void formats(void) {
  std::ifstream myFile;
  myFile.open("Format.txt");
  std::string line;

  while (getline(myFile, line)) Format.push_back(line);

  sizeI = Format.size();
  myFile.close();
}

char h(const ll ind) {
  if (ind <= 9) return ind + 48;
  return ind - 10 + 65;
}

// Return a numerical value of whose digits are stored in std::vector
ll getnum(const std::vector<int> temp, const ll giv) {
  ll num = 1;
  ll ans = 0;

  for (ll i = temp.size() - 1; i >= 0; i--) {
    ans += num * temp[i];
    num *= giv;
  }

  return ans;
}

// To get numerical value of hexadecimal Format
ll gethex(std::vector<int> temp) {
  ll num = 1;
  ll ans = 0;

  for (ll i = temp.size() - 1; i > 1; i--) {
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

void hexa(void) {
  std::ofstream file;
  file.open("MCode.mc", std::ios_base::app);
  file << "0x";
  std::string s;
  ll temppc = pccount;

  if (temppc == 0) s += '0';

  while (temppc != 0) {
    char e = h(temppc % 16);
    temppc /= 16;
    s += e;
  }

  reverse(s.begin(), s.end());
  file << s << " ";
  file << "0x";

  for (int i = 0; i < 32; i++) {
    std::vector<int> t;
    for (int j = 0; j < 4; j++) t.push_back(binary[i++]);

    char a = h(getnum(t, 2));
    file << a;
    i--;
  }

  file << "\n";
  pccount += 4;
}

// To get all the Labels used in Code
int getlab(const std::string label, const int ind) {
  int sizelabel = Label.size();

  for (int i = 0; i < sizelabel; i++) {
    if (label.compare(Label[i].s) == 0) {
      return (Label[i].index - ind) * 2;
    }
  }

  return -1;
}

void IFunction(const int index, const int index1) {
  int size11 = code[index].size();
  int open_bracket = 0;

  for (int i = 0; i < size11; i++) {
    if (code[index][i] == '(') {
      open_bracket = 1;
      break;
    }
    if (code[index][i] == '#') {
      break;
    }
  }

  ll rd, rs1, imme;
  size_t i = 0;
  std::vector<int> temp;

  if (!open_bracket) {
    i++;
    while (code[index][i] != 'x') i++;

    for (i++; code[index][i] != ' ' && code[index][i] != ','; i++)
      temp.push_back(code[index][i] - 48);

    rd = getnum(temp, 10);
    temp.clear();
    while (code[index][i] != 'x') i++;

    for (i++; code[index][i] != ' ' && code[index][i] != ','; i++)
      temp.push_back(code[index][i] - 48);

    rs1 = getnum(temp, 10);
    temp.clear();

    while (code[index][i] == ' ' || code[index][i] == ',') i++;

    int flag1 = 0;
    if (code[index][i] == '-') {
      flag1 = 1;
      i++;
    }

    int flag = 0;
    while (i < code[index].size() && code[index][i] != ' ' &&
           code[index][i] != '#' && code[index][i] != ',') {
      temp.push_back(code[index][i] - 48);
      if (code[index][i] == 'x') flag = 1;
      i++;
    }

    imme = (flag == 0 ? getnum(temp, 10) : gethex(temp));

    if (flag1) imme = getinver(imme, 12);
  } else {
    while (code[index][i] != 'x') i++;

    i++;
    while (code[index][i] != ' ' && code[index][i] != ',') {
      temp.push_back(code[index][i] - 48);
      i++;
    }

    rd = getnum(temp, 10);
    temp.clear();
    while (!(code[index][i] >= 48 && code[index][i] <= 57)) i++;

    int flag1 = 0;
    if (code[index][i - 1] == '-') flag1 = 1;

    int flag = 0;
    while (code[index][i] != '(') {
      temp.push_back(code[index][i] - 48);
      if (code[index][i] == 'x') flag = 1;
      i++;
    }

    imme = (flag == 0 ? getnum(temp, 10) : gethex(temp));

    temp.clear();
    if (flag1) imme = getinver(imme, 12);
    while (code[index][i] != 'x') i++;

    i++;
    while (i < code[index].size() && code[index][i] != ')') {
      temp.push_back(code[index][i] - 48);
      i++;
    }
    rs1 = getnum(temp, 10);
  }

  temp.clear();
  i = 0;
  while (Format[index1][i] != ' ') i++;

  i++;
  int j;
  for (j = 25; j <= 31; j++) binary[j] = Format[index1][i++] - 48;

  i++;
  j = 24;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rd % 2;
    rd /= 2;
  }

  for (j = 17; j <= 19; j++) binary[j] = Format[index1][i++] - 48;

  i++;
  j = 16;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rs1 % 2;
    rs1 /= 2;
  }

  for (int k = 0; k < 12; k++) {
    binary[j--] = imme % 2;
    imme /= 2;
  }

  hexa();
}

void SFunction(const int index, const int index1) {
  ll rs1, rs2, imme;
  size_t i = 0;
  std::vector<int> temp;

  while (code[index][i] != 'x') i++;

  i++;
  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rs2 = getnum(temp, 10);
  temp.clear();

  while (!(code[index][i] >= 48 && code[index][i] <= 57)) i++;

  int flag1 = (i - 1 >= 0 && code[index][i - 1] == '-' ? 1 : 0);

  int flag = 0;
  while (code[index][i] != '(') {
    temp.push_back(code[index][i] - 48);
    if (code[index][i] == 'x') flag = 1;
    i++;
  }

  imme = (flag == 0 ? getnum(temp, 10) : gethex(temp));

  if (flag1) imme = getinver(imme, 12);

  temp.clear();
  while (code[index][i] != 'x') i++;

  i++;
  while (i < code[index].size() && code[index][i] != ')') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rs1 = getnum(temp, 10);
  temp.clear();

  i = 0;
  while (Format[index1][i] != ' ') i++;

  i++;
  int j;
  for (j = 25; j <= 31; j++) binary[j] = Format[index1][i++] - 48;
  i++;

  j = 24;
  for (int k = 0; k < 5; k++) {
    binary[j--] = imme % 2;
    imme /= 2;
  }

  for (j = 17; j <= 19; j++) {
    binary[j] = Format[index1][i++] - 48;
  }
  i++;

  j = 16;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rs1 % 2;
    rs1 /= 2;
  }

  for (int k = 0; k < 5; k++) {
    binary[j--] = rs2 % 2;
    rs2 /= 2;
  }

  for (j = 6; j >= 0; j--) {
    binary[j] = imme % 2;
    imme /= 2;
  }

  hexa();
}

void RFunction(const int index, const int index1) {
  ll rd, rs1, rs2;
  size_t i = 0;
  std::vector<int> temp;

  i++;  // To get rid of x from xor instruction
  while (code[index][i] != 'x') i++;

  i++;

  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rd = getnum(temp, 10);
  temp.clear();

  while (code[index][i] != 'x') i++;

  i++;
  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rs1 = getnum(temp, 10);
  temp.clear();

  while (code[index][i] != 'x') i++;

  i++;
  while (i < code[index].size() && code[index][i] != ' ' &&
         code[index][i] != '#' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rs2 = getnum(temp, 10);
  temp.clear();

  i = 0;
  while (Format[index1][i] != ' ') i++;

  i++;
  int j;
  for (j = 25; j <= 31; j++) binary[j] = Format[index1][i++] - 48;
  i++;

  j = 24;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rd % 2;
    rd /= 2;
  }

  for (j = 17; j <= 19; j++) binary[j] = Format[index1][i++] - 48;

  i++;
  j = 16;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rs1 % 2;
    rs1 /= 2;
  }

  for (int k = 0; k < 5; k++) {
    binary[j--] = rs2 % 2;
    rs2 /= 2;
  }

  for (j = 0; j <= 6; j++) {
    binary[j] = Format[index1][i++] - 48;
  }

  hexa();
}

void UJFunction(const int index, const int index1) {
  ll rd, imme;
  size_t i = 0;
  std::string label;

  while (code[index][i] != 'x') {
    i++;
  }
  i++;

  std::vector<int> temp;
  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rd = getnum(temp, 10);
  temp.clear();

  while (code[index][i] == ' ' || code[index][i] == ',') {
    i++;
  }

  while (i < code[index].size() && code[index][i] != ' ' &&
         code[index][i] != '#') {
    label += code[index][i];
    i++;
  }
  imme = getlab(label, index);
  if (imme < 0) {
    imme = getinver(imme, 20);
  }

  i = 0;
  while (Format[index1][i] != ' ') {
    i++;
  }
  i++;

  int j;
  for (j = 25; j <= 31; j++) binary[j] = Format[index1][i++] - 48;
  i++;

  j = 24;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rd % 2;
    rd /= 2;
  }

  j = 10;
  for (int k = 0; k < 20; k++) {
    if (k <= 9) {
      binary[j--] = imme % 2;
      imme /= 2;
    } else if (k == 10) {
      binary[11] = imme % 2;
      imme /= 2;
      j = 19;
    } else if (k != 19) {
      binary[j--] = imme % 2;
      imme /= 2;
    } else {
      binary[0] = imme % 2;
      imme /= 2;
    }
  }
  hexa();
}

void UFunction(const int index, const int index1) {
  ll rd, imme;
  size_t i = 0;
  std::string label;

  while (code[index][i] != 'x') {
    i++;
  }
  i++;

  std::vector<int> temp;
  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rd = getnum(temp, 10);
  temp.clear();

  while (code[index][i] == ' ' || code[index][i] == ',') {
    i++;
  }

  int flag = 0;
  int flag1 = 0;

  if (code[index][i] == '-') {
    flag1 = 1;
    i++;
  }

  while (i < code[index].size() && code[index][i] != ' ' &&
         code[index][i] != '#') {
    temp.push_back(code[index][i] - 48);
    if (code[index][i] == 'x') flag = 1;
    i++;
  }

  imme = (flag == 0 ? getnum(temp, 10) : gethex(temp));

  if (flag1) imme = getinver(imme, 20);

  i = 0;
  while (Format[index1][i] != ' ') {
    i++;
  }
  i++;

  int j;
  for (j = 25; j <= 31; j++) binary[j] = Format[index1][i++] - 48;
  i++;

  j = 24;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rd % 2;
    rd /= 2;
  }

  j = 19;
  for (int k = 0; k < 20; k++) {
    binary[j--] = imme % 2;
    imme /= 2;
  }
  hexa();
}

void SBFunction(const int index, const int index1) {
  ll rs1, rs2, imme;
  size_t i = 0;
  std::string label;

  while (code[index][i] != 'x') {
    i++;
  }
  i++;

  std::vector<int> temp;
  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rs1 = getnum(temp, 10);
  temp.clear();

  while (code[index][i] != 'x') {
    i++;
  }
  i++;

  while (code[index][i] != ' ' && code[index][i] != ',') {
    temp.push_back(code[index][i] - 48);
    i++;
  }
  rs2 = getnum(temp, 10);
  temp.clear();

  while (code[index][i] == ' ' || code[index][i] == ',') {
    i++;
  }

  while (i < code[index].size() && code[index][i] != ' ' &&
         code[index][i] != '#') {
    label += code[index][i];
    i++;
  }
  imme = getlab(label, index);

  if (imme < 0) {
    imme = getinver(imme, 12);
  }

  i = 0;
  while (Format[index1][i] != ' ') {
    i++;
  }
  i++;

  int j;
  for (j = 25; j <= 31; j++) binary[j] = Format[index1][i++] - 48;

  i++;
  for (j = 17; j <= 19; j++) binary[j] = Format[index1][i++] - 48;

  j = 16;
  for (int k = 0; k < 5; k++) {
    binary[j--] = rs1 % 2;
    rs1 /= 2;
  }

  for (int k = 0; k < 5; k++) {
    binary[j--] = rs2 % 2;
    rs2 /= 2;
  }

  j = 23;
  for (int k = 0; k < 4; k++) {
    binary[j--] = imme % 2;
    imme /= 2;
  }

  j = 6;
  for (int k = 0; k < 6; k++) {
    binary[j--] = imme % 2;
    imme /= 2;
  }

  binary[24] = imme % 2;
  imme /= 2;
  binary[0] = imme % 2;
  imme /= 2;
  hexa();
}

void typenumber(const std::string ins, const int index, const int index1) {
  if (ins == "I") IFunction(index, index1);
  if (ins == "R") RFunction(index, index1);
  if (ins == "S") SFunction(index, index1);
  if (ins == "UJ") UJFunction(index, index1);
  if (ins == "U") UFunction(index, index1);
  if (ins == "SB") SBFunction(index, index1);
}

// To extract instruction type and process them independently
void process(void) {
  size = code.size();
  for (size_t i = 0; i < size; i++) {
    std::string ins;
    size_t j = 0;
    while (j < code[i].size() && code[i][j] != ' ') {
      ins += code[i][j];
      j++;
    }

    size_t sins = ins.size();
    if (ins[sins - 1] == ':' && code[i].size() > sins) {
      ins.clear();
      while (j < code[i].size() && code[i][j] == ' ') j++;
      while (j < code[i].size() && code[i][j] != ' ') ins += code[i][j++];
    }

    for (size_t k = 0; k < sizeI; k++) {
      std::string type;
      int k1 = 0;
      while (Format[k][k1] != ' ') {
        type += (Format[k][k1]);
        k1++;
      }
      if (ins.compare(type) == 0) {
        k1 = Format[k].size() - 1;
        std::string type1;
        while (Format[k][k1] != ' ') {
          type1 += (Format[k][k1]);
          k1--;
        }
        reverse(type1.begin(), type1.end());
        typenumber(type1, i, k);
        break;
      }
    }
  }
}

// To convert Stack Pointer(sp) to x2
void preprocess(void) {
  for (size_t i = 0; i < code.size(); i++) {
    size_t inssize = code[i].size();
    for (size_t j = 1; j < inssize; j++) {
      if (code[i][j - 1] == ' ' && code[i][j] == 's' && j + 1 < inssize &&
          code[i][j + 1] == 'p' && j + 2 < inssize && code[i][j + 2] == ' ') {
        code[i][j] = 'x';
        code[i][j + 1] = '2';
      }
      if (code[i][j - 1] == '(' && code[i][j] == 's' && j + 1 < inssize &&
          code[i][j + 1] == 'p' && j + 2 < inssize && code[i][j + 2] == ')') {
        code[i][j] = 'x';
        code[i][j + 1] = '2';
      }
      if (code[i][j - 1] == ' ' && code[i][j] == 's' && j + 1 < inssize &&
          code[i][j + 1] == 'p' && j + 2 < inssize && code[i][j + 2] == ',') {
        code[i][j] = 'x';
        code[i][j + 1] = '2';
      }
      if (code[i][j - 1] == ',' && code[i][j] == 's' && j + 1 < inssize &&
          code[i][j + 1] == 'p' && j + 2 < inssize && code[i][j + 2] == ',') {
        code[i][j] = 'x';
        code[i][j + 1] = '2';
      }
    }
  }
}

// To process Load Address(la) psudo command
void processla(const int index) {
  size_t i = 0;
  std::string s;

  while (codeinit[index][i] != 'x') i++;

  i++;
  while (codeinit[index][i] != ' ') s += codeinit[index][i++];

  code.push_back("auipc x" + s + " 65536");
  int currentpc = code.size() * 4 - 4;
  currentpc += START;

  while (codeinit[index][i] == ' ') i++;

  std::string labeltype;
  while (i < codeinit[index].size() && codeinit[index][i] != ' ') {
    labeltype += codeinit[index][i++];
  }
  ll labeladdress = 0;

  for (size_t j = 0; j < datalabel.size(); j++) {
    if (labeltype.compare(datalabel[j].name) == 0) {
      labeladdress = datalabel[j].position;
      break;
    }
  }

  labeladdress = labeladdress - currentpc;
  std::string labeladd;
  ll temp1 = abs(labeladdress);

  while (temp1 != 0) {
    labeladd += (temp1 % 10) + 48;
    temp1 /= 10;
  }

  labeladd += (labeladdress < 0 ? '-' : '0');
  reverse(labeladd.begin(), labeladd.end());

  code.push_back("addi x" + s + " x" + s + " " + labeladd);
}

// To process Load Word (lw) psudo command
void processlw(const std::string type, const int index, const ll pos) {
  std::string s, ins, labeladd;
  ll currentpc = code.size() * 4 + START;
  int i = 0;

  while (codeinit[index][i] != 'x') i++;

  i++;

  while (codeinit[index][i] != ' ') s += codeinit[index][i++];

  currentpc = pos - currentpc;
  code.push_back("auipc x" + s + " 65536");
  ll temp1 = abs(currentpc);

  while (temp1 != 0) {
    labeladd += (temp1 % 10) + 48;
    temp1 /= 10;
  }

  labeladd += (currentpc < 0 ? '-' : '0');
  reverse(labeladd.begin(), labeladd.end());
  code.push_back(type + " x" + s + " " + labeladd + "(x" + s + ")");
}

// To expand all psudo instruction if present
void shift(void) {
  int siz = codeinit.size();

  for (int i = 0; i < siz; i++) {
    size_t j;
    for (j = 0; j < codeinit[i].size(); j++) {
      if (codeinit[i][j] == 9) {
        codeinit[i][j] = 32;
      }
    }

    std::string ins;
    j = 0;
    int start = -1;

    while (j < codeinit[i].size() && codeinit[i][j] == ' ') j++;

    start = j;
    while (j < codeinit[i].size() && codeinit[i][j] != ' ') {
      ins += codeinit[i][j];
      j++;
      if (j < codeinit[i].size() && codeinit[i][j] == ':') {
        ins += codeinit[i][j];
        j++;
        break;
      }
    }

    while (j < codeinit[i].size() && codeinit[i][j] == ' ') j++;

    size_t sins = ins.size();

    if (ins[sins - 1] == ':' && codeinit[i].size() > sins) {
      ins.clear();
      while (j < codeinit[i].size() && codeinit[i][j] == ' ') j++;
      start = j;
      while (codeinit[i][j] != ' ') ins += codeinit[i][j++];
    }

    if (ins == "la") {
      processla(i);
      continue;
    } else if (ins == "lw" || ins == "lb" || ins == "lhw") {
      std::string lab;
      j = codeinit[i].size() - 1;

      while (codeinit[i][j] == ' ') j--;

      while (codeinit[i][j] != ' ') {
        lab += codeinit[i][j];
        j--;
      }

      reverse(lab.begin(), lab.end());

      int flag = 1;
      for (j = 0; j < datalabel.size(); j++) {
        if (lab.compare(datalabel[j].name) == 0) {
          flag = 0;
          processlw(ins, i, datalabel[j].position);
          break;
        }
      }

      if (!flag) continue;
    }

    for (size_t k = 0; k < sizeI; k++) {
      std::string type;
      int k1 = 0;

      while (Format[k][k1] != ' ') {
        type += (Format[k][k1]);
        k1++;
      }

      if (ins.compare(type) == 0) {
        std::string add;
        for (size_t l = start; l < codeinit[i].size(); l++)
          add += codeinit[i][l];

        code.push_back(add);
        break;
      }
    }
  }
}

// To extract all the labels from code
void setlabel(void) {
  size_t siz = codeinit.size();
  int count = -1;

  for (size_t i = 0; i < siz; i++) {
    std::string ins;
    size_t j;

    for (j = 0; j < codeinit[i].size() && codeinit[i][j] == ' '; j++)
      ;

    while (j < codeinit[i].size() && codeinit[i][j] != ' ') {
      ins += codeinit[i][j];
      j++;
      if (j < codeinit[i].size() && codeinit[i][j] == ':') {
        ins += codeinit[i][j];
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

    if (ins[sins - 1] == ':' && sins < codeinit[i].size()) {
      while (j < codeinit[i].size() && codeinit[i][j] == ' ') j++;

      ins.clear();
      while (j < codeinit[i].size() && codeinit[i][j] != ' ') {
        ins += codeinit[i][j];
        j++;
      }
    }

    if (ins == "la") {
      count += 2;
      continue;
    } else if (ins == "lw" || ins == "lb" || ins == "lhw") {
      std::string lab;
      j = codeinit[i].size() - 1;
      while (codeinit[i][j] == ' ') j--;

      while (codeinit[i][j] != ' ') {
        lab += codeinit[i][j];
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

    for (size_t k = 0; k < sizeI; k++) {
      std::string type;
      int k1 = 0;

      while (Format[k][k1] != ' ') {
        type += (Format[k][k1]);
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

  for (int i = 0; i < 4000; i++) datamemory[i] = "00";
  read_data();

  files.open("MCode.mc");
  files.close();
  formats();

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

  shift();
  setlabel();
  preprocess();
  process();
  myFile.close();
  file.open("MCode.mc", std::ios_base::app);

  file << s << std::endl;

  // Print the Data Memory Part in Increasing Address Order
  for (int i = 0; i < 400; i++) {
    file << datamemory[i] << " ";
    if ((i + 1) % 4 == 0) file << std::endl;
  }

  file << s << std::endl;
  file.close();
}
