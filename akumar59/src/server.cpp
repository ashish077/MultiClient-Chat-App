#include "../include/server.h"
#include "../include/logger.h"
#include "../include/block.h"
#include "../include/buffer_info.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <list>
#include <algorithm>

#define STDIN 0
using namespace std;



bool compare_blocks(block b1,block b2){
  return b1.listen_port_num < b2.listen_port_num;
}

bool compare_clients(socket_info s1,socket_info s2){
  return s1.port_num < s2.port_num;
}
//LIST OF LOGGED IN CLIENTS
void server::List_clients()
  {
   
            cse4589_print_and_log("[LIST:SUCCESS]\n");
            int x = 0;
            host_info.clients.sort(compare_clients);
            for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
               if (strcmp(iter->status,"logged-in") == 0)
                  cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++x,iter->hostname,iter->ip_addr,iter->port_num);
            }
            cse4589_print_and_log("[LIST:END]\n");
         
  }

//ITERATE OVER THE STATISTICS
  void server::iter_statistics()
  {
       cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
       int x = 0;
            host_info.clients.sort(compare_clients);
            for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter)
              cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n",++x,iter->hostname,iter->num_msg_sent,iter->num_msg_rcv,iter->status);
            cse4589_print_and_log("[STATISTICS:END]\n");
  }

//LIST OF BLOCKED CLIENTS
  void server::iter_blocked_list()
  {
      bool flag = false;
            char *arg[2];
            arg[0] = strtok(buf," ");
            arg[1] = strtok(NULL," ");
            for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
              if(strcmp(iter->ip_addr,arg[1]) == 0){
                int x = 0;
                iter->blocked_list.sort(compare_blocks);
                //iterate over sorted blocked_list 
                cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                for(list<block>::iterator block_iter = iter->blocked_list.begin();block_iter != iter->blocked_list.end();++block_iter){
                  cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++x,block_iter->host,block_iter->ip,block_iter->listen_port_num);
                }
                cse4589_print_and_log("[BLOCKED:END]\n");
                bool flag = true;
            }
        }
  }

// LIST OF CLIENT INFO
  void server::get_send_list_info(char* client_ip)
  {

      // GET  INFO LIKE LOGGED-IN, SEND-LIST-MESSAGE, BUFFER,HOSTNAME,IP-ADDR,PORT-NO  
  
            
            char clientListMsg[4096];
            bzero(&clientListMsg,sizeof(clientListMsg));
            strcat(clientListMsg,"LOGIN ");
            for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
              if(strcmp(iter->status,"logged-in") == 0){
                strcat(clientListMsg,iter->hostname);
                strcat(clientListMsg," ");
                strcat(clientListMsg,iter->ip_addr);
                strcat(clientListMsg," ");
                char at[8];
                bzero(&at,sizeof(at));
                snprintf(at, sizeof(at), "%d", iter->port_num);
                strcat(clientListMsg,at);
                strcat(clientListMsg," ");
              }
            }

            for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
              if(strcmp(iter->ip_addr,client_ip) == 0){
                while(!iter->buffer.empty()){
                  buffer_info buffInfo = iter->buffer.front();
                  strcat(clientListMsg,"BUFFER ");
                  strcat(clientListMsg,buffInfo.fr);
                  strcat(clientListMsg," ");

                  char siz[5];
                  bzero(&siz,sizeof(siz));
                  int length = strlen(buffInfo.mesg);
                  sprintf(siz,"%d",length);
                  strcat(clientListMsg,siz);
                  strcat(clientListMsg," ");

                  strcat(clientListMsg,buffInfo.mesg);
                  strcat(clientListMsg," ");
                  iter->buffer.pop();
                  cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                  cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",buffInfo.fr,buffInfo.des_ip,buffInfo.mesg);
                  cse4589_print_and_log("[%s:END]\n", "RELAYED");
                }
              }
            }

            if(send(head_socket,clientListMsg,strlen(clientListMsg),0)<0){
              cerr<<"send"<<endl;
            }
  }


