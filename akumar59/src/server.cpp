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

bool compare_clients(socket_info si1,socket_info si2){
  return si1.port_num < si2.port_num;
}

bool compare_blocks(block b1,block b2){
  return b1.listen_port_num < b2.listen_port_num;
}
char msg [1024];
char buf [1024];
int newfd;
//LIST OF LOGGED IN CLIENTS
void server::List_clients()
  {
        /* Standard input */
          
         
            cse4589_print_and_log("[LIST:SUCCESS]\n");
            int i = 0;
            information.clients.sort(compare_clients);
            for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
               if (strcmp(iter->status,"logged-in") == 0)
                  cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++i,iter->hostname,iter->ip_addr,iter->port_num);
            }
            cse4589_print_and_log("[LIST:END]\n");
         
  }

//ITERATE OVER THE STATISTICS
  void server::iter_statistics()
  {
       cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
       int i = 0;
            information.clients.sort(compare_clients);
            for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter)
              cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n",++i,iter->hostname,iter->num_msg_sent,iter->num_msg_rcv,iter->status);
            cse4589_print_and_log("[STATISTICS:END]\n");
  }

//LIST OF BLOCKED CLIENTS
  void server::iter_blocked_list()
  {
      bool valid = false;
            char *arg[2];
            arg[0] = strtok(buf," ");
            arg[1] = strtok(NULL," ");
            for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
              if(strcmp(iter->ip_addr,arg[1]) == 0){
                int i = 0;
                iter->blocked_list.sort(compare_blocks);
                /* Have to sort iter->blocked_list */
                cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                for(list<block>::iterator block_iter = iter->blocked_list.begin();block_iter != iter->blocked_list.end();++block_iter){
                  cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++i,block_iter->host,block_iter->ip,block_iter->listen_port_num);
                }
                cse4589_print_and_log("[BLOCKED:END]\n");
                bool valid = true;
            }
        }
  }

// LIST OF CLIENT INFO
  void server::get_send_list_info(char* received_ip)
  {

      /* Get list information, including hostname, ip, port number. */
            /* Logged in and send list message and buffer message */
            char list_message[4096];
            bzero(&list_message,sizeof(list_message));
            strcat(list_message,"LOGIN ");
            for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
              if(strcmp(iter->status,"logged-in") == 0){
                strcat(list_message,iter->hostname);
                strcat(list_message," ");
                strcat(list_message,iter->ip_addr);
                strcat(list_message," ");
                char pn[8];
                bzero(&pn,sizeof(pn));
                snprintf(pn, sizeof(pn), "%d", iter->port_num);
                strcat(list_message,pn);
                strcat(list_message," ");
              }
            }

            for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
              if(strcmp(iter->ip_addr,received_ip) == 0){
                while(!iter->buffer.empty()){
                  buffer_info binfo = iter->buffer.front();
                  strcat(list_message,"BUFFER ");
                  strcat(list_message,binfo.fr);
                  strcat(list_message," ");

                  char l[5];
                  bzero(&l,sizeof(l));
                  int length = strlen(binfo.mesg);
                  sprintf(l,"%d",length);
                  strcat(list_message,l);
                  strcat(list_message," ");

                  strcat(list_message,binfo.mesg);
                  strcat(list_message," ");
                  iter->buffer.pop();
                  cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                  cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",binfo.fr,binfo.des_ip,binfo.mesg);
                  cse4589_print_and_log("[%s:END]\n", "RELAYED");
                }
              }
            }

            if(send(newfd,list_message,strlen(list_message),0)<0){
              cerr<<"send"<<endl;
            }
  }


