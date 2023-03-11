#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>

#include "role.h"
#include "potato.h"

using namespace std;

int main(int argc, char * argv[]){
  if(argc != 4){
    cerr << "usage: ringmaster <port_num> <num_players> <num_hops>" <<endl;
    exit(EXIT_FAILURE);
  }
  
  const char * port_num = argv[1];
  int num_players = atoi(argv[2]);
  int num_hops = atoi(argv[3]);
  
  if(num_players <= 0){
    cerr << "Number of players should be greater than 1" << endl;
    exit(EXIT_FAILURE);
  }
  
  if(atoi(argv[2]) != atof(argv[2])){
    cerr << "Number of players should be an integer" << endl;
    exit(EXIT_FAILURE);
  }
  
  if(num_hops < 0 || num_hops > 512){
    cerr << "Number of hops should be greater than or equal to zero and less than or equal to 512" << endl;
    exit(EXIT_FAILURE);
  }
  
  if(atoi(argv[3]) != atof(argv[3])){
    cerr << "Number of hops should be an integer" << endl;
    exit(EXIT_FAILURE);
  }
  
  cout << "Potato Ringmaster" << endl;
  cout << "Players = " << num_players << endl;
  cout << "Hops = " << num_hops << endl;
  
  int master_fd = actAsServer(port_num);
  int player_connect_fds[num_players];
  string player_ips[num_players];
  int player_ports[num_players];
  for(int i = 0; i < num_players; i++){
    //ringmaster sends ids and num of players to each player
    string player_ip;
    int player_connect_fd = acceptConnection(master_fd,&player_ip);
    player_connect_fds[i] = player_connect_fd;
    send(player_connect_fd, &i, sizeof(i), 0);
    send(player_connect_fd, &num_players, sizeof(num_players), 0);
    
    //ringmaster receives ip address and port from each player
    int player_port;
    //recv(player_connect_fd, &player_ip, sizeof(player_ip), MSG_WAITALL);
    recv(player_connect_fd, &player_port, sizeof(player_port), MSG_WAITALL);
    player_ips[i] = player_ip;
    player_ports[i] = player_port;
    cout << "Player " << i << " is ready to play" << endl;
  }

  for(int i = 0; i < num_players; i++){
    int neighbor_id = (i+1)%num_players;
    char neighbor_ip[100];
    strcpy(neighbor_ip, player_ips[neighbor_id].c_str());
    int neighbor_port = player_ports[neighbor_id];
    send(player_connect_fds[i],&neighbor_ip,sizeof(neighbor_ip),0);
    send(player_connect_fds[i],&neighbor_port,sizeof(neighbor_port),0);
  }
  
  //start the game
  Potato potato(num_hops);
  if(num_hops != 0){
    srand((unsigned int)time(NULL) + num_players);
    int num = rand() % num_players;
    cout << "Ready to start the game, sending potato to player " << num << endl;
    send(player_connect_fds[num],&potato,sizeof(potato),0);
    
    //wait for potato
    fd_set readfds;
    int maxfdp1 = 0;
    for(int i = 0; i < num_players; i++){
      if(player_connect_fds[i] + 1 > maxfdp1){
        maxfdp1 = player_connect_fds[i] + 1;
      }
    }
    FD_ZERO(&readfds);
    for (int i = 0; i < num_players; i++) {
      FD_SET(player_connect_fds[i], &readfds);
    }
    select(maxfdp1, &readfds, NULL, NULL, NULL);
    for (int i = 0; i < num_players; i++) {
      if (FD_ISSET(player_connect_fds[i], &readfds)) {
        recv(player_connect_fds[i], &potato, sizeof(potato), MSG_WAITALL);
        break;
      }
    }
  }
  
  for(int i = 0; i < num_players; i++){
    send(player_connect_fds[i],&potato,sizeof(potato),0);
  }
  
  if(potato.hop_count != 0){
    cout << "Trace of potato:" << endl;
  
    for(int i = 0; i < potato.hop_count; i++){
      if(i != potato.hop_count-1){
        cout << potato.id_path[i] << ", ";
      }else{
        cout << potato.id_path[i];
      }
    }
    cout << endl;
  }
  
  for (int i = 0;i < num_players; i++) {
    close(player_connect_fds[i]);
  }
  close(master_fd);
  
  return 0;
}
