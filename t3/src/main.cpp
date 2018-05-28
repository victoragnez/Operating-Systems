#include <iostream>
#include <thread>
#include <pthread.h>
#include <cstring>
#include <ncurses.h>
#include "POINT.h"
#include "GPIO.h"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::thread;

static void set_scheduling(thread &th, int policy, int priority)
{
	sched_param sch_params;
	sch_params.sched_priority = priority;
	if (pthread_setschedparam(th.native_handle(), policy, &sch_params)) {
		std::cerr << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
	}
}

GPIO::Value curr_button = GPIO::Low;
bool end_button=false;
bool not_end=true, running=false, button_pressed=false;
double v1, v2, curr_light;
auto frametime = milliseconds(1000)/30.0;

void get_value(GPIO & gpio, double & v)
{
	while (not_end) {
		if (running)
			v = gpio.get_value();
		sleep_for(milliseconds(50));
	}
}

void get(GPIO & gpio, GPIO::Value & curr_button)
{
	while (!end_button) {
		GPIO::Value prev_button = curr_button;
		curr_button = gpio.get();
		if (prev_button == GPIO::High && curr_button == GPIO::Low)
			button_pressed = true;
		sleep_for(milliseconds(50));
	}
}

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
		
		sprintf(s, "Aperte o botão uma vez para sair");
		mvprintw(scr_height / 2 + 1, scr_width / 2 - strlen(s) / 2, s);
	}
	refresh();
}

void run()
{
	setup();

	button_pressed = false;
	build_screen(ScreenType::Begin);
	init_ball(1);
	points1 = points2 = 0;
	auto last_time = std::chrono::steady_clock::now();
	auto time = std::chrono::duration<double>::zero();

	while (not_end) {
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
					not_end = false;

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
	end_button = true;
	endwin();
}

int main()
{
	auto GPIO_play1 = GPIO("P9_33");
	auto GPIO_play2 = GPIO("P9_35");
	auto GPIO_light = GPIO("P9_40");
	auto GPIO_button = GPIO("P9_41");

	thread play1(get_value, std::ref(GPIO_play1), std::ref(v1));
	set_scheduling(play1, SCHED_RR, 20);
	
	thread play2(get_value, std::ref(GPIO_play2), std::ref(v2));
	set_scheduling(play2, SCHED_RR, 20);
	
	thread light(get_value, std::ref(GPIO_light), std::ref(curr_light));
	set_scheduling(light, SCHED_RR, 50);
	
	thread button(get, std::ref(GPIO_button), std::ref(curr_button));
	set_scheduling(button, SCHED_RR, 50);

	thread game(run);
	set_scheduling(game, SCHED_RR, 5);
	
	game.join();
	play1.join();
	play2.join();
	button.join();
	light.join();
	
	return EXIT_SUCCESS;
}

