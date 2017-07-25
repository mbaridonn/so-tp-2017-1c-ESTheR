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

char *rutaArchivo;// /home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/src/configMemoria.txt

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

	f = fopen(rutaArchivo, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	printf("\n");
	printf("------------------------------------------\n");

}

void leerArchivo() {
	if (access(rutaArchivo, F_OK) == -1) {
		printf("No se encontró el Archivo\n");
		exit(-1);
	}
	t_config *archivo_config = config_create(rutaArchivo);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
	printf("Leí el archivo y extraje el puerto: %d\n", config->puerto);
}

void msjConexionCon(char *s) {
	printf("\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
}

void modificarRetardoMemoria(int nuevoRetardo) {
	config->retardoMemoria = nuevoRetardo;
}

int main(int argc, char* argv[]) {
	if (argc == 1)
	{
		printf("Falta ingresar el path del archivo de configuracion\n");
		return -1;
	}
	if (argc != 2)
	{
		printf("Numero incorrecto de argumentos\n");
		return -1;
	}
	rutaArchivo = strdup(argv[1]);

	leerArchivo();

	free(rutaArchivo);

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(config->puerto/*8125*/);

	int servidor;
	int cliente;

	pthread_t hilo_comandos, hilo_kernel, hilo_cpu;

	inicializarMemoriaPrincipal(config->tamFrame, config->cantFrames,
			config->entradasCache, config->cacheXProceso, &(config->retardoMemoria));

	printf("Creado hilo para comandos\n");

	if (pthread_create(&hilo_comandos, NULL, atenderComandos, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		printf("Error al crear el thread de comandos.\n");
		exit(-1);
	}

	while (1) {
		esperarConexion(&servidor, &direccionServidor);
		aceptarConexion(&servidor, &cliente);

		int procesoConectado = handshake(&cliente, memoria);

		switch (procesoConectado) {
		case kernel:
			msjConexionCon("el Kernel");
			clienteKernel = cliente;
			printf("Creado hilo para Kernel\n");

			if ((send(cliente, &(config->tamFrame), sizeof(int), 0)) == -1) {
				printf("Error enviando tamaño de Frame\n");
				exit(-1);
			}

			if (pthread_create(&hilo_kernel, NULL, atenderKernel, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
				printf("Error al crear el thread de Kernel.\n");
				exit(-1);
			}
			break;
		case cpu:
			printf("Me conecte con CPU!\n");
			if (pthread_create(&hilo_cpu, NULL, atenderCPU, cliente)) {
				printf("Error al crear el thread de CPU.\n");
				exit(-1);
			}
			break;
		default:
			printf("No me puedo conectar con vos.\n");
			break;
		}
		close(servidor);
	}

	/*pthread_join(hilo_comandos, NULL);
	 pthread_join(hilo_kernel, NULL);*/

	liberarMemoriaPrincipal();

	return 0;
}
