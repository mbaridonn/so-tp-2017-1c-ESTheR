#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <commons/collections/list.h>

int tamFrame = 0;
int cantFrames = 0;

char* memoriaPrincipal;

typedef struct {
	int PID;
	int numPag;
} tablaPagInv;
int cantFramesEstructuraAdm;
tablaPagInv* estructuraAdm;	//Tiene que apuntar SIEMPRE al inicio de la tabla
//En vez de tener varios punteros a MP se podría tener uno solo, e irlo casteando

int* cantPagsPorPID; //Esta tabla NO está en MP. Se accede a través del PID de un proceso

typedef struct {
	int PID;
	int numPag;
	int contenido;//POR AHORA es el nro de frame donde se encuentra la pág
} entradaCache;
entradaCache* memoriaCache;

int entradasCache;
int cacheXProceso;

t_list* colaEntradasCache;//Necesaria para implementar el algoritmo LRU (OJO! Nunca se libera la memoria)

void *reservarMemoria(int tamanio) {
	void *puntero = malloc(tamanio);
	if (puntero == NULL) {
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

int divisionRoundUp(int dividendo, int divisor) {
	if (dividendo <= 0 || divisor <= 0) {
		printf("Esta division funciona unicamente con enteros positivos\n");
		exit(-1);
	}
	return 1 + ((dividendo - 1) / divisor);
}

int max(int a, int b){
	if (a>b) return a;
	else return b;
}

int isInt(char *string){
	int i, stringLength = strlen(string);
	for(i = 0; i < stringLength; i++)
		if(isdigit(string[i]) == 0 || ispunct(string[i]) != 0)
			break;
	if(i != stringLength)
		return 0;
	else
		return 1;
}

void dumpTablaDePags(){// Prueba de que está bien inicializado (después borrar)
	int i;
	for (i = 0; i < cantFrames; i++) {
			printf("%d,", estructuraAdm[i].PID);
	}
}

void inicializarTablaPags(int cantFramesEstructuraAdm) {
	int i = 0;
	for (i = 0; i < cantFrames; i++) {
		if (i < cantFramesEstructuraAdm) {
			estructuraAdm[i].PID = 0; //Supongo que PID 0 significa frame de estructura administrativa
			estructuraAdm[i].numPag = i;
		} else {
			estructuraAdm[i].PID = -1; //Supongo que PID -1 significa frame vacío
		}
	}
	cantPagsPorPID[0] = cantFramesEstructuraAdm;
	dumpTablaDePags();
}

void inicializarCantPagsPorPID() {
	int i;
	for (i = 0; i < cantFrames; i++) {
		cantPagsPorPID[i] = 0;
	}
}

void limpiarCache(){
	int i;
	for (i = 0; i < entradasCache; i++) {
		memoriaCache[i].PID = -1; //Supongo que PID -1 significa entrada vacía
	}
}

void dumpCache(){
	int i;
	for (i = 0; i < entradasCache; i++) {
			printf("%d | %d | %d \n", memoriaCache[i].PID, memoriaCache[i].numPag, memoriaCache[i].contenido);
			//Debería mostrar realmente el contenido, no el frame
	}
}

void inicializarCache(){
	memoriaCache = malloc(sizeof(entradaCache) * entradasCache);
	limpiarCache();
	colaEntradasCache = list_create();
}

void inicializarMemoriaPrincipal(int valorTamFrame, int valorCantFrames, int valorEntradasCache, int valorCacheXProceso){
	tamFrame = valorTamFrame;
	cantFrames = valorCantFrames;
	int memoriaTotal = tamFrame * cantFrames;
	printf("Memoria Total: %i \n", memoriaTotal);
	memoriaPrincipal = reservarMemoria(memoriaTotal);

	int cantBytesEstructuraAdm = sizeof(tablaPagInv) * cantFrames;
	estructuraAdm = memoriaPrincipal;
	cantFramesEstructuraAdm = divisionRoundUp(cantBytesEstructuraAdm, tamFrame);//Coincide con 1er frame para procesos
	printf("bytesEstructuraAdm: %i \n", cantBytesEstructuraAdm);
	printf("cantFramesEstructuraAdm: %i \n", cantFramesEstructuraAdm);

	cantPagsPorPID = malloc(sizeof(int) * cantFrames);//REVISAR TAMAÑO (en realidad alcanza con una entrada por PID)

	inicializarCantPagsPorPID();//Pone en cero la cant de páginas de cada proceso
	inicializarTablaPags(cantFramesEstructuraAdm);

	entradasCache = valorEntradasCache;
	cacheXProceso = valorCacheXProceso;
	inicializarCache();
}

void liberarMemoriaPrincipal(){
	free(memoriaPrincipal);
	free(cantPagsPorPID);
	free(memoriaCache);
}

int cantFramesOcupados(){
	int i, cantFramesOcupados = 0;
	for (i = 0; i < cantFrames; i++) {
		cantFramesOcupados += cantPagsPorPID[i];
	}
	return cantFramesOcupados;
}

void hexDump8Bytes(void *mem, unsigned int len) {//Sacada de una página
	unsigned int i, j;
	int HEXDUMP_COLS = 8;
	for (i = 0; i<len+ ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++) {
		/* print offset */
		if (i % HEXDUMP_COLS == 0) {
			printf("0x%06x: ", i);
		}
		/* print hex data */
		if (i < len) {
			printf("%02x ", 0xFF & ((char*) mem)[i]);
		} else /* end of block, just aligning for ASCII dump */
		{
			printf("   ");
		}
		/* print ASCII dump */
		if (i % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
			for (j = i - (HEXDUMP_COLS - 1); j <= i; j++) {
				if (j >= len) /* end of block, not really printing */
				{
					putchar(' ');
				} else if (isprint(((char*) mem)[j])) /* printable char */
				{
					putchar(0xFF & ((char*) mem)[j]);
				} else /* other char */
				{
					putchar('.');
				}
			}
			putchar('\n');
		}
	}
}

void hexDump16Bytes(void *addr, int len)//Si no se usa, borrar
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;
    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).
        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("  %s\n", buff);

            // Output the offset.
            printf("  %04x ", i);
        }
        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);
        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buff[i % 16] = '.';
        } else {
            buff[i % 16] = pc[i];
        }
        buff[(i % 16) + 1] = '\0';
    }
    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }
    // And print the final ASCII bit.
    printf("  %s\n", buff);
}

