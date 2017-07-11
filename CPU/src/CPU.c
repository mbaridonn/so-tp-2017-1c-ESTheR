#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <parser/metadata_program.h>
#include <commons/config.h>

#include "lib/pcb.h"
#include "libreriaSockets.h"
#include "lib/primitivasAnSISOP.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/CPU/src/configCPU"

int serv_kernel,serv_memoria;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum accionesCPUKernel{
	cpuLibre
};

enum accionesCPUMemoria{
	cpu_mem_leer, cpu_mem_escribir
};

typedef struct {
	int puertoKernel;
	int puertoMemoria;
} t_configuracion;
t_configuracion *config;

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

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puertoKernel = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
	config->puertoMemoria = config_get_int_value(archivo_Modelo, "PUERTO_MEMORIA");
}

void mostrarArchivoConfig(){
	 FILE *f;
	 f=fopen(RUTAARCHIVO,"r");
	 int c;
	 printf("------------------------------------------\n");
	 while ((c = fgetc (f)) != EOF) putchar(c);
	 printf("\n");
	 printf("------------------------------------------\n");
}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo\n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
}

//Funciones CPU

void esperarSenialDeMemoria() {
	char senial[2] = "a";
	if (recv(serv_memoria, senial, 2, 0) == -1) {
		printf("Error al recibir senial antes de enviar paginas\n");
	}
}

void avisarAccionAMemoria(int accion) {
	u_int32_t aux = accion;
	if (send(serv_memoria, &aux, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la accion.\n");
		exit(-1);
	}
	esperarSenialDeMemoria();
}

char * conseguirDatosDeLaMemoria(int PID, int nroPag, int offset, int tamanio){
	char* instruccion = reservarMemoria(tamanio);
	avisarAccionAMemoria(cpu_mem_leer);
	//Uso la misma función, aunque estoy pasando parámetros
	avisarAccionAMemoria(PID);
	avisarAccionAMemoria(nroPag);
	avisarAccionAMemoria(offset);
	avisarAccionAMemoria(tamanio);
	if (recv(serv_memoria, instruccion, tamanio, 0) == -1) {
		printf("Error al recibir instrucción de Memoria\n");
	}
	printf("Instruccion recibida: %s\n", instruccion);
	return instruccion;
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
	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(config->puertoKernel/*8080*/);

	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor2.sin_port = htons(config->puertoMemoria/*8125*/);

	//INICIO PRUEBA ANSISOP
	/*char *programa = strdup(PROGRAMA);
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
	 metadata_destruir(metadata);*/
	//FIN PRUEBA ANSISOP

	conectar(&serv_kernel, &direccionServidor);

	int procesoConectado = handshake(&serv_kernel, cpu);

	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		//Recibo PCB y lo deserializo
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
		printf("PCB start instruccion %d: %d\n",incomingPCB->program_counter, incomingPCB->indice_codigo[incomingPCB->program_counter].start);
		printf("PCB offset instruccion %d: %d\n",incomingPCB->program_counter, incomingPCB->indice_codigo[incomingPCB->program_counter].offset);

		//FALTA inicializarPrimitivasANSISOP(pcbAEjecutar, stackSize, tamPag);  !!!

		conectar(&serv_memoria, &direccionServidor2);
		int procesoConectado2 = handshake(&serv_memoria, cpu);
		switch (procesoConectado2) {
			case memoria:
				printf("Me conecte con Memoria!\n");
				//Solicito siguiente instruccion
				char* instruccion;
				while(!terminoElPrograma()){//ESTO SERÍA SOLO PARA FIFO
					instruccion = conseguirDatosDeLaMemoria(incomingPCB->id_proceso, 0/*Las páginas de código son las primeras en memoria*/,
						incomingPCB->indice_codigo[incomingPCB->program_counter].start,
						incomingPCB->indice_codigo[incomingPCB->program_counter].offset);
					analizadorLinea(instruccion, &functions, &kernel_functions);
					incomingPCB->program_counter++;
					free(instruccion);
				}
				//ANTES DE DESCONECTAR LA CPU, HAY QUE ENVIARLE UN MENSAJE A MEMORIA PARA QUE MATE EL HILO (Y NO ROMPA)
				break;
			default:
				printf("No me puedo conectar con vos.\n");
				break;
			}

		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	return 0;
}
