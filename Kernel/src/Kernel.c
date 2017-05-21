#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/config.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "libreriaSockets.h"
#include "lib/list.h"
#define MAX_PCB 100
#define RUTA_ARCHIVO "/home/utnso/workspace/tp-2017-1c-C-digo-Facilito/Kernel/src/ConfigKernel.txt"

int cliente, cliente2, esperar = 0;

typedef struct {
	int puerto;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void msjConexionCon(char *s) {
	printf(
			"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			s);
} //Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio");
		exit(-1);
	}
	return puntero;
}

typedef struct {
	int id_proceso;
	int contador_paginas;
} t_pcb;

void aumentarContadorPagina(t_pcb *pcb) {
	pcb->contador_paginas++;
}

t_pcb *crearPCB() {
	t_pcb *punteroPCB;
	punteroPCB = reservarMemoria(sizeof(t_pcb));
	punteroPCB->contador_paginas = 0;
	punteroPCB->id_proceso = 1;
	return punteroPCB;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_PROG");
}

void leerArchivo() {
	if (access(RUTA_ARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTA_ARCHIVO);
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

void *proced_script(void *direccionServidor2, t_list *listaPCBs) {
	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	if (archivo == NULL) {
		printf("No se pudo escribir el archivo\n");
		return NULL;
	}
	u_int32_t fsize;
	if (recv(cliente, &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		return NULL;
	}

	char *bufferArchivo = reservarMemoria(fsize + 1);
	if (recv(cliente, bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		return NULL;
	}
	printf("%s\n\n", bufferArchivo);

	list_add(listaPCBs,crearPCB());

	fwrite(bufferArchivo, 1, fsize, archivo);

	free(bufferArchivo);
	fclose(archivo);

	//PARA MEMORIA

	conectar(&cliente2, direccionServidor2);
	handshake(&cliente2, kernel);
	msjConexionCon("una Memoria");

	FILE * archivo2 = fopen("prueba.txt", "rb");
	if (archivo == NULL) {
		printf("No se pudo leer el archivo\n");
		return NULL;
	}
	fseek(archivo2, 0, SEEK_END);
	u_int32_t fsize2 = ftell(archivo2);
	fseek(archivo2, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize2 + 1);
	fread(buffer, fsize2, 1, archivo2);

	buffer[fsize2] = '\0';
	if (send(cliente2, &fsize2, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		return NULL;
	}
	if (send(cliente2, buffer, fsize2 + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		return NULL;
	}
	printf("El archivo se envió correctamente\n");
	free(buffer);
	fclose(archivo2);
	esperar = 0;
	return NULL;
}

int main(void) {

	//Cuándo lee el archivo?
	int master_socket, addrlen, client_socket[30], activity, valread, sd;
	int max_sd;
	struct sockaddr_in direccionServidor;
	char buffer[1025];
	fd_set readfds;

	t_list *listaPCBs;
	listaPCBs = list_create();

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	//PARA MEMORIA
	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor2.sin_port = htons(8125);
	//

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
		while (esperar == 1)
			;
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

			mostrarConexion(cliente, direccionServidor);

			int procesoConectado = handshake(&cliente, kernel);

			switch (procesoConectado) {
			case consola:
				msjConexionCon("una Consola\n");
				agregarSocket(client_socket, &cliente);
				printf("Creando hilo para consola...\n");
				/*pthread_t thread_ID;
				if (pthread_create(&thread_ID, NULL, proced_consola,
						&direccionServidor2)) {
					printf("Error al crear el thread.\n");
					break;
				}*/
				//proced_consola(&direccionServidor2);

				proced_script(&direccionServidor2,listaPCBs);

				esperar = 1;
				//agregarThread(threads_clientes, thread_ID);

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

		/*
		list_destroy_and_destroy_elements(listaPCBs,);
		Que va en el segundo parametro del proced de arriba???
		*/

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