void atenderComandos() {
	char* lineaIngresada;
	char* subcomando;
	int retardoMemoria = 0;
	lineaIngresada = reservarMemoria(100);
	subcomando = reservarMemoria(100);
	printf("Comandos: r(etardo), d(ump), (f)lush, (s)ize\n");
	//Hay que usar un mutex!!
	while (1) {
		char comando;
		fgets(lineaIngresada, 100, stdin);
		comando = lineaIngresada[0];
		switch (comando) {
		case 'r':
			printf("Especifique valor del Retardo (en milisegundos): ");
			fgets(subcomando, 100, stdin);
			subcomando = strtok(subcomando, "\n");
			if (isInt(subcomando)){
				retardoMemoria = atoi(subcomando);
				//config->retardoMemoria = retardoMemoria;               CÓMO MODIFICO EL VALOR, SI NO ESTÁ EN ESTE .C??
				printf("Retardo Memoria seteado en %d",retardoMemoria);
			} else {
				printf("El retardo tiene que ser un número entero positivo\n");
			}
			break;
		case 'd':
			//Falta dump cache y dump estructuras de memoria
			printf("Dump de memoria:\n");
			hexDump8Bytes(estructuraAdm, cantFrames * tamFrame);
			break;
		case 'f':
			limpiarCache();
			printf("El contenido de la caché ha sido vaciado\n");
			break;
		case 's':
			printf("Size:\n"
					"-memory: Indica el tamaño de la memoria en cantidad de frames, la cantidad de frames ocupados"
					" y la cantidad de frames libres\n"
					"-PID: indica el tamaño total del proceso con dicho PID\n");
			fgets(subcomando, 100, stdin);
			subcomando = strtok(subcomando, "\n");
			if (strcmp("memory", subcomando) == 0){
				int cantFramesOcup = cantFramesOcupados();
				printf("Cantidad de frames: %d\n", cantFrames);
				printf("Cantidad de frames ocupados: %d\n", cantFramesOcup);
				printf("Cantidad de frames libres: %d\n", cantFrames - cantFramesOcup);
			} else {
				int PID;
				if (isInt(subcomando)){
					PID = atoi(subcomando);//Si convierte algo que no es un int puede producir comportamiento inesperado
				} else {
					printf("El PID tiene que ser un número entero positivo\n");
				}
				if (0<PID && PID<cantFrames){//LÍMITE SUPERIOR COMPLETAMENTE ARBITRARIO
					printf("El proceso ocupa %d frames\n", cantPagsPorPID[PID]);
				}
				else{
					printf("'%s' no es un PID valido\n", subcomando);
				}
			}
			break;
		default:
			printf("'%c' no es un comando valido\n", comando);
			break;
		}
	}
	free(lineaIngresada);
	free(subcomando);
}

