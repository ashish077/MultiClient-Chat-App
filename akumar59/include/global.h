#ifndef GLOBAL_H_
#define GLOBAL_H_
#define HOSTNAME_LEN 128
#define PATH_LEN 256
#include "infm.h"
class global{
protected:
  infm host_info;
public:
  global();

  void print_error(const char* command_str);
  void print_author();
  void print_ip();
  void print_port();
  
  
};

#endif
