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

int PID, tamPag, clieCPU, contador_paginas;

void inicializar_tablaHeap() {
	tablaHeap = list_create();
}

void inicializar_pid_tamPag_clieCPU_y_contador_paginas(int pid, int tamanioPag, int clienteCPU, int contPags) {
	PID = pid;
	tamPag = tamanioPag;
	clieCPU = clienteCPU;
	contador_paginas = contPags;
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
		if (metadata->estaLibre && metadata->tamanio >= (espacioRequerido + sizeof(heapMetadata))) {
			ptrInicioBloque = entrada->nroPag * tamPag + offset; //Devuelve puntero al metadata (No a los datos!!)
		}
		offset += sizeof(heapMetadata) + metadata->tamanio;
	}
	free(pagina);
	return ptrInicioBloque;
}

u_int32_t reservarMemoriaDinamica(/*t_valor_variable*/int espacioRequerido) {
	u_int32_t ptrInicioBloque = 0;
	if (espacioRequerido > tamPag - 2 * sizeof(heapMetadata)) {
		printf("El espacio requerido supera el tamaño máximo reservable por petición\n");
		//Enviar mensaje a CPU para que finalice abruptamente (Exit Code -8)
		enviarIntACPU(&clieCPU, noSePudoReservarMemoria);
		return 0; //VER SI ES EL MEJOR VALOR A RETORNAR (IMPORTA?)
	}

	//Chequeo si no hay espacio disponible en una página ya asignada
	bool tieneEspacioDisponible(entradaTablaHeap* entrada) {
		return (entrada->PID == PID) && (entrada->tamanioDisponible >= espacioRequerido + sizeof(heapMetadata));
	}
	t_list* entradasConEspacio = list_filter(tablaHeap, (void*) tieneEspacioDisponible);
	if (!list_is_empty(entradasConEspacio)) {
		//Hay que iterar hasta que una página tenga espacio CONTIGUO suficiente
		int cantEntradasConEspacio = list_size(entradasConEspacio);
		int iteradorEntrada = 0;
		entradaTablaHeap* entrada;
		u_int32_t posMetadataInicial = 0;
		while (!posMetadataInicial && iteradorEntrada < cantEntradasConEspacio) {
			entrada = list_get(entradasConEspacio, iteradorEntrada);
			posMetadataInicial = tieneEspacioContiguoSuficiente(entrada, espacioRequerido);
			//Si tiene espacio suficiente devuelve el puntero al inicio del metadata (no al bloque de datos!!), si falla devuelve 0
			iteradorEntrada++;
		}
		if (posMetadataInicial) { //Si se encontró, inserto Metadata en Memoria
			heapMetadata metadataInicio, metadataFin;
			char *bytesLeidos = solicitarLecturaAMemoria(entrada->PID, entrada->nroPag, posMetadataInicial, sizeof(heapMetadata));
			memcpy(&metadataInicio, bytesLeidos, sizeof(heapMetadata));
			free(bytesLeidos);
			metadataFin.estaLibre = true;
			metadataFin.tamanio = metadataInicio.tamanio - (espacioRequerido + sizeof(heapMetadata));
			metadataInicio.estaLibre = false;
			metadataInicio.tamanio = espacioRequerido;
			solicitarEscrituraAMemoria(entrada->PID, entrada->nroPag, posMetadataInicial, sizeof(heapMetadata), &metadataInicio);
			solicitarEscrituraAMemoria(entrada->PID, entrada->nroPag, posMetadataInicial+sizeof(heapMetadata)+espacioRequerido, sizeof(heapMetadata), &metadataFin);
			ptrInicioBloque = posMetadataInicial + sizeof(heapMetadata);

			//Actualizar entrada de la tabla de heap
			entrada->tamanioDisponible -= espacioRequerido + sizeof(heapMetadata);
		}
	}
	list_destroy(entradasConEspacio);//No hay que destruir los elementos!! Son los mismos que los de la tabla original

	if (!ptrInicioBloque) { //Si no existe alguna página con lugar suficiente, se pide una página nueva
		int paginasRequeridas = 1; //Siempre!! (No se puede allocar espacio mayor a una pág)
		int confirmacion = solicitarAsignarPaginasAProceso(PID, paginasRequeridas);
		if(confirmacion==noHayPaginas){
			printf("No se pueden asignar mas paginas al proceso. No hay mas páginas en Memoria\n");
			//Enviar mensaje a CPU para que finalice abruptamente (Exit Code -9)
			enviarIntACPU(&clieCPU, confirmacion);
			return 0; //VER SI ES EL MEJOR VALOR A RETORNAR (IMPORTA?)
		}
		contador_paginas++;
		entradaTablaHeap* entradaNueva = reservarMemoria(sizeof(entradaTablaHeap));
		entradaNueva->PID = PID;
		entradaNueva->nroPag = contador_paginas-1;
		entradaNueva->tamanioDisponible = tamPag - (2 * sizeof(heapMetadata) + espacioRequerido);
		list_add(tablaHeap, entradaNueva);
		//Insertar Metadata en Memoria
		heapMetadata metadataInicio, metadataFin;
		metadataInicio.estaLibre = false;
		metadataInicio.tamanio = espacioRequerido;
		metadataFin.estaLibre = true;
		metadataFin.tamanio = tamPag - (2 * sizeof(heapMetadata) + espacioRequerido);
		solicitarEscrituraAMemoria(PID, contador_paginas-1, 0, sizeof(heapMetadata), &metadataInicio);
		solicitarEscrituraAMemoria(PID, contador_paginas-1, sizeof(heapMetadata)+espacioRequerido, sizeof(heapMetadata), &metadataFin);
		ptrInicioBloque = (contador_paginas-1) * tamPag + sizeof(metadataInicio);
	}

	enviarIntACPU(&clieCPU, sePudoReservarMemoria);
	return ptrInicioBloque;	//Hay que devolver un t_puntero a la memoria reservada (a los datos, no al metadata!!)
}