int hash(int PID, int nroPag) {
	//HASH INICIAL:
	//indiceEnLaTabla = PID;
	//indiceEnLaTabla += nroPag;
	//indiceEnLaTabla %= cantFrames;
	int indiceEnLaTabla = 0.5*(PID+nroPag)*(PID+nroPag+1)+nroPag;//"Cantor pairing function" es una buena función de hash?
	indiceEnLaTabla %= cantFrames;
	return max(indiceEnLaTabla,cantFramesEstructuraAdm);
	//La funcion de hash no deberia devolver un frame ocupado por la tabla de páginas
}

int buscarPagina(int PID, int nroPag) {
	int frameBuscado = -1;
	int frameBase = hash(PID, nroPag);
	int frameActual = frameBase;
	while (frameBuscado == -1) {
		if (estructuraAdm[frameActual].PID == PID
				&& estructuraAdm[frameActual].numPag == nroPag)
			frameBuscado = frameActual;
		frameActual++;
		if (frameActual == cantFrames) {//Si llega al final y no encuentra la página solicitada,
			frameActual = cantFramesEstructuraAdm;//comienza a buscar desde que terminan los frames de la estructura administrativa
			//Capaz sería más fácil (aunque menos eficiente) que vuelva a la pos 0
		}
		if (frameActual == frameBase) {
			printf("No se encontró la página\n");
			return -1;
		}
	}
	return frameBuscado * tamFrame;	//Devuelve el byte donde comienza el frame (para direccionarlo usando un puntero a char)
}

int proximaPagina(int PID, int nroPag) {
	int proxPag = -1;
	if (nroPag < (cantPagsPorPID[PID] - 1)/*empieza por la cero*/)
		proxPag = nroPag + 1;
	return proxPag;
}

int proximoFrameLibre(int frameBase) {
	int frameActual = frameBase;
	while (1) {
		if (estructuraAdm[frameActual].PID == -1)
			return frameActual;
		frameActual++;
		if (frameActual == cantFrames) {//Si llega al final y no encuentra la página solicitada,
			frameActual = cantFramesEstructuraAdm; //Comienza a buscar desde que terminan los frames de la estructura administrativa
			//Capaz sería más fácil (aunque menos eficiente) que vuelva a la pos 0
		}
		if (frameActual == frameBase) {
			printf("No se encontró ninguna página libre\n");
			return -1;
		}
	}
}

