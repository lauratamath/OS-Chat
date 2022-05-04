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
#include <time.h>

#define LENGTH 2048
#define SIZE 256
char name[20];
char to[20] = "all";
int socketDesc = 0;

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

void chatManager_recv() {
	char message[LENGTH] = {};
	while (1) {
		int receive = recv(socketDesc, message, LENGTH, 0);
	if (receive > 0) {
      printf("%s", message);
      strOverwriteStdout();
    } else if (receive == 0) {
			break;
    } 
	memset(message, 0, sizeof(message));
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
			printf("\nEl chat ha sido abandonado\n");
			json_object *END_CONEX = json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(END_CONEX, "request", json_object_new_string("END_CONEX ")); //Finalizar la conexion con el servidor
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

			//printf("%s", request);
	
			sprintf(buffer, "%s",message);
			send(socketDesc, request, strlen(request), 0);
		}
		else if(strcmp(message, "/changeStatus") == 0){
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

			//printf("%s", request);
	
			sprintf(buffer, "%s",message);
			send(socketDesc, request, strlen(request), 0);
		}
		else if(strcmp(message, "/activeUsers") == 0){
			json_object *GET_USER= json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(GET_USER, "request", json_object_new_string("GET_USER")); //Array con la ip de un usuario en especifico
			json_object *jstring_user = json_object_new_string(to);
			json_object_array_add(jarray,jstring_user);
			json_object_object_add(GET_USER, "body", jarray);

			const char* request = json_object_to_json_string(GET_USER);

			//printf("%s", request);
	
			sprintf(buffer, "%s",message);
			send(socketDesc, request, strlen(request), 0);
		}
		else if(strcmp(message, "/private") == 0){
			printf("Ingrese el nombre del usuario que desea hablar:\n");
			char input[32];
			fgets(input,32,stdin);
			strTrimLf(input,32); 

			sprintf(buffer, "%s\n", message);
			json_object *POST_CHAT = json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(POST_CHAT, "request", json_object_new_string("POST_CHAT")); //crea un nuevo mensaje para algun chat
			json_object *jstring_message = json_object_new_string(buffer);
			json_object *jstring_from = json_object_new_string(name);
			json_object *jstring_time = json_object_new_string("19:15");
			json_object *jstring_to = json_object_new_string(input); //Entabla la conversacion
			json_object_array_add(jarray,jstring_message);
			json_object_array_add(jarray,jstring_from);	
			json_object_array_add(jarray,jstring_time);
			json_object_array_add(jarray,jstring_to);
	
			json_object_object_add(POST_CHAT, "body", jarray);

			const char* request = json_object_to_json_string(POST_CHAT);

			send(socketDesc, request, strlen(request), 0);
		}
		else if(strcmp(message, "/general") == 0){
			sprintf(buffer, "%s\n", message);
			json_object *POST_CHAT = json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(POST_CHAT, "request", json_object_new_string("POST_CHAT"));  //Crea un nuevo mensjae para el grupo
			json_object *jstring_message = json_object_new_string(buffer);
			json_object *jstring_from = json_object_new_string(name);
			json_object *jstring_time = json_object_new_string("20:10");
			json_object *jstring_to = json_object_new_string(to);
			json_object_array_add(jarray,jstring_message);
			json_object_array_add(jarray,jstring_from);	
			json_object_array_add(jarray,jstring_time);
			json_object_array_add(jarray,jstring_to);
	
			json_object_object_add(POST_CHAT, "body", jarray);

			const char* request = json_object_to_json_string(POST_CHAT);

			//printf("%s", request);
		   send(socketDesc, request, strlen(request), 0);
		}
		else{
			sprintf(buffer, "%s\n", message);
			json_object *POST_CHAT = json_object_new_object();
			json_object *jarray = json_object_new_array();
			json_object_object_add(POST_CHAT, "request", json_object_new_string("POST_CHAT")); //Crea un nuevo mensjae para el grupo
			json_object *jstring_message = json_object_new_string(buffer);
			json_object *jstring_from = json_object_new_string(name);
			json_object *jstring_time = json_object_new_string("20:15");
			json_object *jstring_to = json_object_new_string(to);
			json_object_array_add(jarray,jstring_message);
			json_object_array_add(jarray,jstring_from);	
			json_object_array_add(jarray,jstring_time);
			json_object_array_add(jarray,jstring_to);
	
			json_object_object_add(POST_CHAT, "body", jarray);

			const char* request = json_object_to_json_string(POST_CHAT);

			//printf("%s", request);
		   send(socketDesc, request, strlen(request), 0);
		}
		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
}

int main(int argc, char **argv){
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "3.141.170.53";
	int port = atoi(argv[1]);
	

	printf("Ingrese su nombre: ");
	fgets(name, 32, stdin);
	strTrimLf(name, strlen(name));
	

	if (strlen(name) > 20 || strlen(name) < 3){
		printf("El nombre no puede ser mayor a 30 ni menor a  3 caracteres.\n");
		return EXIT_FAILURE;
	}

	//if (strlen(name) == json_object_to_json_string(GET_USER)){
	//	printf("Este usuario ya existe.\n");
	//	return EXIT_FAILURE;
	//}

	struct sockaddr_in server_addr;
	
	// Ajustes del socket
	socketDesc = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	 // Connect to Server
	int err = connect(socketDesc, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	// Manda el nombre
	json_object *INIT_CONEX = json_object_new_object();
	json_object *jarray = json_object_new_array();
	json_object_object_add(INIT_CONEX, "request", json_object_new_string("INIT_CONEX")); // Establecer una conexion al servidor
	json_object *jstring_time = json_object_new_string("19:20");
	json_object *jstring_name = json_object_new_string(name);
	json_object_array_add(jarray,jstring_time);
	json_object_array_add(jarray,jstring_name);
	json_object_object_add(INIT_CONEX, "body", jarray);

	const char* request = json_object_to_json_string(INIT_CONEX);
	printf("%s\n",request);
	send(socketDesc, request, strlen(request), 0);
	printf("----- Este es el chat UVG -----\n\n");
	pthread_t send_msg_thread;

	if(pthread_create(&send_msg_thread, NULL, (void *) chatManager, NULL) != 0){
		printf("ERROR: pthread\n");
    return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void *) chatManager_recv, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	// Call chat 
	chatManager();
	close(socketDesc);

	return EXIT_SUCCESS;
}

