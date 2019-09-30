#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#define PORTTCP 3490
#define MY_IP "127.0.0.1"
#define MAX_QUEUE 10
#define MAX_MSG_SIZE 1024

#define HOSTUDP "localhost"
#define PORTUDP "9876"

#define PORTTCP1 "143"

#define MENSAJE_ERROR_USUARIO_MAIL

struct arg_struct {
	int arg1;
	int arg2;
	char *data;
};

int findFin(char *data) {
	//return strchr(data, '\n') != NULL;
	char *tmp = data;
	int i;
	for (i = 0; *tmp != '\0' && *tmp != '\n'; i++) {
		tmp += 1;
	}
	return *tmp == '\n';
}

char* extraer_usuario(char *data) {
	char *tmp = data;
	int i;
	for (i = 0; *tmp != '\0' && *tmp != ' '; i++) {
		tmp += 1;
	}

	// char* identifier;
	// strncpy ( identifier, data, i );
	if (*tmp != '\0')
		tmp += 1;

	// salteo el "login" (no hago chequeo de que diga login)
	tmp = strchr(tmp, ' ');

	if (tmp != NULL)
		tmp += 1;

	char *aux = tmp;
	for (i = 0; *tmp != '\0' && *tmp != ' '; i++) {
		tmp += 1;
	}

	char *usuario = malloc((i + 1) * sizeof(char));
	strncpy(usuario, aux, i);
	usuario[i] = '\0';

	return usuario;
}

void partir_por_espacio(char *data, char **antes, char **despues) {
	char *tmp = data;
	int i;
	for (i = 0; *tmp != '\0' && *tmp != ' '; i++) {
		tmp += 1;
	}

	*antes = malloc((i + 1) * sizeof(char));
	strncpy(*antes, data, i);
	*(*antes + i) = '\0';

	tmp += 1;

	char *aux = tmp;

	for (i = 0; *tmp != '\0' && *tmp != ' '; i++) {
		tmp += 1;
	}

	*despues = malloc((i + 1) * sizeof(char));
	strncpy(*despues, aux, i);
	*(*despues + i) = '\0';

}

void pushData(void *args) {
	int sent_data_size;
	char *data = malloc(MAX_MSG_SIZE);
	int data_size = MAX_MSG_SIZE;

	int received_data_size;

	int src = ((struct arg_struct*) args)->arg1;
	int dst = ((struct arg_struct*) args)->arg2;

	if (strcmp("", ((struct arg_struct*) args)->data)) {
		sent_data_size = send(dst, ((struct arg_struct*) args)->data,
				strlen(((struct arg_struct*) args)->data), 0);
	}

	while (1) {

		ssize_t size;

		if ((size = received_data_size = recv(src, data, data_size, 0)) == -1) {
			fprintf(stderr, "recv: %s (%d)\n", strerror(errno), errno);
		}

		if (received_data_size <= 0)
			break;

		sent_data_size = send(dst, data, received_data_size, 0);

		if (sent_data_size < 0)
			break;

	}

	shutdown(src, SHUT_RDWR);
	shutdown(dst, SHUT_RDWR);
	close(src);
	close(dst);

}
char* dameTag(char *data) {
	char *tmp = data;
	int i;
	for (i = 0; *tmp != '\0' && *tmp != ' '; i++) {
		tmp += 1;
	}

	char *antes = malloc((i + 1) * sizeof(char));
	strncpy(antes, data, i);
	antes[i] = '\0';
	return antes;
}

