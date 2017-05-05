#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/config.h>
#include <unistd.h>
#include <stdio.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/workspace/tp-2017-1c-C-digo-Facilito/Kernel/src/ConfigKernel.txt"

typedef struct {
	int puerto;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void msjConexionCon(char *s){
	printf("\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
} //Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio");
		exit(-1);
	}
	return puntero;
}

/*typedef struct {
	int id_Proceso;
	int contador_Paginas;
}t_pcb;

void aumentarContadorPagina(t_pcb *pcb){
	pcb->contador_Paginas++;
}
t_pcb *crearPCB(){
	t_pcb *punteroPCB;
	punteroPCB = reservarMemoria(sizeof(t_pcb));
	punteroPCB->id_Proceso = 1;
	return punteroPCB;
}*/


void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_PROG");
}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	printf("Leí el archivo y extraje el puerto: %d", config->puerto);
}

void faltaDeParametros(int argc) {
	if (argc == 1) {
		printf("Te falto el parametro  \n");
	}
	exit(-1);
}

void mostrarConexion(int cliente, struct sockaddr_in direccionServidor) {
	printf("Nueva conexion , socket fd: %d , ip: %s , puerto: %d \n", cliente,
			inet_ntoa(direccionServidor.sin_addr),
			ntohs(direccionServidor.sin_port));
}

int main(void) {

	//Cuándo lee el archivo?
	int opt = 1;
	int master_socket, addrlen, cliente, client_socket[30], activity, valread, sd;
	int max_sd;
	struct sockaddr_in direccionServidor;

	char buffer[1025];
	fd_set readfds;

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(8080);

	int i;
	for (i = 0; i < MAX_CLIENTS; i++) {
		client_socket[i] = 0;
	}

	esperarConexion(&master_socket, &direccionServidor);

	addrlen = sizeof(direccionServidor);
	puts("Esperando conexiones...");

	while (1) {
		FD_ZERO(&readfds);
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		int i;
		for (i = 0; i < MAX_CLIENTS; i++) {
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
					(struct sockaddr *) &direccionServidor,
					(socklen_t*) &addrlen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			mostrarConexion(cliente,direccionServidor);

			int procesoConectado = handshake(&cliente, kernel);
			switch (procesoConectado) {
			case consola:
				msjConexionCon("una Consola");
				FILE *archivo;
				archivo = fopen("prueba.txt", "w");
				if (archivo == NULL) {
					printf("No se pudo escribir el archivo\n");
					return EXIT_FAILURE;
				}

				u_int32_t fsize;
				if (recv(cliente, &fsize, sizeof(u_int32_t), 0) == -1) {
					printf("Error recibiendo longitud del archivo\n");
					return EXIT_FAILURE;
				}
				char *bufferArchivo = malloc(fsize + 1);
				if (recv(cliente, bufferArchivo, fsize+1, 0) == -1) {
					printf("Error recibiendo el archivo\n");
					return EXIT_FAILURE;
				}
				printf("%s\n\n",bufferArchivo);

				fwrite(bufferArchivo, 1, fsize+1, archivo);
				fclose(archivo);

				//agrega nuevo socket al array de sockets    NO CONVENDRÍA QUE ESTÉ AL PRINCIPIO?
				agregarSocket(client_socket, &cliente);
				break;

			case cpu:
				msjConexionCon("CPU");
				break;

			/*case memoria:                                 SON NECESARIOS??
				printf("Me conecte con Memoria!\n");
				break;
			case file_system:
				printf("Me conecte con File System!\n");
				break;*/
			default:
				printf("No me puedo conectar con vos.\n");
				break;
			}
		}

		//cierra todas las conexiones
		for (i = 0; i < MAX_CLIENTS; i++) {
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
}
