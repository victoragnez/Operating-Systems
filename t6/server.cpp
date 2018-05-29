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
const int SNAKE_LEN = 15;
const int MAX_TRIES = 100;
const int mx[] = {1,0,-1,0}, my[] = {0,1,0,-1};

int n = 30, m = 40;
char grid[300][300];

#define DOWN 0
#define RIGHT 1
#define LEFT 2
#define UP 3
#define EMPTY -1

class SNAKE{
	typedef pair<int, int> Node;
	private:
	const int client, id;
	int dir;
	bool running;
	thread * comunicating;
	vector<Node> snake;
	
	Node rand_pos(){
		return make_pair(rand()%n, rand()%m);
	}
	Node move(Node no, int d){
		return make_pair(no.first + mx[d], no.second + my[d]);
	}
	bool ok(){
		for(int i = 0; i < (int)snake.size(); i++)
			if(snake[i].first < 0 || snake[i].first >= n ||
				snake[i].second < 0 || snake[i].second >= m ||
				grid[snake[i].first][snake[i].second] != EMPTY)
						return false;
		for(int i = 0; i < (int)snake.size(); i++)
			for(int j = 0; j < (int)snake.size(); j++)
				if(snake[i] == snake[j])
					return false;
		return true;
	}
	
	static void comunicate(bool & running, const int & client){
		char buffer[1<<10];
		while(running){
			int n = recv(client, buffer, sizeof(buffer), 0);
			sprintf(buffer, "OK %d", rand());
			n = send(client, buffer, strlen(buffer), 0);
			sleep_for(milliseconds(1000));
		}
		close(client);
	}
	
	public:
	SNAKE(int socket_id, int ID):client(socket_id), id(ID), dir(-1), running(true){
		comunicating = new thread(comunicate, ref(running), ref(client));
		int cnt = MAX_TRIES;
		while(cnt--){
			snake.clear();
			snake.push_back(rand_pos());
			int ord[] = {0, 1, 2, 3};
			random_shuffle(ord, ord+4);
			for(int i = 0; i < SNAKE_LEN-1; i++){
				for(int k = 0; k < 4; k++){
					snake.push_back(move(snake.back(), ord[k]));
					if(ok())
						break;
					snake.pop_back();
				}
			}
			if((int)snake.size() == SNAKE_LEN){
				auto initial = snake;
				for(int k = 0; k < 4; k++){
					dir = ord[k];
					advance();
					bool res = ok();
					snake = initial;
					if(res)
						break;
					dir = -1;
				}
				if(dir != -1)
					break;
			}
		}
		if(dir == -1 || id == -1)
			running = false;
	}
	vector<Node> getpos(){
		return snake;
	}
	Node head(){
		return snake[0];
	}
	void advance(){
		vector<Node> tmp;
		tmp.push_back(move(snake[0], dir));
		for(int i = 1; i < SNAKE_LEN; i++)
			tmp.push_back(snake[i-1]);
		snake = tmp;
	}
	bool alive(){
		return running;
	}
	int getid(){
		return id;
	}
	void join(){
		comunicating->join();
	}
};

mutex use_data;
vector<SNAKE> snakes;

int getID(){
	static bool used[MAX_SNAKES];
	for(int i = 0; i < MAX_SNAKES; i++)
		used[i] = false;
	if((int)snakes.size() > 10*MAX_SNAKES){
		vector<SNAKE> alives_only;
		for(int i = 0; i < (int)snakes.size(); i++)
			if(snakes[i].alive())
				alives_only.emplace_back(snakes[i]);
			else
				snakes[i].join();
		snakes.swap(alives_only);
	}
	for(int i = 0; i < (int)snakes.size(); i++)
		if(snakes[i].getid() >= 0 && snakes[i].alive())
			used[snakes[i].getid()] = true;
	for(int i = 0; i < MAX_SNAKES; i++)
		if(!used[i])
			return i;
	return -1;
}

void connect_clients(int portno){
	int socketId, newSocketId;
	struct sockaddr_in serv_addr;
	struct sockaddr	clie_addr;

	socketId = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr,0,sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	int binded = bind(socketId, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if(binded < 0)
		exit(-1);
	listen(socketId, MAX_SNAKES);
	socklen_t clienlenght = sizeof(struct sockaddr);
	while(1){
		newSocketId = accept(socketId, (struct sockaddr *) &clie_addr, &clienlenght);
		if(newSocketId < 0)
			continue;
		use_data.lock();
		snakes.push_back(SNAKE(newSocketId, getID()));
		use_data.unlock();
	}
}

int main (int argc, char *argv[]){
	if(argc != 2){
		cerr << "Formato: " << argv[0] << " [porta]" << endl;
		return 0;
	}
	srand(time(NULL));	
	memset(grid, EMPTY, sizeof(grid));
	connect_clients(atoi(argv[1]));
}
