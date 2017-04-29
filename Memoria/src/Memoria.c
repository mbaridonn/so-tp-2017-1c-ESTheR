#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/src/configMemoria.txt"
#define LONGMAX 1000
typedef struct {
	int puerto;
	int cantFrames;
	int tamFrame;
}t_configuracion;
t_configuracion *config;

void *reservarMemoria(int tamanio){
	void *puntero = malloc (tamanio);
	if(puntero == NULL){
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo){
	config = reservarMemoria(sizeof(t_configuracion));
	config -> puerto = config_get_int_value(archivo_Modelo, "PUERTO");
	config -> cantFrames = config_get_int_value(archivo_Modelo, "MARCOS");
	config -> tamFrame = config_get_int_value(archivo_Modelo, "MARCO_SIZE");
}

void leerArchivo(){
	if (access(RUTAARCHIVO, F_OK) == -1){
		printf("No se encontró el Archivo\n");
		exit(-1);
	}
		t_config *archivo_config = config_create(RUTAARCHIVO);
		settearVariables(archivo_config);
		config_destroy(archivo_config);
		printf("Leí el archivo y extraje el puerto: %d\n", config -> puerto);
}

void esperarConexion(int *servidor, struct sockaddr_in *direccionServidor) {

	(*servidor) = socket(AF_INET, SOCK_STREAM, 0);
	int activado = 1;
	setsockopt((*servidor), SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if (bind((*servidor), (void*) &(*direccionServidor), sizeof((*direccionServidor))) != 0){
		perror("Falló el bind\n");
		exit(-1);
	}

	printf("Estoy escuchando\n");
	listen((*servidor), SOMAXCONN);
}

void aceptarConexion(int *servidor, int *cliente) {

	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	(*cliente) = accept((*servidor), (void*) &direccionCliente, &tamanioDireccion);
	printf("Recibí una conexión en %d!!\n", (*cliente));
}

void recibirMensajeDe(int *cliente, char *buffer) {

	int bytesRecibidos = recv((*cliente), buffer, LONGMAX, 0);
	if (bytesRecibidos <= 0) {
		perror("El chabón se desconectó\n");
		exit(-1);
	}
	buffer[bytesRecibidos] = '\0';
	printf("Me llegaron %d bytes con %s\n", bytesRecibidos, buffer);
}

void conectar(int *cliente, struct sockaddr_in *direccionServidor) { //Es necesaria?

	(*cliente) = socket(AF_INET, SOCK_STREAM, 0);
	if (connect((*cliente), (void*) &(*direccionServidor), sizeof((*direccionServidor))) != 0) {
		perror("No se pudo conectar\n");
		exit(-1);
	}
}

int main(void){

	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(config->puerto);

	int servidor;
	int cliente;
	char* buffer = reservarMemoria(LONGMAX);

	int memoriaTotal = config->tamFrame * config->cantFrames;

	char* memoria = reservarMemoria(memoriaTotal);//Por ahora, esta va a ser la memoria es un único bloque no paginado

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