char* respuestaServer(char *entrada) {
	char *aux;
	char *tagFin;
	char *comandoInicio;
	char *comandoFin;

	tagFin = strchr(entrada, ' ');
	if (tagFin == NULL) {
		return "* BAD Error in IMAP command received by server.\n";
	}
	if (tagFin + 1 == "\n") {
		return "* BAD Error in IMAP command received by server.\n";
	}
	comandoInicio = tagFin + 1;
	aux = comandoInicio;
	tagFin = tagFin - 1;

	aux = strchr(comandoInicio, ' ');
	if (aux == NULL) {
		aux = strchr(comandoInicio, '\r');
	}

	int tamComando = 0;
	char *iter = comandoInicio;
	for (; iter != aux; iter++) {
		tamComando += 1;
	}
	char *respuesta = malloc(strlen(comandoInicio) + 1);
	strncpy(respuesta, comandoInicio, tamComando);
	respuesta[tamComando] = '\0';

	aux = respuesta;
	while (*respuesta != '\0') {
		*respuesta = toupper(*respuesta);
		respuesta++;
	}
	respuesta = aux;

	if (strcmp(respuesta, "CAPABILITY") == 0) {
		free(respuesta);

		char *tag = dameTag(entrada);
		int tamanio =
				strlen(
						"* CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE AUTH=PLAIN\n")
						+ strlen(tag)
						+ strlen(
								" OK Pre-login capabilities listed, post-login capabilities have more.\n")
						+ 1;
		char *ret = malloc(tamanio);
		strcpy(ret,
				"* CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE AUTH=PLAIN\n");
		strcat(ret, tag);
		strcat(ret,
				" OK Pre-login capabilities listed, post-login capabilities have more.\n");
		return ret;
	}
	if (strcmp(respuesta, "STARTTLS") == 0) {
		free(respuesta);

		char *tag = dameTag(entrada);
		int tamanio = strlen(tag) + strlen(" BAD TLS support isn't enabled.\n")
				+ 1;
		char *ret = malloc(tamanio);
		strcpy(ret, tag);
		strcat(ret, " BAD TLS support isn't enabled.\n");
		return ret;
	}
	if (strcmp(respuesta, "AUTHENTICATE") == 0) {
		free(respuesta);

		char *tag = dameTag(entrada);
		int tamanio = strlen(tag)
				+ strlen(" NO [ALERT] Unsupported authentication mechanism.\n")
				+ 1;
		char *ret = malloc(tamanio);
		strcpy(ret, tag);
		strcat(ret, " NO [ALERT] Unsupported authentication mechanism.\n");
		return ret;
	}
	if (strcmp(respuesta, "LOGIN") == 0) {
		free(respuesta);
		return "LOGIN";
	}
	if (strcmp(respuesta, "NOOP") == 0) {
		free(respuesta);
		char *tag = dameTag(entrada);
		int tamanio = strlen(tag) + strlen(" OK NOOP completed.\n") + 1;
		char *ret = malloc(tamanio);
		strcpy(ret, tag);
		strcat(ret, " OK NOOP completed.\n");
		return ret;
	}
	if (strcmp(respuesta, "LOGOUT") == 0) {
		free(respuesta);
		return "LOGOUT";
	} else {
		free(respuesta);
		char *tag = dameTag(entrada);
		int tamanio = strlen(tag)
				+ strlen(" BAD Error in IMAP command received by server.\n")
				+ 1;
		char *ret = malloc(tamanio);
		strcpy(ret, tag);
		strcat(ret, " BAD Error in IMAP command received by server.\n");
		return ret;
	}

}

int esValidoString(char *args) {
	char *temp = strchr(args, ' ');
	if (temp != NULL) {
		temp += 1;
		temp = strchr(temp, ' ');
	}
	return temp != NULL;
}

