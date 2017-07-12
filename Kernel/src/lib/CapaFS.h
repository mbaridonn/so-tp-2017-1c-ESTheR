#ifndef CAPAFS_H_
#define CAPAFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <parser/parser.h>
#include "pcb.h"
#include <commons/collections/list.h>
#include "accionesDeKernel.h"

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
u_int32_t abrirArchivo(int PID, /*t_direccion_archivo*/char* direccion, t_banderas flags);
void cerrarArchivo(int PID, u_int32_t fileDescriptor);
int borrarArchivo(int PID, u_int32_t fileDescriptor);
void moverCursorArchivo(int PID, u_int32_t fileDescriptor, /*t_valor_variable*/int posicion);
void leerArchivo(int PID, u_int32_t fileDescriptor, /*t_valor_variable*/int tamanio);
void escribirArchivo(int PID, u_int32_t fileDescriptor, char* bytesAEscribir, /*t_valor_variable*/int tamanio);

#endif /* CAPAFS_H_ */
