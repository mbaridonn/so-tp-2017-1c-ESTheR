#ifndef CAPAFS_H_
#define CAPAFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <parser/parser.h>
#include "pcb.h"
#include "estructurasComunes.h"
#include "accionesDeKernel.h"
#include <commons/collections/list.h>

#define CANT_PROC_TABLA_ARCH 100 //ES DEMASIADO? OCUPA MUCHA MEMORIA?
#define CANT_ARCH_TABLA_ARCH 200 //ES DEMASIADO? OCUPA MUCHA MEMORIA?

typedef struct{
	char* nombreArchivo;
	int contadorAperturas;
} entradaTablaArchivosGlobal;

typedef struct{
	char* flags;
	u_int32_t fdGlobal;
	int offset;
} entradaTablaArchivosDeProceso;

void inicializarTablasDeArchivos();
void abrirArchivo(int PID, /*t_direccion_archivo*/char* direccion, t_banderas flags);
void entradaTablaArchivosDeProceso_limpiar(entradaTablaArchivosDeProceso *entrada);
void entradaTablaArchivosGlobal_limpiar(entradaTablaArchivosGlobal *entrada);
void cerrarArchivo(int PID, u_int32_t fileDescriptor);
void borrarArchivo(int PID, u_int32_t fileDescriptor);
void moverCursorArchivo(int PID, u_int32_t fileDescriptor, /*t_valor_variable*/int posicion);
void leerArchivo(int PID, u_int32_t fileDescriptor,/*t_puntero*/u_int32_t informacion, /*t_valor_variable*/int tamanio);
void escribirArchivo(int PID, u_int32_t fileDescriptor, /*char*?*/void* informacion, /*t_valor_variable*/int tamanio);


#endif /* CAPAFS_H_ */
