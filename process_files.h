#ifndef __PROCESS_FILES_H__
#define __PROCESS_FILES_H__

extern const int32_t START;
extern const int32_t ARCH_SIZE;

extern std::vector<std::string> codeinit;
extern std::vector<std::string> formats;

typedef struct {
  std::string name;
  int32_t position;
} seg;

extern std::vector<seg> datalabel;

void __write_mc(const int32_t binary[], int32_t &pc);

int process_files(const std::string &assembly_file,
                  const std::string &formats_file,
                  const std::string &mc_filename);

int save_mc(void);

#endif
