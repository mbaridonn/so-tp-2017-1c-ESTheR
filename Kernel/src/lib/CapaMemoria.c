#include "CapaMemoria.h"

typedef struct {
	int tamanio;
	bool estaLibre;
} heapMetadata;

typedef struct {
	int PID;
	int nroPag;
	int tamanioDisponible; //Sumatoria de tamaño de las particiones libres (aunque haya fragmentación)
} entradaTablaHeap;
t_list* tablaHeap; //La tabla está asociada al PCB a través del PID. Es una única para todos los procesos.

int PID, tamPag;

void inicializar_tablaHeap() {
	tablaHeap = list_create();
}

void inicializar_pid_y_tamPag(int pid, int tamanioPag) {
	PID = pid;
	tamPag = tamanioPag;
}

void entradaTablaHeap_destroy(entradaTablaHeap *puntero) { //Necesario para eliminar entradas de la lista
	free(puntero);
}

int solicitarLiberarPaginaDeProceso(int pid,int nroPag){
	avisarAccionAMemoria(k_mem_liberar_pagina);
	enviarIntAMemoria(pid);
	enviarIntAMemoria(nroPag);
	return confirmacionMemoria();
}

void solicitarEscrituraAMemoria(int pid, int nroPagina, int offset, int tamanio,
		char *bytesAEscribir) {
	avisarAccionAMemoria(k_mem_escribir_paginas);
	enviarIntAMemoria(pid);
	enviarIntAMemoria(nroPagina);
	enviarIntAMemoria(offset);
	enviarBufferAMemoria(bytesAEscribir,tamanio);

}

int solicitarAsignarPaginasAProceso(int pid,int cantPags){
	avisarAccionAMemoria(k_mem_asignar_paginas);
	enviarIntAMemoria(pid);
	enviarIntAMemoria(cantPags);
	return confirmacionMemoria();
}

char *solicitarLecturaAMemoria(int pid, int nroPagina, int offset, int tamanio) {
	avisarAccionAMemoria(k_mem_leer_paginas);
	enviarIntAMemoria(pid);
	enviarIntAMemoria(nroPagina);
	enviarIntAMemoria(offset);
	enviarIntAMemoria(tamanio);
	char *bytesLeidos = reservarMemoria(tamanio);
	recibirArchivoDe(&servMemoria, bytesLeidos, tamanio);
	return bytesLeidos;
}

u_int32_t tieneEspacioContiguoSuficiente(entradaTablaHeap* entrada,
		int espacioRequerido) {
	u_int32_t ptrInicioBloque = 0;
	char* pagina = NULL;
	heapMetadata* metadata;
	int offset = 0;
	pagina = solicitarLecturaAMemoria(PID, entrada->nroPag, 0, tamPag);
	while (!ptrInicioBloque && offset < tamPag) {
		metadata = (heapMetadata*) &pagina[offset]; //Hay que usar el offset así para que no se aplique aritmética de punteros
		if (metadata->estaLibre
				&& metadata->tamanio
						>= (espacioRequerido + sizeof(heapMetadata))) {
			ptrInicioBloque = entrada->nroPag * tamPag + offset; //Devuelve puntero al metadata (No a los datos!!)
		}
		offset += sizeof(heapMetadata) + metadata->tamanio;
	}
	free(pagina);
	return ptrInicioBloque;
}

