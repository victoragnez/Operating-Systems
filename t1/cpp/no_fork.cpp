#include<bits/stdc++.h>
#include<signal.h>
#include<unistd.h>
#include<dirent.h>
#include<pwd.h>
using namespace std;
 
#define MAX_PID 32769
vector<int> children[MAX_PID];
int father[MAX_PID], tam[MAX_PID], real_user[MAX_PID];
char exists[MAX_PID];
int current_father;
 
void listdir(const char *name, int indent){
	if(indent == 1){
		char file[1024], temp[1024];
		sprintf(file,"%s/status", name);
		FILE * f = fopen(file,"r");
		while(fscanf(f,"%s",temp)!=EOF){
			if(string(temp) != "Uid:") continue;
			fscanf(f, "%d", &real_user[current_father]);
			break;
		}
		fclose(f);
	}

	if(indent == 3){
		int current_child;
		char file[1024];
		sprintf(file,"%s/children", name);
		FILE * f = fopen(file,"r");
		while(fscanf(f,"%d",&current_child)!=EOF){
			children[current_father].push_back(current_child);
			father[current_child] = current_father;
		}
		fclose(f);
		return;
	}
 
	DIR *dir;
	struct dirent *entry;
 
	if(!(dir = opendir(name))) return;
	while ((entry = readdir(dir)) != NULL) {
		if(entry->d_type == DT_DIR) {
			if(indent == 0 && !('0' <= entry->d_name[0] && entry->d_name[0] <= '9'))
				continue; 
			if(indent == 1 && string(entry->d_name) != "task")
				continue;
			if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			char path[1024];
			if(indent == 0){
				sscanf(entry->d_name, "%d", &current_father);
				exists[current_father] = 1;
			}
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
			//entry->d_name  ~~> nome do diretorio encontrado
			listdir(path, indent + 1);
		}
	}
	closedir(dir);
}

int calc(int v){
	if(!(tam[v] == 0 && exists[v])) return tam[v];
	tam[v] = 1;
	for(int i = 0; i < (int)children[v].size(); i++)
		tam[v] += calc(children[v][i]);
	return tam[v];
}

void dfs(int v, int nvl, FILE * f, bool json=0, bool has_brother=0){
	if(!json) fprintf(f,"%*s%d\n", 4*nvl, "", v);
	else{
		fprintf(f,"%*s\"object_%d\": {\n", 4*nvl, "", v);
		fprintf(f,"%*s\"pid\": \"%d\",\n", 4*nvl+2, "", v);
		fprintf(f,"%*s\"uid\": \"%d\",\n", 4*nvl+2, "", real_user[v]);
		fprintf(f,"%*s\"children\": {\n", 4*nvl+2, "");
	}
	for(int i = 0; i < (int)children[v].size(); i++)
		dfs(children[v][i], nvl+1, f, json, i+1 < (int)children[v].size());
	if(json){
		fprintf(f,"%*s}\n", 4*nvl+2, "");
		fprintf(f,"%*s}", 4*nvl, "");
		if(has_brother) fprintf(f,",");
		fprintf(f,"\n");
	}
}

int main(){
	cout << "Digite 0 para monitorar os processos" << endl;
	cout << "Digite 1 para monitorar o numero de processos total e por usuario" << endl;
	cout << "Digite outra coisa para exibir arvore de um processo" << endl;
	string op;
	cin >> op;
	if(op == "0"){ 
		cout << "Digite a quantidade de segundos na qual devemos monitorar os processos" << endl;
		int tot_sec;
		cin >> tot_sec;
		clock_t ini = clock();
		while(1){
			for(int i = 0; i < MAX_PID; i++){
				father[i] = -1;
				exists[i] = 0;
				tam[i] = 0;
				children[i].clear();
			}
			listdir("/proc", 0);
			system("clear");
			cout << "Monitorando" << endl;
			for(int i = 0; i < MAX_PID; i++) if(calc(i) >= 70) {
				cout << "Suspeito: " << i << " com " << calc(i) << " descendentes";
				if(father[i] != -1){
					cout << " (filho de " << father[i] << ")";
				}
				cout << endl;
			}
	 
			if(clock()-ini >= tot_sec * CLOCKS_PER_SEC)
				break;
		}
		cout << "Fim" << endl;
	}
	else if(op == "1"){
		cout << "Digite a quantidade de segundos na qual devemos monitorar o numero de processos" << endl;
		int tot_sec;
		cin >> tot_sec;
		clock_t ini = clock();
		while(1){
			for(int i = 0; i < MAX_PID; i++){
				father[i] = -1;
				exists[i] = 0;
				tam[i] = 0;
				children[i].clear();
			}
			listdir("/proc", 0);
			system("clear");
			cout << "Monitorando" << endl;
			map<int, int> cont;
			int tot=0;
			for(int i = 0; i < MAX_PID; i++)
				if(exists[i])
					cont[real_user[i]]++, tot++;
			cout << "Total de processos: " << tot << endl;
			for(pair<int, int> p : cont)
				cout << "Usuario: " << getpwuid(p.first)->pw_name << " ---- Numero de processos: " << p.second << endl;
			
			if(clock()-ini >= tot_sec * CLOCKS_PER_SEC)
				break;
		}
		cout << "Fim" << endl;
	}else{
		for(int i = 0; i < MAX_PID; i++){
			father[i] = -1;
			exists[i] = 0;
			children[i].clear();
		}
		listdir("/proc", 0);
		cout << "Digite o numero do processo" << endl;
		int v;
		cin >> v;
		if(!exists[v]){
			cout << "Nao encontrado" << endl;
			return 0;
		}
		FILE * f = fopen("out","w");
		dfs(v,0,f);
		fclose(f);
		f = fopen("out.json","w");
		fprintf(f,"{\n");
		dfs(v,1,f,1);
		fprintf(f,"}\n");
		fclose(f);
		cout << "Encontrado. Arvore salva nos arquivos out e out.json" << endl;
	}
}
			
