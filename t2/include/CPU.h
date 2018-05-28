#ifndef __CPU_H__
#define __CPU_H__

#include <ios>
#include <cassert>
#include <chrono>
#include <thread>

namespace CPU
{
	extern bool init;
	extern int times[8];
	
	struct Process {
		int pid, user;
		float cpu;
	};

	double percent(double init_interval=1.0);
	Process top_process();
}

#endif