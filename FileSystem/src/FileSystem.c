#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/FileSystem/src/configFyleSystem"
#define LONGMAX 1000
typedef struct {
	int puerto;
}t_configuracion;
t_configuracion *config;

void *reservarMemoria(int tamanioArchivo){
	void *puntero = malloc (tamanioArchivo);
	if(puntero == NULL){
		printf("No hay más espacio");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo){
	config = reservarMemoria(sizeof(t_configuracion));
	config -> puerto = config_get_int_value(archivo_Modelo, "PUERTO");
}

void leerArchivo(){
	if (access(RUTAARCHIVO, F_OK) == -1){
		printf("No se encontró el Archivo");
		exit (-1);
	}
		t_config *archivo_config = config_create(RUTAARCHIVO);
		settearVariables(archivo_config);
		config_destroy(archivo_config);
		printf("Leí el archivo y extraje el puerto: %d", config -> puerto);
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

int recibirMensajeDe(int *cliente, char *buffer) {

	int bytesRecibidos = recv((*cliente), buffer, LONGMAX, 0);

	buffer[bytesRecibidos] = '\0';
	printf("Me llegaron %d bytes con %s\n", bytesRecibidos, buffer);

	return 0;
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

	recibirMensajeDe(&cliente, buffer);

	close(cliente);

	return 0;
}
