#include <bits/stdc++.h>
#include <ncurses.h>
#include <pwd.h>
using namespace std;
using namespace chrono;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

bool not_ended = true;

namespace Process{

	struct process {
		int pid, uid;
		float mem;
		int min_page_flt, maj_page_flt;
	};

	vector<process> procs;

	void getProcs() {
		procs.clear();
		char buffer[1<<8];
		sprintf(buffer,"ps -ax -o pid,uid,%%mem,min_flt,maj_flt --no-headers --sort=-%%mem");
		FILE * read;
		read = popen(buffer, "r");
		if(!read)
			throw std::ios_base::failure("error reading ps output");
		while(1){
			process cur;
			if(fscanf(read, "%d %d %f %d %d", &cur.pid, &cur.uid, &cur.mem, &cur.min_page_flt, &cur.maj_page_flt) == EOF)
				break;
			procs.push_back(cur);
		}
		pclose(read);
	}

};

namespace Cache{

	struct cache {
		char type[1<<6], size[1<<6];
	};

	vector<cache> caches;

	void getCache() {
		caches.clear();
		FILE * read;
		read = popen("lscpu | grep cache", "r");
		if(!read)
			throw std::ios_base::failure("error reading scpu output");
		while(1){
			cache cur;
			if(fscanf(read, "%s %*s %s", cur.type, cur.size) == EOF)
				break;
			caches.push_back(cur);
		}
		pclose(read);
	}

};

namespace MemStats {
	int memtotal, memused, memfree, membuff, memcached;
	int swaptotal, swapused, swapfree, swapcached;
	
	void getStats() {
		memcached = 0;
		FILE * read;
		read = fopen("/proc/meminfo", "r");
		if(!read)
			throw std::ios_base::failure("error reading /proc/meminfo");
		while(1){
			char S[1<<6];
			if(fscanf(read, "%s", S) == EOF)
				break;
			string s(S);
			if(s == "MemTotal:")
				fscanf(read, "%d", &memtotal);
			if(s == "MemFree:")
				fscanf(read, "%d", &memfree);
			if(s == "Buffers:")
				fscanf(read, "%d", &membuff);
			if(s == "Cached:"){
				int t;
				fscanf(read, "%d", &t);
				memcached += t;
			}
			if(s == "SReclaimable:"){
				int t;
				fscanf(read, "%d", &t);
				memcached += t;
			}
			if(s == "Shmem:"){
				int t;
				fscanf(read, "%d", &t);
				memcached -= t;
			}
			if(s == "SwapTotal:")
				fscanf(read, "%d", &swaptotal);
			if(s == "SwapFree:")
				fscanf(read, "%d", &swapfree);
			if(s == "SwapCached:")
				fscanf(read, "%d", &swapcached);
		}
		memused = memtotal - memfree - memcached - membuff;
		swapused = swaptotal - swapfree - swapcached;
		fclose(read);
	}
	
};

namespace UI {

	unsigned height, width;
	unsigned n_procs;
	thread in_thread;
	bool quit = false;

	const int LEFT_MARGIN = 1;
	const int BARS_SPACE = 4;
	const int BAR_LENGTH = 100;
	const int NEUTRAL_COLOR = 99;

	void get_input() {
		while (!quit) {
			int ch = getch();
			if (ch == KEY_ENTER || ch == '\n')
				quit = true;
		}
	}

	void setup() {
		initscr();
		curs_set(0);
		start_color();

		noecho();
		cbreak();
		keypad(stdscr, TRUE);

		init_pair(99, COLOR_WHITE, COLOR_BLACK);
		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(3, COLOR_CYAN, COLOR_BLACK);

		getmaxyx(stdscr, height, width);
		n_procs = height - (BARS_SPACE+1);
		in_thread = thread(get_input);
		in_thread.detach();
	}

	void exit() {
		endwin();
	}
	/*
	void fake_stats() {
		MemStats::memtotal = 10;
		MemStats::membuff = 1;
		MemStats::memcached = 1;
		MemStats::memused = 1;

		MemStats::swaptotal = 10;
		MemStats::swapcached = 1;
		MemStats::swapused = 1;
	}*/

	void bar(int* sections, int n_sections, int line) {
		int shift = LEFT_MARGIN;
		for (auto i = 0; i < n_sections; ++i) {
			auto color = (i == n_sections-1) ? NEUTRAL_COLOR : i+1;
			mvhline(line, shift, ('|' | COLOR_PAIR(color)), sections[i]);
			shift += sections[i];
		}
	}

	void RAM() {
		// RAM bar section lengths
		int s[4];
		double total = MemStats::memtotal;
		s[0] = (MemStats::memused/total) * BAR_LENGTH;
		s[1] = (MemStats::memcached/total) * BAR_LENGTH;
		s[2] = (MemStats::membuff/total) * BAR_LENGTH;
		s[3] = BAR_LENGTH - (s[0]+s[1]+s[2]);

		// RAM bar header
		mvprintw(0, LEFT_MARGIN, "RAM: %iMb, %iMb, %iMb, %iMb (", MemStats::memused/1000,
			MemStats::memcached/1000, MemStats::membuff/1000, MemStats::memfree/1000);
		attron(COLOR_PAIR(1)); printw("in use, ");
		attron(COLOR_PAIR(2)); printw("cached, ");
		attron(COLOR_PAIR(3)); printw("buffered, ");
		attron(COLOR_PAIR(NEUTRAL_COLOR)); printw("free)");

		bar(s, 4, 1);
	}

