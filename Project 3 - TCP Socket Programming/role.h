#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

int actAsServer(const char * port);
int acceptConnection(int socket_fd, string * ip);
int actAsClient(const char * hostname, const char * port);
string getIPAddr(int socket_fd);
int getPort(int socket_fd);