u_int32_t reservarMemoriaDinamica(int espacioRequerido) { //En realidad es un t_valor_variable (que es igual que un int)
	u_int32_t ptrInicioBloque = 0;
	if (espacioRequerido > tamPag - 2 * sizeof(heapMetadata)) {
		printf(
				"El espacio requerido supera el tamaño máximo reservable por petición\n");
		//Enviar mensaje a CPU para que finalice abruptamente (Exit Code -8)
		return 0; //VER SI ES EL MEJOR VALOR A RETORNAR (IMPORTA?)
	}

	//Chequeo si no hay espacio disponible en una página ya asignada
	bool tieneEspacioDisponible(entradaTablaHeap* entrada) {
		return (entrada->PID == PID)
				&& (entrada->tamanioDisponible
						>= espacioRequerido + sizeof(heapMetadata));
	}
	t_list* entradasConEspacio = list_filter(tablaHeap,
			(void*) tieneEspacioDisponible);
	if (!list_is_empty(entradasConEspacio)) {
		//Hay que iterar hasta que una página tenga espacio CONTIGUO suficiente
		int cantEntradasConEspacio = list_size(entradasConEspacio);
		int iteradorEntrada = 0;
		entradaTablaHeap* entrada;
		u_int32_t posMetadataInicial = 0;
		while (!posMetadataInicial && iteradorEntrada < cantEntradasConEspacio) {
			entrada = list_get(entradasConEspacio, iteradorEntrada);
			posMetadataInicial = tieneEspacioContiguoSuficiente(entrada,
					espacioRequerido);
			//Si tiene espacio suficiente devuelve el puntero al inicio del metadata (no al bloque de datos!!), si falla devuelve 0
			iteradorEntrada++;
		}
		if (posMetadataInicial) { //Si se encontró, inserto Metadata en Memoria
			heapMetadata metadataInicio, metadataFin;
			char *bytesLeidos = solicitarLecturaAMemoria(entrada->PID,
					entrada->nroPag, posMetadataInicial, sizeof(heapMetadata));
			memcpy(&metadataInicio, bytesLeidos, sizeof(heapMetadata));
			free(bytesLeidos);
			metadataFin.estaLibre = true;
			metadataFin.tamanio = metadataInicio.tamanio
					- (espacioRequerido + sizeof(heapMetadata));
			metadataInicio.estaLibre = false;
			metadataInicio.tamanio = espacioRequerido;
			solicitarEscrituraAMemoria(entrada->PID, entrada->nroPag, posMetadataInicial, sizeof(heapMetadata), &metadataInicio);
			solicitarEscrituraAMemoria(entrada->PID, entrada->nroPag, posMetadataInicial+sizeof(heapMetadata)+espacioRequerido, sizeof(heapMetadata), &metadataFin);
			ptrInicioBloque = posMetadataInicial + sizeof(heapMetadata);

			//Actualizar entrada de la tabla de heap
			entrada->tamanioDisponible -= espacioRequerido
					+ sizeof(heapMetadata);
		}
	}
	list_destroy(entradasConEspacio);//No hay que destruir los elementos!! Son los mismos que los de la tabla original

	if (!ptrInicioBloque) { //Si no existe alguna página con lugar suficiente, se pide una página nueva
		int paginasRequeridas = 1; //Siempre!! (No se puede allocar espacio mayor a una pág)
		int confirmacion = solicitarAsignarPaginasAProceso(PID, paginasRequeridas);
		if(confirmacion==noHayPaginas){
			// ERROR PENDIENTE !!!!!!!!!!!!
		}
		//PCB->contador_paginas++; HAY QUE TENER EL PCB MAN
		entradaTablaHeap* entradaNueva = reservarMemoria(
				sizeof(entradaTablaHeap));
		entradaNueva->PID = PID;
		entradaNueva->nroPag = 0;		//PCB->contador_paginas
		entradaNueva->tamanioDisponible = tamPag
				- (2 * sizeof(heapMetadata) + espacioRequerido);
		list_add(tablaHeap, entradaNueva);
		//Insertar Metadata en Memoria
		heapMetadata metadataInicio, metadataFin;
		metadataInicio.estaLibre = false;
		metadataInicio.tamanio = espacioRequerido;
		metadataFin.estaLibre = true;
		metadataFin.tamanio = tamPag
				- (2 * sizeof(heapMetadata) + espacioRequerido);
		//solicitarEscrituraAMemoria(PID, PCB->contador_paginas, 0, sizeof(heapMetadata), &metadataInicio);
		//solicitarEscrituraAMemoria(PID, PCB->contador_paginas, sizeof(heapMetadata)+espacioRequerido, sizeof(heapMetadata), &metadataFin);
		ptrInicioBloque = /*PCB->contador_paginas * */tamPag
				+ sizeof(metadataInicio);		//ESTÁ BIEN??
	}

	return ptrInicioBloque;	//Hay que devolver un t_puntero a la memoria reservada (a los datos, no al metadata!!)
	// PENDIENTE. Capaz convenga enviar mensaje indirectamente.
}

