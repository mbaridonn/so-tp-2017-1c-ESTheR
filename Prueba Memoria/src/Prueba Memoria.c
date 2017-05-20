#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

int tamFrame = 0;
int cantFrames = 0;

typedef struct {
	int PID;
	int numPag;
} tablaPagInv;

int cantFramesEstructuraAdm;
char* memoriaPrincipal;
tablaPagInv* estructuraAdm;	//Tiene que apuntar SIEMPRE al inicio de la tabla
//En vez de tener varios punteros a MP se podría tener uno solo, e irlo casteando

int* cantPagsPorPID; //Esta tabla NO está en MP. Se accede a través del PID de un proceso


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
		printf("Esta division funciona unicamente con enteros positivos");
		exit(-1); //En realidad tendría más sentido que se recupere
	}
	return 1 + ((dividendo - 1) / divisor);
}

void inicializarTablaPags(int cantFramesEstructuraAdm) {
	int i = 0;
	for (i = 0; i < cantFrames; i++) {
		if (i < cantFramesEstructuraAdm) {
			estructuraAdm[i].PID = 0; //Supongo que PID 0 significa frame de estructura administrativa
			estructuraAdm[i].numPag = i; //ESTO ESTA MAS QUE HARDCODEADO PARA PROBAR. BORRAR POR FAVOR
		} else {
			estructuraAdm[i].PID = -1; //Supongo que PID -1 significa frame vacío
		}
	}
	cantPagsPorPID[0]=cantFramesEstructuraAdm;
	for (i = 0; i < cantFrames; i++) { // Prueba de que está bien inicializado (después borrar)
		printf("%d,", estructuraAdm[i].PID);
	}
}

void atenderComandos() {
	int retardo = 0;
	printf("Comandos: r(etardo), d(ump), (f)lush, (s)ize");
	while (1) {
		char comando;
		scanf(" %c", &comando); //Importante el espacio antes del %c (el scanf es una poronga)
		//Guarda, si se escriben varios caracteres, cada ciclo del while va leyendo uno!!
		switch (comando) {
		case 'r':
			printf("Especifique valor del Retardo (en milisegundos)");
			//fgets(); Hace falta validar TODO lo que podría salir mal!
			//config->retardoMemoria = (habría que pedir el nuevo valor)
			//Debería usar un mutex?
			break;
		case 'd':
			printf("Dump de memoria:\n");
			//Que tengo que mostrar?
			//Hay que usar un mutex
			break;
		case 'f':
			printf("Flush\n");
			//Este comando deberá limpiar completamente el contenido de la Caché
			break;
		case 's':
			printf("Size\n");
			//Memory: Indicará el tamaño de la memoria en cantidad de frames, la cantidad de frames ocupados y la cantidad de frames libres
			//PID: indicará el tamaño total de un proceso
			break;
		default:
			printf("'%c' no es un comando valido", comando);
			break;
		}
	}
}

int hash(int PID, int nroPag) {
	int indiceEnLaTabla = 0;
	//Pendiente
	indiceEnLaTabla += nroPag;
	indiceEnLaTabla %= cantFrames;
	return indiceEnLaTabla;
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
		}
		if (frameActual == frameBase) {
			printf("No se encontró la página");
			return -1;
		}
	}
	return frameBuscado * tamFrame;	//Devuelve el byte donde comienza el frame (para direccionarlo usando un puntero a char)
}

int proximaPagina(int PID, int nroPag){
	int proxPag = -1;
	if (nroPag < (cantPagsPorPID[PID] - 1)/*empieza por la cero*/) proxPag=nroPag+1;
	return proxPag;
}

int proximoFrameLibre(int frameBase){
	int frameActual = frameBase;
	while (1){
		if (estructuraAdm[frameActual].PID == -1) return frameActual;
		frameActual++;
		if (frameActual == cantFrames) {//Si llega al final y no encuentra la página solicitada,
			frameActual = cantFramesEstructuraAdm;//comienza a buscar desde que terminan los frames de la estructura administrativa
		}
		if (frameActual == frameBase) {
			printf("No se encontró ninguna página libre");
			return -1;
		}
	}
}

