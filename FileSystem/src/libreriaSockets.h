#include <arpa/inet.h>
#include <commons/log.h>

#define MAX_CLIENTS 30
#define LONGMAX 1000

t_log *fileSystem_log;

void inicializarLog();
void agregarSocket(int client_socket[], int *cliente);
//void handshake(int *cliente, int *unProceso, int *procesoAConocer);
int handshake(int *cliente, int proceso);
void esperarConexion(int *servidor, struct sockaddr_in *direccionServidor);
void aceptarConexion(int *servidor, int *cliente);
void recibirMensajeDe(int *cliente, char *buffer);
void conectar(int *cliente, struct sockaddr_in *direccionServidor);

void agregarSocket(int client_socket[], int *cliente) {
	int i = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (client_socket[i] == 0) {
			client_socket[i] = *cliente;
			log_info(fileSystem_log, "Agregando a conjunto de sockets como %d ", i);
//			printf("Agregando a conjunto de sockets como %d \n", i);
			break;
		}
	}
}

void inicializarLog(){
	fileSystem_log = log_create("/home/utnso/git/tp-2017-1c-C-digo-Facilito/FileSystem/Debug/FileSystem.log", "CódigoFacilito-FS\n", true, LOG_LEVEL_TRACE);
}

//Handshake anterior, lo dejo por las dudas. Mas adelante lo borramos
/* void handshake(int *cliente, int *unProceso, int *procesoAConocer) {
	printf("Estoy haciendo el handshake\n");
	send((*cliente), unProceso, sizeof(int), 0);
	recv((*cliente), procesoAConocer, sizeof(int), 0);
}*/

int handshake(int *cliente, int proceso) {
	char unProceso[2]; unProceso[0] = '0' + proceso; unProceso[1] = '\0';
	char procesoAConocer[2];
	send((*cliente), unProceso, 2, 0);
	recv((*cliente), procesoAConocer, 2, 0);
	return atoi(procesoAConocer);
}

void esperarConexion(int *servidor, struct sockaddr_in *direccionServidor) {
	(*servidor) = socket(AF_INET, SOCK_STREAM, 0);
	int activado = 1;
	setsockopt((*servidor), SOL_SOCKET, SO_REUSEADDR, &activado,sizeof(activado));

	if (bind((*servidor), (void*) &(*direccionServidor),sizeof((*direccionServidor))) != 0) {
		log_error(fileSystem_log, "Falló el bind");
		//perror("Falló el bind. \n");
		exit(-1);
	}
	log_info(fileSystem_log, "Estoy escuchando");
	//printf("Estoy escuchando \n");
	listen((*servidor), SOMAXCONN);
}

void aceptarConexion(int *servidor, int *cliente) {
	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion;
	(*cliente) = accept((*servidor), (void*) &direccionCliente, &tamanioDireccion);
	log_info(fileSystem_log, "Recibí una conexión en %d!! ", (*cliente));
//	printf("Recibí una conexión en %d!! \n", (*cliente));
}

void recibirMensajeDe(int *cliente, char *buffer) {
	int bytesRecibidos = recv((*cliente), buffer, LONGMAX, 0);
	if (bytesRecibidos <= 0) {
		log_error(fileSystem_log, "El chabón se desconectó o bla");
//		perror("El chabón se desconectó o bla. \n");
		exit(-1);
	}

	buffer[bytesRecibidos] = '\0';
	log_info(fileSystem_log, "Me llegaron %d bytes con %s ", bytesRecibidos, buffer);
//	printf("Me llegaron %d bytes con %s \n", bytesRecibidos, buffer);
}

void conectar(int *cliente, struct sockaddr_in *direccionServidor) {
	(*cliente) = socket(AF_INET, SOCK_STREAM, 0);
	if (connect((*cliente), (void*) &(*direccionServidor),sizeof((*direccionServidor))) != 0) {
		log_error(fileSystem_log, "No se pudo conectar");
//		perror("No se pudo conectar \n");
		exit(-1);
	}
}