void liberarMemoriaDinamica(/*t_puntero*/u_int32_t puntero) {
	int nroPagDelBloque = puntero / tamPag;
	int offsetDelBloque = puntero % tamPag - sizeof(heapMetadata);//Quiero apuntar al metadata, no al bloque de datos

	int espacio_liberado = 0;

	printf("nroPag: %d\n", nroPagDelBloque);

	bool esEntradaDeProcesoYPagina(entradaTablaHeap* entrada) {
		return entrada->PID == PID && entrada->nroPag == nroPagDelBloque;
	}
	entradaTablaHeap* entradaAModificar = list_find(tablaHeap, (void*) esEntradaDeProcesoYPagina);
	if (!entradaAModificar) {
		printf("Dicha memoria no está asignada al proceso\n");
		//Enviar mensaje a CPU para que finalice abruptamente
		enviarIntACPU(&clieCPU,noSePudoLiberarMemoria);
		return;
	}

	char* pagina = solicitarLecturaAMemoria(entradaAModificar->PID, entradaAModificar->nroPag, 0, tamPag);

	//Actualizo metadata
	heapMetadata *metadataAModificar, *metadataADerecha, *metadataAIzquierda;
	metadataAModificar = (heapMetadata*) &pagina[offsetDelBloque];
	//Qué pasa si la dirección es de una página pero el offset no corresponde a un metadata??             (!!!)
	metadataAModificar->estaLibre = true;

	espacio_liberado = metadataAModificar->tamanio;

	//Actualizo entrada de la tabla de heap
	entradaAModificar->tamanioDisponible += /*sizeof(heapMetadata) +*/metadataAModificar->tamanio;//ES NECESARIO? CREO QUE NO

	//Consolidacion a derecha
	metadataADerecha = (heapMetadata*) &pagina[offsetDelBloque + sizeof(heapMetadata) + metadataAModificar->tamanio];
	if (metadataADerecha->estaLibre) {
		metadataAModificar->tamanio += metadataADerecha->tamanio + sizeof(heapMetadata);
		entradaAModificar->tamanioDisponible += sizeof(heapMetadata);
		solicitarEscrituraAMemoria(entradaAModificar->PID, entradaAModificar->nroPag, offsetDelBloque, sizeof(heapMetadata), metadataAModificar);
	}
	//Consolidacion a izquierda (tengo que buscar el bloque anterior desde el principio de la página)
	int offsetMetadataIzq = 0;
	while (offsetMetadataIzq < offsetDelBloque) {
		metadataAIzquierda = (heapMetadata*) &pagina[offsetMetadataIzq];
		if (metadataAIzquierda->estaLibre) {
			heapMetadata* metadataSiguiente = (heapMetadata*) &pagina[offsetMetadataIzq + sizeof(heapMetadata) + metadataAIzquierda->tamanio];
			if (metadataSiguiente->estaLibre) {
				metadataAIzquierda->tamanio += sizeof(heapMetadata) + metadataSiguiente->tamanio;
				entradaAModificar->tamanioDisponible += sizeof(heapMetadata);
				solicitarEscrituraAMemoria(entradaAModificar->PID, entradaAModificar->nroPag, offsetMetadataIzq, sizeof(heapMetadata), metadataAIzquierda);
			}
		}
		offsetMetadataIzq += sizeof(heapMetadata) + metadataAIzquierda->tamanio;
	}

	//Si una página tiene todos los bloques marcados como libres, debe ser liberada.
	if (entradaAModificar->tamanioDisponible == tamPag - sizeof(heapMetadata)) {
		int confirmacion = solicitarLiberarPaginaDeProceso(entradaAModificar->PID, entradaAModificar->nroPag);
		enviarIntACPU(&clieCPU,confirmacion);
		//Si confirmacion==falloLiberacionPagina no se hace nada, si confirmacion==exitoLiberacionPagina se decrementa contador_paginas
	} else {//No se libera la pagina
		enviarIntACPU(&clieCPU, sePudoLiberarMemoria);
	}
	actualizar_estadisticas_cant_bytes_liberados_de(PID,espacio_liberado);
	free(pagina);
}

void liberar_tablaHeap() {
	list_destroy_and_destroy_elements(tablaHeap, (void*) entradaTablaHeap_destroy);
}
