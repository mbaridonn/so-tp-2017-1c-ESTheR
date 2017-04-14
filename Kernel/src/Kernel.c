#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#define LONGMAX 1000

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

int main(void) {

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(8080);

	int servidor;
	int cliente;
	char* buffer = malloc(LONGMAX);

	esperarConexion(&servidor, &direccionServidor);

	aceptarConexion(&servidor, &cliente);

	recibirMensajeDe(&cliente, buffer);

	//---------------------------------------------------------------------------------

	//----------------------------------------------------------------------------------

	//Conexion con los otros modulos (Memoria, Cpu, y FileSystem)
	close(servidor);

	esperarConexion(&servidor, &direccionServidor);

	aceptarConexion(&servidor, &cliente);

	send(cliente, buffer, LONGMAX, 0);

	free(buffer);

	close(servidor);

//puts("Proceso Kernel");
//return EXIT_SUCCESS;
	return 0;
}