	void swap() {
		// swap bar section lengths
		int s[3];
		double total = MemStats::swaptotal;
		s[0] = (MemStats::swapused/total) * BAR_LENGTH;
		s[1] = (MemStats::swapcached/total) * BAR_LENGTH;
		s[2] = BAR_LENGTH - (s[0]+s[1]);

		// swap bar header
		mvprintw(2, LEFT_MARGIN, "SWAP: %iMb, %iMb, %iMb (", MemStats::swapused/1000,
			MemStats::swapcached/1000, MemStats::swapfree/1000);
		attron(COLOR_PAIR(1)); printw("in use, ");
		attron(COLOR_PAIR(2)); printw("cached, ");
		attron(COLOR_PAIR(NEUTRAL_COLOR)); printw("free)");

		bar(s, 3, 3);
	}

	void top_procs() {
		move(BARS_SPACE, LEFT_MARGIN);
		printw("pid\tuid\t%%mem\tminor PFs\tmajor PFs\tuser");
		for (unsigned i = 0; i < n_procs && i < Process::procs.size(); ++i) {
			auto proc = Process::procs[i];
			move(BARS_SPACE+i+1, LEFT_MARGIN);
			printw("%d\t%d\t%.2f\t%9d\t%9d\t%s", proc.pid, proc.uid, proc.mem,
				proc.min_page_flt, proc.maj_page_flt, getpwuid(proc.uid)->pw_name);
		}
	}

	void build() {
		// fake_stats();
		RAM();
		swap();
		top_procs();
	}

	void draw () {
		clear();
		build();
		refresh();
	}
};

void export_json() {
	FILE * write = fopen("out.json", "w");
	fprintf(write,"{\n");
	fprintf(write, "\"Memory Info\": {\n");
	fprintf(write, "    \"Total Memory (kB)\": \"%d\",\n", MemStats::memtotal);
	fprintf(write, "    \"Memory Used (kB)\": \"%d\",\n", MemStats::memused);
	fprintf(write, "    \"Memory Free (kB)\": \"%d\",\n", MemStats::memfree);
	fprintf(write, "    \"Memory for Buffers (kB)\": \"%d\",\n", MemStats::membuff);
	fprintf(write, "    \"Memory Cached (kB)\": \"%d\",\n", MemStats::memcached);
	fprintf(write, "    \"Total Swap (kB)\": \"%d\",\n", MemStats::swaptotal);
	fprintf(write, "    \"Used Swap (kB)\": \"%d\",\n", MemStats::swapused);
	fprintf(write, "    \"Cached Swap (kB)\": \"%d\",\n", MemStats::swapcached);
	fprintf(write, "    \"Free Swap (kB)\": \"%d\",\n", MemStats::swapfree);
	fprintf(write, "    \"Cache Memory\": {\n");
	for(int i = 0; i < (int)Cache::caches.size(); i++){
		Cache::cache c = Cache::caches[i];
		fprintf(write, "        \"%s Cache\": \"%s\"", c.type, c.size);
		if(i+1 < (int)Cache::caches.size())
			fprintf(write,",\n");
		else
			fprintf(write,"\n");
	}
	fprintf(write, "    }\n");
	fprintf(write, "},\n");
	fprintf(write, "\"Process List\": {\n");
	for(int i = 0; i < (int)Process::procs.size(); i++){
		Process::process p = Process::procs[i];
		fprintf(write, "    \"Proc_%d\": {\n", p.pid);
		fprintf(write, "        \"pid\": \"%d\",\n", p.pid);
		fprintf(write, "        \"uid\": \"%d\",\n", p.uid);
		fprintf(write, "        \"user\": \"%s\",\n", getpwuid(p.uid)->pw_name);
		fprintf(write, "        \"%%mem\": \"%.1f\",\n", p.mem);
		fprintf(write, "        \"minor page faults\": \"%d\",\n", p.min_page_flt);
		fprintf(write, "        \"major page faults\": \"%d\"\n", p.maj_page_flt);
		if(i+1 < (int)Process::procs.size())
			fprintf(write, "    },\n");
		else
			fprintf(write, "    }\n");
	}
	fprintf(write,"}\n}\n");
	fclose(write);
	//visualizar em https://jsoneditoronline.org/
}

int main() {
	printf("Digite a duração máxima do monitoramento, em segundos (para sair antes, pressione enter):\n");
	int tot_sec;
	scanf("%d%*c", &tot_sec);
	auto end = system_clock::now() + duration<int>(tot_sec);

	UI::setup();
	while (system_clock::now() < end && !UI::quit) {
		Process::getProcs();
		Cache::getCache();
		MemStats::getStats();
		UI::draw();
		export_json();
		sleep_for(milliseconds(200));
	}
	UI::exit();

	cout << "Fim" << endl;

	return 0;
}
