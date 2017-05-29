#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include "lib/funcionesMemoria.h"

int main() {
	int tamFrameLocal, cantFramesLocal;
	puts("Ingrese tamFrame: ");
	scanf("%i", &tamFrameLocal);
	puts("Ingrese cantFrames: ");
	scanf("%i", &cantFramesLocal);

	inicializarMemoriaPrincipal(tamFrameLocal, cantFramesLocal);

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

	liberarMemoriaPrincipal();

	return EXIT_SUCCESS;
}
