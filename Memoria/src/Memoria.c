#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define LONGMAX 1000

int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	int cliente = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(cliente, (void*) &direccionServidor, sizeof(direccionServidor))
			!= 0) {
		perror("No se pudo conectar");
		return 1;
	}

	char mensaje[LONGMAX];

	int bytesRecibidos = recv(cliente, mensaje, LONGMAX, 0);

	mensaje[bytesRecibidos] = '\0';
	printf("Me llegaron %d bytes con %s\n", bytesRecibidos, mensaje);

	close(cliente);

	return 0;
}
