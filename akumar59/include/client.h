#ifndef CLIENT_H
#define CLIENT_H
//#include "common.h"
#include "global.h"
class client:public global{
public:
  client(char *port);
};

#endif
