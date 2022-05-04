// Dependencies
#include <arpa/inet.h>
#include <errno.h>
#include <json-c/json.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Global variables
int MAX_USERS = 50;
int BUFFER_LIMIT = 4096;
int user_counter = 0;
int universal_unique_id = 12345678

// Object tht will contain all the user data
typedef struct{
	int status;
	int socket_instance;
	int universal_unique_id;
	struct sockaddr_in ip_address;
	char name[20];
} user_obj;

json_object *all_chat;

user_obj *users[MAX_USERS];

pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

// This is pending TODO
void str_overwrite_stdout() {
	printf("\r%s", "> ");
	fflush(stdout);
}

// Removes lfs
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

// Shows user ip address
void print_client_addr(struct sockaddr_in addr){
	printf("IP: %d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xff,
		(addr.sin_addr.s_addr & 0xff00) >> 8,
		(addr.sin_addr.s_addr & 0xff0000) >> 16,
		(addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Add users to queue
void queue_add(user_obj *cl){
	pthread_mutex_lock(&users_mutex);
	int i;
	for(i=0; i < MAX_USERS; ++i){
		if(!users[i]){
			users[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&users_mutex);
}

// Remove users from queue
void queue_remove(int universal_unique_id){
	pthread_mutex_lock(&users_mutex);
	int i;
	for(i=0; i < MAX_USERS; ++i){
		if(users[i]){
			if(users[i]->universal_unique_id == universal_unique_id){
				users[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&users_mutex);
}

// Send message to all
void send_message(char *s, int universal_unique_id){
	pthread_mutex_lock(&users_mutex);
	int i;
	for(i=0; i<MAX_USERS; ++i){
		if(users[i]){
			if(users[i]->universal_unique_id != universal_unique_id){
				if(write(users[i]->socket_instance, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&users_mutex);
}

// Send message to only one user
void send_private_message(char *s, char *name){
		pthread_mutex_lock(&users_mutex);
		int i;
		for(i=0; i<MAX_USERS; ++i){
			if(users[i]){
				if(users[i]->name == name){
					if(write(users[i]->socket_instance, s, strlen(s)) < 0){
						perror("ERROR: write to descriptor failed");
						break;
					}
				}
			}
		}

	pthread_mutex_unlock(&users_mutex);
}

// Client manager
void *handle_client(void *arg){
	char buff_out[BUFFER_LIMIT];
	char conex_request[64];
	int leave_flag = 0;

	user_counter++;
	user_obj *cli = (user_obj *)arg;

	// Name validation
	if(recv(cli->socket_instance, conex_request, 64, 0) <= 0){
		printf("No se escribió el nombre... \n");
		leave_flag = 1;
	} else{
		json_object *parsed_json;
		json_object *body;
		json_object *name;
		parsed_json = json_tokener_parse(conex_request);

		json_object_object_get_ex(parsed_json,"body", &body);
		name = json_object_array_get_idx(body,1); 

		strcpy(cli->name, json_object_get_string(name));

		json_object *INIT_CONEX = json_object_new_object();
		json_object_object_add(INIT_CONEX, "response", json_object_new_string("INIT_CONEX"));
		json_object_object_add(INIT_CONEX, "code", json_object_new_int(200));
		write(cli->socket_instance, json_object_to_json_string(INIT_CONEX), 
		strlen(json_object_to_json_string(INIT_CONEX)));
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->universal_unique_id);
	}

	bzero(buff_out, BUFFER_LIMIT);

	// Socket will keep listening
	while(1){
		char temp_req[32];
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->socket_instance, buff_out, BUFFER_LIMIT, 0);
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

				if(strcmp(temp_req,"POST_CHAT")==0){
					json_object *message;
					json_object *from;
					json_object *delivered_at;
					json_object *to;
					char to_chat[256];

					message = json_object_array_get_idx(body,0);
					from = json_object_array_get_idx(body,1);
					delivered_at = json_object_array_get_idx(body,2);
					to = json_object_array_get_idx(body,3);

					sprintf(to_chat, "%s: %s at %s\n", json_object_get_string(from),
					json_object_get_string(message),json_object_get_string(delivered_at));
					json_object *aux_mssg = json_object_new_string(to_chat);
					if(1){
						printf("perfectly working from all post_chat\n");
						json_object_array_add(all_chat,aux_mssg);
						send_message(to_chat, cli->universal_unique_id);
					}else{
						char *aux_to = json_object_get_string(to);
						send_private_message(to_chat,aux_to);
					}

					//strcpy(mssg, json_object_get_string(message));
					json_object *POST_CHAT = json_object_new_object();
					json_object_object_add(POST_CHAT, "response", json_object_new_string("POST_CHAT"));
					json_object_object_add(POST_CHAT, "code", json_object_new_int(200));
					write(cli->socket_instance, json_object_to_json_string(POST_CHAT), 
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
						//pthread_mutex_lock(&users_mutex);
        		int i;
						for(i=0; i<MAX_USERS; ++i){
							if(users[i]){
								sprintf(user_info, "%s, status: %d\n", users[i]->name, users[i]->status);
								json_object_array_add(users,json_object_new_string(user_info));
                bzero(user_info, 56);
							}
        		}
        		//pthread_mutex_unlock(&users_mutex);
						json_object_object_add(GET_USER, "body", users);
					}else{
						//pthread_mutex_lock(&users_mutex);
						int i;
						for(i=0; i<MAX_USERS; ++i){
							if(users[i]){
								if(users[i]->name == user_type){}
								sprintf(user_info, "status: %d\n", users[i]->status);
							}
            }
						//pthread_mutex_unlock(&users_mutex);
						json_object_object_add(GET_USER, "body", json_object_new_string(user_info));
					}

					json_object_object_add(GET_USER, "response", json_object_new_string("GET_USER"));
					json_object_object_add(GET_USER, "code", json_object_new_int(200));
					write(cli->socket_instance, json_object_to_json_string(GET_USER), 
					strlen(json_object_to_json_string(GET_USER)));
				}
			}
		} else if (receive == 0 || strcmp(buff_out, "/exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			json_object *END_CONEX = json_object_new_object();
			json_object_object_add(END_CONEX, "response", json_object_new_string("END_CONEX"));
			json_object_object_add(END_CONEX, "code", json_object_new_int(200));
			write(cli->socket_instance, json_object_to_json_string(END_CONEX), 
			strlen(json_object_to_json_string(END_CONEX)));
			send_message(buff_out, cli->universal_unique_id);
			leave_flag = 1;
		} else {
			printf("Error 404 - Not Found\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_LIMIT);
		bzero(temp_req,32);
	}

  //Delete user from the list and end thread
	close(cli->socket_instance);
  queue_remove(cli->universal_unique_id);
 	free(cli);
  user_counter--;
  pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	all_chat = json_object_new_array();
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  // Socket configuration
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  // Ignore signals from the pipe
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
	}

	// Bind the socket to the addres and port
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR: Socket binding failed");
    return EXIT_FAILURE;
  }

  // Catch listener error
  if (listen(listenfd, 10) < 0) {
    perror("ERROR: Socket listening failed");
    return EXIT_FAILURE;
	}

	printf("Universidad del Valle de Guatemala\n");
	printf("Sistemas Operativos\n");
	printf("Autores: \n  Laura Tamath \n  Martín España \n");
	printf("Proyecto Chat\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		// Check max number of users in the server
		if((user_counter + 1) == MAX_USERS){
			char max_mssg[BUFFER_LIMIT];
			sprintf(max_mssg, "Max users reached. Rejected");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			write(connfd, max_mssg, strlen(max_mssg));
			close(connfd);
		}

		// User info settings
		user_obj *cli = (user_obj *)malloc(sizeof(user_obj));
		cli->ip_address = cli_addr;
		cli->socket_instance = connfd;
		cli->universal_unique_id = universal_unique_id++;
		cli->status = 0;

		// Add client and fork thread
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		// Save resources
		sleep(1);
	}

	return 0;
}
