#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <commons/config.h>

#include "lib/pcb.h"
#include "libreriaSockets.h"
#include "lib/primitivasAnSISOP.h"

char *rutaArchivo;// /home/utnso/git/tp-2017-1c-C-digo-Facilito/CPU/src/configCPU

int serv_kernel, serv_memoria, planificacion, quantum, instrucciones_ejecutadas,
		stackSize, tamPag, motivo_liberacion, quantum_sleep;

bool descCPU, matarProceso, esta_ejecutando = false;

enum algoritmos_planificacion {
	FIFO, RR
};

enum motivos_liberacion_CPU{
	mot_quantum, mot_finalizo, mot_error, mot_bloqueado
};

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

typedef struct {
	char* ipKernel;
	int puertoKernel;
	char* ipMemoria;
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
		signal_ANSISOP, .AnSISOP_reservar = reservar, .AnSISOP_liberar = liberar,
		.AnSISOP_abrir = abrir, .AnSISOP_borrar = borrar, .AnSISOP_cerrar =
				cerrar, .AnSISOP_moverCursor = moverCursor, .AnSISOP_escribir =
				escribir, .AnSISOP_leer = leer };

void mostrarPcb(t_pcb *pcb_prueba) {
	log_info(cpu_log, "id_proceso: %d ", pcb_prueba->id_proceso);
	log_info(cpu_log, "program_counter: %d ", pcb_prueba->program_counter);
	log_info(cpu_log, "cant_instrucciones: %d ", pcb_prueba->cant_instrucciones);
	/*log_info(cpu_log, "cant_paginas_de_codigo: %d ", pcb_prueba->cant_paginas_de_codigo);
	log_info(cpu_log, "indice_codigo->start: %u ", pcb_prueba->indice_codigo->start);
	log_info(cpu_log, "indice_codigo->offset: %u ", pcb_prueba->indice_codigo->offset);
	log_info(cpu_log, "stackPointer: %d \n", pcb_prueba->stackPointer);
	log_info(cpu_log,"cantElementosStack: %d ",pcb_prueba->indice_stack->elements->elements_count);
	log_info(cpu_log, "etiquetas_size: %d ", pcb_prueba->etiquetas_size);
	log_info(cpu_log, "indice_etiquetas: %c ", *(pcb_prueba->indice_etiquetas));*/
	log_info(cpu_log, "exit_code: %d \n", pcb_prueba->exit_code);
}

void mili_sleep(int retardo){
	float nuevo_retardo = (float)retardo/(float)1000;
	sleep(nuevo_retardo);
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->ipKernel = strdup(config_get_string_value(archivo_Modelo, "IP_KERNEL"));
	config->puertoKernel = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
	config->ipMemoria = strdup(config_get_string_value(archivo_Modelo, "IP_MEMORIA"));
	config->puertoMemoria = config_get_int_value(archivo_Modelo, "PUERTO_MEMORIA");
}

