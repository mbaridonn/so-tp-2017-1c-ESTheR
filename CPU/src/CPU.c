#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <commons/config.h>

#include "lib/pcb.h"
#include "libreriaSockets.h"
#include "lib/primitivasAnSISOP.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/CPU/src/configCPU"

int serv_kernel, serv_memoria, planificacion, quantum, instrucciones_ejecutadas, stackSize,tamPag;

enum algoritmos_planificacion {
	FIFO, RR
};

enum procesos {
	kernel, cpu, consola, file_system, memoria
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

void mostrarPcb(t_pcb *pcb_prueba) {
	printf("id_proceso: %d \n", pcb_prueba->id_proceso);
	printf("program_counter: %d \n", pcb_prueba->program_counter);
	printf("cant_instrucciones: %d \n", pcb_prueba->cant_instrucciones);
	printf("cant_paginas_de_codigo: %d \n", pcb_prueba->cant_paginas_de_codigo);
	printf("indice_codigo->start: %u \n", pcb_prueba->indice_codigo->start);
	printf("indice_codigo->offset: %u \n", pcb_prueba->indice_codigo->offset);
	printf("stackPointer: %d \n", pcb_prueba->stackPointer);
	printf("etiquetas_size: %d \n", pcb_prueba->etiquetas_size);
	//	printf("indice_etiquetas: %c \n",*(pcb_prueba->indice_etiquetas));
	printf("exit_code: %d \n\n", pcb_prueba->exit_code);
}

void testearSerializado() {
	char *script = "begin\n"
			"variables a, b\n"
			"a = 3\n"
			"b = 5\n"
			"a = b + 12\n"
			"end\n";

	t_pcb *pcb_a_serializar = crearPCB(script, 1024, 1024);

	mostrarPcb(pcb_a_serializar);

	void *serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb_a_serializar, &serialized_pcb, &serialized_buffer_index);

	t_pcb* pcb_a_deserializar = calloc(1, sizeof(t_pcb));
	int pcb_serializado_index = 0;
	deserializar_pcb(&pcb_a_deserializar, pcb_a_serializar,
			&pcb_serializado_index);

	mostrarPcb(pcb_a_deserializar);
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puertoKernel = config_get_int_value(archivo_Modelo,
			"PUERTO_KERNEL");
	config->puertoMemoria = config_get_int_value(archivo_Modelo,
			"PUERTO_MEMORIA");
}

void mostrarArchivoConfig() {
	FILE *f;
	f = fopen(RUTAARCHIVO, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
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

char * conseguirDatosDeLaMemoria(int PID, int nroPag, int offset, int tamanio) {
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
	archivo = fopen("prueba.txt", "w"); //POR QUÉ PRUEBA.TXT              ???
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

void cpu_kernel_aviso_desocupada() {
	solicitarA(&serv_kernel, "Kernel");
	u_int32_t accion = cpuLibre;
	esperarSenialDeKernel();
	if (send(serv_kernel, &accion, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando aviso de liberado a Kernel.\n");
	}
}

void recibir_planificacion() {
	if (recv(serv_kernel, &planificacion, sizeof(int), 0) == -1) {
		printf("Error al recibir el tipo de algoritmo de planificacion.\n");
		exit(-1);
	}
}

void recibir_quantum() {
	if (recv(serv_kernel, &quantum, sizeof(int), 0) == -1) {
		printf("Error al recibir el quantum de planificacion.\n");
		exit(-1);
	}
}

void recibir_planificacion_y_quantum() {
	recibir_planificacion();
	recibir_quantum();
	if (planificacion == FIFO) {
		printf("Me llego que la planificacion es FIFO y el QUANTUM en:%d\n",
				quantum);
	}
	if (planificacion == RR) {
		printf("Me llego que la planificacion es RR y el QUANTUM en:%d\n",
				quantum);
	} // Tambien se podria haber hecho un mensaje de kernel a cpu ejecutar_instrucciones(pcb,cantInstrucciones)
}

t_pcb *recibir_pcb() {
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
	return incomingPCB;
}

void conectarse_con_memoria(struct sockaddr_in *direccionServidor2) {
	conectar(&serv_memoria, direccionServidor2);
	int procesoConectado2 = handshake(&serv_memoria, cpu);
	switch (procesoConectado2) {
	case memoria:
		printf("Me conecte con Memoria!\n");
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}
}

bool hay_que_seguir_ejecutando(){
	return instrucciones_ejecutadas < quantum;
}

void ejecutar_instrucciones(t_pcb *un_pcb) {
	//Solicito siguiente instruccion
	char* instruccion;
	int codigoError;
	instrucciones_ejecutadas = 0;
	inicializarPrimitivasANSISOP(un_pcb, stackSize, tamPag, serv_kernel);
	while (!terminoElPrograma() && !(codigoError = hayError()) && hay_que_seguir_ejecutando()) { //ESTO SERÍA SOLO PARA FIFO
		instruccion = conseguirDatosDeLaMemoria(un_pcb->id_proceso, 0,/*Las páginas de código son las primeras en memoria*/
				un_pcb->indice_codigo[un_pcb->program_counter].start,
				un_pcb->indice_codigo[un_pcb->program_counter].offset);
		analizadorLinea(instruccion, &functions, &kernel_functions);
		un_pcb->program_counter++;
		instrucciones_ejecutadas++;
		free(instruccion);
	}
	if (codigoError != 0) {
		un_pcb->exit_code = codigoError;
		//VER QUE MÁS HAY QUE HACER !!
	}
	//ANTES DE DESCONECTAR LA CPU, HAY QUE ENVIARLE UN MENSAJE A MEMORIA PARA QUE MATE EL HILO (Y NO ROMPA)
}

void mostrar_datos_pcb(t_pcb *un_pcb) {
	printf("PCB id: %d\n", un_pcb->id_proceso);
	printf("PCB start instruccion %d: %d\n", un_pcb->program_counter,
			un_pcb->indice_codigo[un_pcb->program_counter].start);
	printf("PCB offset instruccion %d: %d\n", un_pcb->program_counter,
			un_pcb->indice_codigo[un_pcb->program_counter].offset);
}

void conectarse_con_kernel(struct sockaddr_in *direccionServidor){
	conectar(&serv_kernel, direccionServidor);

		int procesoConectado = handshake(&serv_kernel, cpu);

		switch (procesoConectado) {
		case kernel:
			printf("Me conecte con el Kernel!\n");
			recibir_planificacion_y_quantum();
			stackSize = recibirUIntDeKernel();
			tamPag = recibirUIntDeKernel();
			printf("Recibi stackSize: %d y tamPag: %d\n",stackSize,tamPag);
			break;
		default:
			printf("No me puedo conectar con vos.\n");
			break;
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
	//testearSerializado();

	conectarse_con_memoria(&direccionServidor2);
	conectarse_con_kernel(&direccionServidor);

	while(1){
		t_pcb *un_pcb = recibir_pcb();
		mostrar_datos_pcb(un_pcb); // Es para chequear que llegue bien, no es un procedimiento NECESARIO
		ejecutar_instrucciones(un_pcb);
		free(un_pcb);
		cpu_kernel_aviso_desocupada();
	}

	return 0;
}
