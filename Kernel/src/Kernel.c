#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

int main(void) {

	//Esto tiene que ir en Kernel o en otro proyecto aparte que se llame Socket?
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = httons(8080);

	int servidor = socket(AF_INET, SOCK_STREAM, 0);

	if(bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0){
		perror("Falló el bind");
		return 1;
	}

	printf("Estoy escuchando");
	listen(servidor,100);

	//puts("Proceso Kernel");
	//return EXIT_SUCCESS;
	return 0;
}
