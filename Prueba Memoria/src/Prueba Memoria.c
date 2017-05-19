#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

typedef struct {
	int numFrame; //Hace falta? En la parte de función de hash dice que:
	//Por una cuestión de simplicidad, asociamos el número de frame con el índice en el que se encuentra la entrada en la página.
	int PID;
	int numPag;
} tablaPagInv;

int cantFramesEstructuraAdm;
int tamFrame = 0;
int cantFrames = 0;
char* memoriaPrincipal;
tablaPagInv* estructuraAdm;	//Tiene que apuntar SIEMPRE al inicio de la tabla

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

void inicializarTablaPags(int cantFramesEstructuraAdm) { //Está bien? No está haciendo cagadas con puntero?
	int i = 0;
	for (i = 0; i < cantFrames; i++) {
		if (i < cantFramesEstructuraAdm) {
			estructuraAdm[i].PID = 0; //Supongo que PID 0 significa frame de estructura administrativa
			estructuraAdm[i].numPag = i; //ESTO ESTA MAS QUE HARDCODEADO PARA PROBAR. BORRAR PORFAVOR
		} else {
			estructuraAdm[i].PID = -1; //Supongo que PID -1 significa frame vacío
		}
	}
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
			//fgets(); Hace falta validar TODO lo que podría salir mal?
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
		if (frameActual == cantFrames) {/*Si llega al final y no encuentra la página solicitada, la busca desde el principio.*/
			frameActual = cantFramesEstructuraAdm;/*Comienza a buscar desde que terminan los frames de la estructura administrativa.*/
		}
		if (frameActual == frameBase) {
			printf("No se encontró la página");
			return -1;
		}
	}
	return frameBuscado * tamFrame;	//Debería devolver el frame o los bytes (frameBuscado*tamFrame)??
}

void inicializarPrograma(int PID, int cantPags) {
	/*Hay que asignar páginas necesarias para el segmento de código y para el de datos. Inmediatamente despues de
	 las paginas asignadas al codigo, le vas a asignar las paginas al stack de tal manera que esten contiguas.
	 1) gracias al PCB sabes cuantas paginas va a ocupar el codigo de tu programa, en el campo 'Paginas de codigo'
	 2) las paginas del stack estan seteados por archivo de configuracion cuanto pueden pesar (el campo STACK_SIZE)
	 Chequear al momento de escribir en una pagina nueva que la pagina nueva no sea de un PID distinto o que no este asignada
	 a nadie. Nunca ningun proceso le manda a escribir a la memoria mas de lo que tiene una pagina
	 */
}

char *leerPagina(int PID, int nroPag, int offset, int tamanio) { //El tipo puede variar
	int posByteComienzoPag;
	if (offset + tamanio > tamFrame) {
		printf("Se está intentando leer más allá del límite de la página");
		exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	if ((posByteComienzoPag = buscarPagina(PID, nroPag)) == -1) {
		exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
	}
	posByteComienzoPag += offset;
	int i;
	char *bytesLeidos = reservarMemoria(tamanio); //El tipo puede variar
	for (i = 0; i < tamanio; i++) {
		bytesLeidos[i] = memoriaPrincipal[posByteComienzoPag];
		posByteComienzoPag++;
	}
	return bytesLeidos;
}

void escribirPagina(int PID, int nroPag, int offset, int tamanio, char *buffer) { //El tipo de buffer puede variar, no necesariamente int.
	int posByteComienzoPag;
	if (offset + tamanio > tamFrame) {
		printf("Se está intentando escribir más allá del límite de la página");
		exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA ESCRITURA
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

void asignarPaginasAProceso(int PID, int pagsRequeridas) {
	/*Se podría tener una tener una estructura adicional (que no este guardada en "memoria principal") que
	 almacene, para cada PID, la cant de páginas que tiene asignadas*/
	//Ejecutar función de hash con PID y nroPag?
	//Verificar que en la entrada de la tabla de paginas ese frame no este asignado	a nadie. Sino avanzar hasta que esto se cumpla
	//Actualizar esa entrada en la tabla de paginas (y aumentar tambien el contador de pags en la estructura adicional)
	//Seguir asi hasta asignar todas las pags requeridas
	//En caso de que no se pueda asignar más páginas se deberá informar de la imposibilidad de asignar por falta de espacio
	//Algo más/falta algo??
}

void finalizarPrograma(int PID) {
	//Buscar entrada con ese pid
	//Eliminar lógicamente la entrada en tabla de pags (poniendoles PID -1 ??)
	//Repetir hasta que cantidad de entradas eliminadas sea igual a la cant de pags que el proceso tenia asignadas
	//Borrar (físicamente?) la entrada de la estructura auxiliar que almacena, para cada PID, la cant de páginas que tiene asignadas
	//Algo más/falta algo??
}

void ejecutarOperaciones() { // ES DE PRUEBA. BORRAR DESPUES
	/*Antes de ejecutar cada una hay que esperar una cantidad de tiempo configurable (en milisegundos), simulando el tiempo de
	 acceso a memoria*/
	int bytesAEscribir[5] = {1,2,3,4,5};
	escribirPagina(0,0,0,20,bytesAEscribir);
	int *bytesLeidos = leerPagina(0,0,0,20);
	printf("%d",bytesLeidos[2]);
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

	return EXIT_SUCCESS;
}
