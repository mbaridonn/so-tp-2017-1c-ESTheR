//ESTE ESTA ACTUALIZADO
#include <parser/metadata_program.h>
#include "stack.h"

int id_proceso_actual = 0;

enum estadosProceso {
	NEW, READY, EXEC, BLOCK, EXIT
};

typedef struct {
	//obligatorios por el enunciado
	int id_proceso;
	int program_counter;
	int contador_paginas;
	t_intructions *indice_codigo;
	char *indice_etiquetas;
	t_stack *indice_stack;
	int exit_code;
	//adicionales
	int estado_proceso;
} t_pcb;

void aumentarContadorPaginas(t_pcb *pcb) {
	pcb->contador_paginas++;
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio");
		exit(-1);
	}
	return puntero;
}

t_pcb *crearPCB() {
	t_pcb *punteroPCB;
	punteroPCB = reservarMemoria(sizeof(t_pcb));
	punteroPCB->id_proceso = ++id_proceso_actual;
	punteroPCB->estado_proceso = NEW;
	//ver bien cuales de abajo se asignan en 0 al inicio o no
	punteroPCB->contador_paginas = 0;
	punteroPCB->program_counter = 0;
	//punteroPCB->stack_pointer = 0;
	punteroPCB->exit_code = 0;
	return punteroPCB;
}

void serializar_pcb(t_pcb *pcb, void **buffer, int *buffer_size) {
	//CHEQUEAR cuales son sizeof(int) o sizeof(uint32_t)
	serializar_data(&pcb->id_proceso, sizeof(int), buffer, buffer_size);
	serializar_data(&pcb->program_counter, sizeof(uint32_t), buffer, buffer_size);
	serializar_data(&pcb->contador_paginas, sizeof(int), buffer, buffer_size);
	serializar_data(&pcb->estado_proceso, sizeof(int), buffer, buffer_size);
	serializar_data(&pcb->exit_code, sizeof(int), buffer, buffer_size);
}

void deserializar_pcb(t_pcb **pcb, void *data_serializada, int *indice_data_serializada) {
	deserializar_data(&(*pcb)->id_proceso, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->program_counter, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->contador_paginas, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->estado_proceso, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->exit_code, sizeof(int), data_serializada, indice_data_serializada);
}

int serializar_data(void *object, int nBytes, void **buffer, int *lastIndex) {
    void * auxiliar = NULL;
    auxiliar  = realloc(*buffer, nBytes+*lastIndex);
    if(auxiliar  == NULL) {
        return -1;
    }
    *buffer = auxiliar;
    if (memcpy((*buffer + *lastIndex), object, nBytes) == NULL) {
        return -2;
    }
    *lastIndex += nBytes;
    return 0;

}

int deserializar_data(void *object, int nBytes, void *serialized_data, int *lastIndex) {
    if(memcpy(object, serialized_data + *lastIndex, nBytes) == NULL) {
        return -2;
    }
    *lastIndex = *lastIndex + nBytes;
    return 0;
}
