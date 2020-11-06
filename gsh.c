#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include "list.h"

Lista* pids, *grupos;
int viva = 1;

/*      Funcoes auxiliares para a leitura do(s) comando(s)      */
void getComandos(char* cmd, char** comandos){ //Separa a entrada em comandos
    int i = 0;
    char* token = strtok(cmd, "#");
    while(token != NULL){
        token[strlen(token)-1] = '\0';

        comandos[i] = (char*) malloc((strlen(token)+1)*sizeof(char));
        strcpy(comandos[i], token);

        token = strtok(NULL, "#");
        i++;
    }
}

void getParams(char* cmd, char* params[5]){ //Separa os parametros do comando
    int i = 0;
    char* token = strtok(cmd, " ");
    while(token != NULL){
        params[i] = (char*) malloc((strlen(token)+1)*sizeof(char));

        strcpy(params[i], token);
        token = strtok(NULL, " ");
        i++;
    }
}

/*    Funcoes auxiliares para a execucao dos comandos   */
void executaCmd(char* comando, char* param[5]){
    getParams(comando, param); //Pega os parametros
    if(execvp(param[0], param) == -1) exit(1); //executa comando com a lista de parametros
}

pid_t criaProcBackG(int idGroup, char* comando){ //Cria processos em background
    char* param[5] = {NULL};
    pid_t pid = fork();
    if(pid < 0) perror("Failed to fork");
    else{
        if(pid == 0){
            signal(SIGINT, SIG_IGN); //Processos filhos ignoram sigint
            if(idGroup < 0) idGroup = 0; //Se o idGroup <0 entao eh o lider, o pgid sera o seu pid

            if(setpgid(0, idGroup) == -1) perror("Failed to set group"); // Seta o pgid para pertencer ao grupo dado
            else{
                if(rand() % 2)fork(); //Cria ghost
                executaCmd(comando, param);
            }
        }
    }
    pids = insereLista(pid, pids); //Guarda o pid do processo criado
    return pid;
}

/*    Comandos internos    */
void mywait(){ //Libera todos os descendentes status zombie
    Lista* p = pids;
    while(p != NULL){
        pid_t filho = elementoLista(p);
        int status;

        p = proxLista(p);

        if(filho = waitpid(filho, &status, WNOHANG) >0) //Quer dizer que o filho morreu
            pids = removeLista(filho, pids);  //Remove da lista de pids
    }
}

void cleanANDdie(){ //Mata todos os processos vivos, incluindo ghosts
    Lista* p = grupos;
    while(p != NULL){
        kill(-elementoLista(p), SIGKILL); //Mata grupo de processos
        p = proxLista(p);
    }
    liberaLista(pids); 
    liberaLista(grupos);
    viva = 0;
}

/*         Tratamento de sinais               */
//Funcao de tratamento do sinal SIGTSTP: deve enviar o sinal para todos os descendentes e nao se suspender
void trataSIGTSTP(int sig){
    Lista* p = grupos;
    
    while(p!= NULL){
        pid_t gp = elementoLista(p);
        kill(-gp, SIGTSTP);
        
        p = proxLista(p);
    }
}

//Funcao de tratamento do sinal SIGCHLD: se o filho foi assassinado, o grupo dele deve morrer tambem :(
void trataSIGCHLD(int sig){
    Lista* p = pids;
    int status, pgid;
    pid_t pid;

    while (p!= NULL){ //Para cada filho da shell
        pid = elementoLista(p);
        pgid = getpgid(pid);
        if(waitpid(pid, &status, WNOHANG)>0){ //Verifica se esta morto
            if(WIFSIGNALED(status)){ //Se sim, verifica se morreu por um sinal
                kill(-pgid, SIGKILL); //Mata o resto do grupo
                grupos = removeLista(pgid, grupos); //remove da lista de grupos o grupo morto
            }
        }
        p = proxLista(p);
    }
}

