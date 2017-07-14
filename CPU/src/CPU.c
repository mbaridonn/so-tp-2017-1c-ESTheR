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

int serv_kernel, serv_memoria, planificacion, quantum, instrucciones_ejecutadas,
		stackSize, tamPag, motivo_liberacion;

enum algoritmos_planificacion {
	FIFO, RR
};

enum motivos_liberacion_CPU{
	mot_quantum, mot_semaforo, mot_finalizo, mot_error
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

t_metadata_program *getMetadataExample() {
	puts("\nThis is a fake ansisop program (hardcoded)\n");
	char *programa =
			"begin\nvariables f, A, g \n A = 0 \n!Global = 1+A\n print !Global \n jnz !Global Siguiente \n:Proximo\n	\n f = 8 \ng <- doble !Global \n io impresora 20\n	:Siguiente \n imprimir A \ntextPrint Hola Mundo! \n\n sumar1 &g \n print g \n\n sinParam \n \nend\n\nfunction sinParam\n	textPrint Bye\nend\n\n#Devolver el doble del\n#primer parametro\nfunction doble\nvariables f \n f = $0 + $0 \n return fvend\n\nfunction sumar1\n	*$0 = 1 + *$0\nend\n\nfunction imprimir\n wait mutexA\n print $0+1\n signal mutexB\nend\n\n";
//print_instrucciones_size ();
	return metadata_desde_literal(programa); // get metadata from the program
}

t_stack *getStackExample() {
	t_stack * stack_mock = queue_create();
	t_stack_entry * first_entry = malloc(sizeof(t_stack_entry));
	first_entry->pos = 0;
	first_entry->cant_args = 1;
	first_entry->args = malloc(sizeof(t_arg));
	first_entry->args->page_number = 1;
	first_entry->args->offset = 2;
	first_entry->args->tamanio = 3;
	first_entry->cant_vars = 1;
	first_entry->vars = malloc(sizeof(t_var));
	first_entry->vars->var_id = 4;
	first_entry->vars->page_number = 5;
	first_entry->vars->offset = 6;
	first_entry->vars->tamanio = 7;
	first_entry->cant_ret_vars = 1;
	first_entry->ret_vars = malloc(sizeof(t_ret_var));
	first_entry->ret_vars->page_number = 8;
	first_entry->ret_vars->offset = 9;
	first_entry->ret_vars->tamanio = 10;
	first_entry->ret_pos = 0;
	queue_push(stack_mock, first_entry);
	return stack_mock;
}

t_pcb * getPcbExample() {
//1) Get metadata structure from ansisop whole program
	t_metadata_program* newMetadata = getMetadataExample();
//printf("Cantidad de etiquetas: %i \n\n", newMetadata->cantidad_de_etiquetas); // print something
//newMetadata->instrucciones_serializado += 1;
//2) Create the PCB with the metadata information obtained, and extra information of the Kernel
	t_pcb * newPCB = malloc(sizeof(t_pcb));
	newPCB->id_proceso = 7777;
	newPCB->program_counter = newMetadata->instruccion_inicio;
	newPCB->cant_instrucciones = newMetadata->instrucciones_size;
	newPCB->cant_paginas_de_codigo = 666;
//	newMetadata->instrucciones_serializado;
	newPCB->indice_codigo = newMetadata->instrucciones_serializado;
	newPCB->stackPointer = 10;
	newPCB->indice_stack = getStackExample();
	newPCB->etiquetas_size = newMetadata->etiquetas_size;
	newPCB->indice_etiquetas = newMetadata->etiquetas;
	newPCB->exit_code = 999;

	return newPCB;
}

void mostrarPcb(t_pcb *pcb_prueba) {
	printf("id_proceso: %d \n", pcb_prueba->id_proceso);
	printf("program_counter: %d \n", pcb_prueba->program_counter);
	printf("cant_instrucciones: %d \n", pcb_prueba->cant_instrucciones);
	printf("cant_paginas_de_codigo: %d \n", pcb_prueba->cant_paginas_de_codigo);
	printf("indice_codigo->start: %u \n", pcb_prueba->indice_codigo->start);
	printf("indice_codigo->offset: %u \n", pcb_prueba->indice_codigo->offset);
	printf("stackPointer: %d \n", pcb_prueba->stackPointer);
	printf("cantElementosStack: %d \n",
			pcb_prueba->indice_stack->elements->elements_count);
	printf("etiquetas_size: %d \n", pcb_prueba->etiquetas_size);
	printf("indice_etiquetas: %c \n", *(pcb_prueba->indice_etiquetas));
	printf("exit_code: %d \n\n", pcb_prueba->exit_code);
}

void testearSerializado() {
	char *script = "begin\n"
			"variables a, b\n"
			"a = 3\n"
			"b = 5\n"
			"a = b + 12\n"
			"end\n";

	//t_pcb *pcb_a_serializar = crearPCB(script, 1024, 1024);
	t_pcb *pcb_a_serializar = getPcbExample();

	mostrarPcb(pcb_a_serializar);

	void *serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb_a_serializar, &serialized_pcb, &serialized_buffer_index);

