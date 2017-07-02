#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <parser/metadata_program.h>
#include "libreriaSockets.h"
#include "lib/primitivasAnSISOP.h"

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

AnSISOP_funciones functions = {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= dereferenciar,
		.AnSISOP_asignar				= asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel				= irAlLabel,
		.AnSISOP_llamarSinRetorno		= llamarSinRetorno,
		.AnSISOP_llamarConRetorno		= llamarConRetorno,
		.AnSISOP_finalizar 				= finalizar,
		.AnSISOP_retornar				= retornar
};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_wait		= wait,
		.AnSISOP_signal		= signal,
		.AnSISOP_reservar	= reservar,
		.AnSISOP_liberar	= liberar,
		.AnSISOP_abrir		= abrir,
		.AnSISOP_borrar		= borrar,
		.AnSISOP_cerrar		= cerrar,
		.AnSISOP_moverCursor= moverCursor,
		.AnSISOP_escribir	= escribir,
		.AnSISOP_leer		= leer
};

static const char* PROGRAMA = //Programa de prueba para AnSISOP
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";

//Función de prueba para AnSISOP
char *const conseguirDatosDeLaMemoria(char *prgrama, t_puntero_instruccion inicioDeLaInstruccion, t_size tamanio) {
	char *aRetornar = calloc(1, 100);
	memcpy(aRetornar, prgrama + inicioDeLaInstruccion, tamanio);
	return aRetornar;
}

int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	//INICIO PRUEBA ANSISOP
	/*printf("Ejecutando\n");
	char *programa = strdup(PROGRAMA);
	t_metadata_program *metadata = metadata_desde_literal(programa);
	int programCounter = 0;
	while(!terminoElPrograma()){
		char* const linea = conseguirDatosDeLaMemoria(programa,
				metadata->instrucciones_serializado[programCounter].start,
				metadata->instrucciones_serializado[programCounter].offset);
		printf("\t Evaluando -> %s", linea);
		analizadorLinea(linea, &functions, &kernel_functions);
		free(linea);
		programCounter++;
	}
	metadata_destruir(metadata);
	printf("================\n");*/
	//FIN PRUEBA ANSISOP

	int cliente;
	char* buffer = malloc(LONGMAX);

	conectar(&cliente, &direccionServidor);

	int procesoConectado = handshake(&cliente, cpu);

	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		//recibirMensajeDe(&cliente, buffer);
		//send((&cliente),1,sizeof(int),0);
		int num = 0;
		if(recv(cliente, &num, sizeof(int), 0) == -1)
			printf("Error recibiendo el PID\n");
		else
			printf("Recibi el PID: %d\n",num);
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
