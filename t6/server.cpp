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

#define DOWN 0
#define RIGHT 1
#define LEFT 2
#define UP 3
#define NONE 4
#define EMPTY 'E'

const int MAX_SNAKES = 10;
const int SNAKE_LEN = 20;
const int MAX_TRIES = 100;
const int mx[] = {1,0,-1,0}, my[] = {0,1,0,-1};
const int n = 30, m = 40;

mutex use_data;
char grid[n][m];

class SNAKE{
	typedef pair<int, int> Node;
	private:
	const int client, id;
	int dir;
	bool running, alive;
	thread * comunicating;
	vector<Node> snake;
	
	static Node rand_pos(){
		return make_pair(rand()%n, rand()%m);
	}
	static Node move(Node no, int d){
		return make_pair(no.first + mx[d], no.second + my[d]);
	}
	static bool ok(vector<Node> & snake){
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
	static void advance(vector<Node> & snake, int dir){
		vector<Node> tmp;
		tmp.push_back(move(snake[0], dir));
		for(int i = 1; i < SNAKE_LEN; i++)
			tmp.push_back(snake[i-1]);
		snake = tmp;
	}
	static void comunicate(SNAKE * s){
		auto & client = s->client;
		auto & dir = s->dir;
		auto & running = s->running;
		auto & alive = s->alive;
		
		char buffer[n*m+100];
		int tt;
		
		auto end = [&](int num){
			cerr << "error " << num << endl;
			s->alive = running = false;
			close(client);
		};
		
		tt = send(client, "1",1,0);
		if(tt < 0){
			end(0);
			return;
		}
		
		tt = recv(client, buffer, sizeof(buffer), 0);
		if(tt < 0){
			end(1);
			return;
		}
		bool random = (buffer[0] == '1');
		
		running = true;
		while(running && alive){
			buffer[0] = '1';
			int ptr = 1;
			for(int i = 0; i < n; i++)
				for(int j = 0; j < m; j++)
					buffer[ptr++] = grid[i][j];
			tt = send(client, buffer, ptr, 0);
			if(tt < 0){
				end(2);
				return;
			}
			if(random)
				dir = rand()%4;
			else{
				tt = recv(client, buffer, sizeof(buffer), 0);
				if(tt < 0){
					end(3);
					break;
				}
				if(buffer[0] - '0' != NONE)
					dir = buffer[0] - '0';
			}
			sleep_for(milliseconds(20));
		}
		
		if(alive)
			send(client, "0", 1, 0);
	}
	
	static void init(SNAKE * s){
		auto & client = s->client;
		auto & id = s->id;
		auto & dir = s->dir;
		auto & running = s->running;
		auto & alive = s->alive;
		auto & snake = s->snake;
		{
			char buffer[10];
			int tt;
			tt = recv(client, buffer, sizeof(buffer), 0);
			if(buffer[0] == '0' || tt != 1){
				if(tt != 1)
					cerr << "Mensagem inesperada do cliente (n, id) = (" << tt << " " << id << "): " << buffer << endl;
				alive = running = false;
				close(client);
				return;
			}
		}
		dir = -1;
		unique_lock<mutex>lck(use_data);
		int cnt = MAX_TRIES;
		while(cnt--){
			snake.clear();
			snake.push_back(rand_pos());
			int ord[] = {0, 1, 2, 3};
			random_shuffle(ord, ord+4);
			for(int i = 0; i < SNAKE_LEN-1; i++){
				for(int k = 0; k < 4; k++){
					snake.push_back(move(snake.back(), ord[k]));
					if(ok(snake))
						break;
					snake.pop_back();
				}
			}
			if((int)snake.size() == SNAKE_LEN){
				auto initial = snake;
				for(int k = 0; k < 4; k++){
					dir = ord[k];
					advance(snake, dir);
					bool res = ok(snake);
					snake = initial;
					if(res)
						break;
					dir = -1;
				}
				if(dir != -1)
					break;
			}
		}
		if(dir == -1){
			send(client, "0", 1, 0);
			alive = running = false;
			close(client);
		}
		else
			comunicate(s);
	}
	
	public:
	SNAKE(int socket_id, int ID):client(socket_id), id(ID), dir(-1), running(false), alive(true){
		if(id == -1){
			send(client, "a", 1, 0);
			alive = false;
			close(client);
			return;
		}
		char buffer[10];
		sprintf(buffer, "%d", id);
		if(send(client, buffer, sizeof(buffer), 0) < 0){
			alive = false;
			cerr << "error 4" << endl;
			close(client);
			return;
		}
		comunicating = new thread(init,this);
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
	bool isalive(){
		return alive;
	}
	bool isrunning(){
		return running;
	}
	int getid(){
		return id;
	}
	int getclient(){
		return client;
	}
	void join(){
		comunicating->join();
	}
	void kill(bool can_restart = false){
		if(!can_restart){
			running = alive = false;
			join();
			close(client);	
			return;	
		}
		running = false;
		join();
		comunicating = new thread(init,this);
	}
};

vector<SNAKE> snakes;

int getID(){
	static bool used[MAX_SNAKES];
	for(int i = 0; i < MAX_SNAKES; i++)
		used[i] = false;
	if((int)snakes.size() > 10*MAX_SNAKES){
		vector<SNAKE> alives_only;
		for(SNAKE & s : snakes)
			if(s.isalive())
				alives_only.emplace_back(s);
			else
				s.join();
		snakes.swap(alives_only);
	}
	for(SNAKE & s : snakes)
		if(s.getid() >= 0 && s.isalive())
			used[s.getid()] = true;
	for(int i = 0; i < MAX_SNAKES; i++)
		if(!used[i])
			return i;
	return -1;
}

bool kill;
int portno;

void connect_clients(){
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
	while(!kill){
		newSocketId = accept(socketId, (struct sockaddr *) &clie_addr, &clienlenght);
		if(newSocketId < 0)
			continue;
		if(kill){
			close(newSocketId);
			break;
		}
		snakes.push_back(SNAKE(newSocketId, getID()));
	}
	for(SNAKE & s : snakes)
		s.kill();
	close(socketId);
}

void getinput(){ //recebe comandos de fim de jogo e de matar cobras
	char key;
	while(!kill){
		/* codar: key = key_pressed() */
		if(key == 'q' || key == 'Q'){
			kill = true;
			int serverid;
			struct sockaddr_in new_serv_addr;
			serverid = socket(AF_INET, SOCK_STREAM, 0);
			memset(&new_serv_addr,0,sizeof(new_serv_addr));
	
			struct hostent *server;
			server = gethostbyname("127.0.0.1");
			new_serv_addr.sin_family = AF_INET;
			new_serv_addr.sin_port = htons(portno);
			memcpy(&new_serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

			connect(serverid, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr));
			
			close(serverid);
		}
			
		if('0' <= key && key <= '9'){
			for(SNAKE & s : snakes){
				if(s.isalive() && s.getid() == key-'0')
					s.kill();
			}
		}
	}
}

void show_end(){} //mostra a tela de fim e aguarda apertar uma tecla

void desenha(){} //desenha o grid

void run(){
	while(!kill){
		static int aux[n][m];
		memset(aux, 0, sizeof(aux));
		unique_lock<mutex>lck(use_data);
		for(SNAKE & s : snakes)
			if(s.isrunning()){
				s.advance();
				auto pos = s.getpos();
				for(auto p : pos){
					if(0 <= p.first && p.first < n && 0 <= p.second && p.second < m)
						aux[p.first][p.second]++;
				}
			}
		for(SNAKE & s : snakes)
			if(s.isrunning()){
				auto pos = s.getpos();
				bool bad = false;
				for(auto p : pos)
					if(!(0 <= p.first && p.first < n && 0 <= p.second && p.second < m)){
						bad = true;
						break;
					}
				auto p = s.head();
				if(!bad && aux[p.first][p.second] > 1)
					bad = true;
				if(bad)
					s.kill(true);
			}
		memset(grid, EMPTY, sizeof(grid));
		for(SNAKE & s : snakes)
			if(s.isrunning()){
				auto pos = s.getpos();
				for(auto p : pos)
					grid[p.first][p.second] = 'a' + s.getid();
				auto p = s.head();
				grid[p.first][p.second] = 'A' + s.getid();
			}
		lck.unlock();
		desenha();
		sleep_for(milliseconds(300));
	}
}

int main (int argc, char *argv[]){
	if(argc != 2){
		cerr << "Formato: " << argv[0] << " [porta]" << endl;
		return 0;
	}
	srand(time(NULL));	
	memset(grid, EMPTY, sizeof(grid));
	vector<thread> threads;
	portno = atoi(argv[1]);
	threads.push_back(thread(connect_clients));
	threads.push_back(thread(getinput));
	threads.push_back(thread(run));
	for(thread & t : threads)
		t.join();
	show_end();
}

