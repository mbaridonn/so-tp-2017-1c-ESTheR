#ifndef LIB_PCB_H_
#define LIB_PCB_H_

#include <parser/metadata_program.h>
#include "stack.h"

typedef struct {
	int id_proceso;
	int program_counter;
	int cant_instrucciones;
	int cant_paginas_de_codigo;
	int contadorPags;
	t_intructions *indice_codigo;
	int stackPointer;
	t_stack *indice_stack;
	int etiquetas_size;
	char *indice_etiquetas;
	int exit_code;
} t_pcb;

void *reservarMemoria(int tamanioArchivo);
t_pcb *crearPCB(char *bufferScript,u_int32_t cant_pags_script, int tamPag, int stackSize);
int serializar_data(void *object, int nBytes, void **buffer, int *lastIndex);
void serialize_t_instructions(t_intructions *intructions, void **buffer, int *buffer_size);
void serialize_instrucciones(t_intructions *instrucciones, int instrucciones_size, void **buffer, int *buffer_size);
void serializar_pcb(t_pcb *pcb, void **buffer, int *buffer_size);
int deserializar_data(void *object, int nBytes, void *serialized_data, int *lastIndex);
void deserialize_instrucciones(t_intructions **instrucciones, int instrucciones_size, void **serialized_data, int *serialized_data_size);
void deserializar_pcb(t_pcb **pcb, void *data_serializada, int *indice_data_serializada);

#endif /* LIB_PCB_H_ */