void atenderCliente(void *args) {
	int socket_to_client = ((struct arg_struct*) args)->arg1;
	int identificador = ((struct arg_struct*) args)->arg2;
	char *id_verdadera = malloc(12);
	sprintf(id_verdadera, "%d", identificador);

	//primitiva SEND
	int sizeMensaje =
			strlen(
					"* OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE AUTH=PLAIN] Dovecot ready.\n");
	int sent_data_size =
			send(socket_to_client,
					"* OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE AUTH=PLAIN] Dovecot ready.\n",
					sizeMensaje, 0);
	printf(
			"Enviado al cliente (%d bytes): * OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE IDLE AUTH=PLAIN] Dovecot ready.\n",
			sent_data_size);

	//primitiva RECEIVE
	char *data_cliente = malloc(MAX_MSG_SIZE);
	char *data_recieved_cliente = malloc(MAX_MSG_SIZE);
	char *ayuda = malloc(MAX_MSG_SIZE);

	int data_size = MAX_MSG_SIZE;
	int received_data_size;
	int intentosValidos = 0;
	char *respuesta = NULL;
	char *id_respuesta;
	char *ip_respuesta;
	int recibi = 0;
	int error = 0;
	do {
		recibi = 0;
		intentosValidos = 0;
		do {

			if (respuesta != NULL && error == 0) {
				char *tag = dameTag(data_cliente);
				int tamanio =
						strlen(tag)
								+ strlen(
										" BAD Error in IMAP command received by server.\n")
								+ 1;
				char *ret = malloc(tamanio);
				strcpy(ret, tag);
				strcat(ret, " BAD Error in IMAP command received by server.\n");
				int sent_data_size = send(socket_to_client, ret, tamanio, 0);
				printf("Enviado al cliente (%d bytes): %s", tamanio, ret);
				free(ret);
			}
			error = 0;
			respuesta = "PEPE";
			while ((strcmp(respuesta, "LOGIN") != 0) && (intentosValidos < 7)) {
				strcpy(data_cliente, "");
				do {

					received_data_size = recv(socket_to_client,
							data_recieved_cliente, data_size, 0);

					char *aux = strchr(data_recieved_cliente, '\n');
					char *aux2 = data_recieved_cliente;
					char *aux3 = ayuda;

					if (aux != NULL) {
						while (aux2 != aux) {
							*aux3 = *aux2;
							aux3 += 1;
							aux2 += 1;
						}
						*aux3 = '\n';
						strcpy(data_recieved_cliente, ayuda);
					}

					strcat(data_cliente, data_recieved_cliente);
					strcpy(data_recieved_cliente, "");

				} while (received_data_size >= 0 && !findFin(data_cliente));
				char *auxr = strchr(data_cliente, '\r');
				char *auxn = strchr(auxr, '\n');
				*(auxn + 1) = '\0';

				printf("Enviado por el cliente: %s\n", data_cliente);

				intentosValidos += 1;

				respuesta = respuestaServer(data_cliente);

				if (strcmp(respuesta, "LOGOUT") == 0) {
					char *tag = dameTag(data_cliente);
					int tamanio = strlen("* BYE Logging out\n") + strlen(tag)
							+ strlen(" OK Logout completed.\n") + 1;
					char *ret = malloc(tamanio);
					strcpy(ret, "* BYE Logging out\n");
					strcat(ret, tag);
					strcat(ret, " OK Logout completed.\n");

					int sent_data_size = send(socket_to_client, ret, tamanio,
							0);
					printf("Enviado al cliente (%d bytes): %s\n", tamanio, ret);
					free(id_verdadera);
					free(data_cliente);
					free(ret);
					close(socket_to_client);
					return;
				}
				if (strcmp(respuesta, "LOGIN") != 0) {
					int sizeMensaje = strlen(respuesta);
					int sent_data_size = send(socket_to_client, respuesta,
							sizeMensaje, 0);
					printf("Enviado al cliente (%d bytes): %s\n",
							sent_data_size, respuesta);
				}
			}

			if (intentosValidos >= 7) {
				//liberar memoria MANUEL
				int sizeMensaje = strlen(
						"* BYE Too many invalid IMAP commands.\n");
				int sent_data_size = send(socket_to_client,
						"* BYE Too many invalid IMAP commands.\n", sizeMensaje,
						0);
				printf("Enviado al cliente (%d bytes): %s\n", sent_data_size,
						respuesta);

				free(id_verdadera);
				free(data_cliente);
				close(socket_to_client);
				return;
			}

		} while (!esValidoString(data_cliente));

		char *usuario = extraer_usuario(data_cliente);

		//primitiva SOCKET UDP
		int client_socket = socket(AF_INET, SOCK_DGRAM, 0);

		//obtenemos la direccion con getaddrinfo
		struct addrinfo hints, *res;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		int pepe = getaddrinfo(HOSTUDP, PORTUDP, &hints, &res);

		printf("Esto es lo que tien PEPE %s\n", gai_strerror(pepe));

		int intentos = 0;

		char *data_serverInfo = malloc(MAX_MSG_SIZE);

		char *auxMsg = malloc(12);
		auxMsg = strcpy(auxMsg, id_verdadera);

		char *msg = strcat(strcat(auxMsg, " "), usuario);

		int msg_size = strlen(msg);
		while (intentos < 10 && !recibi) {

			//primitiva SEND

			int sent_msg_size = sendto(client_socket, msg, msg_size, 0,
					res->ai_addr, res->ai_addrlen);

			printf("Enviado al servidor (%d bytes): %s\n", sent_msg_size, msg);

			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

			//primitiva RECEIVE
			int data_size = MAX_MSG_SIZE;
			struct sockaddr_in server_addr;
			int server_addr_size = sizeof(server_addr);
			int received_data_size = recvfrom(client_socket, data_serverInfo,
					data_size, 0, (struct sockaddr*) &server_addr,
					&server_addr_size);

			if (received_data_size >= 0) {
				partir_por_espacio(data_serverInfo, &id_respuesta,
						&ip_respuesta);

				if (!strcmp(id_respuesta, id_verdadera)) {
					recibi = 1;
				}
			} else {
				intentos++;
			}

		}
		if(intentos >= 10 && recibi != 1){
			free(ayuda);
			free(data_recieved_cliente);
			free(data_cliente);
			close(socket_to_client);
			return;

		}

		printf("Recibido del servidor (%d bytes): %s\n", received_data_size,
				data_serverInfo);

		if(strcmp(ip_respuesta, "ERROR") == 0){

			error = 1;
			char *tag = dameTag(data_cliente);
								int tamanio = strlen(tag)
										+ strlen(" NO [AUTHENTICATIONFAILED] Authentication failed.\n") + 1;
								char *ret = malloc(tamanio);
								*ret = '\0';
								strcat(ret, tag);
								strcat(ret, " NO [AUTHENTICATIONFAILED] Authentication failed.\n");
						send(socket_to_client,
								ret,
								tamanio, 0);
						free(ret);
		}

	} while (strcmp(ip_respuesta, "ERROR") == 0);
	if (recibi) {
		//if (server_addr.sa_data == ?){
		if (strcmp(ip_respuesta, "ERROR")) {

			//primitiva SOCKET
			int serverIMAP_socket = socket(AF_INET, SOCK_STREAM, 0);

			//obtenemos la direccion con getaddrinfo
			struct addrinfo hints1, *res1;
			memset(&hints1, 0, sizeof hints1);
			hints1.ai_family = AF_INET;
			hints1.ai_socktype = SOCK_STREAM;
			getaddrinfo(ip_respuesta, PORTTCP1, &hints1, &res1);

			//primitiva CONNECT
			connect(serverIMAP_socket, res1->ai_addr, res1->ai_addrlen);

			char *skip = malloc(MAX_MSG_SIZE);
			strcpy(skip, "");
			do {
				//primitiva RECEIVE
				char *data_serverIMAP = malloc(MAX_MSG_SIZE);
				int data_size = MAX_MSG_SIZE;
				received_data_size = recv(serverIMAP_socket, data_serverIMAP,
						data_size, 0);
				strcat(skip, data_serverIMAP);
			} while (received_data_size >= 0 && !findFin(skip));

			printf("Recibido del servidor (%d bytes): %s\n", received_data_size,
					skip);

			if (received_data_size >= 0) {
				//hilos para enviar datos y
				//esperar respuesta de los servidores y del cliente

				pthread_t threads1;
				pthread_t threads2;

				struct arg_struct args1;
				args1.arg1 = socket_to_client;
				args1.arg2 = serverIMAP_socket;
				args1.data = data_cliente;
				pthread_create(&threads1, NULL, pushData, (void*) &args1);

				struct arg_struct args2;
				args2.arg1 = serverIMAP_socket;
				args2.arg2 = socket_to_client;
				args2.data = "";
				pthread_create(&threads2, NULL, pushData, (void*) &args2);

				pthread_join(threads1, NULL) || pthread_join(threads2, NULL); /* Wait until threads are finished */

			} else {
				//primitiva CLOSE
				close(serverIMAP_socket);
			}

			freeaddrinfo(res1);

		} else {
			// Falta concatenar el tag
			free(ip_respuesta);
			free(id_respuesta);

			sizeMensaje = strlen(
					"NO [AUTHENTICATIONFAILED] Authentication failed.");
			send(socket_to_client,
					"NO [AUTHENTICATIONFAILED] Authentication failed.",
					sizeMensaje, 0);
			//primitiva CLOSE
			close(socket_to_client);

		}
		//}else{//primitiva CLOSE close(client_socket);}
	} else {
		free(ip_respuesta);
		free(id_respuesta);

		//primitiva CLOSE
		close(socket_to_client);
	}
	//end if data_cliente

}

int main(void) {
	//primitiva SOCKET TCP
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	//primitiva BIND
	struct sockaddr_in server_addr;
	socklen_t server_addr_size = sizeof server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORTTCP);
	server_addr.sin_addr.s_addr = inet_addr(MY_IP);
	bind(server_socket, (struct sockaddr*) &server_addr, server_addr_size);

	//primitiva LISTEN
	listen(server_socket, MAX_QUEUE);

	int identificador = 0;

	while (1) {
		//primitiva ACCEPT
		struct sockaddr_in client_addr;
		socklen_t client_addr_size = sizeof client_addr;
		int socket_to_client = accept(server_socket,
				(struct sockaddr*) &client_addr, &client_addr_size);

		struct arg_struct args;
		args.arg1 = socket_to_client;
		args.arg2 = identificador;
		args.data = NULL;

		pthread_t threads;
		pthread_create(&threads, NULL, atenderCliente, (void*) &args);

		//primitiva CLOSE
		//close(socket_to_client);

		identificador += 3;
	}

	//CLOSE del socket que espera conexiones
	close(server_socket);
}
