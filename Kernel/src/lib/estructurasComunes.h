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
	k_cpu_error, k_cpu_accion_OK, k_cpu_bloquear, k_cpu_continuar
};

enum motivos_liberacion_CPU{
	mot_quantum, mot_finalizo, mot_error, mot_bloqueado
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
	cpu_k_reservar, cpu_k_liberar, cpu_k_wait, cpu_k_signal
};

enum confirmacion {
	noHayPaginas, hayPaginas, falloLiberacionPagina, exitoLiberacionPagina, noSePudoReservarMemoria, sePudoReservarMemoria,
	noSePudoLiberarMemoria, sePudoLiberarMemoria
};

enum acciones {
	startProgram, endProgram
};

typedef struct{
	int clie_consola;
	int pid;
}proceso_por_cliente;

typedef struct{
	int clie_consola;
	int pid;
	char *bufferArchivo;
	int fsize;
}pedido_script;

typedef struct{
	int clie_CPU;
	int libre;
} cliente_CPU;

typedef struct{
	int pid;
	int pos_semaforo;
}bloqueo;

typedef struct{
	int pid;
	int cant_rafagas_ejec;
	int cant_op_alocar;
	int cant_bytes_alocados;
	int cant_op_liberar;
	int cant_bytes_liberados;
	int cant_syscalls_ejec;
}estadisticas_proceso;

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
t_list *lista_pedidos_script;
t_list *lista_detenciones_pendientes;
t_list *lista_bloqueos;
t_list *lista_estadisticas_de_procesos;
t_list *lista_proceso_por_cliente;


int cliente, cliente2, servMemoria, servFS,cant_historica_procesos_memoria, cant_procesos_finalizados, cant_procesos_detenidos;;
u_int32_t tamanioPagMemoria;

#endif /* ESTRUCTURASCOMUNES_H_ */
