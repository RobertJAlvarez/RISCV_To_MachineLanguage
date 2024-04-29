#ifndef __PROCESS_CODE_H__
#define __PROCESS_CODE_H__

#include <stdint.h>

#include <string>

extern const int32_t START;

typedef struct {
  std::string s;
  int index;
} lab;

typedef struct {
  std::string name;
  int32_t position;
} seg;

extern std::vector<std::string> codeinit;
extern std::vector<lab> labels;
extern std::vector<seg> datalabel;

void pre_process_code(void);

#endif