//REMOVE A CLIENT FROM BLOCK LIST
  void server::unblock_client(int sock_index)
  {     
        char *arg[2]; 
        arg[1] = strtok(NULL," ");
        for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
            if(iter->fd == sock_index){
                for(list<block>::iterator inter_unblock = iter->blocked_list.begin();inter_unblock != iter->blocked_list.end();++inter_unblock){
                    if(strcmp(arg[1],inter_unblock->ip) == 0)
                      iter->blocked_list.erase(inter_unblock);
                 }
            }
        }
  }

 // ADD A CLIENT TO THE BLOCK LIST
  void server::block_client(int sock_index){
        char *arg[2];
              arg[1] = strtok(NULL," ");
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(iter->fd == sock_index){
                  block b;
                  strcpy(b.ip,arg[1]);
                  for(list<socket_info>::iterator iter_blockinfo = host_info.clients.begin();iter_blockinfo != host_info.clients.end();++iter_blockinfo){
                    if(strcmp(iter_blockinfo->ip_addr,arg[1]) == 0){
                      b.listen_port_num = iter_blockinfo->port_num;
                      strcpy(b.host,iter_blockinfo->hostname);
                    }
                }
                  iter->blocked_list.push_back(b);
              }
        }
  }

   //SEND A BROADCAST MESSAGE
  void server::send_broadcast(int sock_index){
       cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
              char from_client_ip[32];
              bzero(&from_client_ip,sizeof(from_client_ip));
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(iter->fd == sock_index){
                  strcpy(from_client_ip,iter->ip_addr);
                  iter->num_msg_sent++;
                }
              }

              char *arg[2];
              arg[1] = strtok(NULL,"");

              char new_msg[1024];
              bzero(&new_msg,sizeof(new_msg));
              strcat(new_msg,"BROADCAST ");
              strcat(new_msg,from_client_ip);
              strcat(new_msg," ");
              strcat(new_msg,arg[1]);

              //FIND BLOCKED IP ADDRS FROM CLIENT_LIST
              cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n",from_client_ip,arg[1]);
              cse4589_print_and_log("[%s:END]\n", "RELAYED");
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(iter->fd != sock_index && strcmp(iter->status,"logged-in") == 0){
                  if(send(iter->fd,new_msg,strlen(new_msg),0)<0){
                    print_error("BROADCAST");
                  }
                  iter->num_msg_rcv++;
                }
                if(iter->fd != sock_index && strcmp(iter->status,"logged-out") == 0){
                  buffer_info buffInfo;
                  strcpy(buffInfo.des_ip,iter->ip_addr);
                  strcpy(buffInfo.mesg,arg[1]);
                  strcpy(buffInfo.fr,from_client_ip);
                  iter->buffer.push(buffInfo);
                  iter->num_msg_rcv++;
                }
              }

  }
  //
  void server::refresh()
  {     // refresh to ground state
      char clientListMsg[4096];
              bzero(&clientListMsg,sizeof(clientListMsg));
              char *client_ip = strtok(NULL," ");
              strcat(clientListMsg,"REFRESH ");
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(strcmp(iter->status,"logged-in") == 0){
                  strcat(clientListMsg,iter->hostname);
                  strcat(clientListMsg," ");
                  strcat(clientListMsg,iter->ip_addr);
                  strcat(clientListMsg," ");
                  char pn[8];
                  bzero(&pn,sizeof(pn));
                  snprintf(pn, sizeof(pn), "%d", iter->port_num);
                  strcat(clientListMsg,pn);
                  strcat(clientListMsg," ");
                }
            }

  }

  void server::send_message(int sock_index)
  {
     char from_client_ip[32];
              bzero(&from_client_ip,sizeof(from_client_ip));
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(iter->fd == sock_index){
                  strcpy(from_client_ip,iter->ip_addr);
                  iter->num_msg_sent++;
                }
              }
              //arg[1]= message destination
              //arg[2]= actual message content 
              char *arg[3];
              arg[1] = strtok(NULL," ");
              arg[2] = strtok(NULL,"");

              char new_msg[1024];
              bzero(&new_msg,sizeof(new_msg));
              strcat(new_msg,"SEND ");
              strcat(new_msg,(const char*) from_client_ip);
              strcat(new_msg," ");
              strcat(new_msg,arg[1]);
              strcat(new_msg," ");
              strcat(new_msg,arg[2]);
              
              bool blocked = false;
              bool log = true;
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(strcmp(iter->ip_addr,arg[1]) == 0){
                  if(strcmp(iter->status,"logged-out") == 0){
                    log = false;
                  }
                  for(list<block>::iterator block_iter = iter->blocked_list.begin();block_iter != iter->blocked_list.end();++block_iter){
                    if(strcmp(block_iter->ip,from_client_ip) == 0)
                      blocked = true;
                  }
                }
              }
              

              // check if destinatin ip address is in blocked_list 
              if(log && !blocked){
                cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_client_ip,arg[1],arg[2]);
                cse4589_print_and_log("[%s:END]\n", "RELAYED");
                for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                  if(strcmp(iter->ip_addr,arg[1]) == 0){
                    if(send(iter->fd,new_msg,strlen(new_msg),0)<0){
                      cerr<<"failed to send message"<<endl;
                    }
                    iter->num_msg_rcv++;
                    break;
                  }
                }
                bzero(&msg,sizeof(msg));
              }
              //if destiantion ip address is logged out buffer it
              if(!log && !blocked){
               

                buffer_info buffi;
                 
                strcpy(buffi.des_ip,arg[1]);
                strcpy(buffi.mesg,arg[2]);
                strcpy(buffi.fr,from_client_ip);

                for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                  if(strcmp(iter->ip_addr,arg[1]) == 0){
                    iter->buffer.push(buffi);
                    iter->num_msg_rcv++;
                  }
                }
              }

  }



