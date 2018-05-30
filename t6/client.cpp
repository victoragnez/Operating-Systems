#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <cstring>
#include <vector>
#include <utility>
#include <thread>
#include <ncurses.h>

using namespace std;
using std::this_thread::sleep_for;
using std::chrono::milliseconds;

#define DOWN '0'
#define RIGHT '1'
#define LEFT '2'
#define UP '3'
#define NONE '4'

const int MAX_SNAKES = 10;
const int SNAKE_LEN = 20;
const int MAX_TRIES = 100;
const int n = 30, m = 40;

char grid[n][m], key;
int dir, id;
bool not_ended = true;

void gameover(bool esperado = false){ //AQUI CARLOS (pode apagar tudo; só codei pra testes)
	printf("Fim de jogo\n");
	if(!esperado) printf("O motivo pode ser: falha na conexao, alocacao, ou removido pelo servidor\n");
}

void menu(){ //AQUI CARLOS (pode apagar tudo; só codei pra testes)
	printf("aperte 1 para novo jogo ou 0 para sair\n");
} 

void options(){ //AQUI CARLOS (pode apagar tudo; só codei pra testes)
	printf("aperte 1 para jogo randomico e 0 para jogo normal\n");
}

void desenha(){ //AQUI CARLOS (pode apagar tudo; só codei pra testes)
	system("clear");
	printf("vc eh o %d\n", id);
	for(int i = 0; i < n; i++){
		for(int j = 0; j < m; j++)
			printf("%c", grid[i][j]);
		printf("\n");
	}
}

void ler(){ //AQUI CARLOS (pode apagar tudo; só codei pra testes)
	while(not_ended){
		scanf(" %c", &key);
		if(key == 'w') key = UP;
		if(key == 'a') key = LEFT;
		if(key == 's') key = DOWN;
		if(key == 'd') key = RIGHT;
		/* key = "key pressed" */
	}
}

void connect_server(char * host, int portno){
	int serverid;
	struct sockaddr_in new_serv_addr;
	serverid = socket(AF_INET, SOCK_STREAM, 0);
	memset(&new_serv_addr,0,sizeof(new_serv_addr));
	
	struct hostent *server;
	server = gethostbyname(host);
	if(server == NULL){
		cerr << "error getting host" << endl;
		return;
	}
	new_serv_addr.sin_family = AF_INET;
	new_serv_addr.sin_port = htons(portno);
	memcpy(&new_serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
	
	if(connect(serverid, (struct sockaddr *)&new_serv_addr, sizeof(new_serv_addr)) < 0){
		cerr << "error connecting to host" << endl;
		return;
	}
	
	static char buffer[n*m + 100];
	int tt;
	
	auto end = [&](int num){
		cerr << "error " << num << endl;
		close(serverid);
		gameover();
	};
	
	tt = recv(serverid, buffer, sizeof(buffer), 0); //recebe o id
	if(tt <= 0){
		end(0);
		return;
	}
	
	if(buffer[0] < '0' || buffer[0] > '9'){
		close(serverid);
		gameover();
		return;
	}
	
	id = buffer[0] - '0';
	
	bool esperado = true, random = false;
	
	while(true){
		menu();
		key = NONE;
		while(key != '0' && key != '1')
			sleep_for(milliseconds(10));
		
		sprintf(buffer, "%c", key);
		
		if(buffer[0] == '1'){
			options();
			key = NONE;
			while(key != '0' && key != '1')
				sleep_for(milliseconds(10));
			random = (key=='1');
		}
		
		tt = send(serverid, buffer, strlen(buffer), 0); //envia se quer sair do jogo ou começar um novo (1 = novo, 0 = sair)
		if(tt <= 0){
			end(1);
			return;
		}

		if(buffer[0] != '1')
			break;
			
		key = NONE;
		
		while(true){ //jogo
			tt = recv(serverid, buffer, sizeof(buffer), 0); // flag se continua seguida do grid
			if(tt <= 0){
				end(4);
				return;
			}
			if(buffer[0] == '0')
				break;
			
			int ptr = 1;
			for(int i = 0; i < n; i++)
				for(int j = 0; j < m; j++)
					grid[i][j] = buffer[ptr++];
			desenha();
			if(!random){
				if(key < '0' || key > '3')
					key = NONE;
			}
			else key = rand()%4 + '0';
			sprintf(buffer, "%c", key);
		
			tt = send(serverid, buffer, strlen(buffer), 0); //envia a tecla pressionada
			if(tt <= 0){
				end(5);
				return;
			}
			
			key = NONE;
		}
		
	}
	close(serverid);
	gameover(esperado);
}

int main (int argc, char *argv[]){
	if(argc != 3){
		cerr << "Formato: " << argv[0] << " [ip] [porta]" << endl;
		return 0;
	}
	thread leitura(ler), jogo(connect_server, argv[1], atoi(argv[2]));
	jogo.join();
	not_ended = false;
	leitura.join();
}
