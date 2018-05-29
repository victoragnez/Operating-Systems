#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <cstring>
#include <vector>
#include <utility>
#include <thread>
#include <ncurses.h>

using namespace std;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

void error(const char *msg){
	perror(msg);
	exit(1);
}

static const int MAX_SNAKES = 10;
const int SNAKE_LEN = 6;
const int MAX_TRIES = 100;
const int mx[] = {1,0,-1,0}, my[] = {0,1,0,-1};

int n, m;
char grid[300][300];

#define DOWN 0
#define RIGHT 1
#define LEFT 2
#define UP 3
#define EMPTY -1

void connect_server(char * host, int portno){
	int serverid;
	struct sockaddr_in new_serv_addr;
	serverid = socket(AF_INET, SOCK_STREAM, 0);
	memset(&new_serv_addr,0,sizeof(new_serv_addr));
	
	struct hostent *server;
	server = gethostbyname(host);
	if(server == NULL){
		cerr << "error getting host" << endl;
		return;
	}
	new_serv_addr.sin_family = AF_INET;
	new_serv_addr.sin_port = htons(portno);
	memcpy(&new_serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
	
	if(connect(serverid, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr)) < 0){
		cerr << "error connecting to host" << endl;
		return;
	}
	
	char buffer[1<<10];
	while(1){
		sprintf(buffer, "vai %d", rand());
		int n = send(serverid, buffer, strlen(buffer), 0);
		sleep_for(milliseconds(20));
		n = recv(serverid, buffer, sizeof(buffer), 0);
	}
}

int main (int argc, char *argv[]){
	if(argc != 3){
		cerr << "Formato: " << argv[0] << " [ip] [porta]" << endl;
		return 0;
	}
	srand(time(NULL));	
	connect_server(argv[1], atoi(argv[2]));
}
