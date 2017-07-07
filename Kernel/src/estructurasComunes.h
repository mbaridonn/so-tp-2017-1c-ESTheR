/*
 * estructurasComunes.h
 *
 *  Created on: 31/5/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURASCOMUNES_H_
#define ESTRUCTURASCOMUNES_H_
#define MAX_CLIENTS 30
#include <commons/collections/list.h>
#include "lib/pcb.h"

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum accionesMemoria {
	asignarPaginas, finalizarProceso
};

enum accionesCPU{
	cpuLibre
};

enum confirmacion {
	noHayPaginas, hayPaginas
};

enum acciones {
	startProgram
};

typedef struct{
	int clie_CPU;
	int libre;
} cliente_CPU;

typedef struct {
	int PUERTO_PROG;
	int PUERTO_CPU;
	char IP_MEMORIA[15];
	int PUERTO_MEMORIA;
	char IP_FS[15];
	int PUERTO_FS;
	int QUANTUM;
	int QUANTUM_SLEEP;
	char ALGORITMO[30];
	int GRADO_MULTIPROG;
	char SEM_IDS[10][30]; // Debería ser una lista alfanumerica
	int SEM_INIT[10][30]; // Lo mismo pero numerica
	char SHARED_VARS[10][30]; // IDEM SEM_IDS
	int STACK_SIZE;

} t_configuracion;

t_configuracion *config;

t_list *listaPCBs_NEW;
t_list *listaPCBs_READY;
t_list *listaPCBs_EXEC;
t_list *listaPCBs_BLOCK;
t_list *listaPCBs_EXIT;
t_list *listaCPUs;

u_int32_t planificacionActivada = 1;

int cliente, cliente2, servMemoria;
u_int32_t tamanioPagMemoria;

#endif /* ESTRUCTURASCOMUNES_H_ */
