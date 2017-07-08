#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <parser/metadata_program.h>
#include "libreriaSockets.h"
#include "lib/primitivasAnSISOP.h"
#include "lib/pcb.h"

int serv_kernel,serv_memoria;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum accionesCPU{
	cpuLibre
};

AnSISOP_funciones functions = { .AnSISOP_definirVariable = definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar = dereferenciar, .AnSISOP_asignar = asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel = irAlLabel, .AnSISOP_llamarSinRetorno =
				llamarSinRetorno, .AnSISOP_llamarConRetorno = llamarConRetorno,
		.AnSISOP_finalizar = finalizar, .AnSISOP_retornar = retornar };

AnSISOP_kernel kernel_functions = { .AnSISOP_wait = wait, .AnSISOP_signal =
		signal, .AnSISOP_reservar = reservar, .AnSISOP_liberar = liberar,
		.AnSISOP_abrir = abrir, .AnSISOP_borrar = borrar, .AnSISOP_cerrar =
				cerrar, .AnSISOP_moverCursor = moverCursor, .AnSISOP_escribir =
				escribir, .AnSISOP_leer = leer };

static const char* PROGRAMA = //Programa de prueba para AnSISOP
		"begin\n"
				"variables a, b\n"
				"a = 3\n"
				"b = 5\n"
				"a = b + 12\n"
				"end\n"
				"\n";

//Función de prueba para AnSISOP
char * const conseguirDatosDeLaMemoria(char *prgrama,
		t_puntero_instruccion inicioDeLaInstruccion, t_size tamanio) {
	char *aRetornar = calloc(1, 100);
	memcpy(aRetornar, prgrama + inicioDeLaInstruccion, tamanio);
	return aRetornar;
}

void recibirArchivoDe(int *cliente) {
	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	if (archivo == NULL) {
		printf("No se pudo escribir el archivo\n");
		exit(-1);
	}

	u_int32_t fsize = 0;
	if (recv((*cliente), &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		exit(-1);
	}

	char *bufferArchivo = reservarMemoria(fsize + 1);
	if (recv((*cliente), bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		exit(-1);
	}
	printf("%s\n\n", bufferArchivo);

	fwrite(bufferArchivo, 1, fsize, archivo);
	free(bufferArchivo);
}

void solicitarA(int *cliente, char *nombreCli) {
	char a[2] = "a";
	send((*cliente), a, 2, 0);
	printf("Esperando atencion de %s..\n", nombreCli);
	recv((*cliente), a, 2, 0);
}

void cpu_kernel_aviso_desocupada(){
	solicitarA(&serv_kernel,"Kernel");
	u_int32_t accion = cpuLibre;
	if(send(serv_kernel,&accion,sizeof(u_int32_t),0)==-1){
		printf("Error enviando aviso de liberado a Kernel.\n");
	}
}

int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor2.sin_port = htons(8125);

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

	char* buffer = reservarMemoria(LONGMAX);
	free(buffer); //Encontre esto sin usar, lo dejo aca por si al final lo van a usar. Puse el free para que se acuerden de liberarlo.

	conectar(&serv_kernel, &direccionServidor);

	int procesoConectado = handshake(&serv_kernel, cpu);

	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		//RECIBO PCB Y LO DESERIALIZO
		void* tmp_buff = calloc(1, sizeof(int));
		int pcb_size = 0, pcb_size_index = 0;
		recv(serv_kernel, tmp_buff, sizeof(int), 0);
		deserializar_data(&pcb_size, sizeof(int), tmp_buff, &pcb_size_index);
		void *pcb_serializado = calloc(1, (size_t) pcb_size);
		recv(serv_kernel, pcb_serializado, (size_t) pcb_size, 0);
		int pcb_serializado_index = 0;
		t_pcb* incomingPCB = calloc(1, sizeof(t_pcb));
		deserializar_pcb(&incomingPCB, pcb_serializado, &pcb_serializado_index);
		free(tmp_buff);
		free(pcb_serializado);

		printf("PCB id: %d\n", incomingPCB->id_proceso);

		conectar(&serv_memoria, &direccionServidor2);
		int procesoConectado2 = handshake(&serv_memoria, cpu);
		switch (procesoConectado2) {
			case memoria:
				printf("Me conecte con Memoria!\n");
				recibirArchivoDe(&serv_memoria);
				break;
			default:
				printf("No me puedo conectar con vos.\n");
				break;
			}

		break;
	case memoria:
		printf("Me conecte con Memoria!\n");
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}



	return 0;
}
