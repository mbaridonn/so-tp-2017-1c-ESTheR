#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

typedef struct {
	int numFrame;//Hace falta? En la parte de función de hash dice que:
	//Por una cuestión de simplicidad, asociamos el número de frame con el índice en el que se encuentra la entrada en la página.
	int PID;
	int numPag;
} tablaPagInv;

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

void inicializarPrograma(int PID, int cantPags){
	/*Hay que asignar páginas necesarias para el segmento de código y para el de datos. Inmediatamente despues de
	las paginas asignadas al codigo, le vas a asignar las paginas al stack de tal manera que esten contiguas.
	1) gracias al PCB sabes cuantas paginas va a ocupar el codigo de tu programa, en el campo 'Paginas de codigo'
	2) las paginas del stack estan seteados por archivo de configuracion cuanto pueden pesar (el campo STACK_SIZE)
	Chequear al momento de escribir en una pagina nueva que la pagina nueva no sea de un PID distinto o que no este asignada
	a nadie. Nunca ningun proceso le manda a escribir a la memoria mas de lo que tiene una pagina
	*/
}

void leerPagina(int PID, int nroPag, int offset, int tamanio){
	//if (offset+tamanio>tamPag) ERROR
	//Buscar frame en tabla de pags con PID y nroPag (si no se encuentra: ERROR)
	//Multiplicar el frame por tamFrame para obtener el byte donde empieza el frame
	//Sumarle el offset y empezar a leer hasta limite
}

void escribirPagina(int PID, int nroPag, int offset, int tamanio, int buffer){
	//if(offset+tamanio>tamPag) ERROR
	//Buscar frame en tabla de pags con PID y nroPag (si no se encuentra: ERROR)
	//Multiplicar el frame por tamFrame para obtener el byte donde empieza el frame
	//Sumarle el offset y empezar a escribir hasta limite
	//Qué pasa si al escribir estoy pisando datos anteriores? Hay que chequear esto??
}

void asignarPaginasAProceso(int PID, int pagsRequeridas){
	/*Se podría tener una tener una estructura adicional (que no este guardada en "memoria principal") que
	almacene, para cada PID, la cant de páginas que tiene asignadas*/
	//Ejecutar función de hash con PID y nroPag?
	//Verificar que en la entrada de la tabla de paginas ese frame no este asignado	a nadie. Sino avanzar hasta que esto se cumpla
	//Actualizar esa entrada en la tabla de paginas (y aumentar tambien el contador de pags en la estructura adicional)
	//Seguir asi hasta asignar todas las pags requeridas
	//En caso de que no se pueda asignar más páginas se deberá informar de la imposibilidad de asignar por falta de espacio
	//Algo más/falta algo??
}

void finalizarPrograma (int PID){
	//Buscar entrada con ese pid
	//Eliminar lógicamente la entrada en tabla de pags (poniendoles PID -1 ??)
	//Repetir hasta que cantidad de entradas eliminadas sea igual a la cant de pags que el proceso tenia asignadas
	//Borrar (físicamente?) la entrada de la estructura auxiliar que almacena, para cada PID, la cant de páginas que tiene asignadas
	//Algo más/falta algo??
}

int hash(int PID, int nroPag){
	int indiceEnLaTabla = 0;//Pendiente
	return indiceEnLaTabla;
}

int main() {

	int tamFrame = 0;
	int cantFrames = 0;
	puts("Ingrese tamFrame: ");
	scanf("%i", &tamFrame);
	puts("Ingrese cantFrames: ");
	scanf("%i", &cantFrames);
	int memoriaTotal = tamFrame * cantFrames;
	printf("Memoria Total: %i \n", memoriaTotal);
	char* memoria = reservarMemoria(memoriaTotal);
	char* estructuraAdm = reservarMemoria(sizeof(tablaPagInv) * cantFrames);
	int cantFramesEstructuraAdm = divisionRoundUp(sizeof(tablaPagInv) * cantFrames, tamFrame);//Coincide con 1er frame para procesos
	printf("bytesEstructuraAdm: %i \n", sizeof(tablaPagInv) * cantFrames);
	printf("cantFramesEstructuraAdm: %i \n", cantFramesEstructuraAdm);

	printf("Creado hilo para comandos\n");
	pthread_t hilo_comandos;
	if (pthread_create(&hilo_comandos, NULL, atenderComandos, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		printf("Error al crear el thread.\n");
		exit(-1);
	}
	pthread_join(hilo_comandos, NULL);

	return EXIT_SUCCESS;
}
