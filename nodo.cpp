#include "nodo.hpp"
#include "HashMap.hpp"
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>
#include <string>

using namespace std;

void nodo(unsigned int rank) {
    printf("Soy un nodo. Mi rank es %d \n", rank);
	HashMap myHashMap;

	int clock_tag = 0;

    while (true) {
		MPI_Status Stat;
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
		if (Stat.MPI_SOURCE == 0){	//RECIEVED COMMAND FROM CONSOLE

			int message_length;
			MPI_Get_count(&Stat, MPI_CHAR, &message_length);
			char message[message_length];
			MPI_Recv(&message, message_length, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
			clock_tag = max(Stat.MPI_TAG, clock_tag+1);

			if (message[0] == 0){		//EXIT
				printf("Nodo %d saliendo..\n", rank);
				return;

			}else if (message[0] == 1){	//LOAD FILE
				myHashMap.load(string(&message[1]));
				printf("Nodo %d procesó archivo: '%s'\n", rank, &message[1]);
				trabajarArduamente();

				//Send ready msg
				bool response = true;
				MPI_Send(&response, 1, MPI_C_BOOL, 0, clock_tag, MPI_COMM_WORLD);

			}else if (message[0] == 2){ //MEMBER

				bool response = myHashMap.member(&message[1]);
				if (response){
					printf("Nodo %d SI encontró la clave: '%s'\n", rank, &message[1]);
				}else{
					printf("Nodo %d NO encontró la clave: '%s'\n", rank, &message[1]);
				}
				trabajarArduamente();
				MPI_Send(&response, 1, MPI_C_BOOL, 0, clock_tag, MPI_COMM_WORLD);

			}else if (message[0] == 3){	//MAXIMUM

				trabajarArduamente();
				string currentWord = "";
				unsigned int currentCount = 0;
				for (HashMap::iterator it=myHashMap.begin(); it != myHashMap.end(); ++it) {
					if (currentWord == ""){
						currentWord = *it;
						currentCount = 0;
					}

					if (currentWord != *it){
						string response = (char)0 + currentWord;
						MPI_Send(response.c_str(), response.length()+1, MPI_CHAR, 0, currentCount, MPI_COMM_WORLD);
						currentWord = *it;
						currentCount = 1;
					}else{
						currentCount++;
					}

				}
				int finalResponse = ((currentWord == "") + 1);
				string response = (char)finalResponse + currentWord;
				MPI_Send(response.c_str(), response.length()+1, MPI_CHAR, 0, currentCount, MPI_COMM_WORLD);
				printf("Nodo %d terminó de enviar sus palabras\n", rank);

			}else{						//SHOULD NOT HAPPEN
				printf("WTF%d\n", message[0]);
			}
		}
    }
}

void trabajarArduamente() {
    int r = rand() % 2500000 + 500000;
    usleep(r);
}
