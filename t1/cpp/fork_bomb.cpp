#include <bits/stdc++.h>
#include<signal.h>
#include<unistd.h>
#include<dirent.h>
using namespace std;
 
int main(){
	cout << getpid() << endl;
	for(int i=0; i < 5; ++i){ 
		if (fork() == -1) {
			assert(errno == EAGAIN);
			//return -1;
		}
	}
	sleep(30);
	return 0;
}
