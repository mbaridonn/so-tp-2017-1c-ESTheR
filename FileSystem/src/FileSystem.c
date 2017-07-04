#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
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

t_bitarray *bitarray;

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

//Comienzan funciones del FS

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

void crearBloques(){
	char pathBloque[100], copiaPathBloque[100];
	strcpy(pathBloque,config->puntoMontaje);
	char pathRelativoBloque[22] = "Bloques/";
	strcat(pathBloque,pathRelativoBloque);
	int i;
	for (i=0;i<configFS->cantBloques;i++){
		char nombreBloque[10];
		snprintf(nombreBloque, 10, "%d.bin", i);
		strcpy(copiaPathBloque,pathBloque);
		strcat(copiaPathBloque,nombreBloque);
		FILE * bloque = fopen(copiaPathBloque, "w");
		fclose(bloque);
	}
}

void inicializarBitmap(){
	char path[100];
	strcpy(path,config->puntoMontaje);
	char pathRelativo[22] = "Metadata/Bitmap.bin";
	strcat(path,pathRelativo);
	if (access(path, F_OK) == -1) {
		printf("No se encontró el archivo bitmap del FS\n");
		exit(-1);
	}

	int fd = open(path, O_RDWR);
	ftruncate(fd, divisionRoundUp(configFS->cantBloques, 8));//Tiene que tener este tamaño en bytes (pone los bytes en 0?)
	struct stat mystat;
	if (fstat(fd, &mystat) < 0) {
		printf("Error al establecer fstat\n");
		close(fd);
		exit(-1);
	}
	char *bitmap = (char *) mmap(0, mystat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (bitmap == MAP_FAILED) {
		printf("Error al mapear a memoria\n");
		close(fd);
		exit(-1);
	}
	bitarray = bitarray_create_with_mode(bitmap,divisionRoundUp(configFS->cantBloques, 8), LSB_FIRST);
	/*int i;                                NECESARIO PARA PONER TODOS LOS BITS EN 0? LO HACE YA TRUNCATE?
	for (i = 0; i < cantBloques; i++) {
		bitarray_clean_bit(bitarray, i);
	}*/
	close(fd);
}

int tamanioArchivo(char* path){ // Está bien? O lo tendría que obtener de la metadata del archivo???
	FILE * archivo = fopen(path, "r");
	if (!archivo){
		return -1;
	}
	fseek(archivo, 0L, SEEK_END);
	int tamanio = ftell(archivo);
	fseek(archivo, 0L, SEEK_SET);
	return tamanio;
}

int primerBloqueVacio(){
	int i, bloque=-1;
	for (i=0; (bloque == -1) && (i < configFS->cantBloques);i++){
		if (bitarray_test_bit(bitarray,i)==0) bloque = i;
	}
	return bloque;
}

void crearDirectorio(char* path){
	int i = strlen(path)-1;
	while(path[i]!='/') i--;//i termina teniendo la posición de la última barra
	char* directorio = strndup(path,i+1);//Se copia hasta la posición de la última barra inclusive
	mkdir(directorio, 0700);
	free(directorio);
}

void crearArchivo(char* pathRelativo){
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path,config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path,pathArchivos);
	strcat(path,pathRelativo);
	FILE * archivo = fopen(path, "w");
	if (!archivo){ //Si los directorios todavía no están creados, hay que hacerlo
		crearDirectorio(path);
		FILE * archivo = fopen(path, "w");//PARECE QUE NO FUNCIONA!!
		if (!archivo){
			printf("Error al crear archivo despúes de crear el directorio\n");
			exit (-1);
		}
	}
	fclose(archivo);//ROMPE CUANDO EL DIRECTORIO NO ESTÁ CREADO!!
	//Crear File Metadata
	t_config *fileMetadata = config_create(path);
	char keyTamanio[8] = "TAMANIO";
	char valorTamanio[2] = "0";
	config_set_value(fileMetadata, keyTamanio, valorTamanio);
	char keyBloques[8] = "BLOQUES";
	char valorBloques[10];
	int bloqueAAsignar = primerBloqueVacio();
	snprintf(valorBloques, 10, "[%d]", bloqueAAsignar);
	config_set_value(fileMetadata, keyBloques, valorBloques);
	bitarray_set_bit(bitarray,bloqueAAsignar);
	config_save(fileMetadata);
	config_destroy(fileMetadata);
}