void asignarPaginasAProceso(int PID, int pagsRequeridas) {
	int nroPag = 0;
	int frameAAsignar;
	while (nroPag < pagsRequeridas) {
		//frameAAsignar = proximoFrameLibre(buscarPagina(PID, nroPag)/ tamFrame); /*buscarPagina devuelve la pos en bytes*/
		//buscarPagina puede devolver -1!! (proximoFrameLibre no debería recibir la funcion de hash directamente??)
		frameAAsignar = proximoFrameLibre(hash(PID,nroPag));
		if (frameAAsignar == -1) {	//No se encontró ninguna página libre
			printf("Sólo se pudieron asignar %d páginas al proceso %d\n", nroPag, PID);
			exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		estructuraAdm[frameAAsignar].PID = PID;
		estructuraAdm[frameAAsignar].numPag = nroPag;
		nroPag++;
		cantPagsPorPID[PID]++;
	}
	printf("Se pudieron asignar las páginas al proceso %d\n",PID);
}

void inicializarPrograma(int PID, int cantPags) {
	/*Hay que asignar páginas necesarias para el segmento de código y para el de datos. Inmediatamente despues de
	 las paginas asignadas al codigo, le vas a asignar las paginas al stack de tal manera que esten contiguas (no necesariamente).
	 1) gracias al PCB sabes cuantas paginas va a ocupar el codigo de tu programa, en el campo 'Paginas de codigo'
	 2) las paginas del stack estan seteados por archivo de configuracion cuanto pueden pesar (el campo STACK_SIZE)
	 Chequear al momento de escribir en una pagina nueva que la pagina nueva no sea de un PID distinto o que no este asignada
	 a nadie. Nunca ningun proceso le manda a escribir a la memoria mas de lo que tiene una pagina*/

	if (cantPagsPorPID[PID]) {//Chequea que el programa no esté ya inicializado (es decir, que la cant de pags sea distinata de cero)
		printf("El programa ya se encuentra inicializado\n");
		exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	asignarPaginasAProceso(PID, cantPags);//Si asignarPaginasAProceso me manda un error, lo tendría que tratar (por ahora hay un exit)
}

int cantEntradasDeProcesoEnCache(int PID){
	int i, cantEntradas = 0;
	for (i = 0; (cantEntradas<3) && (i<entradasCache); i++) {
		if(memoriaCache[i].PID == PID)
		{
			cantEntradas++;
		}
	}
	return cantEntradas;
}

void int_destroy(int *puntero) {//Necesario para eliminar entradas de la lista
    free(puntero);
}

void setearEntradaCache(int* entrada, int PID, int nroPag, int contenido){
	memoriaCache[*entrada].PID = PID;
	memoriaCache[*entrada].numPag = nroPag;
	memoriaCache[*entrada].contenido = contenido;
	//Tengo que reservar memoria dinámica, sino la entrada se pierde al finalizar la función
	int* entradaALista = reservarMemoria(sizeof(int));
	*entradaALista=*entrada;
	list_add(colaEntradasCache, entradaALista);
}

void ingresarEntradaEnCache(int PID, int nroPag){
	int entradaAReemplazar = -1;
	//Si ya se encuentra en cache, se actualiza la misma entrada
	int j;
	for (j = 0; (entradaAReemplazar==-1) && (j<entradasCache); j++) {
		if(memoriaCache[j].PID == PID && memoriaCache[j].numPag == nroPag) entradaAReemplazar = j;
	}
	if (entradaAReemplazar != -1){
		bool esEntradaDeProcesoYPagina(int* entrada){
			return memoriaCache[*entrada].PID == PID && memoriaCache[*entrada].numPag == nroPag;
		}
		list_remove_and_destroy_by_condition(colaEntradasCache, (void*)esEntradaDeProcesoYPagina, (void*)int_destroy);
	}
	else{
		//Si se alcanzó el máximo, hay que sustituir una pág de ese proceso
		if(cantEntradasDeProcesoEnCache(PID) >= cacheXProceso){
			bool esEntradaDeProceso(int *entrada) {
				return memoriaCache[*entrada].PID == PID;
			}
			entradaAReemplazar = *(int*)list_find(colaEntradasCache, (void*)esEntradaDeProceso);//Obtengo entrada más "antigua" del proceso
			//Elimino la entradaAReemplazar de la lista
			list_remove_and_destroy_by_condition(colaEntradasCache, (void*)esEntradaDeProceso, (void*)int_destroy);
		} else {//Si no se alcanzó el limite, la sustitución es global.
			//Si hay entradas libres, lo asigno ahí
			int i;
			for (i = 0; (entradaAReemplazar==-1) && (i<entradasCache); i++) {
				if(memoriaCache[i].PID == -1) entradaAReemplazar = i;
			}
			//Sino, se corre el algoritmo de reemplazo LRU
			if(entradaAReemplazar==-1)
			{
				entradaAReemplazar = *(int*)list_get(colaEntradasCache, 0);//Obtengo entrada más "antigua"
				list_remove_and_destroy_element(colaEntradasCache, 0,(void*)int_destroy);//Y la elimino de la lista
			}
		}
	}
	setearEntradaCache(&entradaAReemplazar, PID, nroPag, buscarPagina(PID, nroPag));
}

char *leerPagina(int PID, int nroPag, int offset, int tamanio) {
	//Si la página se encuentra en caché, no hace falta acceder a memoria (se omite el retardo)
	int k, estaEnCache=-1;
	for (k = 0; (estaEnCache==-1) && (k<entradasCache); k++) {
			if(memoriaCache[k].PID == PID && memoriaCache[k].numPag == nroPag) estaEnCache = 1;
	}
	//En cualquier caso actualizo la caché, ya sea para agregar la entrada(si no está), o para actualizar el último acceso
	ingresarEntradaEnCache(PID, nroPag);
	if (estaEnCache!=1){
		//sleep(retardoMemoria)    PENDIENTE!!!!
	}
	int posByteComienzoPag;
	int tamanioRestante = 0;
	int proxPag;
	if (offset + tamanio > tamFrame) {
		proxPag = proximaPagina(PID, nroPag);
		if (proxPag == -1) {
			printf("Se está intentando leer más allá del límite de memoria asignada\n");
			exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		tamanioRestante = tamanio - (tamFrame - offset);//Si hay otra pág, me guardo el tamaño restante
	}
	if ((posByteComienzoPag = buscarPagina(PID, nroPag)) == -1) {//No se encontró la pagina
		exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	posByteComienzoPag += offset;
	int i;
	char *bytesLeidos = reservarMemoria(tamanio);
	for (i = 0;
			i < tamanio - tamanioRestante/*en caso de que haya que leer más de otra pág*/;
			i++) {
		bytesLeidos[i] = memoriaPrincipal[posByteComienzoPag];
		posByteComienzoPag++;
	}
	if (tamanioRestante) { //Hay que leer el resto de otra página
		char *bytesRestantesLeidos = leerPagina(PID, proxPag, 0,
				tamanioRestante); //Lee lo que esta en la página que sigue
		//Si leerPagina me manda un error, lo tendría que tratar (por ahora hay un exit)
		int j = 0;
		while (i < tamanio) {
			bytesLeidos[i] = bytesRestantesLeidos[j];
			i++;
			j++;
		}
		free(bytesRestantesLeidos);
	}
	return bytesLeidos;
}

void escribirPagina(int PID, int nroPag, int offset, int tamanio, char *buffer) {
	int posByteComienzoPag;
	if (offset + tamanio > tamFrame) {
		int proxPag = proximaPagina(PID, nroPag);
		if (proxPag == -1) {
			printf("Se está intentando escribir más allá del límite de la página\n");
			exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA ESCRITURA
		}
		int tamanioRestante = tamanio - (tamFrame - offset);
		escribirPagina(PID, proxPag, 0, tamanioRestante,
				&buffer[tamanio - tamanioRestante]);
		//Le paso la posición del buffer desde la que tiene que seguir escribiendo
		//Si escribirPagina me manda un error, lo tendría que tratar (por ahora hay un exit)
	}
	if ((posByteComienzoPag = buscarPagina(PID, nroPag)) == -1) {
		exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	posByteComienzoPag += offset;
	int i;
	for (i = 0; i < tamanio; i++) {
		memoriaPrincipal[posByteComienzoPag] = buffer[i];
		posByteComienzoPag++;
	}
	//Se agrega (o actualiza) la entrada en memoria cache
	ingresarEntradaEnCache(PID,nroPag);
}

void finalizarPrograma(int PID) {
	int nroPag = cantPagsPorPID[PID] - 1;
	int paginaABorrar;
	while (nroPag >= 0) {
		if ((paginaABorrar = buscarPagina(PID, nroPag)) == -1) {//No se encontró la pagina
			exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		paginaABorrar = paginaABorrar / tamFrame; /*buscarPagina devuelve la pos en bytes*/
		estructuraAdm[paginaABorrar].PID = -1;
		cantPagsPorPID[PID]--; //Conviene ir decrementándolo 1 a 1 por si falla algo
		//Borrar (físicamente?) la entrada de la estructura auxiliar que almacena, para cada PID, la cant de páginas que tiene asignadas
		nroPag--;
	}
	//Elimino las páginas del proceso en caché
	int i;
	for (i = 0; i < entradasCache; i++) {
		if(memoriaCache[i].PID == PID) memoriaCache[i].PID = -1;
	}
	printf("Se ha finalizado el proceso %d\n", PID);
}

void ejecutarOperaciones() { // ES DE PRUEBA. BORRAR DESPUES
	/*Antes de ejecutar cada una hay que esperar una cantidad de tiempo configurable (en milisegundos), simulando el tiempo de
	 acceso a memoria*/
	inicializarPrograma(1, 3);
	inicializarPrograma(2, 5);
	dumpTablaDePags();
	int bytesAEscribir[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	escribirPagina(2, 0, 0, 40, bytesAEscribir);
	int *bytesLeidos = leerPagina(2, 0, 0, 40);
	printf("%d", bytesLeidos[8]);
	free(bytesLeidos);
}