	t_pcb* pcb_a_deserializar = calloc(1, sizeof(t_pcb));
	int pcb_serializado_index = 0;
	deserializar_pcb(&pcb_a_deserializar, serialized_pcb,
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

void avisarAccionAKernel(int accion) {
	u_int32_t aux = accion;
	if (send(serv_kernel, &aux, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la accion a Kernel.\n");
		exit(-1);
	}
	esperarSenialDeKernel();
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
	esperarSenialDeKernel();
	u_int32_t accion = cpuLibre;
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

bool hay_que_seguir_ejecutando() {
	return instrucciones_ejecutadas < quantum || quantum < 0;
}

void ejecutar_instrucciones(t_pcb *un_pcb) {
	//Solicito siguiente instruccion
	char* instruccion;
	int codigoError;
	instrucciones_ejecutadas = 0;// Solo sirve para tenerlo inicializado en algo.
	inicializarPrimitivasANSISOP(un_pcb, stackSize, tamPag, serv_kernel,serv_memoria);
	while (!terminoElPrograma() && !(codigoError = hayError())
			&& hay_que_seguir_ejecutando()) { //ESTO SERÍA SOLO PARA FIFO
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
		motivo_liberacion = mot_error;
		//VER QUE MÁS HAY QUE HACER !!
	}
	if(terminoElPrograma()){
		motivo_liberacion = mot_finalizo;
	}
	if(instrucciones_ejecutadas == quantum){
		motivo_liberacion = mot_quantum;
	} // FALTA los ifs de wait y signal
}

void mostrar_datos_pcb(t_pcb *un_pcb) {
	printf("PCB id: %d\n", un_pcb->id_proceso);
	printf("PCB start instruccion %d: %d\n", un_pcb->program_counter,
			un_pcb->indice_codigo[un_pcb->program_counter].start);
	printf("PCB offset instruccion %d: %d\n", un_pcb->program_counter,
			un_pcb->indice_codigo[un_pcb->program_counter].offset);
}

void conectarse_con_kernel(struct sockaddr_in *direccionServidor) {
	conectar(&serv_kernel, direccionServidor);

	int procesoConectado = handshake(&serv_kernel, cpu);

	switch (procesoConectado) {
	case kernel:
		printf("Me conecte con el Kernel!\n");
		recibir_planificacion_y_quantum();
		stackSize = recibirUIntDeKernel();
		tamPag = recibirUIntDeKernel();
		printf("Recibi stackSize: %d y tamPag: %d\n", stackSize, tamPag);
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}
}

void enviar_un_PCB_a_Kernel(t_pcb *pcb) {
	//Serializo el PCB y lo envio a Kernel
	//abstraer la asquerosidad de abajo
	void * serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb, &serialized_pcb, &serialized_buffer_index);

	if (send(serv_kernel, &serialized_buffer_index, (size_t) sizeof(int), 0)
			< 0) {
		printf("El envio de la longitud del buffer a Kernel fallo\n");
		exit(-1);
	}
	if (send(serv_kernel, serialized_pcb, (size_t) serialized_buffer_index, 0)
			< 0) {
		printf("El envio de pcb a Kernel fallo\n");
		exit(-1);
	}
	printf("PCB enviado a CPU exitosamente\n");
}

void enviar_motivo_liberacion(){
	if (send(serv_kernel, &motivo_liberacion, sizeof(int), 0) == -1) {
		printf("El envio de pcb a Kernel fallo\n");
		exit(-1);
	}
}

void devolver_pcb_y_liberarse(t_pcb *pcb) {
	cpu_kernel_aviso_desocupada();
	enviarIntAKernel(pcb->id_proceso);
	enviar_motivo_liberacion();
	enviar_un_PCB_a_Kernel(pcb);
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

	while (1) {
		t_pcb *pcb = recibir_pcb();
		mostrarPcb(pcb); // Es para chequear que llegue bien, NO es un procedimiento NECESARIO
		ejecutar_instrucciones(pcb);
		free(pcb);
		devolver_pcb_y_liberarse(pcb);
	}

	return 0;
}
