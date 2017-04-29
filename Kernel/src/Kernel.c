#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/config.h>
#include <unistd.h>
#define LONGMAX 1000
#define RUTAARCHIVO "/home/utnso/workspace/tp-2017-1c-C-digo-Facilito/Kernel/src/ConfigKernel.txt"
typedef struct {
	int puerto;
} t_configuracion;
t_configuracion *config;

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_PROG");
}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	printf("Leí el archivo y extraje el puerto: %d", config->puerto);
}

int esperarConexion(int *servidor, struct sockaddr_in *direccionServidor) {

	(*servidor) = socket(AF_INET, SOCK_STREAM, 0);
	int activado = 1;
	setsockopt((*servidor), SOL_SOCKET, SO_REUSEADDR, &activado,
			sizeof(activado));

	if (bind((*servidor), (void*) &(*direccionServidor),
			sizeof((*direccionServidor))) != 0) {
		perror("Falló el bind.");
		return 1;
	}

	printf("Estoy escuchando");
	listen((*servidor), SOMAXCONN);

	return 0;
}

void aceptarConexion(int *servidor, int *cliente) {

	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	(*cliente) = accept((*servidor), (void*) &direccionCliente,
			&tamanioDireccion);

	printf("Recibí una conexión en %d!!\n", (*cliente));
}

int recibirMensajeDe(int *cliente, char *buffer) {

	int bytesRecibidos = recv((*cliente), buffer, LONGMAX, 0);
	if (bytesRecibidos <= 0) {
		perror("El chabón se desconectó o bla.");
		return 1;
	}

	buffer[bytesRecibidos] = '\0';
	printf("Me llegaron %d bytes con %s\n", bytesRecibidos, buffer);

	return 0;
}

int conectar(int *cliente, struct sockaddr_in *direccionServidor) {

	(*cliente) = socket(AF_INET, SOCK_STREAM, 0);
	if (connect((*cliente), (void*) &(*direccionServidor),
			sizeof((*direccionServidor))) != 0) {
		perror("No se pudo conectar");
		return 1;
	}

	return 0;
}

void faltaDeParametros(int argc) {
	if (argc == 1) {
		printf("Te falto el parametro  \n");
	}
	exit(-1);
}

int main(void) {

	int opt = 1;
	int master_socket, addrlen, cliente, client_socket[30], max_clients = 30,activity, i, valread, sd;
	int max_sd;
	struct sockaddr_in direccionServidor;

	char buffer[1025];
	fd_set readfds;

	for (i = 0; i < max_clients; i++) {
		client_socket[i] = 0;
	}

	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//setea master socket para que reciba multiples conexiones
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
			sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(8080);

	if (bind(master_socket, (struct sockaddr *) &direccionServidor, sizeof(direccionServidor))
			< 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener en puerto %d \n", 8080);

	//maximo de 3 conexiones pendientes
	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(direccionServidor);
	puts("Esperando conexiones...");

	while (1) {
		FD_ZERO(&readfds);

		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i];

			if (sd > 0)
				FD_SET(sd, &readfds);

			if (sd > max_sd)
				max_sd = sd;
		}

		//espera indefinidamente actividad en algun socket
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if (activity < 0) {
			printf("select error");
		}

		//maneja conexion entrante
		if (FD_ISSET(master_socket, &readfds)) {
			if ((cliente = accept(master_socket,
					(struct sockaddr *) &direccionServidor, (socklen_t*) &addrlen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//info que por ahi es util
			printf(
					"Nueva conexion , socket fd: %d , ip: %s , puerto: %d \n",
					cliente, inet_ntoa(direccionServidor.sin_addr),
					ntohs(direccionServidor.sin_port));

			recibirMensajeDe(&cliente, buffer);

			//agrega nuevo socket al array de sockets
			for (i = 0; i < max_clients; i++) {
				if (client_socket[i] == 0) {
					client_socket[i] = cliente;
					printf("Agregando a conjunto de sockets como %d\n", i);
					break;
				}
			}
		}

		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds)) {
				if ((valread = read(sd, buffer, 1024)) == 0) {
					getpeername(sd, (struct sockaddr*) &direccionServidor,
							(socklen_t*) &addrlen);
					printf("Cliente desconectado , ip %s , puerto %d \n",
							inet_ntoa(direccionServidor.sin_addr),
							ntohs(direccionServidor.sin_port));

					close(sd);
					client_socket[i] = 0;
				}
			}
		}
	}

	return 0;

	/*struct sockaddr_in direccionServidor;
	 direccionServidor.sin_family = AF_INET;
	 direccionServidor.sin_addr.s_addr = INADDR_ANY;
	 direccionServidor.sin_port = htons(8080);

	 int servidor;
	 int clienteConsola;
	 char* buffer = malloc(LONGMAX);

	 leerArchivo();

	 esperarConexion(&servidor, &direccionServidor);
	 aceptarConexion(&servidor, &clienteConsola);
	 recibirMensajeDe(&clienteConsola, buffer);
	 close(servidor);

	 free(buffer);

	 return 0;*/
}