//to remove a client from blocked list
  void server::unblock_client(int i)
  {
        char *arg[2];
        arg[1] = strtok(NULL," ");
        for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
            if(iter->fd == i){
                for(list<block>::iterator unblock_iter = iter->blocked_list.begin();unblock_iter != iter->blocked_list.end();++unblock_iter){
                    if(strcmp(arg[1],unblock_iter->ip) == 0)
                      iter->blocked_list.erase(unblock_iter);
                 }
            }
        }
  }

 // to add a client to a blocked list
  void server::block_client(int i){
        char *arg[2];
              arg[1] = strtok(NULL," ");
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                if(iter->fd == i){
                  block b;
                  strcpy(b.ip,arg[1]);
                  for(list<socket_info>::iterator block_info_iter = information.clients.begin();block_info_iter != information.clients.end();++block_info_iter){
                    if(strcmp(block_info_iter->ip_addr,arg[1]) == 0){
                      b.listen_port_num = block_info_iter->port_num;
                      strcpy(b.host,block_info_iter->hostname);
                    }
                }
                  iter->blocked_list.push_back(b);
              }
        }
  }


  void server::send_broadcast(int i){
       cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
              char from_client_ip[32];
              bzero(&from_client_ip,sizeof(from_client_ip));
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                if(iter->fd == i){
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

              /* Find all blocked ip address in from_client_ip's blocked_list */
              /* Undo. */
              cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n",from_client_ip,arg[1]);
              cse4589_print_and_log("[%s:END]\n", "RELAYED");
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                if(iter->fd != i && strcmp(iter->status,"logged-in") == 0){
                  if(send(iter->fd,new_msg,strlen(new_msg),0)<0){
                    print_error("BROADCAST");
                  }
                  iter->num_msg_rcv++;
                }
                if(iter->fd != i && strcmp(iter->status,"logged-out") == 0){
                  buffer_info binfo;
                  strcpy(binfo.des_ip,iter->ip_addr);
                  strcpy(binfo.mesg,arg[1]);
                  strcpy(binfo.fr,from_client_ip);
                  iter->buffer.push(binfo);
                  iter->num_msg_rcv++;
                }
              }

  }

  void server::refresh()
  {
      char list_message[4096];
              bzero(&list_message,sizeof(list_message));
              char *received_ip = strtok(NULL," ");
              strcat(list_message,"REFRESH ");
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                if(strcmp(iter->status,"logged-in") == 0){
                  strcat(list_message,iter->hostname);
                  strcat(list_message," ");
                  strcat(list_message,iter->ip_addr);
                  strcat(list_message," ");
                  char pn[8];
                  bzero(&pn,sizeof(pn));
                  snprintf(pn, sizeof(pn), "%d", iter->port_num);
                  strcat(list_message,pn);
                  strcat(list_message," ");
                }
            }

  }

  void server::send_message(int i)
  {
     char from_client_ip[32];
              bzero(&from_client_ip,sizeof(from_client_ip));
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                if(iter->fd == i){
                  strcpy(from_client_ip,iter->ip_addr);
                  iter->num_msg_sent++;
                }
              }
              /* arg[1]:destinate,arg[2]:msg */
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
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
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
              

              /* Find whether arg[1]:destinate ip address in from_client_ip's blocked_list */
              if(log && !blocked){
                cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_client_ip,arg[1],arg[2]);
                cse4589_print_and_log("[%s:END]\n", "RELAYED");
                for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                  if(strcmp(iter->ip_addr,arg[1]) == 0){
                    if(send(iter->fd,new_msg,strlen(new_msg),0)<0){
                      cerr<<"send"<<endl;
                    }
                    iter->num_msg_rcv++;
                    break;
                  }
                }
                bzero(&msg,sizeof(msg));
              }
              /* If destinate_ip is logged out, buffer it. */
              if(!log && !blocked){
               

                buffer_info bi;
                /* Buffered msg "Buffer"+des_ip+msg+from_ip. */
                strcpy(bi.des_ip,arg[1]);
                strcpy(bi.mesg,arg[2]);
                strcpy(bi.fr,from_client_ip);

                for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                  if(strcmp(iter->ip_addr,arg[1]) == 0){
                    iter->buffer.push(bi);
                    iter->num_msg_rcv++;
                  }
                }
              }

  }



