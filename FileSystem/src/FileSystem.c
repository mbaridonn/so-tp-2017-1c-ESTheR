#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <commons/bitarray.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/FileSystem/src/configFyleSystem"

typedef struct {
	int puerto;
	char *puntoMontaje;
} t_configuracion;
t_configuracion *config;

typedef struct {
	int tamBloque;
	int cantBloques;
} t_configuracion_filesystem;
t_configuracion_filesystem *configFS;

t_bitarray *bitmap;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void msjConexionCon(char *s){
	printf("\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
} //Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.

int nuevohandshake(int *cliente, int proceso) {
	char unProceso[2]; unProceso[0] = '0' + proceso; unProceso[1] = '\0';
	char procesoAConocer[2];
	send((*cliente), unProceso, 2, 0);
	recv((*cliente), procesoAConocer, 2, 0);
	return atoi(procesoAConocer);
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO");
	config->puntoMontaje = strdup(config_get_string_value(archivo_Modelo, "PUNTO_MONTAJE"));
}

void mostrarArchivoConfig(){
	 FILE *f;

	 f=fopen(RUTAARCHIVO,"r");
	 int c;
	 printf("------------------------------------------\n");
	 while ((c = fgetc (f)) != EOF) putchar(c);
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

bool validarArchivo(char* path){
	FILE * archivo = fopen(path, "r");
	if (!archivo){
		return false;//ERROR DE ARCHIVO NO ENCONTRADO (LO TIENE QUE DEVOLVER AL KERNEL)
	}
	fclose(archivo);
	return true;
}

void leerArchivoConfiguracionFS(){
	char path[100];
	strcpy(path,config->puntoMontaje);
	char pathRelativo[22] = "Metadata/Metadata.bin";
	strcat(path,pathRelativo);

	if (access(path, F_OK) == -1) {
		printf("No se encontró el archivo de configuración del FS\n");
		exit(-1);
	}
	t_config *archivo_config_fs = config_create(path);
	configFS = reservarMemoria(sizeof(t_configuracion_filesystem));
	configFS->tamBloque = config_get_int_value(archivo_config_fs, "TAMANIO_BLOQUES");
	configFS->cantBloques = config_get_int_value(archivo_config_fs, "CANTIDAD_BLOQUES");
	config_destroy(archivo_config_fs);
}

void inicializarBitmap(){//DEJÉ ACÁ
	//bitmap = bitarray_create_with_mode(char *bitarray, size_t size, bit_numbering_t mode);
}

int tamanioArchivo(char* path){
	FILE * archivo = fopen(path, "r");
	if (!archivo){
		return -1;
	}
	fseek(archivo, 0L, SEEK_END);
	int tamanio = ftell(archivo);
	fseek(archivo, 0L, SEEK_SET);
	return tamanio;
}

void crearDirectorio(char* path){
	int i = strlen(path)-1;
	while(path[i]!='/') i--;//i termina teniendo la posición de la última barra
	char* directorio = strndup(path,i+1);//Se copia hasta la posición de la última barra inclusive
	mkdir(directorio, 0700);
	free(directorio);
}

void crearArchivo(char* pathRelativo){
	char path[100];
	strcpy(path,config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path,pathArchivos);
	strcat(path,pathRelativo);
	FILE * archivo = fopen(path, "w");
	if (!archivo){ //Si los directorios todavía no están creados, hay que hacerlo
		crearDirectorio(path);
		FILE * archivo = fopen(path, "w");
		if (!archivo){
			printf("Error al crear archivo despúes de crear el directorio");
			exit (-1);
		}
	}
	fclose(archivo);
	//Falta agregar contenido al archivo (tamaño 0, 1 bloque asignado)
}

int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	leerArchivo();

	leerArchivoConfiguracionFS();
	inicializarBitmap();

	//Prueba
	crearArchivo("Pepo.bin");
	crearArchivo("Lolo/Pepin.bin");

	int cliente;
	char* buffer = malloc(LONGMAX);

	conectar(&cliente, &direccionServidor);

	int procesoConectado = nuevohandshake(&cliente, file_system);
	switch (procesoConectado) {
	case kernel:
		msjConexionCon("Kernel");
		recibirMensajeDe(&cliente, buffer);
		break;
	default:
		printf("No me puedo conectar con vos.\n");
		exit(-1);
		break;
	}

	close(cliente);

	return 0;
}
