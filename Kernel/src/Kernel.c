#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <commons/config.h>
#include <unistd.h>
#include <pthread.h>
#include "libreriaSockets.h"
#include "conexionesSelect.h"
#include "estructurasComunes.h"
#include "accionesDeKernel.h"

#define RUTA_ARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Kernel/src/ConfigKernel.txt"

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->PUERTO_PROG = config_get_int_value(archivo_Modelo, "PUERTO_PROG");
	config->PUERTO_CPU = config_get_int_value(archivo_Modelo, "PUERTO_CPU");
	char *aux = config_get_string_value(archivo_Modelo, "IP_MEMORIA");
	strcpy(config->IP_MEMORIA, aux);
	config->PUERTO_MEMORIA = config_get_int_value(archivo_Modelo,
			"PUERTO_MEMORIA");
	aux = config_get_string_value(archivo_Modelo, "IP_FS");
	strcpy(config->IP_FS, aux);
	config->PUERTO_FS = config_get_int_value(archivo_Modelo, "PUERTO_FS");
	config->QUANTUM = config_get_int_value(archivo_Modelo, "QUANTUM");
	config->QUANTUM_SLEEP = config_get_int_value(archivo_Modelo,
			"QUANTUM_SLEEP");
	aux = config_get_string_value(archivo_Modelo, "ALGORITMO");
	strcpy(config->ALGORITMO, aux);
	config->GRADO_MULTIPROG = config_get_int_value(archivo_Modelo,
			"GRADO_MULTIPROG");
	//FALTA SEM_IDS, SEM_INIT, SHARED_VARS
	config->STACK_SIZE = config_get_int_value(archivo_Modelo, "STACK_SIZE");

}