void borrarArchivo(char *pathRelativo){
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path,config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path,pathArchivos);
	strcat(path,pathRelativo);

	t_config *archivo = config_create(path);
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	int cantBloques;
	if(tamArchivo==0) cantBloques=1;
	else cantBloques = divisionRoundUp(tamArchivo,configFS->tamBloque);
	char **bloquesArchivo = malloc(sizeof(char)*10*cantBloques);//Supongo que un bloque no puede tener más de 10 dígitos
	bloquesArchivo = config_get_array_value(archivo, "BLOQUES");//Devuelve una array de c-strings
	int i;
	//Liberar bloques en el Bitmap
	for (i=0;i<cantBloques;i++){
		bitarray_clean_bit(bitarray, atoi(bloquesArchivo[i]));
	}
	config_destroy(archivo);
	//Borrar File Metadata
	remove(path);
	free(bloquesArchivo);
}

char* obtenerDatos(char* pathRelativo, int offset, int size){
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path,config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path,pathArchivos);
	strcat(path,pathRelativo);

	t_config *archivo = config_create(path);
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	if (offset >= tamArchivo){
		printf("Se está intentando leer más allá del fin del archivo\n");
		exit(-1);//SE DEBERIÁ DEVOLVER UN MENSAJE AL KERNEL
	}
	int cantBloques;
	if(tamArchivo==0) cantBloques=1;
	else cantBloques = divisionRoundUp(tamArchivo,configFS->tamBloque);
	char **bloquesArchivo = malloc(sizeof(char)*10*cantBloques);//Supongo que un bloque no puede tener más de 10 dígitos
	bloquesArchivo = config_get_array_value(archivo, "BLOQUES");//Devuelve una array de c-strings

	int bloqueInicial = offset / configFS->tamBloque;
	int offsetEnBloque = offset % configFS->tamBloque;
	int sizeRestante = 0;

	if (offsetEnBloque + size > configFS->tamBloque) {//Si se pasa del bloque
		int proximoBloque = bloqueInicial + 1;
		if (proximoBloque == cantBloques) {
			printf("Se está intentando leer más allá del fin del archivo\n");
			exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		sizeRestante = size - (configFS->tamBloque - offsetEnBloque);//Si hay otro bloque, me guardo el tamaño restante
	}

	char pathBloque[100];
	strcpy(pathBloque,config->puntoMontaje);
	char pathRelativoBloque[22] = "Bloques/";
	strcat(pathBloque,pathRelativoBloque);
	char nombreBloque[10];
	snprintf(nombreBloque, 10, "%s.bin", bloquesArchivo[bloqueInicial]);
	strcat(pathBloque,nombreBloque);
	FILE * bloque = fopen(pathBloque, "r");
	char *bytesLeidos = reservarMemoria(size);
	fread(bytesLeidos,size-sizeRestante,sizeof(char),bloque);//En caso de que haya que leer más de otra pág
	fclose(bloque);

	if (sizeRestante) { //Hay que leer el resto de otro bloque
		char *bytesRestantesLeidos = obtenerDatos(pathRelativo, offset+(size-sizeRestante), sizeRestante); //Lee lo que esta en el bloque que sigue
		//Si obtenerDatos me manda un error, lo tendría que tratar (por ahora hay un exit)
		int i = strlen(bytesLeidos);
		int j = 0;
		while (i < size) {
			bytesLeidos[i] = bytesRestantesLeidos[j];
			i++;
			j++;
		}
		free(bytesRestantesLeidos);
	}

	config_destroy(archivo);
	free(bloquesArchivo);

	return bytesLeidos;//Habría que liberar la memoria en la función que la llame
}

int cantidadDeBloquesLibres(){
	int i, cantBloquesLibres=0;
	for (i=0;i<configFS->cantBloques;i++){
		if (bitarray_test_bit(bitarray,i)==0) cantBloquesLibres++;
	}
	return cantBloquesLibres;
}

char *unificarString(char** bloquesArchivo){
	char* stringUnificado;
	//Aplanar string            DEJÉ ACÁ!!!
	return stringUnificado;
}

