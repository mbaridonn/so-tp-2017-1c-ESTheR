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

char *rutaArchivo;// = "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/src/configMemoria.txt";

t_log *memoria_log;

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
	config->entradasCache = config_get_int_value(archivo_Modelo, "ENTRADAS_CACHE");
	config->cacheXProceso = config_get_int_value(archivo_Modelo, "CACHE_X_PROC");
	//config->reemplazoCache = config_get_int_value(archivo_Modelo, "REEMPLAZO_CACHE"); Guarda, es un string!!
	config->retardoMemoria = config_get_int_value(archivo_Modelo, "RETARDO_MEMORIA");
}

void mostrarArchivoConfig() {
	FILE *f;
	f = fopen(rutaArchivo, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF) putchar(c);
	printf("\n");
	printf("------------------------------------------\n");
}

void leerArchivo() {
	if (access(rutaArchivo, F_OK) == -1) {
		log_error(memoria_log, "No se encontró el Archivo.");
		exit(-1);
	}
	t_config *archivo_config = config_create(rutaArchivo);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
	log_info(memoria_log, "Leí el archivo y extraje el puerto: %d\n", config->puerto);
}

void msjConexionCon(char *s) {
	log_info(memoria_log, "\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
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

	memoria_log = log_create("/home/utnso/git/tp-2017-1c-C-digo-Facilito/Memoria/Debug/Memoria.log", "CódigoFacilito-Memoria\n", true, LOG_LEVEL_TRACE);

	leerArchivo();

	free(rutaArchivo);

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(config->puerto/*8125*/);

	int servidor;
	int cliente;

	pthread_t hilo_comandos, hilo_kernel, hilo_cpu;

	inicializarMemoriaPrincipal(config->tamFrame, config->cantFrames,
			config->entradasCache, config->cacheXProceso, &(config->retardoMemoria), memoria_log);

	log_info(memoria_log, "Creado hilo para comandos");

	if (pthread_create(&hilo_comandos, NULL, atenderComandos, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
		log_error(memoria_log, "Error al crear el thread de comandos.");
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
			log_info(memoria_log, "Creado hilo para Kernel");

			if ((send(cliente, &(config->tamFrame), sizeof(int), 0)) == -1) {
				log_error(memoria_log, "Error enviando tamaño de Frame");
				exit(-1);
			}

			if (pthread_create(&hilo_kernel, NULL, atenderKernel, NULL)) { //Está bien pasarle NULL si no recibe parámetros?
				log_error(memoria_log, "Error al crear el thread de Kernel.");
				exit(-1);
			}
			break;
		case cpu:
			log_info(memoria_log, "Me conecte con CPU!");
			if (pthread_create(&hilo_cpu, NULL, atenderCPU, cliente)) {
				log_error(memoria_log, "Error al crear el thread de CPU.");
				exit(-1);
			}
			break;
		default:
			log_error(memoria_log, "No me puedo conectar con vos.");
			break;
		}
		close(servidor);
	}

	/*pthread_join(hilo_comandos, NULL);
	 pthread_join(hilo_kernel, NULL);*/

	liberarMemoriaPrincipal();

	return 0;
}