void leerArchivo() {
	if (access(RUTA_ARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTA_ARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	printf("Leí el archivo y extraje el puerto: %d\n", config->PUERTO_PROG);
}

void faltaDeParametros(int argc) {
	if (argc == 1) {
		printf("Te falto el parametro  \n");
	}
	exit(-1);
}

void conectarseConMemoria(int *servMemoria,
		struct sockaddr_in *direccionServidor2) {
	conectar(servMemoria, direccionServidor2);
	handshake(servMemoria, kernel);
	msjConexionCon("una Memoria");
}

int obtenerTamanioDePagina(int *servMemoria) {
	int tamPaginaMemoria;
	if (recv((*servMemoria), &tamPaginaMemoria, sizeof(int), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		return -1;
	}
	printf("El tamanio recibido es: %d\n", tamPaginaMemoria);
	return tamPaginaMemoria;
}

cliente_CPU *crearClienteCPU(int *c_cpu) {
	cliente_CPU *nuevaCPU;
	nuevaCPU = reservarMemoria(sizeof(cliente_CPU));
	nuevaCPU->clie_CPU = (*c_cpu);
	nuevaCPU->libre = 1;
	return nuevaCPU;
}

int hayProcesosReady() {
	return !list_is_empty(listaPCBs_READY);
}

void enviar_un_PCB_a_CPU(t_pcb *pcb,int *unaCPU) {
	//PARA CPU

	//Serializo el PCB y lo envio a CPU
	//abstraer la asquerosidad de abajo
	void * serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb, &serialized_pcb, &serialized_buffer_index);

	if (send((*unaCPU), &serialized_buffer_index, (size_t) sizeof(int), 0)
			< 0) {
		printf("Send serialized_buffer_length to CPU failed\n");
		exit(-1);
	}
	if (send((*unaCPU), serialized_pcb, (size_t) serialized_buffer_index, 0)
			< 0) {
		printf("Send serialized_pcb to CPU failed\n");
		exit(-1);
	}
	printf("PCB enviado a CPU exitosamente\n");
}

int hay_CPUs_Conectadas(){
	return !list_is_empty(listaCPUs);
}

void asignarProcesosSegunFIFO() {
	cliente_CPU *cpuDesocupada;
	t_pcb *pcb;
	printf("************ Planificando segun FIFO *****************\n");
	while (1) {
		if (hayProcesosReady() && hay_CPUs_Conectadas()) {
			bool estaDesocupada(cliente_CPU *unaCPU) {
				return unaCPU->libre == 1;
			}
			cpuDesocupada = list_find(listaCPUs, (void*) estaDesocupada);
			cpuDesocupada->libre = 0;
			pcb = list_get(listaPCBs_READY,0);
			enviar_un_PCB_a_CPU(pcb,&cpuDesocupada->clie_CPU);
			quitar_PCB_de_Lista(listaPCBs_READY,pcb);
			list_add(listaPCBs_EXEC,pcb);
		}
	}

}

void asignarProcesosSegunRoundRobin() {
	printf("************ Planificando segun RR *****************\n");
}

void planificar() {
	if (strcmp("FIFO\n", config->ALGORITMO)) {
		asignarProcesosSegunFIFO();
	} else {
		asignarProcesosSegunRoundRobin();
	}

}

void abrirHiloPlanificador() {
	pthread_t hilo_comandos;
	if (pthread_create(&hilo_comandos, NULL, planificar, NULL)) {
		printf("Error al crear el thread de comandos.\n");
		exit(-1);
	}
}

int main(void) {

	leerArchivo();
	int client_socket[30], procesos_por_socket[30], i, procesoConectado;
	int fdCPU;
	struct sockaddr_in direccionServidor;

	listaPCBs_NEW = list_create();
	listaPCBs_READY = list_create();
	listaPCBs_EXEC = list_create();
	listaPCBs_BLOCK = list_create();
	listaPCBs_EXIT = list_create();
	listaCPUs = list_create();

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1"); // Estos a que hacen referencia en realidad?
	direccionServidor.sin_port = htons(8080);

	//PARA MEMORIA
	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr(config->IP_MEMORIA);
	direccionServidor2.sin_port = htons(config->PUERTO_MEMORIA);

	conectarseConMemoria(&servMemoria, &direccionServidor2);
	tamanioPagMemoria = obtenerTamanioDePagina(&servMemoria);

	inicializarVec(client_socket);
	inicializarVec(procesos_por_socket);

	esperarConexionDe(&direccionServidor);

	//habilitarConsolaKernel(); DEBERIA FUNCIONAR XD
	abrirHiloPlanificador();

	while (1) {
		prepararSockets(client_socket);
		esperarActividad();
		if (esConexionEntrante()) {
			manejarConexionEntrante(&direccionServidor); //La vieja y confiable.
			mostrarNuevaConexion(direccionServidor);

			procesoConectado = realizarHandshake(kernel);

			switch (procesoConectado) {
			case consola:
				msjConexionCon("una Consola\n");
				setInformacionSockets(client_socket, procesos_por_socket,
						consola);
				break;

			case cpu:
				msjConexionCon("CPU");
				cliente_CPU *nuevaCPU = crearClienteCPU(&cliente);
				list_add(listaCPUs, nuevaCPU);
				setInformacionSockets(client_socket, procesos_por_socket, cpu);
				break;

			case file_system:
				printf("Me conecte con File System!\n");
				break;

			default:
				printf("No me puedo conectar con vos.\n");
				break;
			}
		}

		/*
		 list_destroy_and_destroy_elements(listaPCBs_NEW,);
		 Que va en el segundo parametro del proced de arriba???
		 */

		setClienteActual(socketQueTuvoActividad(client_socket));
		i = numeroSocketQueTuvoActividad(client_socket);
		if (clienteActualSeDesconecto()) {
			cerrarConexionClienteActual(&direccionServidor);
			liberarPosicion(client_socket, i);
			liberarPosicion(procesos_por_socket, i);
		} else {
			int proceso = procesos_por_socket[i];
			switch (proceso) { // ACA VAN TODOS LOS CASES DE CUANDO HAY MOVIMIENTO EN UN SOCKET PORQUE SOLICITA ALGO
			case consola:
				printf("Hubo movimiento en una consola\n");
				confirmarAtencionA(&client_socket[i]);
				atenderAConsola(&client_socket[i]);
				break;
			default:
				break;
			}

		}
	}

	return 0;
}
