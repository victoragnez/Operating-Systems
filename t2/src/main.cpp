#include <iostream>
#include <pwd.h>
#include "GPIO.h"
#include "CPU.h"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;
using std::chrono::seconds;

static int max_cpu = -1;
static auto green_led = GPIO("P9_22", GPIO::Output); // 2
static auto yellow_led = GPIO("P9_26", GPIO::Output); // 14
static auto red_led = GPIO("P9_30", GPIO::Output); // 112

bool panic() {
	std::cout << "panic!\n";
	if (max_cpu < 0) {
		std::cout << "insufficient CPU load for panic\n";
		return false;
	}

	auto top = CPU::top_process();

	char command[1024];
	sprintf(command,"kill -9 %d", top.pid);
	system(command);
	std::cerr << "process " << top.pid << " killed. It belonged to user " << 
	getpwuid(top.user)->pw_name << ", and was consuming " << top.cpu << "%% of CPU." << std::endl;

	return true;
}

void set_leds(bool g, bool y, bool r) {
	green_led.set_value((g ? GPIO::High : GPIO::Low));
	yellow_led.set_value((y ? GPIO::High : GPIO::Low));
	red_led.set_value((r ? GPIO::High : GPIO::Low));
}

void watch_CPU() {
	auto cpu = CPU::percent();

	if (cpu < 25) {
		std::cout << "low\n";
		set_leds(1, 0, 0);
		max_cpu = -1;
	} else if (cpu < 50) {
		std::cout << "middle\n";
		set_leds(0, 1, 0);
		max_cpu = -1;
	} else if (cpu < 75) {
		std::cout << "high\n";
		set_leds(0, 0, 1);
		max_cpu = -1;
	} else {
		std::cout <<"max\n";
		if (max_cpu == -1)
			max_cpu = 0;

		if (max_cpu == 0) {
			set_leds(1, 1, 1);
			max_cpu = 1;
		} else {
			set_leds(0, 0, 0);
			max_cpu = 0;
		}
	}
}

int main()
{
	auto button = GPIO("P9_14", GPIO::Input);	

	auto queries = 100;
	auto button_checks = 10;
	auto panic_sleep = 2;

	auto last_value = GPIO::Low;
	for (auto i = 0; i < queries; ++i) {
		for (auto b = 0; b < button_checks; ++b) {
			auto curr = button.get_value();
			if (last_value == GPIO::High && curr == GPIO::Low) {
				if (panic()) {
					set_leds(0, 0, 0);
					sleep_for(seconds(panic_sleep));
					last_value = curr;
					break;
				}
			}
			last_value = curr;
			sleep_for(milliseconds(100));
		}

		watch_CPU();
	}

	return 0;
}