void agregarBloques(t_config *archivo,int cantBloquesAAgregar){
	if (cantBloquesAAgregar<cantidadDeBloquesLibres()){
		printf("No hay suficientes bloques para asignar al archivo\n");
		exit(-1);//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE
	}
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	int cantBloques;
	if(tamArchivo==0) cantBloques=1;
	else cantBloques = divisionRoundUp(tamArchivo,configFS->tamBloque);
	char **bloquesArchivo = malloc(sizeof(char)*10*(cantBloques+cantBloquesAAgregar));//Supongo que un bloque no puede tener más de 10 dígitos
	bloquesArchivo = config_get_array_value(archivo, "BLOQUES");//Devuelve una array de c-strings
	while(cantBloquesAAgregar!=0){
		char primerBloqueLibre[9];
		snprintf(primerBloqueLibre, 10, "%d.bin", primerBloqueVacio());
		strcpy(bloquesArchivo[cantBloques],primerBloqueLibre);
		bitarray_set_bit(bitarray,primerBloqueVacio());
		cantBloques++;
		cantBloquesAAgregar--;
	}
	char *strBloquesArchivoPlano = unificarString(bloquesArchivo);
	config_set_value(archivo,"BLOQUES",strBloquesArchivoPlano);
	config_save(archivo);
	free(strBloquesArchivoPlano);
}

void guardarDatos(char* pathRelativo, int offset, int size, char* buffer){
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path,config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path,pathArchivos);
	strcat(path,pathRelativo);

	t_config *archivo = config_create(path);
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	int cantBloques;
	if(tamArchivo==0) cantBloques=1;
	else cantBloques = divisionRoundUp(tamArchivo,configFS->tamBloque);

	//Asigno bloques extra (de ser necesario)
	int cantBloquesNecesaria = divisionRoundUp((offset + size),configFS->tamBloque);
	if (cantBloquesNecesaria>cantBloques){
		agregarBloques(archivo,cantBloquesNecesaria-cantBloques);
		//Si agregarBloques me manda un error, lo tendría que tratar (por ahora hay un exit)
	}

	int cantBloquesFinal = max(cantBloques,cantBloquesNecesaria);
	char **bloquesArchivo = malloc(sizeof(char)*10*cantBloquesFinal);//Supongo que un bloque no puede tener más de 10 dígitos
	bloquesArchivo = config_get_array_value(archivo, "BLOQUES");//Devuelve una array de c-strings

	int bloqueInicial = offset / configFS->tamBloque;
	int offsetEnBloque = offset % configFS->tamBloque;

	if (offsetEnBloque + size > configFS->tamBloque) {//Si se pasa del bloque
		int sizeRestante = size - (configFS->tamBloque - offsetEnBloque);
		guardarDatos(pathRelativo, offset+(size-sizeRestante), sizeRestante, &buffer[size - sizeRestante]);
		//Le paso la posición del buffer desde la que tiene que seguir escribiendo
	}

	char pathBloque[100];
	strcpy(pathBloque,config->puntoMontaje);
	char pathRelativoBloque[22] = "Bloques/";
	strcat(pathBloque,pathRelativoBloque);
	char nombreBloque[10];
	snprintf(nombreBloque, 10, "%s.bin", bloquesArchivo[bloqueInicial]);
	strcat(pathBloque,nombreBloque);
	FILE * bloque = fopen(pathBloque, "r+");
	fwrite(buffer, sizeof(char), configFS->tamBloque-offsetEnBloque, bloque);
	fclose(bloque);

	//Actualizo tamaño del archivo
	char tamArchivoFinal[10];
	snprintf(buffer, 10, "%d", max(tamArchivo, offset+size));
	config_set_value(archivo,"TAMANIO",tamArchivoFinal);
	config_save(archivo);

	config_destroy(archivo);
	free(bloquesArchivo);
}

int main(void) {
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8080);

	leerArchivo();

	leerArchivoConfiguracionFS();
	crearBloques();
	inicializarBitmap();

	//Prueba
	/*crearArchivo("Pepo.bin");
	crearArchivo("Lolo/Pepin.bin");
	borrarArchivo("Pepo.bin");*/

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

	bitarray_destroy(bitarray);

	close(cliente);

	return 0;
}
