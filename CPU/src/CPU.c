#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define LONGMAX 1000

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void handshake(int *cliente, int *unProceso, int *procesoAConocer) {
	send((*cliente), unProceso, sizeof(int), 0);
	recv((*cliente), procesoAConocer, sizeof(int), 0);
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

	int cliente;
	char* buffer = malloc(LONGMAX);

	conectar(&cliente, &direccionServidor);

	int unaCpu = cpu;
	int *proceso;
	handshake(&cliente, &unaCpu, proceso);
	int procesoConectado = *proceso;
	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		recibirMensajeDe(&cliente, buffer);
		break;
	case memoria:
		printf("Me conecte con Memoria!\n");
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(cliente);

	return 0;
}
