#include "CPU.h"

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

bool CPU::init = false;
int CPU::times[8];

double CPU::percent(double init_interval) {
	char file_path[] = "/proc/stat";

	if (!init) {
		auto file = fopen(file_path, "r");
		fscanf(file, "%*s");
		for(int i = 0; i < 8; i++)
			assert(fscanf(file, "%d", times+i)!=EOF);
		fclose(file);

		int milli_interval = init_interval*1000;
		sleep_for(milliseconds(milli_interval));

		init = true;
	}

	double total = 0;
	int diff[8];

	auto file = fopen(file_path, "r");
	fscanf(file, "%*s");
	for(int i = 0; i < 8; i++) {
		int new_time;
		assert(fscanf(file, "%d", &new_time)!=EOF);
		diff[i] = times[i] - new_time;
		total += diff[i];
		times[i] = new_time;
	}
	fclose(file);
	return (total - diff[3] - diff[4]) * 100.0 / total;
}

CPU::Process CPU::top_process() {
	FILE * read;
	Process top;
	read = popen("ps -ax -o uid,pid,%cpu, --no-headers --sort=-%cpu | head -n 1", "r");
	if(!read)
		throw std::ios_base::failure("error reading ps output");
	assert(fscanf(read,"%d%d%f", &top.user, &top.pid, &top.cpu)!=EOF);
	pclose(read);

	return top;

}

void func() {
	CPU::init = false;
}