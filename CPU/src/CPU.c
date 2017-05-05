#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "libreriaSockets.h"

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	int cliente;
	char* buffer = malloc(LONGMAX);

	conectar(&cliente, &direccionServidor);

	int procesoConectado = handshake(&cliente, cpu);

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
