#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <json-c/json.h>

#define LENGTH 2048
char name[20];
char to[20] = "all";
int socked = 0;

// TO DO FALTA
void strOverwriteStdout() { // Sobreescribe el destino del proceso
  printf("%s", "> ");
  fflush(stdout); // Limpieza del buffer
}

void strTrimLf (char* array, int length) {//quitar el lf a los strings
  int i;
  for (i = 0; i < length; i++) { // separa \n
    if (array[i] == '\n') {
      array[i] = '\0';
      break;
    }
  }
}

void chatManager() {
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {}; //numero de caracteres

	while(1) {
		strOverwriteStdout();
		fgets(message, LENGTH, stdin);
		strTrimLf(message, LENGTH);

		//Strcmp compara 2 strings y si estos son iguales, regresa 0
		if (strcmp(message, "/exit") == 0) {
			break;
    	} 
		else if(strcmp(message, "/help") == 0){
			printf("Listado de comandos\n");
			printf(">/private: inicia un chat privado\n");
			printf(">/general:regresar al chat general"); 
			printf(">/info:informacion de usuario"); //Nombre y status
			printf(">/chageStatus: cambiar de estado\n"); //activo(0)-inactivo(1)-ocupado(2)
			printf(">/activeUsers: devuelve la lista de usuarios activos\n");
			printf(">/exit: cierra la conexion del chat\n");
		}
		else if(strcmp(message, "/info") == 0){
			printf("A quien desea buscar:\n");
			char input[32];
			fgets(input,32,stdin);
			chatManager_recv(input,32);

			json_object *GET_USER= json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(GET_USER, "request", json_object_new_string("GET_USER")); //Array con la ip de un usuario en especifico
			json_object *jstring_user = json_object_new_string(input);
			json_object_array_add(jarray,jstring_user);
			json_object_object_add(GET_USER, "body", jarray);

			const char* request = json_object_to_json_string(GET_USER);

			printf("%s", request);
	
			sprintf(buffer, "%s",message);
			send(sockfd, request, strlen(request), 0);
		}
		else if(strcmp(message, "/chageStatus") == 0){
			printf("Ingrese el numero correspondiente a su estado: 0 = activo, 1 = inactivo, 2 = ocupado:\n");
			int state;
			char stateSTR[2];
			scanf("%d", &state);

			sprintf(stateSTR,"%d",state);

			json_object *PUT_STATUS = json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(PUT_STATUS, "request", json_object_new_string("PUT_STATUS")); //Cambia el estado del cliente
			json_object *jstring_state = json_object_new_string(stateSTR);
			json_object_array_add(jarray,jstring_state);
			json_object_object_add(PUT_STATUS, "body", jarray);

			const char* request = json_object_to_json_string(PUT_STATUS);

			printf("%s", request);
	
			sprintf(buffer, "%s",message);
			send(sockfd, request, strlen(request), 0);
		}
		}
		else if(strcmp(message, "/activeUsers") == 0){
		// "GET_USER")); //Obtiene la lista de los usuarios conectados
		}
		
