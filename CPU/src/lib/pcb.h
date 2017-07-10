#include <parser/metadata_program.h>
#include "stack.h"

int id_proceso_actual = 0;

enum estadosProceso {
	NEW, READY, EXEC, BLOCK, EXIT
};

typedef struct {
	int id_proceso;
	int program_counter;
	int cant_instrucciones;
	int exit_code;
	int cant_paginas_de_codigo;
	t_intructions *indice_codigo;
	t_stack *indice_stack;
	char *indice_etiquetas;
} t_pcb;

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio");
		exit(-1);
	}
	return puntero;
}

t_pcb *crearPCB(char *bufferScript,u_int32_t cant_pags_script) {
	t_pcb *punteroPCB;
	punteroPCB = reservarMemoria(sizeof(t_pcb));
	punteroPCB->id_proceso = ++id_proceso_actual;
	t_metadata_program *metadata = metadata_desde_literal(bufferScript);
	punteroPCB->indice_codigo = metadata->instrucciones_serializado;
	punteroPCB->cant_instrucciones = metadata->instrucciones_size;
	punteroPCB->indice_etiquetas = NULL;
	punteroPCB->indice_stack = reservarMemoria(sizeof(t_stack));
	punteroPCB->cant_paginas_de_codigo = cant_pags_script;
	punteroPCB->program_counter = 0;
	punteroPCB->exit_code = 0; //En realidad 0 significa que termino bien.
	return punteroPCB;
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

void serialize_t_instructions(t_intructions *intructions, void **buffer, int *buffer_size) {
    serializar_data(&intructions->start, sizeof(t_puntero_instruccion), buffer, buffer_size);
    serializar_data(&intructions->offset, sizeof(t_size), buffer, buffer_size);
}

void serialize_instrucciones(t_intructions *instrucciones, int instrucciones_size, void **buffer, int *buffer_size) {
    int indice = 0;
    for(indice = 0; indice < instrucciones_size ; indice++) {
        serialize_t_instructions(instrucciones+indice, buffer, buffer_size);
    }
}

void serializar_pcb(t_pcb *pcb, void **buffer, int *buffer_size) { //FALTA SERIALIZAR ETIQUETAS
	//CHEQUEAR cuales son sizeof(int) o sizeof(uint32_t)
	serializar_data(&pcb->id_proceso, sizeof(int), buffer, buffer_size);
	serializar_data(&pcb->program_counter, sizeof(int/*uint32_t*/), buffer, buffer_size);
	serializar_data(&pcb->cant_paginas_de_codigo, sizeof(int), buffer, buffer_size);
	serializar_data(&pcb->cant_instrucciones, sizeof(int), buffer, buffer_size);
	serialize_instrucciones(pcb->indice_codigo, pcb->cant_instrucciones, buffer, buffer_size);
	serializar_data(&pcb->exit_code, sizeof(int), buffer, buffer_size);
}

int deserializar_data(void *object, int nBytes, void *serialized_data, int *lastIndex) {
    if(memcpy(object, serialized_data + *lastIndex, nBytes) == NULL) {
        return -2;
    }
    *lastIndex = *lastIndex + nBytes;
    return 0;
}

void deserialize_instrucciones(t_intructions **instrucciones, int instrucciones_size, void **serialized_data, int *serialized_data_size) {
    *instrucciones = calloc(instrucciones_size, sizeof(t_intructions));
    int indice = 0;
    //instrucciones_size tiene la cantidad de instrucciones cargadas
    for(indice = 0; indice < instrucciones_size; indice ++) {
        deserializar_data(&(*instrucciones+indice)->start, sizeof(t_puntero_instruccion), serialized_data, serialized_data_size);
        deserializar_data(&(*instrucciones+indice)->offset, sizeof(int), serialized_data, serialized_data_size);
    }
}

void deserializar_pcb(t_pcb **pcb, void *data_serializada, int *indice_data_serializada) {
	deserializar_data(&(*pcb)->id_proceso, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->program_counter, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->cant_paginas_de_codigo, sizeof(int), data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->cant_instrucciones, sizeof(int), data_serializada, indice_data_serializada);
    deserialize_instrucciones(&(*pcb)->indice_codigo, (*pcb)->cant_instrucciones, data_serializada, indice_data_serializada);
	deserializar_data(&(*pcb)->exit_code, sizeof(int), data_serializada, indice_data_serializada);
}