server::server(char* port){

  
  //port number of server
  strcpy(host_info.port_number,port);

  // ip address of server
  struct hostent *ht;    //dont know what this line is 
  char hostname[1024];
   
  if (gethostname(hostname,1024) < 0){
    cerr<<"failed to gethostname"<<endl;
    exit(1);
  }
  if ((ht=gethostbyname(hostname)) == NULL){
    cerr<<"not a known hostname"<<endl;
    exit(1);
  }

  struct in_addr **addr_list = (struct in_addr **)ht->h_addr_list;
  for(int i = 0;addr_list[i] != NULL;++i){
    strcpy(host_info.ip_address,inet_ntoa(*addr_list[i]));
  }

  // creating a socket
  if ((host_info.listener = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    cerr<<"socket\n";
    exit(1);
  }

  // bind the socket to  an address
  struct sockaddr_in server_addr;
  bzero(&server_addr,sizeof(server_addr));
  server_addr.sin_family = AF_INET; 
  server_addr.sin_port = htons(atoi(port)); 
  server_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(host_info.listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    cerr<<"binding failed";
    exit(1);
  }

  //listening for clients
  if (listen(host_info.listener, 8) < 0) {
    cerr<<"listen";
    exit(1);
  }

  //
  fd_set master_list, watch_list; 
  FD_ZERO(&master_list); 
  FD_ZERO(&watch_list);
  FD_SET(host_info.listener,&master_list);
  FD_SET(STDIN,&master_list);

  
  //int selret,fdaccept=0;

  int caddr_len = host_info.listener;
  int addrlen;
 
  int nbytes;
  //main loop
  struct sockaddr_in client_addr;
  while(true){
    watch_list = master_list; 
    int selret;
    selret = select(caddr_len+1, &watch_list, NULL, NULL, NULL);
    if (selret < 0) {
      cerr<<"error";
      exit(1);
    }
    for(int sock_index = 0; sock_index <= caddr_len; sock_index++) {
      memset((void *)&buf,'\0',sizeof(buf));
      if (FD_ISSET(sock_index, &watch_list)) {
        if (sock_index == STDIN){
          read(STDIN,buf,1024);
          buf[strlen(buf)-1]='\0';
          if (strcmp(buf,"AUTHOR")==0){
            print_author();
          }
          if (strcmp(buf,"IP")==0){
              print_ip();
          }
          if (strcmp(buf,"PORT")==0){
            print_port();
          }
          if (strcmp(buf,"LIST")==0){
            List_clients();

          }
          if (strncmp(buf,"BLOCKED",7)==0){
         
                iter_blocked_list();
          }          
          if (strcmp(buf,"STATISTICS")==0){
                        iter_statistics();
          }
        }
        else if (sock_index == host_info.listener) {
          // NEW CONNECTIONS
          addrlen = sizeof(client_addr);
          if ((head_socket = accept(host_info.listener, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) == -1) {
            cerr<<"accept";
          } 
          else {
            FD_SET(head_socket, &master_list);  
            if (head_socket > caddr_len) {     
              caddr_len = head_socket;
            }

            
            
            bool flag = true;
            struct sockaddr_in *sin = (struct sockaddr_in*)&client_addr;
            char *client_ip = inet_ntoa(sin->sin_addr);
            for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
              if(strcmp(client_ip,iter->ip_addr) == 0){
                strcpy(iter->status,"logged-in");
                iter->fd = head_socket;
                flag = false;
              }
            }            
            if(flag){
              const char *sts = "logged-in";
              struct socket_info sock;
              sock.list_id = host_info.clients.size()+1;
            
            
            //GET HOSTNAME
              struct in_addr *addr_temp;
              struct sockaddr_in saddr;
              struct hostent *hoste;
              if(!inet_aton(client_ip,&saddr.sin_addr)){
                cerr<<"inet_aton"<<endl;
                exit(1);
              }
            
              if((hoste = gethostbyaddr((const void *)&saddr.sin_addr,sizeof(client_ip),AF_INET)) == NULL){
                cerr<<"gethostbyaddr"<<endl;
                exit(1);
              }
              char *n = hoste->h_name;
              strcpy(sock.hostname,n);

              
              strcpy(sock.ip_addr,client_ip);
              sock.fd = head_socket;
              strncpy(sock.status,sts,strlen(sts));

              char client_port[8];
              bzero(&client_port,sizeof(client_port));
              if(recv(head_socket,client_port,sizeof(client_port),0) <= 0){
                cerr<<"port"<<endl;
              }
              sock.port_num = atoi(client_port);

              host_info.clients.push_back(sock);
            }
            
              get_send_list_info(client_ip);
          }
        } 
        else {
          char msg[1024];
          bzero(&msg,sizeof(msg));
          /* Handle data from a client */
          if ((nbytes = recv(sock_index, msg, sizeof(msg), 0)) <= 0) {
            /* Got error or connection closed by client */
            if (nbytes == 0) {
              /* Connection closed */
              host_info.clients_number--;
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(iter->fd == sock_index){
                  const char *stso = "logged-out";
                  strncpy(iter->status,stso,strlen(stso));
                }
              }
            } 
            else {
              cerr<<"recv";
            }
            close(sock_index); 
            //REMOVE FROM THE MASTER_LIST SET
            FD_CLR(sock_index, &master_list); 
          }  
          else {
           
            // CLIENT DATA REVIEVED

            char original_msg[1024];
            char buffer_msg[1024];
            bzero(&original_msg,sizeof(original_msg));
            bzero(&buffer_msg,sizeof(buffer_msg));
            strcpy(original_msg,msg);
            strcpy(buffer_msg,msg);
            char *arg_zero = strtok(msg," ");
            
           
             if(strcmp(arg_zero,"SEND") == 0){
              send_message(sock_index) ;
             }
             else if(strcmp(arg_zero,"REFRESH") == 0){  
             refresh();
             }
             else if(strcmp(arg_zero,"BROADCAST") == 0){
               send_broadcast(sock_index); 
             }
             else if(strcmp(arg_zero,"BLOCK") == 0){
              char *arg[2];
              arg[1] = strtok(NULL," ");
              for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
                if(iter->fd == sock_index){
                  block b;
                  strcpy(b.ip,arg[1]);
                  for(list<socket_info>::iterator iter_blockinfo = host_info.clients.begin();iter_blockinfo != host_info.clients.end();++iter_blockinfo){
                    if(strcmp(iter_blockinfo->ip_addr,arg[1]) == 0){
                      b.listen_port_num = iter_blockinfo->port_num;
                      strcpy(b.host,iter_blockinfo->hostname);
                    }
                }
                  iter->blocked_list.push_back(b);
              }
        }
          //   block_client(sock_index);
             }
             else if(strcmp(arg_zero,"UNBLOCK") == 0){
             unblock_client(sock_index);
             }
          }
        }
      } 
    }
  }
} 

bool server::isvalid(char *server_ip){
  for(list<socket_info>::iterator iter = host_info.clients.begin();iter != host_info.clients.end();++iter){
    if(strcmp(server_ip,iter->ip_addr) == 0)
     { return true;}
  }
  return false;
}




