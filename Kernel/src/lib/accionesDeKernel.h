#ifndef LIB_ACCIONESDEKERNEL_H_
#define LIB_ACCIONESDEKERNEL_H_

#include <stdio.h>
#include "estructurasComunes.h"
#include "pcb.h"
#define FALSE 0
#define TRUE 1

int recibirAccionDe(int *cliente);
u_int32_t recibirTamArchivo(int *unCliente);
void recibirArchivoDe(int *unCliente, char *bufferArchivo, u_int32_t fsize);
void validarAperturaArchivo(FILE *archivo);
int divisionRoundUp(int dividendo, int divisor);
void enviarArchivoAMemoria(char *buffer, u_int32_t tamBuffer);
void esperarSenialDeMemoria();
void esperarSenialDeFS();
void kernel_mem_start_process(int *process_id, u_int32_t *cant_pags);
u_int32_t confirmacionMemoria();
void pcb_destroy(t_pcb *puntero);
void quitar_PCB_de_Lista(t_list *lista,t_pcb *pcb);
void avisarAccionAMemoria(int accion);
void avisarAccionAFS(int accion);
void finalizarUnProceso(t_pcb *pcb);
void tomarAccionSegunConfirmacion(u_int32_t confirmacion, t_pcb *pcb);
void avisarAConsolaSegunConfirmacion(int confirmacion, int *consola);
void enviarPIDaConsola(int pid, int *consola);
void proced_script(int *unCliente);

#endif /* LIB_ACCIONESDEKERNEL_H_ */