// Funcao de tratamento do sinal SIGINT (Ctrl-C)
void trataSIGINT(int sig){
    int haVivos = 0;
    char resposta;
    Lista* p = pids;

    while(p != NULL){ //Verifica se ha filhos vivos
        if(waitpid(elementoLista(p), NULL, WNOHANG) == 0){
            haVivos = 1;
            break;
        }
        p = proxLista(p);
    }

    if(haVivos){ //Ha filhos vivos
        fputs("\nTem certeza que deseja encerrar?(s/n) ", stdout);
        fgets(&resposta, 2, stdin);
        getchar(); //Limpa buffer

        if(resposta == 's' || resposta == 'S'){ // Se a resposta for afirmativa
            p = pids;
            while(p != NULL){ //mata todos os processos filhos (ghosts nao)
                pid_t pid = elementoLista(p);
                if(waitpid(pid, NULL, WNOHANG) == 0){
                    if(kill(pid, SIGKILL) == -1)
                        perror ("Failed to kill child");
                }
                p = proxLista(p);
            }
            //Libera listas de pids e de pgids
            liberaLista(pids);
            liberaLista(grupos);         
        }else//Se for negativa, retorna pra shell
            return;
    }
    // Caso nao haja filhos vivos ou a resposta foi afirmativa
    signal(SIGINT, SIG_DFL); //Seta o sinal para o default
    raise(SIGINT); //Relanca o sinal que mata o processo
}

/*     Ghost Shell     */
int main(){
    pids = criaLista(); //Cria lista de id dos processos
    grupos = criaLista(); //Cria lista de grupos

    //Muda o tratamento do sinal SIGTSTP
    signal(SIGTSTP, trataSIGTSTP);

    //Muda o tratamento do sinal SIGCHLD, 
    //que manipula lista de grupos (tratamento do sigint e sigtstp podem pegar a estrutura inconsistente)
    struct sigaction act2;
    act2.sa_handler = trataSIGCHLD; //novo handler
    act2.sa_flags = 0;
    if((sigemptyset(&act2.sa_mask) == -1) || (sigaddset(&act2.sa_mask, SIGINT) == -1)//bloqueia sigint e sigtstp
        || (sigaddset(&act2.sa_mask, SIGTSTP) || sigaction(SIGCHLD, &act2, NULL) == -1))
        perror("Failed to set SIGCHLD handler");

    /* Durante o tratamento do SIGINT todos os sinais sao bloqueados */
    struct sigaction act;
    act.sa_handler = trataSIGINT; //novo handler
    act.sa_flags = 0;

    //seta mascara de bits para bloquear todos os sinais e faz o sigaction
    if((sigfillset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1))
        perror("Failed to set SIGINT to handle Ctrl-C");

    printf("\e[H\e[2J"); //Limpar tela
    while(viva){
        char cmd[100];
        fputs("gsh> ", stdout);
        fgets(cmd, 100, stdin); //Le comando(s)  
    
        char* comandos[6] = {NULL};
        getComandos(cmd, comandos); //Interpreta entrada em comandos separados

        //Verificar se ha comando interno
        if(!strcmp(comandos[0], "clean&die"))
            cleanANDdie();
        else if(!strcmp(comandos[0], "mywait")){
            mywait();
        }else{
            //Se for 1 comando, cria processo foreground
            if(comandos[1] == NULL){
                pid_t p = fork();
                if(p<0) perror("Failed to fork.");
                else{
                    if(p == 0){
                        signal(SIGINT, SIG_IGN); //Processos filhos ignoram sigint
                        if(rand() % 2)fork(); //Cria ghost
                        
                        char* param[5] = {NULL};
                        executaCmd(comandos[0], param);
                    }
                    pids = insereLista(p, pids);
                }
                waitpid(p, NULL, WUNTRACED); //espera processo foreground retornar/morrer
                
            }else{ //Mais de um comando, cria processos background em um unico grupo
                pid_t gid = criaProcBackG(-1, comandos[0]); //Cria lider do grupo, o seu id sera o id do grupo
                grupos = insereLista(gid, grupos); //insere grupo na lista de grupos

                int i =1;
                while(comandos[i] != NULL)
                    criaProcBackG(gid, comandos[i++]); //Cria outros processos no novo grupo
            }
        }
    }
    return 0;
}