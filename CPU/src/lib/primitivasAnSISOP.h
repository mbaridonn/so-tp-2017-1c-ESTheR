#ifndef CPU_SRC_LIB_PRIMITIVASANSISOP_H_
#define CPU_SRC_LIB_PRIMITIVASANSISOP_H_

#include <parser/metadata_program.h>
#include <parser/parser.h>
#include "pcb.h"

enum accionesCPU{
	cpuLibre, cpu_k_abrir_archivo, cpu_k_cerrar_archivo, cpu_k_borrar_archivo, cpu_k_mover_cursor_archivo,
	cpu_k_leer_archivo, cpu_k_escribir_archivo, cpu_k_obtener_valor_compartida, cpu_k_asignar_valor_compartida,
	cpu_k_reservar, cpu_k_liberar, cpu_k_wait, cpu_k_signal
};

enum notificacionesKernelCPU{
	k_cpu_error, k_cpu_accion_OK, k_cpu_bloquear, k_cpu_continuar
};

enum accionesCPUMemoria{
	cpu_mem_leer, cpu_mem_escribir
};

enum confirmacion {
	noHayPaginas, hayPaginas, falloLiberacionPagina, exitoLiberacionPagina, noSePudoReservarMemoria, sePudoReservarMemoria,
	noSePudoLiberarMemoria, sePudoLiberarMemoria
};

void solicitarA(int *cliente, char *nombreCli);
bool terminoElPrograma(void);
int hayError();
void inicializarPrimitivasANSISOP(t_pcb* _pcbAEjecutar, int _stackSize, int _tamPag, int _serv_kernel, int _serv_memoria);
void esperarSenialDeKernel();
char * conseguirDatosDeLaMemoria(int PID, int nroPag, int offset, int tamanio);
u_int32_t recibirUIntDeKernel();
void enviarIntAKernel(int un_int);
bool estaBloqueado();

t_puntero definirVariable(t_nombre_variable identificador_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void finalizar(void);
void retornar(t_valor_variable retorno);

void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);
t_puntero reservar(t_valor_variable espacio);
void liberar(t_puntero puntero);
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo direccion);
void cerrar(t_descriptor_archivo descriptor_archivo);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);

#endif /* CPU_SRC_LIB_PRIMITIVASANSISOP_H_ */
