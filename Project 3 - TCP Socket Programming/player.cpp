#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "potato.h"
#include "role.h"
using namespace std;

int main(int argc, char * argv[]){
  if(argc != 3){
    cerr << "usage: player <machine_name> <port_num>" << endl;
    exit(EXIT_FAILURE);
  }
  const char * hostname = argv[1];
  const char * port = argv[2];
  int master_fd = actAsClient(hostname,port);
  int my_id;
  int num_players;
  recv(master_fd, &my_id, sizeof(my_id), 0);
  recv(master_fd, &num_players, sizeof(num_players), 0);
  cout << "Connected as player " << my_id << " out of " << num_players << " total players" << endl;
  
  //send my server's port to ringmaster
  int server_fd = actAsServer("");
  int player_port = getPort(server_fd);
  send(master_fd,&player_port,sizeof(player_port),0);

  //receive my neighbor's ip address and port
  char right_neighbor_ip[100];
  int right_neighbor_port;
  recv(master_fd, &right_neighbor_ip, sizeof(right_neighbor_ip),MSG_WAITALL);
  recv(master_fd, &right_neighbor_port, sizeof(right_neighbor_port),MSG_WAITALL);
  
  //make connection to my right neighbor
  string temp = to_string(right_neighbor_port);
  const char * temp_port = temp.c_str();
  int right_neighbor_fd = actAsClient(right_neighbor_ip,temp_port);
  
  //make connection to my left neighbor
  string left_neighbor_ip;
  int left_neighbor_fd = acceptConnection(server_fd, &left_neighbor_ip);
  
  //start the game
  Potato potato(0);
  int sockset[3] = {master_fd,left_neighbor_fd,right_neighbor_fd};
  while(1){
    fd_set readfds;
    FD_ZERO(&readfds);
    int maxfdp1 = 0;
    for(int i = 0; i < 3; i++){
      FD_SET(sockset[i],&readfds);
      if(sockset[i]+1>maxfdp1){
        maxfdp1 = sockset[i] + 1;
      }
    }
    select(maxfdp1,&readfds,NULL,NULL,NULL);
    for(int i = 0; i < 3; i++){
      if(FD_ISSET(sockset[i],&readfds)){
        recv(sockset[i],&potato,sizeof(potato),MSG_WAITALL);
        break;
      }
    }
    //shut down all the players
    if(potato.hops == 0){
      break;
    }
    //send potato to player or ringmaster based on hops
    potato.hops--;
    potato.id_path[potato.hop_count] = my_id;
    potato.hop_count++;
    if(potato.hops == 0){
      send(sockset[0],&potato,sizeof(potato),0);
      cout << "I'm it" << endl;
    }else{
      srand((unsigned int)time(NULL) + potato.hop_count);
      int num = rand() % 2;
      if(num == 0){
        send(sockset[1],&potato,sizeof(potato),0);
        cout << "Sending potato to " << (my_id + num_players - 1)%num_players << endl;
      }else{
        send(sockset[2],&potato,sizeof(potato),0);
        cout << "Sending potato to " << (my_id + 1)%num_players << endl;
      }
    }
  }
  
  for(int i = 0;i < 3; i++){
    close(sockset[i]);
  }
  close(server_fd);
  
  return 0;
}
