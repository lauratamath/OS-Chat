#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<sys/types.h>
#include<signal.h>
#include<json-c/json.h>

#define MAX_CLIENTS 25
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	int status;
} client_t;

//char temp_req[16];

json_object *all_chat; 

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(char *s, char *name){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->name == name){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}



/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char conex_request[64];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if(recv(cli->sockfd, conex_request, 64, 0) <= 0){//quite las otras validaviones porque asumian que solo iba el nombre
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else{
		//char response_test[BUFFER_SZ];

		json_object *parsed_json;
		json_object *body;
		json_object *name;
		parsed_json = json_tokener_parse(conex_request);

		json_object_object_get_ex(parsed_json,"body", &body);
		name = json_object_array_get_idx(body,1); 
		//aqui asiganle el nombre parseado a la variable name

		strcpy(cli->name, json_object_get_string(name));

		json_object *INIT_CONEX = json_object_new_object();
		json_object_object_add(INIT_CONEX, "response", json_object_new_string("INIT_CONEX"));
		json_object_object_add(INIT_CONEX, "code", json_object_new_int(200));
		write(cli->sockfd, json_object_to_json_string(INIT_CONEX), 
			strlen(json_object_to_json_string(INIT_CONEX)));
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);
	}

	bzero(buff_out, BUFFER_SZ);

	while(1){
		char temp_req[32];
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0){
			if(strlen(buff_out) > 0){
				json_object *parsed_json;
				json_object *request;
				json_object *body;

				parsed_json = json_tokener_parse(buff_out);
				json_object_object_get_ex(parsed_json,"request", &request);
				json_object_object_get_ex(parsed_json,"body", &body);
				strcpy(temp_req, json_object_get_string(request));
				printf("%s\n", temp_req);

				printf("%s", json_object_get_string(body));
				
				
				if(strcmp(temp_req,"POST_CHAT")==0){
					json_object *message;
					json_object *from;
					json_object *delivered_at;
					json_object *to;
					//char mssg[128];
					char to_chat[256];
					//char aux_to[32];

					message = json_object_array_get_idx(body,0);
					from = json_object_array_get_idx(body,1);
					delivered_at = json_object_array_get_idx(body,2);
					to = json_object_array_get_idx(body,3);
					//strcpy(aux_to, json_object_get_string(to));

					sprintf(to_chat, "%s: %s at %s\n", json_object_get_string(from),
					json_object_get_string(message),json_object_get_string(delivered_at));
					json_object *aux_mssg = json_object_new_string(to_chat);

					printf("Este es el to %s", json_object_get_string(to));
					if(strcmp(json_object_get_string(to),"all") == 0){
						printf("perfectly working from all post_chat\n");
						json_object_array_add(all_chat,aux_mssg);
						send_message(to_chat, cli->uid);
					}else{
						char *aux_to = json_object_get_string(to);
						send_private_message(to_chat,aux_to);
					}

					//strcpy(mssg, json_object_get_string(message));
					json_object *POST_CHAT = json_object_new_object();
					json_object_object_add(POST_CHAT, "response", json_object_new_string("POST_CHAT"));
					json_object_object_add(POST_CHAT, "code", json_object_new_int(200));
					write(cli->sockfd, json_object_to_json_string(POST_CHAT), 
					strlen(json_object_to_json_string(POST_CHAT)));
					str_trim_lf(to_chat,strlen(to_chat));
					printf("%s\n", to_chat);

				}else if(strcmp(temp_req,"GET_USER")==0){
					char user_type[32];
					char user_info[56];
					json_object *aux_type;
					aux_type = json_object_array_get_idx(body,0);

					printf("someone asked for user info\n");
					json_object *users = json_object_new_array();
		      json_object *GET_USER = json_object_new_object();
					
					strcpy(user_type, json_object_get_string(aux_type));
					printf("%s\n",user_type);

          if(strcmp(user_type,"all")==0){
						//pthread_mutex_lock(&clients_mutex);
						int i;
						for(i=0; i<MAX_CLIENTS; ++i){
							if(clients[i]){
								sprintf(user_info, "%s, status: %d \n", clients[i]->name, clients[i]->status);
								json_object_array_add(users,json_object_new_string(user_info));
								bzero(user_info, 56);
							}
        		}
        			//pthread_mutex_unlock(&clients_mutex);
	            json_object_object_add(GET_USER, "body", users);

					}else{
						//pthread_mutex_lock(&clients_mutex);
            int i;
            for(i=0; i<MAX_CLIENTS; ++i){
            	if(clients[i]){
								if(clients[i]->name == user_type){}
                 	sprintf(user_info, "status: %d \n", clients[i]->status);
                }
              }
            //pthread_mutex_unlock(&clients_mutex);
	          json_object_object_add(GET_USER, "body", json_object_new_string(user_info));

					}

					json_object_object_add(GET_USER, "response", json_object_new_string("GET_USER"));
					json_object_object_add(GET_USER, "code", json_object_new_int(200));
					write(cli->sockfd, json_object_to_json_string(GET_USER), 
					strlen(json_object_to_json_string(GET_USER)));

				}
			}
		} else if (receive == 0 || strcmp(buff_out, "/exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			json_object *END_CONEX = json_object_new_object();
			json_object_object_add(END_CONEX, "response", json_object_new_string("END_CONEX"));
			json_object_object_add(END_CONEX, "code", json_object_new_int(200));
			write(cli->sockfd, json_object_to_json_string(END_CONEX), 
			strlen(json_object_to_json_string(END_CONEX)));
			send_message(buff_out, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
		bzero(temp_req,32);
	}

  /* Delete client from queue and yield thread */
	
	close(cli->sockfd);
  	queue_remove(cli->uid);
 	free(cli);
  	cli_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	all_chat = json_object_new_array();
	if(argc != 2){
		printf("Debe ingresar un puerto (i.e.): %s 8888 \n", argv[0]);
		return 1;
	}

	char *ip = "172.31.41.54";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  /* Socket settings */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  /* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    return 1;
	}

	/* Bind */
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR: Socket binding failed");
    return 1;
  }

  /* Listen */
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return 1;
	}

	printf("Universidad del Valle de Guatemala\n");
	printf("Sistemas Operativos\n");
	printf("Autores\n");
	printf("Martín España		Carné: 19258\n");
	printf("Laura Tamath		Carné: 19365\n");
	printf("CHAT DE OS ----- HOWSAPP\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count + 1) == MAX_CLIENTS){
			char max_mssg[BUFFER_SZ];
			sprintf(max_mssg, "Max clients reached. Rejected");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			write(connfd, max_mssg, strlen(max_mssg));
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;
		cli->status = 0;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return 0;
}