void mostrarArchivoConfig() {
	FILE *f;
	f = fopen(rutaArchivo, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	printf("\n");
	printf("------------------------------------------\n");
}

void leerArchivo() {
	if (access(rutaArchivo, F_OK) == -1) {
		log_error(cpu_log, "No se encontró el Archivo");
		exit(-1);
	}
	t_config *archivo_config = config_create(rutaArchivo);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
}

//Funciones CPU

void avisarAccionAKernel(int accion) {
	u_int32_t aux = accion;
	if (send(serv_kernel, &aux, sizeof(u_int32_t), 0) == -1) {
		log_error(cpu_log, "Error enviando la accion a Kernel");
		exit(-1);
	}
	esperarSenialDeKernel();
}

void cpu_kernel_aviso_desocupada() {
	solicitarA(&serv_kernel, "Kernel");
	esperarSenialDeKernel();
	u_int32_t accion = cpuLibre;
	if (send(serv_kernel, &accion, sizeof(u_int32_t), 0) == -1) {
		log_error(cpu_log, "Error enviando aviso de liberado a Kernel");
		exit (-1);
	}
}

void recibir_planificacion() {
	if (recv(serv_kernel, &planificacion, sizeof(int), 0) == -1) {
		log_error(cpu_log, "Error al recibir el tipo de algoritmo de planificacion");
		exit(-1);
	}
}

void recibir_quantum() {
	if (recv(serv_kernel, &quantum, sizeof(int), 0) == -1) {
		log_error(cpu_log, "Error al recibir el quantum de planificacion");
		exit(-1);
	}
}

void recibir_quantum_sleep() {
	if (recv(serv_kernel, &quantum_sleep, sizeof(int), 0) == -1) {
		log_error(cpu_log, "Error al recibir el quantum sleep de planificacion");
		exit(-1);
	}
}

void recibir_planificacion_y_quantum() {
	recibir_planificacion();
	recibir_quantum();
	recibir_quantum_sleep();
	if (planificacion == FIFO) {
		log_info(cpu_log, "Me llego que la planificacion es FIFO y el QUANTUM en:%d\n", quantum);
	}
	if (planificacion == RR) {
		log_info(cpu_log, "Me llego que la planificacion es RR y el QUANTUM en:%d\n", quantum);
	}
}

t_pcb *recibir_pcb() {
	//Recibo PCB y lo deserializo
	void* tmp_buff = calloc(1, sizeof(int));
		int pcb_size = 0, pcb_size_index = 0;
		if(recv(serv_kernel, tmp_buff, sizeof(int), 0)==-1){
			log_error(cpu_log, "Error al recibir tamanio del pcb serializado.");
		}
		deserializar_data(&pcb_size, sizeof(int), tmp_buff, &pcb_size_index);
		void *pcb_serializado = calloc(1, (size_t) pcb_size);
		enviarSenialAKernel();
		if(recv(serv_kernel, pcb_serializado, (size_t) pcb_size, MSG_WAITALL)==-1){
			log_error(cpu_log, "Error al recibir el pcb serializado.");
		}
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
		log_info(cpu_log, "Me conecte con Memoria!");
		break;
	default:
		log_info(cpu_log, "No me puedo conectar con vos");
		break;
	}
}

bool hay_que_seguir_ejecutando() {
	return instrucciones_ejecutadas < quantum || quantum < 0;
}

char* depurarSentencia(char* sentencia) {
	int i = strlen(sentencia);
	while (string_ends_with(sentencia, "\n")) {
		i--;
		sentencia = string_substring_until(sentencia, i);
	}
	return sentencia;
}

void ejecutar_instrucciones(t_pcb *un_pcb) {
	//Solicito siguiente instruccion
	char* instruccion;
	int codigoError;
	instrucciones_ejecutadas = 0;// Solo sirve para tenerlo inicializado en algo.
	inicializarPrimitivasANSISOP(un_pcb, stackSize, tamPag, serv_kernel,serv_memoria, cpu_log);
	while (!terminoElPrograma() && !(codigoError = hayError()) && hay_que_seguir_ejecutando() && !estaBloqueado()) {
		esta_ejecutando = true;
		instruccion = conseguirDatosDeLaMemoria(un_pcb->id_proceso, 0,/*Las páginas de código son las primeras en memoria*/
				un_pcb->indice_codigo[un_pcb->program_counter].start, un_pcb->indice_codigo[un_pcb->program_counter].offset);
		log_info(cpu_log, "Instruccion recibida: %s", instruccion);
		analizadorLinea(depurarSentencia(instruccion), &functions, &kernel_functions);
		un_pcb->program_counter++;
		instrucciones_ejecutadas++;
		free(instruccion);
		mili_sleep(quantum_sleep);
	}
	if (matarProceso) codigoError = -12;//Defino nuevo Exit Code -12: CPU desconectada con SIGINT (!!)
	if (codigoError != 0) {
		un_pcb->exit_code = codigoError;
		motivo_liberacion = mot_error;
	}else{
		if(terminoElPrograma()){
				motivo_liberacion = mot_finalizo;
		}else{
			if(instrucciones_ejecutadas == quantum){
					motivo_liberacion = mot_quantum;
			}
		}
		if(estaBloqueado()){
			motivo_liberacion = mot_bloqueado;
		}
	}
	esta_ejecutando = false;
}

