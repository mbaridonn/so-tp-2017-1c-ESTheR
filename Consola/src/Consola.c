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

void msjConexionCon(char *s) {
	printf(
			"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			s);
} //Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.

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
void mostrarArchivoConfig() {
	FILE *f;

	f = fopen(RUTAARCHIVO, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
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

void mandarArchivo(int *cliente) {

	char *lineaIngresada, *comando, *nombreArchivo;
	FILE *archivo;

	printf("Ingrese mandarArchivo + nombre de su archivo\n");
	printf("Ejemplo: mandarArchivo prueba.txt\n\n");

	lineaIngresada = malloc(100);

	fgets(lineaIngresada, 100, stdin);

	comando = strtok(lineaIngresada, " ");
	nombreArchivo = strtok(NULL, "\n");

	if (strcmp("mandarArchivo", comando) != 0) {
		printf("El comando ingresado no existe");
	} else {
		archivo = fopen(nombreArchivo, "rb");
		if (archivo == NULL) {
			printf("No se pudo leer el archivo\n");
			exit(-1);
		}
	}

	fseek(archivo, 0, SEEK_END);
	u_int32_t fsize = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);

	char *buffer = malloc(fsize + 1);
	fread(buffer, fsize, 1, archivo);
	fclose(archivo);
	buffer[fsize] = '\0';
	if (send(*cliente, &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send(*cliente, buffer, fsize + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		exit(-1);
	}
	printf("El archivo se envió correctamente\n");

	free(lineaIngresada);
	free(buffer);

}

int main(void) {

	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1"); //En realidad se le debería pasar config->ipKernel
	direccionServidor.sin_port = htons(config->puerto);

	int cliente;

	conectar(&cliente, &direccionServidor);

	int procesoConectado = handshake(&cliente, consola);

	switch (procesoConectado) {
	case kernel:
		msjConexionCon("Kernel");

		mandarArchivo(&cliente);

		break;

	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(cliente);

	return 0;
}
