#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

class Potato{
  public:
    int hops;
    int hop_count;
    int id_path[512];
    Potato(int hops): hops(hops), hop_count(0){
      memset(&id_path,0,sizeof(id_path));
    }
};
