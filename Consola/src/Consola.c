#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <netinet/in.h>
#include <unistd.h>
#define LONGMAX 1000
#define rutaArchivo "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "
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
	config -> puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
}

void leerArchivo(){
	if (access(rutaArchivo, F_OK) == -1){
		printf("No se encontró el Archivo");
		exit (-1);
	}
		t_config *archivo_config = config_create(rutaArchivo);
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

int main(void) {

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	leerArchivo();
	int cliente;
	char mensaje[LONGMAX];

	conectar(&cliente, &direccionServidor);

	scanf("%s", mensaje);

	send(cliente, mensaje, strlen(mensaje), 0);

	close(cliente);

	return 0;
}
