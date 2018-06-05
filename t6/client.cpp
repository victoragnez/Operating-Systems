#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <thread>
#include <stdexcept>
#include <ncurses.h>
#include <random>

std::random_device rd;
std::mt19937 rand_gen(rd());
std::uniform_int_distribution<> uniform_dist(0, 3);

using namespace std;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

const char DOWN = '0';
const char RIGHT = '1';
const char LEFT = '2';
const char UP = '3';
const char NONE = '4';
const char OPT1 = '5';
const char OPT2 = '6';

const int n = 30, m = 120;
char grid[n][m];
int id = -1;

namespace UI {
	char input = NONE;
	bool started = false;
	bool not_ended = true;
	thread reading_thread;

	char get_input(int ch) {
		switch(ch) {
			case KEY_LEFT: return LEFT;
			case KEY_RIGHT: return RIGHT;
			case KEY_UP: return UP;
			case KEY_DOWN: return DOWN;
			case 'z': return OPT1;
			case 'x': return OPT2;
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

		// int term_height, term_width;
		// getmaxyx(stdscr, term_height, term_width);
		// if (term_height < n || term_width < m)
		// 	throw std::runtime_error("Terminal não tem tamanho suficiente");
	}

	void quit() {
		not_ended = false;
		reading_thread.join();
		endwin();
	}

	void menu (bool death) {
		clear();
		int col, row;
		row = n*0.3;
		if (death) {
			mvprintw(row, m*0.4, "GAME OVER");
		} else {
			mvprintw(row, m*0.25, "WELCOME TO SNAKE WORLD");
		}

		col = m*0.35;
		row = n*0.6;
		mvprintw(row, col, "[z] Join Game");
		mvprintw(row+1, col, "[x] Quit");
		refresh();
	} 

	void options() {
		clear();
		int col = m*0.35;
		int row = n*0.4;
		mvprintw(row, col, "[z] Normal Mode");
		mvprintw(row+1, col, "[x] Random Mode");
		refresh();
	}

	void draw() {
		clear();
		move(0, 0);
		printw("you are snake number %d. good luck!\n", id);
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < m; j++)
				printw("%c", grid[i][j]);
			printw("\n");
		}
		refresh();
	}
}

void connect_server(char * host, int portno) {
	struct sockaddr_in new_serv_addr;
	int serverid = socket(AF_INET, SOCK_STREAM, 0);
	memset(&new_serv_addr, 0, sizeof(new_serv_addr));
	
	struct hostent *server;
	server = gethostbyname(host);
	if (server == NULL) {
		cerr << "error getting host " << host << ", port " << portno << endl;
		return;
	}
	new_serv_addr.sin_family = AF_INET;
	new_serv_addr.sin_port = htons(portno);
	memcpy(&new_serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
	
	if (connect(serverid, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr)) < 0) {
		cerr << "error connecting to host " << host << ", port " << portno << endl;
		return;
	}
	
	static char buffer[n*m + 100];
	int n_bytes;
	
	auto end = [&](std::string error) {
		if (UI::started) UI::quit();
		cerr << "connection closed: " << error << endl;
		close(serverid);
	};
	
	// receive id
	n_bytes = recv(serverid, buffer, sizeof(buffer), 0);
	if (n_bytes <= 0) {
		end("failed to receive id from server");
		return;
	}

	if (buffer[0] < '0' || buffer[0] > '9') {
		end("invalid id received from server");
		return;
	}
	
	id = buffer[0] - '0';
	
	bool random = false, death = false;
	char server_input;
	UI::init();
	while (true) {
		UI::input = NONE;
		UI::menu(death);

		// new game ('1') or quit ('0')
		while (UI::input != OPT1 && UI::input != OPT2)
			sleep_for(milliseconds(10));
		server_input = UI::input == OPT1 ? '1' : '0';
		sprintf(buffer, "%c", server_input);

		if (server_input == '1') {
			UI::input = NONE;
			UI::options();
			while (UI::input != OPT1 && UI::input != OPT2)
				sleep_for(milliseconds(10));
			random = (UI::input == OPT2);		
		}

		// send choice to server
		n_bytes = send(serverid, buffer, strlen(buffer), 0);
		if (n_bytes <= 0) {
			end("failed to send UI::menu choice to server");
			return;
		}

		if (server_input == '0') break;

		UI::input = NONE;
		while (true) {
			// receive flag indicating if still alive, followed by game grid
			n_bytes = recv(serverid, buffer, sizeof(buffer), 0); 
			if (n_bytes <= 0) {
				end("failed to receive game data from server");
				return;
			}
			if (buffer[0] == '0') break;
			
			int ptr = 1;
			for (int i = 0; i < n; i++)
				for (int j = 0; j < m; j++)
					grid[i][j] = buffer[ptr++];
			UI::draw();

			// send direction input
			if (!random) {
				if (UI::input < '0' || UI::input > '3')
					server_input = NONE;
				else
					server_input = UI::input;
				UI::input = NONE;
			}
			else server_input = uniform_dist(rand_gen)%4 + '0';
			sprintf(buffer, "%c", server_input);
			n_bytes = send(serverid, buffer, strlen(buffer), 0);
			if (n_bytes <= 0) {
				end("failed to send input to server");
				return;
			}
		}
		death = true;
	}
	UI::quit();
	close(serverid);
}

int main (int argc, char *argv[]) {
	if(argc != 3) {
		cerr << "Formato: " << argv[0] << " [ip] [porta]" << endl;
		return EXIT_FAILURE;
	}
	thread game(connect_server, argv[1], atoi(argv[2]));
	game.join();

	return EXIT_SUCCESS;
}