void liberarMemoriaDinamica(u_int32_t puntero) {//En realidad recibe un t_puntero
	int nroPagDelBloque = puntero / tamPag;
	int offsetDelBloque = puntero % tamPag - sizeof(heapMetadata);//Quiero apuntar al metadata, no al bloque de datos

	bool esEntradaDeProcesoYPagina(entradaTablaHeap* entrada) {
		return entrada->PID == PID && entrada->nroPag == nroPagDelBloque;
	}
	entradaTablaHeap* entradaAModificar = list_find(tablaHeap,
			(void*) esEntradaDeProcesoYPagina);
	if (!entradaAModificar) {
		printf("Dicha memoria no está asignada al proceso\n");
		//Enviar mensaje a CPU para que finalice abruptamente
		return; // PENDIENTE
	}

	char* pagina = solicitarLecturaAMemoria(entradaAModificar->PID, entradaAModificar->nroPag, 0, tamPag);

	//Actualizo metadata
	heapMetadata *metadataAModificar, *metadataADerecha, *metadataAIzquierda;
	metadataAModificar = (heapMetadata*) &pagina[offsetDelBloque];
	//Qué pasa si la dirección es de una página pero el offset no corresponde a un metadata??             (!!!)
	metadataAModificar->estaLibre = true;

	//Actualizo entrada de la tabla de heap
	entradaAModificar->tamanioDisponible +=
			/*sizeof(heapMetadata) +*/metadataAModificar->tamanio;//ES NECESARIO? CREO QUE NO

	//Consolidacion a derecha
	metadataADerecha = (heapMetadata*) &pagina[offsetDelBloque
			+ sizeof(heapMetadata) + metadataAModificar->tamanio];
	if (metadataADerecha->estaLibre) {
		metadataAModificar->tamanio += metadataADerecha->tamanio
				+ sizeof(heapMetadata);
		entradaAModificar->tamanioDisponible += sizeof(heapMetadata);
		solicitarEscrituraAMemoria(entradaAModificar->PID, entradaAModificar->nroPag, offsetDelBloque, sizeof(heapMetadata), metadataAModificar);
	}
	//Consolidacion a izquierda (tengo que buscar el bloque anterior desde el principio de la página)
	int offsetMetadataIzq = 0;
	while (offsetMetadataIzq < offsetDelBloque) {
		metadataAIzquierda = (heapMetadata*) &pagina[offsetMetadataIzq];
		if (metadataAIzquierda->estaLibre) {
			heapMetadata* metadataSiguiente =
					(heapMetadata*) &pagina[offsetMetadataIzq
							+ sizeof(heapMetadata) + metadataAIzquierda->tamanio];
			if (metadataSiguiente->estaLibre) {
				metadataAIzquierda->tamanio += sizeof(heapMetadata)
						+ metadataSiguiente->tamanio;
				entradaAModificar->tamanioDisponible += sizeof(heapMetadata);
				solicitarEscrituraAMemoria(entradaAModificar->PID, entradaAModificar->nroPag, offsetMetadataIzq, sizeof(heapMetadata), metadataAIzquierda);
			}
		}
		offsetMetadataIzq += sizeof(heapMetadata) + metadataAIzquierda->tamanio;
	}

	//Si una página tiene todos los bloques marcados como libres, debe ser liberada.
	if (entradaAModificar->tamanioDisponible == tamPag - sizeof(heapMetadata)) {
		int confirmacion = solicitarLiberarPaginaDeProceso(entradaAModificar->PID, entradaAModificar->nroPag);
		if(confirmacion==falloLiberacionPagina){
			//PUEDE FALLAR
			// ENVIAR MENSAJE A CPU. EN AMBOS CASOS.
		}else{
			//PCB->contador_paginas--;
		}
	}
	free(pagina);
}

void liberar_tablaHeap() {
	list_destroy_and_destroy_elements(tablaHeap,
			(void*) entradaTablaHeap_destroy);
}
