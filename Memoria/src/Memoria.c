#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/src/configMemoria.txt"

typedef struct {
	int puerto;
	int cantFrames;
	int tamFrame;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void *reservarMemoria(int tamanio) {
	void *puntero = malloc(tamanio);
	if (puntero == NULL) {
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO");
	config->cantFrames = config_get_int_value(archivo_Modelo, "MARCOS");
	config->tamFrame = config_get_int_value(archivo_Modelo, "MARCO_SIZE");
}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo\n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	printf("Leí el archivo y extraje el puerto: %d\n", config->puerto);
}

int main(void) {

	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(config->puerto);

	int servidor;
	int cliente;
	char* buffer = reservarMemoria(LONGMAX);

	int memoriaTotal = config->tamFrame * config->cantFrames;

	char* memoria = reservarMemoria(memoriaTotal); //Por ahora, esta va a ser la memoria es un único bloque no paginado

	esperarConexion(&servidor, &direccionServidor);
	aceptarConexion(&servidor, &cliente);
	int laMemoria = memoria;
	int *proceso;
	handshake(&cliente, &laMemoria, proceso);
	int procesoConectado = *proceso;
	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		recibirMensajeDe(&cliente, buffer);
		break;

	case cpu:
		printf("Me conecte con CPU!\n");
		break;

	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}
	close(servidor);

	/*struct sockaddr_in direccionServidor;
	 direccionServidor.sin_family = AF_INET;
	 direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	 direccionServidor.sin_port = htons(8080);

	 int cliente;
	 char* buffer = malloc(LONGMAX);

	 conectar(&cliente, &direccionServidor);

	 recibirMensajeDe(&cliente, buffer);

	 close(cliente);*/

	return 0;
}
