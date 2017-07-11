#ifndef CPU_SRC_LIB_PRIMITIVASANSISOP_H_
#define CPU_SRC_LIB_PRIMITIVASANSISOP_H_

#include <parser/metadata_program.h>
#include <parser/parser.h>

/*registroStack* reg_stack_create();
void restaurarContextoDeEjecucion();*/

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
bool terminoElPrograma(void);//FUNCION DE PRUEBA
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

/*// Variables globales provenientes del main:
extern bool finalizarCPU;
extern bool cpuOciosa;
extern bool huboStackOverflow;
extern int devolvioPcb;
extern bool finalizoPrograma;*/

#endif /* CPU_SRC_LIB_PRIMITIVASANSISOP_H_ */
