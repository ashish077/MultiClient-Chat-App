#ifndef INFM_H
#define INFM_H
#include <list>
#include <socket_info.h>
#include <string.h>

struct infm{
  infm(){
  
    num_clients=0;
    bzero(&ip_address,sizeof(ip_address));
    bzero(&port_number,sizeof(port_number));
  }
  std::list<socket_info> clients;
  std::list<block> block_list;
  char ip_address[1024];
  char port_number[1024];
  int listener;
   
  int num_clients;
};

#endif
