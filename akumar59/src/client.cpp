#include "../include/client.h"
#include "../include/logger.h"
#include "../include/common.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <netdb.h>

#define STDIN 0
using namespace std;

bool compare_client(socket_info s1,socket_info s2){
  return s1.port_num < s2.port_num;
}

bool isvalid(char *server_ip,int p){
  struct sockaddr_in ip4addr;
  ip4addr.sin_family = AF_INET;
  ip4addr.sin_port = htons(p);
  if(inet_pton(AF_INET,server_ip,&ip4addr.sin_addr) != 1)
    return false;
  return true;
}
void client::List_clients(){
  cse4589_print_and_log("[LIST:SUCCESS]\n");
      int cnt = 1;
      host_info.clients.sort(compare_client);
      list<socket_info>::iterator iter = host_info.clients.begin();
      while(iter != host_info.clients.end()){
        if (strcmp(iter->status,"logged-in") == 0)
           cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",cnt++,iter->hostname,iter->ip_addr,iter->port_num);
        iter++;
      }
      cse4589_print_and_log("[LIST:END]\n");
}

client::client(char *port){
  /* Save port number */
  strcpy(host_info.port_number,port);

  /* Save IP address */
  struct hostent *ht;
  char hostname[1024];
  if (gethostname(hostname,1024) < 0){
    cout<<"gethostname\n";
    exit(1);
  }
  if ((ht=gethostbyname(hostname)) == NULL){
    cout<<"gethostbyname\n";
    exit(1);
  }
  struct in_addr **addr_list = (struct in_addr **)ht->h_addr_list;
  int idx = 0;
  while(addr_list[idx] != NULL)
  {
    strcpy(host_info.ip_address,inet_ntoa(*addr_list[idx]));
    idx++;
  }

  /* Create socket. */
  if ((host_info.listener = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    cout<<"socket\n";
    exit(1);
  }

  /* Bind socket */
  struct sockaddr_in client_addr; 
  memset(&client_addr,0,sizeof(client_addr));
  client_addr.sin_family = AF_INET; 
  client_addr.sin_port = htons(atoi(port)); 
  client_addr.sin_addr = *((struct in_addr*)ht->h_addr);
  if (bind(host_info.listener, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
    cout<<"bind";
    exit(1);
  }

  /* Main loop */
  char buf[1024];
  while(true){
    memset(&buf,0,sizeof(buf));
    read(STDIN,buf,1024);
    buf[strlen(buf)-1]='\0';
    if (strcmp(buf,"EXIT") == 0){
      cse4589_print_and_log("[EXIT:SUCCESS]\n");
      cse4589_print_and_log("[EXIT:END]\n");
      break;
    }
    else if (strcmp(buf,"AUTHOR") == 0){
      print_author();
    }
    else if (strcmp(buf,"PORT") == 0){
      print_port();
    }
    else if (strcmp(buf,"IP") == 0){
      print_ip();
    }
    else if (strcmp(buf,"LIST") == 0){
      List_clients();
    }
    else if (strncmp(buf,"LOGIN",5) == 0){
      char *server_ip,*server_port;
      strtok(buf," ");
      server_ip = strtok(NULL," ");
      server_port = strtok(NULL," ");
      

      bool valid_port = true;
      if(server_port == NULL){
        print_error("LOGIN");
        continue;
      }
      idx = 0;   
      while(idx != strlen(server_port)){
        if(server_port[idx] < '0' || server_port[idx] > '9'){
          print_error("LOGIN");
          valid_port = false;
          break;
        }        
        idx++;
      }
      if(!valid_port) continue;
      int port = atoi(server_port);
      if(port < 0 || port > 65535){
        print_error("LOGIN");
        continue;
      }

      /* Invalid ip address */
      if (!isvalid(server_ip,port)){
        print_error("LOGIN");
        continue;
      }
      else{
        struct addrinfo *result,hints;
        memset(&hints,0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(server_ip, server_port, &hints, &result) != 0) {
          print_error("LOGIN");
          continue;
        }
        else{
          /* Get socket fd */
          if ((host_info.listener = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            print_error("LOGIN");
            continue;
          }

          /* Connect to server */
          struct sockaddr_in dest_addr; 
          memset(&dest_addr,0,sizeof(dest_addr));
          dest_addr.sin_family = AF_INET;
          dest_addr.sin_port = htons(port);
          dest_addr.sin_addr.s_addr = inet_addr(server_ip);
          if ((connect(host_info.listener, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))) < 0){
            print_error("LOGIN");
            continue;
          }
           
          char client_port[8];
          memset(&client_port,0,sizeof(client_port));
          strcat(client_port,host_info.port_number);
          if(send(host_info.listener,client_port,strlen(client_port),0)<0){
            cout<<"port\n";
          }


          char buf[1024];
          while(true){
            /* Add the listener to read set */
            memset(&buf,0,sizeof(buf));
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(STDIN,&read_fds);
            FD_SET(host_info.listener,&read_fds);

            int fd_max = host_info.listener;
            select(fd_max+1, &read_fds, NULL, NULL, NULL);
            if (FD_ISSET(STDIN, &read_fds)){
              read(STDIN,buf,1024);
              buf[strlen(buf)-1]='\0';

              if (strcmp(buf,"AUTHOR") == 0){
                print_author();
              }
              else if (strcmp(buf,"PORT") == 0){
                print_port();
              }
              else if (strcmp(buf,"IP") == 0){
                print_ip();
              }
              else if (strcmp(buf,"LIST") == 0){
                  List_clients();
              }
              else if(strcmp(buf,"REFRESH") == 0){
                strcat(buf," ");
                strcat(buf,host_info.ip_address);
                if(send(host_info.listener,buf,strlen(buf),0)<0){
                  print_error("REFRESH");
                }
                cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                cse4589_print_and_log("[REFRESH:END]\n");
              }
              else if(strncmp(buf,"SEND",4) == 0){
                char send_message[1024],*arg[3];;
                memset(&send_message,0,sizeof(send_message));
                strcpy(send_message,buf);
                
                arg[0] = strtok(buf," ");
                idx = 1;
                while(idx != 3){
                  arg[idx] = strtok(NULL," ");
                  idx++;
                }
                /* Whether in current list */
                bool isval = false;
                list<socket_info>::iterator iter = host_info.clients.begin();
                while(iter != host_info.clients.end()){
                  if(strcmp(iter->ip_addr,arg[1]) == 0) 
                    isval = true;
                  iter++;
                }
            
                if(!isval || send(host_info.listener,send_message,strlen(send_message),0)<0){
                  print_error("SEND");
                  continue;
                }
                cse4589_print_and_log("[SEND:SUCCESS]\n");
                cse4589_print_and_log("[SEND:END]\n");
              }
              else if(strncmp(buf,"BROADCAST",9) == 0){
                if(send(host_info.listener,buf,strlen(buf),0)<0){
                  print_error("BROADCAST");
                }
                cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
                cse4589_print_and_log("[BROADCAST:END]\n");
              }
              else if(strncmp(buf,"BLOCK",5) == 0){/* Still need arg[1] */
                char temp_buf[1024];
                memset(&temp_buf,0,sizeof(temp_buf));
                strcpy(temp_buf,buf);
                strtok(temp_buf," ");
                char *block_ip = strtok(NULL," ");
                bool isval = false;
                bool isblocked = false;
                block b;
                list<socket_info>::iterator iter = host_info.clients.begin();
                while(iter != host_info.clients.end()){
                  if(strcmp(iter->ip_addr,block_ip) == 0) {
                    isval = true;
                    b.listen_port_num = iter->port_num;
                    strcpy(b.host,iter->hostname);
                    strcpy(b.ip,iter->ip_addr);
                    break;
                  }
                  iter++;
                }
                list<block>::iterator iter_block = host_info.block_list.begin();
                while(iter_block != host_info.block_list.end()){
                  if(strcmp(block_ip,iter_block->ip) == 0){
                    isblocked = true;
                    break;
                  }
                  iter_block++;
                }

                if(!isval || isblocked){
                  print_error("BLOCK");
                  continue;
                }
                if(send(host_info.listener,buf,strlen(buf),0)<0){
                  print_error("BLOCK");
                  continue;
                }
                else{
                  host_info.block_list.push_back(b);
                }
                cse4589_print_and_log("[BLOCK:SUCCESS]\n");
                cse4589_print_and_log("[BLOCK:END]\n");
              }
              else if(strncmp(buf,"UNBLOCK",7) == 0){
                char temp_buf[1024],*arg[2];
                
                memset(&temp_buf,0,sizeof(temp_buf));
                strcpy(temp_buf,buf);
                arg[0] = strtok(temp_buf," ");
                arg[1] = strtok(NULL," ");

                bool valid = false;
                list<block>::iterator iter_block = host_info.block_list.begin();
                while(iter_block != host_info.block_list.end()){
                  if(strcmp(iter_block->ip,arg[1]) == 0){
                    host_info.block_list.erase(iter_block);
                    valid = true;
                    break;
                  }
                  iter_block++;
                }
                if(!valid || send(host_info.listener,buf,strlen(buf),0)<0){
                  print_error("UNBLOCK");
                  continue;
                }
                cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
                cse4589_print_and_log("[UNBLOCK:END]\n");
              }
              else if(strcmp(buf,"LOGOUT") == 0){
                cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
                close(host_info.listener);
                cse4589_print_and_log("[LOGOUT:END]\n");
                break;
              }
              else if(strcmp(buf,"EXIT") == 0){
                close(host_info.listener);
                cse4589_print_and_log("[EXIT:SUCCESS]\n");
                cse4589_print_and_log("[EXIT:END]\n");
                exit(0);
              }
            }
            else{
              char msg[1024];
              memset(&msg,0,sizeof(msg));
              int recvbytes;
              if((recvbytes = recv(host_info.listener,msg,sizeof(msg),0)) <= 0){
                cout<<"recv\n";
              }
              
              char *arg_zero = strtok(msg," ");

              /* Process received data */
              
              if(FD_ISSET(host_info.listener,&read_fds)){
                
                if(strcmp(arg_zero,"SEND") == 0){
                  cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                  char *arg[4];
                  arg[1] = strtok(NULL," ");
                  arg[2] = strtok(NULL," ");
                  arg[3] = strtok(NULL,"");
                  cse4589_print_and_log("msg from:%s\n[msg]:%s\n",arg[1],arg[3]);
                  cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                }
                else if(strcmp(arg_zero,"BROADCAST") == 0){
                  cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                  char *arg[3];
                  arg[1] = strtok(NULL," ");
                  arg[2] = strtok(NULL,"");
                  cse4589_print_and_log("msg from:%s\n[msg]:%s\n",arg[1],arg[2]);
                  cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                }
                else if(strcmp(arg_zero,"LOGIN") == 0){
                  host_info.clients.clear();
                  while(true){
                    char *list_msg[3];

                    /* If have buffer. */
                    list_msg[0] = strtok(NULL," ");
                    char mesg[512];
                    char messag[4096];
                    memset(&messag,0,sizeof(messag));
                    while(list_msg[0] != NULL && strcmp(list_msg[0],"BUFFER") == 0){
                      char original_messag[4096];
                      memset(&original_messag,0,sizeof(original_messag));
                      strcpy(messag,strtok(NULL,""));
                      strcpy(original_messag,messag);

                      char *fr = strtok(original_messag," ");
                      char *l = strtok(NULL," ");

                      int length = atoi(l);
                      char *next;
                      next = strtok(NULL,"");
                      memset(&mesg,0,sizeof(mesg));
                      strncpy(mesg,next,length);
                      cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                      cse4589_print_and_log("msg from:%s\n[msg]:%s\n",fr,mesg);
                      cse4589_print_and_log("[%s:END]\n", "RECEIVED");

                      list_msg[0] = strtok(messag," ");
                      if(strcmp(list_msg[0],"BUFFER") == 0 || list_msg[0] == NULL) {
                        continue;
                      }

                      while((list_msg[0] = strtok(NULL," ")) != NULL && strcmp(list_msg[0],"BUFFER") != 0)
                        continue;
                    }
                    if(list_msg[0] == NULL){
                      cse4589_print_and_log("[LOGIN:SUCCESS]\n");
                      cse4589_print_and_log("[LOGIN:END]\n");
                      break;
                    }

                    idx = 1;
                    while(idx != 3){
                      memset(&list_msg[idx],0,sizeof(list_msg[idx]));
                      list_msg[idx] = strtok(NULL," ");
                      idx++;
                    }
                    struct socket_info si;
                    strcpy(si.hostname,list_msg[0]);
                    strcpy(si.ip_addr,list_msg[1]);
                    int port_n = atoi(list_msg[2]);
                    si.port_num = port_n;
                    strcpy(si.status,"logged-in");
                    host_info.clients.push_back(si);
                  }
                }
                else if(strcmp(arg_zero,"REFRESH") == 0){
                  host_info.clients.clear();
                  while(true){
                    char *list_msg[3];
                    if((list_msg[0] = strtok(NULL," ")) == NULL)
                      break;
                    
                    idx = 1;
                    while(idx != 3){
                      memset(&list_msg[idx],0,sizeof(list_msg[idx]));
                      list_msg[idx] = strtok(NULL," ");
                      idx++;
                    }
                    struct socket_info si;
                    strcpy(si.hostname,list_msg[0]);
                    strcpy(si.ip_addr,list_msg[1]);
                    int port_n = atoi(list_msg[2]);
                    si.port_num = port_n;
                    strcpy(si.status,"logged-in");
                    host_info.clients.push_back(si);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}