server::server(char* port){
  /* Save port number */
  strcpy(information.port_number,port);

  /* Save IP address */
  struct hostent *ht;    //dont know what this line is 
  char hostname[1024];
  if (gethostname(hostname,1024) < 0){
    cerr<<"failed to gethostname"<<endl;
    exit(1);
  }
  if ((ht=gethostbyname(hostname)) == NULL){
    cerr<<"gethostbyname"<<endl;
    exit(1);
  }
  struct in_addr **addr_list = (struct in_addr **)ht->h_addr_list;
  for(int i = 0;addr_list[i] != NULL;++i){
    strcpy(information.ip_address,inet_ntoa(*addr_list[i]));
  }

  /* Create socket. */
  if ((information.listener = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    cerr<<"socket\n";
    exit(1);
  }

  /* Bind socket */
  struct sockaddr_in my_addr;
  bzero(&my_addr,sizeof(my_addr));
  my_addr.sin_family = AF_INET; 
  my_addr.sin_port = htons(atoi(port)); 
  my_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(information.listener, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
    cerr<<"bind";
    exit(1);
  }

  /* Listen socket */
  if (listen(information.listener, 8) < 0) {
    cerr<<"listen";
    exit(1);
  }

  /* Add the listener to master set */
  fd_set master; 
  fd_set read_fds; 
  FD_ZERO(&master); 
  FD_ZERO(&read_fds);
  FD_SET(information.listener,&master);
  FD_SET(STDIN,&master);

  /* Main loop */
  int fdmax = information.listener;
  int addrlen;
 // int newfd;   //DEFINE IN GLOBAL
  int nbytes;
  
  struct sockaddr_in remoteaddr;
  //for(;;){
  while(true){
    read_fds = master; 
    int ret;
    if ((ret = select(fdmax+1, &read_fds, NULL, NULL, NULL)) < 0) {
      cerr<<"error";
      exit(1);
    }
    for(int i = 0; i <= fdmax; i++) {
      memset((void *)&buf,'\0',sizeof(buf));
      if (FD_ISSET(i, &read_fds)) {
        if (i == STDIN){
          /* Standard input */
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
        else if (i == information.listener) {
          /* Handle new connections */
          addrlen = sizeof(remoteaddr);
          if ((newfd = accept(information.listener, (struct sockaddr*)&remoteaddr, (socklen_t*)&addrlen)) == -1) {
            cerr<<"accept";
          } 
          else {
            FD_SET(newfd, &master);  /* Add to master set */
            if (newfd > fdmax) {     /* Keep track of the maximum */
              fdmax = newfd;
            }

            /* New client */
            
            bool flag = true;
            struct sockaddr_in *sin = (struct sockaddr_in*)&remoteaddr;
            char *received_ip = inet_ntoa(sin->sin_addr);
            for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
              if(strcmp(received_ip,iter->ip_addr) == 0){
                strcpy(iter->status,"logged-in");
                iter->fd = newfd;
                flag = false;
              }
            }            
            if(flag){
              const char *sts = "logged-in";
              struct socket_info sock;
              sock.list_id = information.clients.size()+1;
            
            
            /* Get hostname */
              struct in_addr *addr_temp;
              struct sockaddr_in saddr;
              struct hostent *hoste;
              if(!inet_aton(received_ip,&saddr.sin_addr)){
                cerr<<"inet_aton"<<endl;
                exit(1);
              }
            
              if((hoste = gethostbyaddr((const void *)&saddr.sin_addr,sizeof(received_ip),AF_INET)) == NULL){
                cerr<<"gethostbyaddr"<<endl;
                exit(1);
              }
              char *n = hoste->h_name;
              strcpy(sock.hostname,n);

              /* Other information */
              strcpy(sock.ip_addr,received_ip);
              sock.fd = newfd;
              strncpy(sock.status,sts,strlen(sts));

              char client_port[8];
              bzero(&client_port,sizeof(client_port));
              if(recv(newfd,client_port,sizeof(client_port),0) <= 0){
                cerr<<"port"<<endl;
              }
              sock.port_num = atoi(client_port);

              information.clients.push_back(sock);
            }
            /* Get list information, including hostname, ip, port number. */
            /* Logged in and send list message and buffer message */
              get_send_list_info(received_ip);
          }
        } 
        else {
          char msg[1024];
          bzero(&msg,sizeof(msg));
          /* Handle data from a client */
          if ((nbytes = recv(i, msg, sizeof(msg), 0)) <= 0) {
            /* Got error or connection closed by client */
            if (nbytes == 0) {
              /* Connection closed */
              information.clients_number--;
              for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
                if(iter->fd == i){
                  const char *stso = "logged-out";
                  strncpy(iter->status,stso,strlen(stso));
                }
              }
            } 
            else {
              cerr<<"recv";
            }
            close(i); 
            FD_CLR(i, &master); /* Remove from master set */
          } 
          else {
            /* We got some data from a client */

            char original_msg[1024];
            char buffer_msg[1024];
            bzero(&original_msg,sizeof(original_msg));
            bzero(&buffer_msg,sizeof(buffer_msg));
            strcpy(original_msg,msg);
            strcpy(buffer_msg,msg);
            char *arg_zero = strtok(msg," ");
            
            // switch (arg_zero)
            // {
            // case "SEND":
            //   /* code */
            //    send_message();
            //   break;
            // case "REFRESH":

            //   /* code */
            //    refresh();
            //   break;
            // case "BROADCAST":
            //   /* code */
            //   send_broadcast()
            //   break;
            // case "BLOCK":
            //   /* code */
            //   block_client();
            //   break;
            // case "UNBLOCK":
            //   /* code */
            //   unblock_client();
            //   break;
            
            // default:
            //   break;
            // }
            if(strcmp(arg_zero,"SEND") == 0){
             send_message(i) ;
            }
            else if(strcmp(arg_zero,"REFRESH") == 0){  
            refresh();
            }
            else if(strcmp(arg_zero,"BROADCAST") == 0){
              send_broadcast(i); 
            }
            else if(strcmp(arg_zero,"BLOCK") == 0){
            block_client(i);
            }
            else if(strcmp(arg_zero,"UNBLOCK") == 0){
            unblock_client(i);
            }
          }
        }
      } 
    }
  }
 }

bool server::isvalid(char *server_ip){
  for(list<socket_info>::iterator iter = information.clients.begin();iter != information.clients.end();++iter){
    if(strcmp(server_ip,iter->ip_addr) == 0)
      return true;
  }
  return false;
}




