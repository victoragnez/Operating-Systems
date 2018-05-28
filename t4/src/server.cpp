#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <thread>
#include <pthread.h>
#include <cstring>
#include <ncurses.h>
#include "POINT.h"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::thread;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

bool not_end, running, button_pressed;
double v1, v2, curr_light;
auto frametime = milliseconds(1000)/20.0;

enum Light {
	Low,
	Medium,
	High
};

void set_light(Light l)
{
	if (l == Low)
		attrset(A_DIM);
	if (l == Medium)
		attrset(A_NORMAL);
	if (l == High)
		attrset(A_BOLD);
}

POINT ball, racket1, racket2, direction;
int points1, points2;
int scr_height, scr_width, racket_half_height;
const int margin = 1;

void setup()
{
	initscr();
	noecho();
	curs_set(0);

	getmaxyx(stdscr, scr_height, scr_width);

	racket_half_height = scr_height/16;

	racket1.x = margin;
	racket2.x = scr_width - margin;
}

void move_racket(POINT & p, double v)
{
	p.y = round(v*(scr_height - 2*(racket_half_height + 1 + margin)) + margin + racket_half_height);
}

void move_rackets()
{
	move_racket(racket1, v1);
	move_racket(racket2, v2);
}

void init_ball(bool p1)
{
	ball.x = scr_width / 2;
	ball.y = scr_height / 2;
	if (p1)
		direction = POINT(-1,0);
	else
		direction = POINT(1,0);
}

void move_ball()
{
	ball = ball + direction;

	if ((int)round(ball.y) <= margin || (int)round(ball.y) >= scr_height - 1 - margin)
		direction.y = -direction.y;
	
	if ((int)round(ball.x) <= (int)racket1.x) {
		if ((int)racket1.y - racket_half_height <= (int)round(ball.y) && (int)round(ball.y) <= (int)racket1.y + racket_half_height) {
			direction = POINT(racket_half_height, round(ball.y) - racket1.y);
			direction = direction / direction.abs();
		}
		else {
			init_ball(0);
			points2++;
		}
	}
	else if ((int)round(ball.x) >= (int)racket2.x) {
		if ((int)racket2.y - racket_half_height <= (int)round(ball.y) && (int)round(ball.y) <= (int)racket2.y + racket_half_height) {
			direction = POINT(-racket_half_height, round(ball.y) - racket2.y);
			direction = direction / direction.abs();
		}
		else {
			init_ball(1);
			points1++;
		}
	}
}

enum ScreenType{
	Begin,
	Run,
	End
};

void build_screen(ScreenType tp)
{
	clear();
	if (tp == Begin) {
		char s[] = "Aperte o botão para começar!";
		mvprintw(scr_height / 2, scr_width / 2 - strlen(s) / 2, s);
	}
	if (tp == Run) {
		mvvline(int(racket1.y - racket_half_height), int(racket1.x), ACS_CKBOARD, 2*racket_half_height + 1);
		mvvline(int(racket2.y - racket_half_height), int(racket2.x), ACS_CKBOARD, 2*racket_half_height + 1);
		mvvline(int(round(ball.y)), int(round(ball.x)), ACS_CKBOARD, 1);
		char s[16];
		sprintf(s, "%d x %d", points1, points2);
		mvprintw(scr_height - 1, scr_width / 2 - strlen(s) / 2, s);
	}
	if (tp == End) {
		char s[128];
	
		sprintf(s, "Fim de jogo!");
		mvprintw(1, scr_width / 2 - strlen(s) / 2, s);
	
		if (points1 > points2)
			sprintf(s,"Jogador 1 ganhou!");
		else
			sprintf(s,"Jogador 2 ganhou!");		
		mvprintw(scr_height / 2 - 1, scr_width / 2 - strlen(s) / 2, s);
	
		sprintf(s, "%d x %d", points1, points2);
		mvprintw(scr_height / 2, scr_width / 2 - strlen(s) / 2, s);
	
		sprintf(s, "Aperte o botão para sair");
		mvprintw(scr_height / 2 + 1, scr_width / 2 - strlen(s) / 2, s);
	}
	refresh();
}

void run()
{
	button_pressed = false;
	not_end = true;
	running = false;
	setup();
	build_screen(ScreenType::Begin);
	init_ball(1);
	points1 = points2 = 0;
	auto last_time = std::chrono::steady_clock::now();
	auto time = std::chrono::duration<double>::zero();
	
	bool playing = true;
	while (playing) {
		if (running) {
			if (button_pressed) {
				running = false;
				button_pressed = false;
			}
			else {
				if (curr_light < 0.1) 
					set_light(Light::Low);
				else if (curr_light < 0.6) 
					set_light(Light::Medium);
				else 
					set_light(Light::High);

				move_rackets();
				move_ball();
	
				if (points1 >= 5 || points2 >= 5)
					playing = false;

				build_screen(ScreenType::Run);
			}
		}
		else {
			if (button_pressed) {
				running = true;
				button_pressed = false;
			}	
		}

		auto now = std::chrono::steady_clock::now();
		time = now - last_time;
		last_time = now;
		if (time < frametime)
			sleep_for(frametime-time);
	}
	build_screen(ScreenType::End);
	button_pressed = false;
	while (!button_pressed)
		sleep_for(frametime);
	not_end = false;
	endwin();
}

void comunicate(){
	int sockfd, newsockfd, portno = 12346;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0) 
		error("ERROR on binding");

	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd, 
			(struct sockaddr *) &cli_addr, 
			&clilen);
	if (newsockfd < 0) 
		error("ERROR on accept");
	while(not_end){
		n = write(newsockfd,"1",1);
		if (n < 0) error("ERROR writing to socket");
		char buffer[64];
		bzero(buffer,64);
		n = read(newsockfd,buffer,63);
		if (n < 0) error("ERROR reading from socket");
		int button;
		sscanf(buffer, "%lf,%lf,%lf,%d", &v1, &v2, &curr_light, &button);
		if(button == 1)
			button_pressed = true;
		
		sleep_for(milliseconds(50));
	}
	n = write(newsockfd,"0",1);
	if (n < 0) error("ERROR writing to socket");
	
	close(newsockfd);
	close(sockfd);
}

int main()
{
	thread comunication(comunicate);
	thread game(run);
	comunication.join();
	game.join();
}
