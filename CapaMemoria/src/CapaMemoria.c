#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>

typedef struct{
	int tamanio;
	bool estaLibre;
} heapMetadata;

typedef struct{
	int PID;
	int nroPag;
	int tamanioDisponible;//Sumatoria de tamaño de las particiones libres (aunque haya fragmentación)
} entradaTablaHeap;
t_list* tablaHeap;//La tabla está asociada al PCB a través del PID. Es una única para todos los procesos (ESTÁ BIEN?)

//DESDE ACÁ NO COPIAR

int PID = 1, tamPag = 512;//DATOS QUE CONOZCO

void *reservarMemoria(int tamanio) {
	void *puntero = malloc(tamanio);
	if (puntero == NULL) {
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

//HASTA ACÁ NO COPIAR

void entradaTablaHeap_destroy(entradaTablaHeap *puntero) {//Necesario para eliminar entradas de la lista
	free(puntero);
}

u_int32_t tieneEspacioContiguoSuficiente(entradaTablaHeap* entrada, int espacioRequerido){
	u_int32_t ptrInicioBloque = 0;
	char* pagina = NULL;
	heapMetadata* metadata;
	int offset = 0;
	//Envía mensaje a Memoria: pagina = leerPagina(PID, entrada->nroPag, 0, tamPag);
	while(!ptrInicioBloque && offset<tamPag){
		metadata = (heapMetadata*) &pagina[offset];//Hay que usar el offset así para que no se aplique aritmética de punteros
		if(metadata->estaLibre && metadata->tamanio >= (espacioRequerido+sizeof(heapMetadata))){
			ptrInicioBloque = entrada->nroPag * tamPag + offset;//Devuelve puntero al metadata (No a los datos!!)
		}
		offset += sizeof(heapMetadata) + metadata->tamanio;
	}
	return ptrInicioBloque;
}

u_int32_t reservarMemoriaDinamica(int espacioRequerido){//En realidad es un t_valor_variable (que es igual que un int)
	u_int32_t ptrInicioBloque = 0;
	if(espacioRequerido > tamPag-2*sizeof(heapMetadata)){
		printf("El espacio requerido supera el tamaño máximo reservable por petición\n");
		//Enviar mensaje a Memoria para que finalice abruptamente (Exit Code -8)
		return 0;//VER SI ES EL MEJOR VALOR A RETORNAR (IMPORTA?)
	}

	//Chequeo si no hay espacio disponible en una página ya asignada
	bool tieneEspacioDisponible(entradaTablaHeap* entrada){
		return (entrada->PID==PID) && (entrada->tamanioDisponible>=espacioRequerido+sizeof(heapMetadata));
	}
	t_list* entradasConEspacio = list_filter(tablaHeap,(void*) tieneEspacioDisponible);
	if(!list_is_empty(entradasConEspacio)){
		//Hay que iterar hasta que una página tenga espacio CONTIGUO suficiente
		int cantEntradasConEspacio = list_size(entradasConEspacio);
		int iteradorEntrada = 0;
		entradaTablaHeap* entrada;
		u_int32_t posMetadataInicial = 0;
		while(!posMetadataInicial && iteradorEntrada<cantEntradasConEspacio){
			entrada = list_get(entradasConEspacio,iteradorEntrada);
			posMetadataInicial=tieneEspacioContiguoSuficiente(entrada, espacioRequerido);
			//Si tiene espacio suficiente devuelve el puntero al inicio del metadata (no al bloque de datos!!), si falla devuelve 0
			iteradorEntrada++;
		}
		if(posMetadataInicial){//Si se encontró, inserto Metadata en Memoria
			heapMetadata metadataInicio, metadataFin;//CAPAZ HAYA QUE CAMBIARLOS A PUNTEROS
			//Envía mensaje a Memoria: metadataInicio = leerPagina(entrada->PID, entrada->nroPag, posMetadataInicial, sizeof(heapMetadata));
			metadataFin.estaLibre = true;
			metadataFin.tamanio = metadataInicio.tamanio - (espacioRequerido + sizeof(heapMetadata));
			metadataInicio.estaLibre = false;
			metadataInicio.tamanio = espacioRequerido;
			//Envía mensaje a Memoria: escribirPagina(entrada->PID, entrada->nroPag, posMetadataInicial, sizeof(heapMetadata), &metadataInicio);
			//Envía mensaje a Memoria: escribirPagina(entrada->PID, entrada->nroPag, posMetadataInicial+sizeof(heapMetadata)+espacioRequerido, sizeof(heapMetadata), &metadataFin);
			ptrInicioBloque = posMetadataInicial + sizeof(heapMetadata);

			//Actualizar entrada de la tabla de heap (ALCANZA?)
			entrada->tamanioDisponible -= espacioRequerido + sizeof(heapMetadata);
		}
	}
	list_destroy(entradasConEspacio);//No hay que destruir los elementos!! Son los mismos que los de la tabla original

	if (!ptrInicioBloque){ //Si no existe alguna página con lugar suficiente, se pide una página nueva
		//int paginasRequeridas = 1; Siempre!! (No se puede allocar espacio mayor a una pág)
		//Envía mensaje a Memoria: asignarPaginasAProceso(PID, pagsRequeridas); (PUEDE FALLAR!!)
		//PCB->contador_paginas++;
		entradaTablaHeap* entradaNueva = reservarMemoria(sizeof(entradaTablaHeap));
		entradaNueva->PID = PID;
		entradaNueva->nroPag = 0;//PCB->contador_paginas
		entradaNueva->tamanioDisponible = tamPag-(2*sizeof(heapMetadata)+espacioRequerido);
		list_add(tablaHeap, entradaNueva);
		//Insertar Metadata en Memoria
		heapMetadata metadataInicio, metadataFin;
		metadataInicio.estaLibre = false;
		metadataInicio.tamanio = espacioRequerido;
		metadataFin.estaLibre = true;
		metadataFin.tamanio = tamPag-(2*sizeof(heapMetadata)+espacioRequerido);
		//Envía mensaje a Memoria: escribirPagina(PID, PCB->contador_paginas, 0, sizeof(heapMetadata), &metadataInicio);
		//Envía mensaje a Memoria: escribirPagina(PID, PCB->contador_paginas, sizeof(heapMetadata)+espacioRequerido, sizeof(heapMetadata), &metadataFin);
		ptrInicioBloque = /*PCB->contador_paginas * */ tamPag + sizeof(metadataInicio);//ESTÁ BIEN??
	}

	return ptrInicioBloque;//Hay que devolver un t_puntero a la memoria reservada (a los datos, no al metadata!!)
}

void liberarMemoriaDinamica(u_int32_t puntero){//En realidad recibe un t_puntero
	int nroPagDelBloque = puntero / tamPag;
	int offsetDelBloque = puntero % tamPag - sizeof(heapMetadata);//Quiero apuntar al metadata, no al bloque de datos

	bool esEntradaDeProcesoYPagina(entradaTablaHeap* entrada){
		return entrada->PID==PID && entrada->nroPag==nroPagDelBloque;
	}
	entradaTablaHeap* entradaAModificar = list_find(tablaHeap,(void*) esEntradaDeProcesoYPagina);
	if(!entradaAModificar){//FIND DEVUELVE NULL SI NINGUNO CUMPLE??
		printf("Dicha memoria no está asignada al proceso\n");
		//Enviar mensaje a Memoria para que finalice abruptamente
		return;
	}

	char* pagina = NULL;
	//Envía mensaje a Memoria: pagina = leerPagina(entradaAModificar->PID, entradaAModificar->nroPag, 0, tamPag);

	//Actualizo metadata
	heapMetadata *metadataAModificar, *metadataADerecha, *metadataAIzquierda;
	metadataAModificar = (heapMetadata*) &pagina[offsetDelBloque];
	//Qué pasa si la dirección es de una página pero el offset no corresponde a un metadata??             (!!!)
	metadataAModificar->estaLibre = true;

	//Actualizo entrada de la tabla de heap
	entradaAModificar->tamanioDisponible += /*sizeof(heapMetadata) +*/ metadataAModificar->tamanio;//ES NECESARIO? CREO QUE NO

	//Consolidacion a derecha
	metadataADerecha = (heapMetadata*) &pagina[offsetDelBloque+sizeof(heapMetadata)+metadataAModificar->tamanio];
	if(metadataADerecha->estaLibre){
		metadataAModificar->tamanio += metadataADerecha->tamanio + sizeof(heapMetadata);
		entradaAModificar->tamanioDisponible += sizeof(heapMetadata);
		//Envía mensaje a Memoria: escribirPagina(entradaAModificar->PID, entradaAModificar->nroPag, offsetDelBloque, sizeof(heapMetadata), metadataAModificar);
	}
	//Consolidacion a izquierda (tengo que buscar el bloque anterior desde el principio de la página)
	int offsetMetadataIzq = 0;
	while(offsetMetadataIzq<offsetDelBloque){
		metadataAIzquierda = (heapMetadata*) &pagina[offsetMetadataIzq];
		if(metadataAIzquierda->estaLibre){
			heapMetadata* metadataSiguiente = (heapMetadata*) &pagina[offsetMetadataIzq+sizeof(heapMetadata)+metadataAIzquierda->tamanio];
			if(metadataSiguiente->estaLibre){
				metadataAIzquierda->tamanio += sizeof(heapMetadata) + metadataSiguiente->tamanio;
				entradaAModificar->tamanioDisponible += sizeof(heapMetadata);
				//Envía mensaje a Memoria: escribirPagina(entradaAModificar->PID, entradaAModificar->nroPag, offsetMetadataIzq, sizeof(heapMetadata), metadataAIzquierda);
			}
		}
		offsetMetadataIzq += sizeof(heapMetadata) + metadataAIzquierda->tamanio;
	}

	//Si una página tiene todos los bloques marcados como libres, debe ser liberada.
	if(entradaAModificar->tamanioDisponible == tamPag-sizeof(heapMetadata)){
		//Envía mensaje a Memoria: liberarPagina(entradaAModificar->PID, entradaAModificar->nroPag)
		//PUEDE FALLAR
		//PCB->contador_paginas--;
	}
}

int main(void) {
	tablaHeap = list_create();

	//FALTA: MEMORY LEAKS
	//Al finalizar un proceso, el Kernel deberá informar si un proceso liberó todas las estructuras en las páginas de Heap.

	list_destroy_and_destroy_elements(tablaHeap,(void*)entradaTablaHeap_destroy);

	return EXIT_SUCCESS;
}
