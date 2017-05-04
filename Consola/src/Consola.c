#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "

typedef struct {
	//char ipKernel[10]; FALTA IMPLEMENTAR
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
	recv((*cliente), procesoAConocer, 2, 0);
	send((*cliente), unProceso, 2, 0);
	return atoi(procesoAConocer);
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio \n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	//config->ipKernel = config_get_string_value(archivo_Modelo,"IP_KERNEL");
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
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
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
	printf("Leí el archivo y extraje el puerto: %d \n", config->puerto);
}

int main(void) {

	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");//En realidad se le debería pasar config->ipKernel
	direccionServidor.sin_port = htons(config->puerto);

	int cliente;

	conectar(&cliente, &direccionServidor);

	int procesoConectado = nuevohandshake(&cliente, consola);

	switch (procesoConectado) {
	case kernel:
		msjConexionCon("Kernel");

		FILE * archivo = fopen("prueba.txt", "rb");
		if (archivo == NULL) {
			printf("No se pudo leer el archivo\n");
			return EXIT_FAILURE;
		}
		fseek(archivo, 0, SEEK_END);
		u_int32_t fsize = ftell(archivo);
		fseek(archivo, 0, SEEK_SET);

		char *buffer = malloc(fsize + 1);
		fread(buffer, fsize, 1, archivo);
		fclose(archivo);
		buffer[fsize] = '\0';
		if (send(cliente, &fsize, sizeof(u_int32_t), 0) == -1) {
			printf("Error enviando longitud del archivo\n");
			return EXIT_FAILURE;
		}
		if (send(cliente, buffer, fsize+1, 0) == -1) {
			printf("Error enviando archivo\n");
			return EXIT_FAILURE;
		}
		printf("El archivo se envió correctamente\n");

		break;

	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(cliente);

	return 0;
}
