#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#define rutaArchivo "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/src/configMemoria.txt"
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
	if (access(rutaArchivo, F_OK) == -1){
		printf("No se encontró el Archivo");
		exit (-1);
	}
		t_config *archivo_config = config_create(rutaArchivo);
		settearVariables(archivo_config);
		config_destroy(archivo_config);
		printf("Leí el archivo y extraje el puerto: %d", config -> puerto);
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

int conectar(int *cliente, struct sockaddr_in *direccionServidor) { //Es necesaria?

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
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(8081);

	int servidor;
	int cliente;
	char* buffer = malloc(LONGMAX);

	leerArchivo();
	esperarConexion(&servidor, &direccionServidor);
	aceptarConexion(&servidor, &cliente);
	recibirMensajeDe(&cliente, buffer);
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
