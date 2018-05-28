#include <bits/stdc++.h>
#include <ncurses.h>
#include <pwd.h>
#include <unistd.h>
using namespace std;
using namespace chrono;

using std::this_thread::sleep_for;
using std::chrono::milliseconds;

volatile long long * v;

int bigrand(){ return max(0,abs((rand()<<16)^rand())); } 

void cria(int t){
	v = new volatile long long [t];
	for(volatile int i = 0; i < t; i++)
		v[i] = bigrand() ^ v[bigrand()%(i+1)];
	cout << "Done." << endl;
	scanf("%*c");
	delete [] v;
}

int main(){
	srand(time(NULL));
	cout << "pid: " << getpid() << endl;
	cout << "Allocating a array of 32KB" << endl;
	cria(1<<12);
	cout << "Allocating a array of 1MB" << endl;
	cria(1<<17);
	cout << "Allocating a array of 128MB" << endl;
	cria(1<<24);
	cout << "Allocating a array of 512MB" << endl;
	cria(1<<26);
	cout << "Allocating a array of 1GB" << endl;
	cria(1<<27);
	cout << "Allocating a array of 2GB" << endl;
	cria(1<<28);
	cout << "Allocating a array of 4GB" << endl;
	cria(1<<29);
}

/*
sudo su
swapoff -a && swapon -a
*/
