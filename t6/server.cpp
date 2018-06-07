#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cassert>
#include <ncurses.h>

using namespace std;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

#define NONE 4
#define EMPTY '.'

const int MAX_SNAKES = 10;
const int SNAKE_LEN = 20;
const int MAX_TRIES = 100;
const int mx[] = {1,0,0,-1}, my[] = {0,1,-1,0};
const int n = 30, m = 120;

mutex use_data;
char grid[n][m];

class SNAKE{
	typedef pair<int, int> Node;
	private:
	const int client, id;
	int dir, ultdir;
	bool running, alive, killed;
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
			for(int j = i + 1; j < (int)snake.size(); j++)
				if(snake[i] == snake[j])
					return false;
		return true;
	}
	static void advance(vector<Node> & snake, int dir){
		vector<Node> tmp;
		tmp.emplace_back(move(snake[0], dir));
		for(int i = 1; i < SNAKE_LEN; i++)
			tmp.emplace_back(snake[i-1]);
		snake = tmp;
	}
	static void comunicate(SNAKE * s){
		auto & client = s->client;
		auto & dir = s->dir;
		auto & ultdir = s->ultdir;
		auto & running = s->running;
		auto & alive = s->alive;
		auto & killed = s->killed;
		
		char buffer[n*m+100];
		int tt;
		
		auto end = [&](int num){
			cerr << "error " << num << endl;
			if(alive)
				close(client);
			alive = false;
			running = false;
		};
		
		running = true;
		while(running && alive && !killed){
			buffer[0] = '1';
			int ptr = 1;
			use_data.lock();
			for(int i = 0; i < n; i++)
				for(int j = 0; j < m; j++)
					buffer[ptr++] = grid[i][j];
			use_data.unlock();
			tt = send(client, buffer, ptr, 0);
			buffer[ptr] = 0;
			if(tt <= 0){
				end(0);
				return;
			}
			tt = recv(client, buffer, sizeof(buffer), 0);
			if(tt <= 0){
				end(1);
				break;
			}
			int cur = buffer[0] - '0';
			if(cur != NONE && cur != (ultdir^3))
				dir = cur;
			sleep_for(milliseconds(20));
		}
		if(alive && !killed)
			send(client, "0", 1, 0);
	}
	
	static void init(SNAKE * s){
		auto & client = s->client;
		auto & id = s->id;
		auto & dir = s->dir;
		auto & ultdir = s->ultdir;
		auto & running = s->running;
		auto & alive = s->alive;
		auto & killed = s->killed;
		auto & snake = s->snake;
		{
			char buffer[10];
			int tt;
			tt = recv(client, buffer, sizeof(buffer), 0);
			if(buffer[0] == '0' || tt != 1 || killed){
				if(tt != 1)
					cerr << "Mensagem inesperada do cliente (n, id) = (" << tt << " " << id << "): " << buffer << endl;
				if(alive)
					close(client);
				alive = false;
				running = false;
				return;
			}
		}
		dir = -1;
		unique_lock<mutex>lck(use_data);
		int cnt = MAX_TRIES;
		while(cnt--){
			snake.clear();
			snake.emplace_back(rand_pos());
			int ord[] = {0, 1, 2, 3};
			random_shuffle(ord, ord+4);
			for(int i = 0; i < SNAKE_LEN-1; i++){
				for(int k = 0; k < 4; k++){
					snake.emplace_back(move(snake.back(), ord[k]));
					if(ok(snake)){
						if(!i)
							ultdir = ord[k]^3;
						break;
					}
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
		lck.unlock();
		if(dir == -1){
			if(alive)
				close(client);
			alive = false;
			running = false;
		}
		else
			comunicate(s);
	}
	
	public:
	SNAKE(int socket_id, int ID):client(socket_id), id(ID), dir(-1), running(false), alive(true), killed(false){
		if(id == -1){
			send(client, "a", 1, 0);
			if(alive)
				close(client);
			alive = false;
			return;
		}
		char buffer[10];
		sprintf(buffer, "%d", id);
		if(send(client, buffer, sizeof(buffer), 0) < 0){
			cerr << "error 2" << endl;
			if(alive)
				close(client);
			alive = false;
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
		tmp.emplace_back(move(snake[0], dir));
		for(int i = 1; i < SNAKE_LEN; i++)
			tmp.emplace_back(snake[i-1]);
		snake = tmp;
		ultdir = dir;
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
	void kill(bool can_restart = false){
		if(!can_restart){
			killed = true;
			running = false;
			comunicating->join();
			delete comunicating;
			comunicating = NULL;
			if(alive)
				close(client);
			alive = false;
			return;	
		}
		running = false;
		comunicating->join();
		delete comunicating;
		comunicating = new thread(init,this);
	}
	~SNAKE(){
		assert(!alive);
	}
};

vector<SNAKE*> snakes;

int getID(){
	static bool used[MAX_SNAKES];
	for(int i = 0; i < MAX_SNAKES; i++)
		used[i] = false;
	if((int)snakes.size() > 3*MAX_SNAKES){
		vector<SNAKE*> alives_only;
		for(SNAKE * &s : snakes) if(s){
			if(s->isalive())
				alives_only.emplace_back(s);
			else{
				delete s;
				s = NULL;
			}
		}
		snakes.swap(alives_only);
	}
	for(SNAKE * &s : snakes) if(s)
		if(s->getid() >= 0 && s->isalive())
			used[s->getid()] = true;
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

	{
		int option = 1;
		setsockopt(socketId, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	}
	auto binded = ::bind(socketId, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if(binded < 0)
		exit(-1);
	listen(socketId, MAX_SNAKES);
	socklen_t clienlenght = sizeof(struct sockaddr);
	while(!kill){
		newSocketId = accept(socketId, (struct sockaddr *) &clie_addr, &clienlenght);
		if(newSocketId < 0)
			continue;
		if (kill){
			close(newSocketId);
			break;
		}
		snakes.emplace_back(new SNAKE(newSocketId, getID()));
	}
	for(SNAKE * &s : snakes) if(s){
		if(s->isalive())
			s->kill();
		delete s;
		s = NULL;
	}
	close(socketId);
}

namespace UI {
	char input = NONE;
	bool started = false;
	bool not_ended = true;
	thread reading_thread;

	const char QUIT = 'q';

	char get_input(int ch) {
		if (ch >= '0' && ch <= '9')
			return ch;
		switch(ch) {
			case 'q': return QUIT;
			case 'Q': return QUIT;
			default: return NONE;
		}
	}

	void read() {
		int in_ch;
		sleep_for(milliseconds(100));
		while (not_ended) {
			in_ch = getch();
			if (in_ch != ERR)
				UI::input = get_input(in_ch);
			sleep_for(milliseconds(10));
		}
	}

	void init() {
		initscr();
		curs_set(0);

		noecho();
		cbreak();
		keypad(stdscr, true);
		nodelay(stdscr, true);

		started = true;
		reading_thread = thread(read);
	}

	void quit() {
		not_ended = false;
		reading_thread.join();
		endwin();
	}

	void draw() {
		clear();
		move(0, 0);
		printw("you are the server. press Q to quit. press player id to kick him.\n");
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < m; j++)
				printw("%c", grid[i][j]);
			printw("\n");
		}
		refresh();
	}
}

void getinput(){
	while(!kill){
		if (UI::input == UI::QUIT){
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
			
		if (UI::input >= '0' && UI::input <= '9'){
			for(SNAKE * &s : snakes) if(s){
				if(s->isalive() && s->getid() == UI::input-'0'){
					s->kill();
					delete s;
					s = NULL;
				}
			}
		}
	}
}

void run(){
	UI::init();
	while (!kill){
		static int aux[n][m];
		memset(aux, 0, sizeof(aux));
		unique_lock<mutex>lck(use_data);
		for(SNAKE * &s : snakes) if(s)
			if(s->isrunning()){
				s->advance();
				auto pos = s->getpos();
				for(auto p : pos){
					if(0 <= p.first && p.first < n && 0 <= p.second && p.second < m)
						aux[p.first][p.second]++;
				}
			}
		for(SNAKE * &s : snakes) if(s)
			if(s->isrunning()){
				auto pos = s->getpos();
				bool bad = false;
				for(auto p : pos)
					if(!(0 <= p.first && p.first < n && 0 <= p.second && p.second < m)){
						bad = true;
						break;
					}
				auto p = s->head();
				if(!bad && aux[p.first][p.second] > 1)
					bad = true;
				if(bad)
					s->kill(true);
			}
		memset(grid, EMPTY, sizeof(grid));
		for(SNAKE * &s : snakes) if(s){
			if(s->isrunning()){
				auto pos = s->getpos();
				for(auto p : pos)
					grid[p.first][p.second] = 'a' + s->getid();
				auto p = s->head();
				grid[p.first][p.second] = '0' + s->getid();
			}
		}
		lck.unlock();
		UI::draw();
		sleep_for(milliseconds(300));
	}
	UI::quit();
}

int main (int argc, char *argv[]){
	if(argc != 2){
		cerr << "Call format: " << argv[0] << " [port]" << endl;
		return 1;
	}
	srand(time(NULL));	
	memset(grid, EMPTY, sizeof(grid));
	vector<thread> threads;
	portno = atoi(argv[1]);
	threads.emplace_back(thread(connect_clients));
	threads.emplace_back(thread(getinput));
	threads.emplace_back(thread(run));
	for (thread & t : threads)
		t.join();
}
