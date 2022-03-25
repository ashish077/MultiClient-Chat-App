#ifndef BUFFER_INFM_H
#define BUFFER_INFM_H

#include <string.h>

struct buffer_infm{
  buffer_infm(){
    bzero(&des_ip,sizeof(des_ip));
    bzero(&mesg,sizeof(mesg));
    bzero(&fr,sizeof(fr));
  }
  char des_ip[32];
  char mesg[1024];
  char fr[32];
};


#endif
