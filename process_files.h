#ifndef __PROCESS_FILES_H__
#define __PROCESS_FILES_H__

extern const int32_t ARCH_SIZE;

void __write_mc(const int32_t binary[], int32_t &pc);

int process_files(const std::string &assembly_file,
                  const std::string &formats_file,
                  const std::string &mc_filename);

int save_mc(void);

#endif
