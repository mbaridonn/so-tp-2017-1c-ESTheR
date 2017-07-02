#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/config.h>
#include <unistd.h>
#include <pthread.h>
#include "libreriaSockets.h"
#include "lib/list.h"
#include "lib/pcb.h"
#include "conexionesSelect.h"
#include "estructurasComunes.h"
#include "accionesDeKernel.h"

#define RUTA_ARCHIVO "/home/utnso/workspace/tp-2017-1c-C-digo-Facilito/Kernel/src/ConfigKernel.txt"

typedef struct {
	int puerto;
} t_configuracion;
t_configuracion *config;

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

void conectarseConMemoria(int *servMemoria,
	struct sockaddr_in *direccionServidor2) {
	conectar(&servMemoria, direccionServidor2);
	handshake(&servMemoria, kernel);
	msjConexionCon("una Memoria");
}

int obtenerTamanioDePagina(int *servMemoria) {
	int tamPaginaMemoria;
	if (recv((*servMemoria), &tamPaginaMemoria, sizeof(int), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		return -1;
	}
	return tamPaginaMemoria;
}

int main(void) {

	//Cuándo lee el archivo?
	int client_socket[30], procesos_por_socket[30], i, procesoConectado,
			servMemoria;
	u_int32_t tamanioPagMemoria;
	struct sockaddr_in direccionServidor;

	t_list *listaPCBs_NEW = list_create();
	t_list *listaPCBs_READY = list_create();
	t_list *listaPCBs_EXEC = list_create();
	t_list *listaPCBs_BLOCK = list_create();
	t_list *listaPCBs_EXIT = list_create();

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	//PARA MEMORIA
	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor2.sin_port = htons(8125);

	//conectarseConMemoria(&servMemoria, &direccionServidor2);
	//Te deshice la abstraccion de arriba porque rompia con el tamaño de Pag

	conectar(&servMemoria, &direccionServidor2);
	handshake(&servMemoria, kernel);
	msjConexionCon("una Memoria");

	tamanioPagMemoria = obtenerTamanioDePagina(&servMemoria);
	printf("El tamanio recibido es: %d\n",tamanioPagMemoria);

	inicializarVec(client_socket);
	inicializarVec(procesos_por_socket);

	esperarConexionDe(&direccionServidor);

	while (1) {
		prepararSockets(client_socket);
		esperarActividad();
		if (esConexionEntrante()) {
			manejarConexionEntrante(&direccionServidor); //La vieja y confiable.
			mostrarNuevaConexion(direccionServidor);

			procesoConectado = realizarHandshake(kernel);

			switch (procesoConectado) {
			case consola:
				msjConexionCon("una Consola\n");
				setInformacionSockets(client_socket, procesos_por_socket,
						consola);
				break;

			case cpu:
				msjConexionCon("CPU");
				break;

			case file_system:
				printf("Me conecte con File System!\n");
				break;

			default:
				printf("No me puedo conectar con vos.\n");
				break;
			}
		}

		/*
		 list_destroy_and_destroy_elements(listaPCBs_NEW,);
		 Que va en el segundo parametro del proced de arriba???
		 */

		setClienteActual(socketQueTuvoActividad(client_socket));
		i = numeroSocketQueTuvoActividad(client_socket);
		if (clienteActualSeDesconecto()) {
			cerrarConexionClienteActual(&direccionServidor);
			liberarPosicion(client_socket, i);
			liberarPosicion(procesos_por_socket, i);
		} else {
			int proceso = procesos_por_socket[i];
			switch (proceso) { // ACA VAN TODOS LOS CASES DE CUANDO HAY MOVIMIENTO EN UN SOCKET PORQUE SOLICITA ALGO
			case consola:
				printf("Hubo movimiento en una consola\n");
				confirmarAtencionA(&client_socket[i]);
				atenderAConsola(&servMemoria, listaPCBs_NEW,
						&client_socket[i]);
				break;
			default:
				break;
			}

		}
	}

	return 0;
}
