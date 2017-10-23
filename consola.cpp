#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <queue>
#include "consola.hpp"
#include "HashMap.hpp"
#include "mpi.h"

using namespace std;

//0 = req
//1 = rel
//2 = ack
//3 = relAll
//4 = ready

#define CMD_LOAD    "load"
#define CMD_ADD     "addAndInc"
#define CMD_MEMBER  "member"
#define CMD_MAXIMUM "maximum"
#define CMD_QUIT    "quit"
#define CMD_SQUIT   "q"

static unsigned int np;

// Crea un ConcurrentHashMap distribuido
static void load(list<string> params) {
	MPI_Status Stat;

	bool message;
	queue<unsigned int> freeNodes;

	//All nodes are ready before starting
	for (unsigned int i = 1; i < np; i++){
		freeNodes.push(i+1);
	}

	//For each file find a ready node to send it to
    for (list<string>::iterator it=params.begin(); it != params.end(); ++it) {
		if (freeNodes.empty()){
			MPI_Recv(&message, 1, MPI_C_BOOL, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
			freeNodes.push(Stat.MPI_SOURCE);
		}

		string command = (char)1 + *it;
		MPI_Send(command.c_str(), command.length()+1, MPI_CHAR, freeNodes.front(), 0, MPI_COMM_WORLD);
		freeNodes.pop();
    }

	//Wait for all nodes to be ready
	while(freeNodes.size() < np-1){
		MPI_Recv(&message, 1, MPI_C_BOOL, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
		freeNodes.push(Stat.MPI_SOURCE);
	}

    cout << "La listá esta procesada" << endl;
}

// Esta función debe avisar a todos los nodos que deben terminar
static void quit() {
	char command=0; //EXIT

	//BCAST
	MPI_Request reqs[np-1];   // required variable for non-blocking calls
   	MPI_Status stats[np-1];   // required variable for Waitall routine
	for (unsigned int i = 1; i < np; i++){
		MPI_Isend(&command, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &reqs[i-1]);
	}
	MPI_Waitall(np-1, reqs, stats);
}

// Esta función calcula el máximo con todos los nodos
static void maximum() {
	char command=3; //MAXIMUM

	MPI_Status Stat;
	unsigned int remainingNodes = np-1;
	HashMap tmpMap;

	//BCAST
	MPI_Request reqs[np-1];   // required variable for non-blocking calls
	MPI_Status stats[np-1];   // required variable for Waitall routine
	for (unsigned int i = 1; i < np; i++){
		MPI_Isend(&command, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &reqs[i-1]);
	}
	MPI_Waitall(np-1, reqs, stats);

	//RECIEVE NODE WORDS
	while(remainingNodes > 0){
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
		int message_length;
		MPI_Get_count(&Stat, MPI_CHAR, &message_length);
		char message[message_length];
		MPI_Recv(&message, message_length, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);

		if (message[0] > 0){	//NODE FINISHED PROCESSING
			remainingNodes--;
		}
		if (message[0] == 2){	//NODE WAS EMPTY
			continue;
		}

		string word = string(&message[1]);

		pair<string, unsigned int> wordPair (word, Stat.MPI_TAG); // <word, count>
		for (unsigned int i = 0; i < wordPair.second; i++){
			tmpMap.addAndInc(word);
		}
	}

	if (tmpMap.size() > 0){
		pair<string, unsigned int> result = tmpMap.maximum();
		cout << "El máximo es <" << result.first <<"," << result.second << ">" << endl;
	}else{
		cout << "No hay elementos en el HashMap" << endl;
	}


}

// Esta función busca la existencia de *key* en algún nodo
static void member(string key) {
	string command = (char)2 + key; //MEMBER CMD + KEY

	unsigned int remainingNodes = np-1;
	MPI_Status Stat;

	//BCAST
	MPI_Request reqs[np-1];   // required variable for non-blocking calls
   	MPI_Status stats[np-1];   // required variable for Waitall routine
	for (unsigned int i = 1; i < np; i++){
		MPI_Isend(command.c_str(), command.length()+1, MPI_CHAR, i, 0, MPI_COMM_WORLD, &reqs[i-1]);
	}
	MPI_Waitall(np-1, reqs, stats);
	//END BCAST


	//RECV RESPONSES
	bool esta = false;
	bool message;

	while(remainingNodes > 0){
		MPI_Recv(&message, 1, MPI_C_BOOL, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
		esta = (esta || message);
		remainingNodes--;
	}

    cout << "El string <" << key << (esta ? ">" : "> no") << " está" << endl;
}


// Esta función suma uno a *key* en algún nodo
static void addAndInc(string key) {

    // TODO: Implementar

    cout << "Agregado: " << key << endl;
}


/* static int procesar_comandos()
La función toma comandos por consola e invoca a las funciones correspondientes
Si devuelve true, significa que el proceso consola debe terminar
Si devuelve false, significa que debe seguir recibiendo un nuevo comando
*/

static bool procesar_comandos() {

    char buffer[BUFFER_SIZE];
    size_t buffer_length;
    char *res, *first_param, *second_param;

    // Mi mamá no me deja usar gets :(
    res = fgets(buffer, sizeof(buffer), stdin);

    // Permitimos salir con EOF
    if (res==NULL)
        return true;

    buffer_length = strlen(buffer);
    // Si es un ENTER, continuamos
    if (buffer_length<=1)
        return false;

    // Sacamos último carácter
    buffer[buffer_length-1] = '\0';

    // Obtenemos el primer parámetro
    first_param = strtok(buffer, " ");

    if (strncmp(first_param, CMD_QUIT, sizeof(CMD_QUIT))==0 ||
        strncmp(first_param, CMD_SQUIT, sizeof(CMD_SQUIT))==0) {

        quit();
        return true;
    }

    if (strncmp(first_param, CMD_MAXIMUM, sizeof(CMD_MAXIMUM))==0) {
        maximum();
        return false;
    }

    // Obtenemos el segundo parámetro
    second_param = strtok(NULL, " ");
    if (strncmp(first_param, CMD_MEMBER, sizeof(CMD_MEMBER))==0) {
        if (second_param != NULL) {
            string s(second_param);
            member(s);
        }
        else {
            printf("Falta un parámetro\n");
        }
        return false;
    }

    if (strncmp(first_param, CMD_ADD, sizeof(CMD_ADD))==0) {
        if (second_param != NULL) {
            string s(second_param);
            addAndInc(s);
        }
        else {
            printf("Falta un parámetro\n");
        }
        return false;
    }

    if (strncmp(first_param, CMD_LOAD, sizeof(CMD_LOAD))==0) {
        list<string> params;
        while (second_param != NULL)
        {
            string s(second_param);
            params.push_back(s);
            second_param = strtok(NULL, " ");
        }

        load(params);
        return false;
    }

    printf("Comando no reconocido");
    return false;
}

void consola(unsigned int np_param) {
    np = np_param;
    printf("Comandos disponibles:\n");
    printf("  "CMD_LOAD" <arch_1> <arch_2> ... <arch_n>\n");
    printf("  "CMD_ADD" <string>\n");
    printf("  "CMD_MEMBER" <string>\n");
    printf("  "CMD_MAXIMUM"\n");
    printf("  "CMD_SQUIT"|"CMD_QUIT"\n");
	//MPI_Status Stat;
	//char inmsg;
	//MPI_Probe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &Stat);
	//printf(" ----------%d", Stat.MPI_SOURCE);
	//MPI_Recv(&inmsg, 1, MPI_CHAR, source, tag, MPI_COMM_WORLD, &Stat);
    bool fin = false;
    while (!fin) {
        printf("> ");
        fflush(stdout);
        fin = procesar_comandos();
    }
	cout << "Consola saliendo.." << endl;
}