void asignarPaginasAProceso(int PID, int pagsRequeridas){
	int nroPag = 0;
	int frameAAsignar;
	while(nroPag<pagsRequeridas-1){
		frameAAsignar = proximoFrameLibre(buscarPagina(PID, nroPag)/tamFrame/*buscarPagina devuelve la pos en bytes*/);
		if (frameAAsignar == -1){//No se encontró ninguna página libre
			printf("Sólo se pudieron asignar %d páginas",nroPag);
			exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		estructuraAdm[frameAAsignar].PID = PID;
		estructuraAdm[frameAAsignar].numPag = nroPag;
		nroPag++;
		cantPagsPorPID[PID]++;
	}
}

void inicializarPrograma(int PID, int cantPags) {
	/*Hay que asignar páginas necesarias para el segmento de código y para el de datos. Inmediatamente despues de
	 las paginas asignadas al codigo, le vas a asignar las paginas al stack de tal manera que esten contiguas (no necesariamente).
	 1) gracias al PCB sabes cuantas paginas va a ocupar el codigo de tu programa, en el campo 'Paginas de codigo'
	 2) las paginas del stack estan seteados por archivo de configuracion cuanto pueden pesar (el campo STACK_SIZE)
	 Chequear al momento de escribir en una pagina nueva que la pagina nueva no sea de un PID distinto o que no este asignada
	 a nadie. Nunca ningun proceso le manda a escribir a la memoria mas de lo que tiene una pagina*/

	if (!cantPagsPorPID[PID]){//Chequea que el programa no esté ya inicializado
		printf("El programa ya se encuentra inicializado");
		exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	cantPagsPorPID[PID]=0;//Tengo que inicializarlo xq asignarPaginasAProceso le suma la cantPags
	asignarPaginasAProceso(PID,cantPags);//Si asignarPaginasAProceso me manda un error, lo tendría que tratar (por ahora hay un exit)
}

char *leerPagina(int PID, int nroPag, int offset, int tamanio) {
	int posByteComienzoPag;
	int tamanioRestante = 0;
	int proxPag;
	if (offset + tamanio > tamFrame) {
		proxPag = proximaPagina(PID, nroPag);
		if(proxPag == -1){
			printf("Se está intentando leer más allá del límite de memoria asignada");
			exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		tamanioRestante = tamanio-(tamFrame-offset);//Si hay otra pág, me guardo el tamaño restante
	}
	if ((posByteComienzoPag = buscarPagina(PID, nroPag)) == -1) {//No se encontró la pagina
		exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	posByteComienzoPag += offset;
	int i;
	char *bytesLeidos = reservarMemoria(tamanio);
	for (i = 0; i < tamanio-tamanioRestante/*en caso de que haya que leer más de otra pág*/; i++) {
		bytesLeidos[i] = memoriaPrincipal[posByteComienzoPag];
		posByteComienzoPag++;
	}
	if (tamanioRestante){ //Hay que leer el resto de otra página
		char *bytesRestantesLeidos = leerPagina(PID,proxPag,0,tamanioRestante);//Lee lo que esta en la página que sigue
		//Si leerPagina me manda un error, lo tendría que tratar (por ahora hay un exit)
		int j=0;
		while(i < tamanio){
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
		if (proxPag == -1){
			printf("Se está intentando escribir más allá del límite de la página");
			exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA ESCRITURA
		}
		int tamanioRestante = tamanio-(tamFrame-offset);
		escribirPagina(PID, proxPag,0,tamanioRestante,&buffer[tamanio-tamanioRestante]);
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
	//Qué pasa si al escribir estoy pisando datos anteriores? Hay que chequear esto??
}

void finalizarPrograma(int PID) {

	//Repetir hasta que cantidad de entradas eliminadas sea igual a la cant de pags que el proceso tenia asignadas
	//Borrar (físicamente?) la entrada de la estructura auxiliar que almacena, para cada PID, la cant de páginas que tiene asignadas
	//Algo más/falta algo??
	int nroPag = cantPagsPorPID[PID]-1;
	int paginaABorrar;
	while (nroPag>=0){
		if ((paginaABorrar = buscarPagina(PID, nroPag)) == -1) {//No se encontró la pagina
			exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		estructuraAdm[paginaABorrar].PID = -1;
		cantPagsPorPID[PID]--;//Conviene ir decrementándolo 1 a 1 por si falla algo
		nroPag--;
	}
}

void ejecutarOperaciones() { // ES DE PRUEBA. BORRAR DESPUES
	/*Antes de ejecutar cada una hay que esperar una cantidad de tiempo configurable (en milisegundos), simulando el tiempo de
	 acceso a memoria*/
	int bytesAEscribir[10] = {0,1,2,3,4,5,6,7,8,9};
	escribirPagina(0,0,0,40,bytesAEscribir);
	int *bytesLeidos = leerPagina(0,0,0,40);
	printf("%d",bytesLeidos[8]);
	free(bytesLeidos);
}

int main() {

	puts("Ingrese tamFrame: ");
	scanf("%i", &tamFrame);
	puts("Ingrese cantFrames: ");
	scanf("%i", &cantFrames);
	int memoriaTotal = tamFrame * cantFrames;
	printf("Memoria Total: %i \n", memoriaTotal);
	memoriaPrincipal = reservarMemoria(memoriaTotal);

	int cantBytesEstructuraAdm = sizeof(tablaPagInv) * cantFrames;
	estructuraAdm = malloc(cantBytesEstructuraAdm);	//La tabla de páginas no está EN MEMORIA. La tengo aparte pero
	//no accedo a los bytes que ocuparía en la MP (así simulo que está ahí)
	cantFramesEstructuraAdm = divisionRoundUp(cantBytesEstructuraAdm, tamFrame);//Coincide con 1er frame para procesos
	printf("bytesEstructuraAdm: %i \n", cantBytesEstructuraAdm);
	printf("cantFramesEstructuraAdm: %i \n", cantFramesEstructuraAdm);

	cantPagsPorPID = malloc(sizeof(int)*cantFrames);//REVISAR TAMAÑO (en realidad alcanza con una entrada por PID)
	/*Se puede tener una tener una estructura adicional como ésta (que no este guardada en "memoria principal") que
	almacene, para cada PID, la cant de páginas que tiene asignadas??*/

	inicializarTablaPags(cantFramesEstructuraAdm);

	printf("Creado hilo para comandos\n");
	pthread_t hilo_comandos;
	if (pthread_create(&hilo_comandos, NULL, atenderComandos, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		printf("Error al crear el thread de comandos.\n");
		exit(-1);
	}

	printf("Creado hilo para operaciones\n"); //Hilo de prueba, eliminar después!!
	pthread_t hilo_operaciones;
	if (pthread_create(&hilo_operaciones, NULL, ejecutarOperaciones, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		printf("Error al crear el thread de operaciones.\n");
		exit(-1);
	}

	pthread_join(hilo_operaciones, NULL);
	pthread_join(hilo_comandos, NULL);

	free(memoriaPrincipal);
	free(estructuraAdm);
	free(cantPagsPorPID);

	return EXIT_SUCCESS;
}
