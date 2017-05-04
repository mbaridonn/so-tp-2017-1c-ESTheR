#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/FileSystem/src/configFyleSystem"

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

int nuevohandshake(int *cliente, int proceso) {
	char unProceso[2]; unProceso[0] = '0' + proceso; unProceso[1] = '\0';
	char procesoAConocer[2];
	send((*cliente), unProceso, 2, 0);
	recv((*cliente), procesoAConocer, 2, 0);
	return atoi(procesoAConocer);
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO");
}

void mostrarArchivoConfig(){
	 FILE *f;

	 f=fopen(RUTAARCHIVO,"r");
	 int c;
	 printf("------------------------------------------\n");
	 while ((c = fgetc (f)) != EOF) putchar(c);
	 printf("\n");
	 printf("------------------------------------------\n");

}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo\n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
	printf("Leí el archivo y extraje el puerto: %d\n", config->puerto);
}


int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	leerArchivo();
	int cliente;
	char* buffer = malloc(LONGMAX);

	conectar(&cliente, &direccionServidor);

	int procesoConectado = nuevohandshake(&cliente, file_system);
	switch (procesoConectado) {
	case kernel:
		msjConexionCon("Kernel");
		recibirMensajeDe(&cliente, buffer);
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(cliente);

	return 0;
}