void conectarse_con_kernel(struct sockaddr_in *direccionServidor) {
	conectar(&serv_kernel, direccionServidor);

	int procesoConectado = handshake(&serv_kernel, cpu);

	switch (procesoConectado) {
	case kernel:
		log_info(cpu_log, "Me conecte con el Kernel!");
		recibir_planificacion_y_quantum();
		stackSize = recibirUIntDeKernel();
		tamPag = recibirUIntDeKernel();
		log_info(cpu_log, "Recibi stackSize: %d y tamPag: %d", stackSize, tamPag);
		break;
	default:
		log_error(cpu_log, "No me puedo conectar con vos");
		break;
	}
}

void enviar_un_PCB_a_Kernel(t_pcb *pcb) {
	//Serializo el PCB y lo envio a Kernel
	void * serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb, &serialized_pcb, &serialized_buffer_index);

	if (send(serv_kernel, &serialized_buffer_index, (size_t) sizeof(int), 0)< 0) {
		log_error(cpu_log, "El envio de la longitud del buffer a Kernel fallo");
		exit(-1);
	}
	esperarSenialDeKernel();
	if (send(serv_kernel, serialized_pcb, (size_t) serialized_buffer_index, MSG_WAITALL)< 0) {
		log_error(cpu_log, "El envio de pcb a Kernel fallo");
		exit(-1);
	}
	log_info(cpu_log, "PCB enviado a Kernel exitosamente", stackSize, tamPag);
}

void enviar_motivo_liberacion(){
	if (send(serv_kernel, &motivo_liberacion, sizeof(int), 0) == -1) {
		log_info(cpu_log, "El envio de pcb a Kernel fallo");
		exit(-1);
	}
}

void devolver_pcb_y_liberarse(t_pcb *pcb) {
	cpu_kernel_aviso_desocupada();
	enviarIntAKernel(pcb->id_proceso);
	esperarSenialDeKernel();
	enviar_motivo_liberacion();
	esperarSenialDeKernel();
	enviar_un_PCB_a_Kernel(pcb);
	mostrarPcb(pcb);
}

void desconectarCPU(int senial){
	if (senial == SIGUSR1){
		if(esta_ejecutando){
			descCPU = true;
		}else{
			enviarIntAMemoria(cpu_mem_finalizar_cpu);
			esperarSenialDeMemoria();

			log_destroy(cpu_log);
			exit(0);
		}
	}
	if (senial == SIGINT){
		if(esta_ejecutando){
			descCPU = true;
			matarProceso = true;
		}else{
			enviarIntAMemoria(cpu_mem_finalizar_cpu);
			esperarSenialDeMemoria();

			log_destroy(cpu_log);
			exit(0);
		}
	}
}

int main(int argc, char* argv[]) {

	signal(SIGUSR1, desconectarCPU);
	signal(SIGINT, desconectarCPU);
	descCPU = false;
	matarProceso = false;

	if (argc == 1)
	{
		printf("Falta ingresar el path del archivo de configuracion\n");
		return -1;
	}
	if (argc != 2)
	{
		printf("Numero incorrecto de argumentos\n");
		return -1;
	}
	rutaArchivo = strdup(argv[1]);

	inicializarLog();
	leerArchivo();

	free(rutaArchivo);

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(config->ipKernel);
	direccionServidor.sin_port = htons(config->puertoKernel/*8080*/);

	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr(config->ipMemoria);
	direccionServidor2.sin_port = htons(config->puertoMemoria/*8125*/);

	conectarse_con_memoria(&direccionServidor2);
	conectarse_con_kernel(&direccionServidor);

	while (!descCPU) {
		t_pcb *pcb = recibir_pcb();
		mostrarPcb(pcb); // Es para chequear que llegue bien, NO es un procedimiento NECESARIO
		ejecutar_instrucciones(pcb);
		devolver_pcb_y_liberarse(pcb);
		free(pcb);
	}

	enviarIntAMemoria(cpu_mem_finalizar_cpu);
	esperarSenialDeMemoria();

	log_destroy(cpu_log);

	return 0;
}
