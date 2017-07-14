#ifndef ESTRUCTURASCOMUNES_H_
#define ESTRUCTURASCOMUNES_H_

#include <sys/types.h>
#include <commons/collections/list.h>
#define MAX_CLIENTS 30

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum accionesFS{
	k_fs_validar_archivo, k_fs_crear_archivo, k_fs_borrar_archivo, k_fs_leer_archivo, k_fs_escribir_archivo
};

enum notificacionesKernelCPU{
	k_cpu_error, k_cpu_accion_OK
};

enum motivos_liberacion_CPU{
	mot_quantum, mot_semaforo, mot_finalizo, mot_error
};

enum algoritmos_planificacion{
	FIFO,RR
};

enum accionesMemoria {
	k_mem_inicializarPrograma,k_mem_finalizar_programa,k_mem_asignar_paginas, k_mem_leer_paginas, k_mem_escribir_paginas, k_mem_liberar_pagina
};

enum accionesCPU{
	cpuLibre, cpu_k_abrir_archivo, cpu_k_cerrar_archivo, cpu_k_borrar_archivo, cpu_k_mover_cursor_archivo,
	cpu_k_leer_archivo, cpu_k_escribir_archivo, cpu_k_obtener_valor_compartida, cpu_k_asignar_valor_compartida,
	cpu_k_reservar, cpu_k_liberar
};

enum confirmacion {
	noHayPaginas, hayPaginas, falloLiberacionPagina, exitoLiberacionPagina
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
	char** SEM_IDS;
	int* SEM_INIT;
	char** SHARED_VARS;
	int* SHARED_VARS_VALUES;//Extra: Almacena valor de las variables globales
	int STACK_SIZE;
} t_configuracion;

t_configuracion *config;

t_list *listaPCBs_NEW;
t_list *listaPCBs_READY;
t_list *listaPCBs_EXEC;
t_list *listaPCBs_BLOCK;
t_list *listaPCBs_EXIT;
t_list *listaCPUs;

int cliente, cliente2, servMemoria, servFS;
u_int32_t tamanioPagMemoria;

#endif /* ESTRUCTURASCOMUNES_H_ */
