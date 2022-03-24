#ifndef GLOBAL_H_
#define GLOBAL_H_
#define HOSTNAME_LEN 128
#define PATH_LEN 256
#include "info.h"
class global{
protected:
  info host_info;
public:
  global();

  void print_error(const char* command_str);
  void print_author();
  void print_ip();
  void print_port();
  
  
};

#endif
