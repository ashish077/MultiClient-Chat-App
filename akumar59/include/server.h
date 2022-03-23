#ifndef SERVER_H
#define SERVER_H
#include "common.h"

class server:public common{
public:
  server(char* port);
  bool isvalid(char *server_ip);
  void send_broadcast(int i);
  void block_client(int i);
  void unblock_client(int i);
  void refresh();
  void get_send_list_info(char*);
  void send_message(int i);
  void iter_blocked_list();
  void iter_statistics();
  void List_clients();  
};

#endif
