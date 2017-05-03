#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#define LONGMAX 1000
#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "
typedef struct {
	int puerto;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void handshake(int *cliente, int *unProceso, int *procesoAConocer) {
	send((*cliente), unProceso, sizeof(int), 0);
	recv((*cliente), procesoAConocer, sizeof(int), 0);
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
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	printf("Leí el archivo y extraje el puerto: %d \n", config->puerto);
}

int conectar(int *cliente, struct sockaddr_in *direccionServidor) {

	(*cliente) = socket(AF_INET, SOCK_STREAM, 0);
	if (connect((*cliente), (void*) &(*direccionServidor),
			sizeof((*direccionServidor))) != 0) {
		perror("No se pudo conectar \n");
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

	//

	conectar(&cliente, &direccionServidor);

	int unaConsola = consola;
	int *proceso;
	handshake(&cliente, &unaConsola, proceso);

	int procesoConectado = *proceso;
	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		FILE* archivo;
		archivo = fopen("prueba.txt", "r");
		if (archivo == NULL) {
			printf("No se pudo leer el archivo\n");
			return EXIT_FAILURE;
		}

		int bytesLeidos = 0;
		char buffer[10];
		while (bytesLeidos = fread(buffer, 1, 10, archivo)) {
			if (send(cliente, buffer, 10, 0) == -1) {
				printf("Error enviando archivo");
			}

		}
		fclose(archivo);
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(cliente);

	//

	//
	/*conectar(&cliente, &direccionServidor);
	 printf("Ingrese un mensaje: ");
	 scanf("%s", mensaje);
	 send(cliente, mensaje, strlen(mensaje), 0);
	 close(cliente);*/
	//
	return 0;
}
