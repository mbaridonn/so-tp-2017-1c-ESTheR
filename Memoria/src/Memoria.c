#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#include <pthread.h>
#include "libreriaSockets.h"
#include "lib/funcionesMemoria.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/src/configMemoria.txt"

typedef struct {
	int puerto;
	int cantFrames;
	int tamFrame;
	int entradasCache;
	int cacheXProceso;
	//int reemplazoCache ES NECESARIO? Es el único algoritmo de reemplazo posible
	int retardoMemoria;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO");
	config->cantFrames = config_get_int_value(archivo_Modelo, "MARCOS");
	config->tamFrame = config_get_int_value(archivo_Modelo, "MARCO_SIZE");
	config->entradasCache = config_get_int_value(archivo_Modelo,
			"ENTRADAS_CACHE");
	config->cacheXProceso = config_get_int_value(archivo_Modelo,
			"CACHE_X_PROC");
	//config->reemplazoCache = config_get_int_value(archivo_Modelo, "REEMPLAZO_CACHE"); Guarda, es un string!!
	config->retardoMemoria = config_get_int_value(archivo_Modelo,
			"RETARDO_MEMORIA");
}

void mostrarArchivoConfig() {
	FILE *f;

	f = fopen(RUTAARCHIVO, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	printf("\n");
	printf("------------------------------------------\n");

}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo\n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
	printf("Leí el archivo y extraje el puerto: %d\n", config->puerto);
}

void msjConexionCon(char *s) {
	printf(
			"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			s);
}

void modificarRetardoMemoria(int nuevoRetardo) {
	config->retardoMemoria = nuevoRetardo;
}

void *recibirArchivoDe(int *cliente) {
	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	if (archivo == NULL) {
		printf("No se pudo escribir el archivo\n");
		return EXIT_FAILURE;
	}

	u_int32_t fsize = 0;
	//recibirMensajeDe(&cliente, buffer); NO CONVENDRÍA USAR ESA ABSTRACCIÓN??
	if (recv((*cliente), &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		return EXIT_FAILURE;
	}

	char *bufferArchivo = reservarMemoria(fsize + 1);
	if (recv((*cliente), bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		return EXIT_FAILURE;
	}
	printf("%s\n\n", bufferArchivo);

	fwrite(bufferArchivo, 1, fsize, archivo);
	free(bufferArchivo);
}

void enviarSenialAKernel(int *cliente) {
	char senial[2] = "a";
	if (send((*cliente), senial, 2, 0) == -1) {
		printf("Error al enviar la senial antes de asignar paginas\n");
	}
}

int recibir_cant_paginas(int *cliente) {
	u_int32_t cant_pags;
	if (recv((*cliente), &cant_pags, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo la cantidad de paginas.\n");
		return EXIT_FAILURE;
	}
	return cant_pags;
}

int recibir_process_id(int *cliente) {
	int process_id;
	if (recv((*cliente), &process_id, sizeof(int), 0) == -1) {
		printf("Error recibiendo el process_id\n");
		return EXIT_FAILURE;
	}
	return process_id;
}

void kernel_mem_asignarPaginas(int *cliente) {
	u_int32_t process_id, cant_pags;
	enviarSenialAKernel(cliente);
	process_id = recibir_process_id(cliente);
	cant_pags = recibir_cant_paginas(cliente);
	printf("Me llego el ID: %d y Cantidad de Paginas: %d\n", process_id,cant_pags);
	asignarPaginasAProceso(process_id,cant_pags);
}

int main(void) {

	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	//direccionServidor.sin_port = htons(config->puerto);
	direccionServidor.sin_port = htons(8125);

	int servidor;
	int cliente;

	inicializarMemoriaPrincipal(config->tamFrame, config->cantFrames,
			config->entradasCache, config->cacheXProceso);

	printf("Creado hilo para comandos\n");
	pthread_t hilo_comandos;
	if (pthread_create(&hilo_comandos, NULL, atenderComandos, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		printf("Error al crear el thread de comandos.\n");
		exit(-1);
	}

	esperarConexion(&servidor, &direccionServidor);
	aceptarConexion(&servidor, &cliente);

	int procesoConectado = handshake(&cliente, memoria);

	switch (procesoConectado) {
	case kernel:
		msjConexionCon("el Kernel");
		printf("Creado hilo para Kernel\n");
		//Lo primero que tiene que hacer la memoria cuando se conecta con el kernel es pasarle el tamaño de página.
		//Esto se hace una sola vez, cuando se conectan al principio. Lo demás se hace cada vez que el kernel crea un nuevo proceso
		if ((send(cliente, &(config->tamFrame), sizeof(int), 0)) == -1) {//Convendria que sea u_int32_t
			printf("Error enviando tamaño de Frame\n");
			exit(-1);
		}

		/*pthread_t hilo_kernel;
		 if (pthread_create(&hilo_kernel, NULL, atenderKernel, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		 printf("Error al crear el thread de Kernel.\n");
		 exit(-1);
		 }*/

		/*El Proceso Kernel deberá solicitarle al Proceso Memoria que le asigne lás páginas necesarias para almacenar
		 el código del programa y el stack. También le enviará el código completo del Programa.
		 Si no se pudiera obtener espacio suficiente para algunas de las estructuras necesarias del proceso,
		 entonces se le debe informar*/

		/*
		 Existe la posibilidad de que los procesos le soliciten al Kernel bloques de memoria dinámica.
		 El Kernel será el encargado de administrar el ciclo de vida de estos bloques de memoria,
		 denominados Heap, de forma tal que permita al proceso reservar y liberar bloques de forma aleatoria.
		 VER FUNCIONAMIENTO DE MEMORIA DINÁMICA EN LA PARTE DE KERNEL
		 */

		/*
		 Cada página dedicada a la gestión de memoria dinámica podrá almacenar uno o más bloques de
		 datos. Para poder determinar la consistencia de los bloques de datos almacenados en una página se
		 guardará metadata específica dentro de la misma, la que nos permitirá determinar cuáles son las
		 secciones de memoria sobre las cuales el proceso puede trabajar. Para ello, utilizaremos la siguiente
		 estructura:
		 typedef struct HeapMetadata {
		 uint32_t size,
		 Bool isFree
		 }
		 */
		recibirArchivoDe(&cliente);
		kernel_mem_asignarPaginas(&cliente);
		break;

	case cpu:
		printf("Me conecte con CPU!\n");
		//Por cada conexión, la Memoria creará un hilo dedicado a atenderlo, que quedará a la espera de solicitudes de operaciones.
		//Hay que atacar los problemas de concurrencia que surjan.

		/*Ante cualquier solicitud de acceso a la memoria principal, deberá esperar una cantidad de tiempo configurable
		 (en milisegundos), simulando el tiempo de acceso a memoria. En caso de que la solicitud sea resuelta por la Memoria Caché,
		 no se deberá esperar.*/

		/*En caso de que un CPU solicite memoria y no se le pueda asignar por falta de espacio, se
		 deberá informar  que no hay más espacio disponible.*/

		break;

	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}
	close(servidor);

	pthread_join(hilo_comandos, NULL);

	liberarMemoriaPrincipal();

	return 0;
}
