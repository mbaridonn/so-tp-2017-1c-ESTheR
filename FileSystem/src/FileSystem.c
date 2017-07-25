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
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/bitarray.h>
//#include <commons/log.h>
#include "libreriaSockets.h"

char *rutaArchivo;// = "/home/utnso/git/tp-2017-1c-C-digo-Facilito/FileSystem/src/configFyleSystem";

int clienteKernel;
//t_log *fileSystem_log;

enum accionesFS{
	k_fs_validar_archivo, k_fs_crear_archivo, k_fs_borrar_archivo, k_fs_leer_archivo, k_fs_escribir_archivo
};

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

void msjConexionCon(char *s) {
	//printf("\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
	log_info(fileSystem_log,"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			 s);
}

int nuevohandshake(int *cliente, int proceso) {
	char unProceso[2];
	unProceso[0] = '0' + proceso;
	unProceso[1] = '\0';
	char procesoAConocer[2];
	send((*cliente), unProceso, 2, 0);
	recv((*cliente), procesoAConocer, 2, 0);
	return atoi(procesoAConocer);
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		log_error(fileSystem_log, "No hay más espacio");
		//printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO");
	config->puntoMontaje = strdup(config_get_string_value(archivo_Modelo, "PUNTO_MONTAJE"));
}

void mostrarArchivoConfig() {
	FILE *f;

	f = fopen(rutaArchivo, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	//log_info(fileSystem_log, "\n");
	printf("\n");
	printf("------------------------------------------\n");

}

void leerArchivo() {
	if (access(rutaArchivo, F_OK) == -1) {
		log_error(fileSystem_log, "No se encontró el Archivo");
		//printf("No se encontró el Archivo\n");
		exit(-1);
	}
	t_config *archivo_config = config_create(rutaArchivo);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	mostrarArchivoConfig();
	log_info(fileSystem_log, "Leí el archivo y extraje el puerto: %d", config->puerto);
	//printf("Leí el archivo y extraje el puerto: %d\n", config->puerto);
}

int divisionRoundUp(int dividendo, int divisor) {
	if (dividendo <= 0 || divisor <= 0) {
		log_error(fileSystem_log, "Esta division funciona unicamente con enteros positivos");
		//printf("Esta division funciona unicamente con enteros positivos\n");
		exit(-1);
	}
	return 1 + ((dividendo - 1) / divisor);
}

int max(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}

//Comienzan funciones del FS

bool validarArchivo(char* pathRelativo) {
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path, config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path, pathArchivos);
	strcat(path, pathRelativo);

	FILE * archivo = fopen(path, "r");
	if (!archivo) {
		return false;
	}
	fclose(archivo);
	return true;
}

void leerArchivoConfiguracionFS() {
	char path[100];
	strcpy(path, config->puntoMontaje);
	char pathRelativo[22] = "Metadata/Metadata.bin";
	strcat(path, pathRelativo);

	if (access(path, F_OK) == -1) {
		log_error(fileSystem_log, "No se encontró el Archivo de Configuración");
		//printf("No se encontró el archivo de configuración del FS\n");
		exit(-1);
	}
	t_config *archivo_config_fs = config_create(path);
	configFS = reservarMemoria(sizeof(t_configuracion_filesystem));
	configFS->tamBloque = config_get_int_value(archivo_config_fs,"TAMANIO_BLOQUES");
	configFS->cantBloques = config_get_int_value(archivo_config_fs,"CANTIDAD_BLOQUES");
	config_destroy(archivo_config_fs);
}

void crearBloques() {
	char pathBloque[100], copiaPathBloque[100];
	strcpy(pathBloque, config->puntoMontaje);
	char pathRelativoBloque[22] = "Bloques/";
	strcat(pathBloque, pathRelativoBloque);
	int i;
	for (i = 0; i < configFS->cantBloques; i++) {
		char nombreBloque[10];
		snprintf(nombreBloque, 10, "%d.bin", i);
		strcpy(copiaPathBloque, pathBloque);
		strcat(copiaPathBloque, nombreBloque);
		FILE * bloque = fopen(copiaPathBloque, "a");//Creo el bloque si no existe
		fclose(bloque);
	}
}

void inicializarBitmap() {
	char path[100];
	strcpy(path, config->puntoMontaje);
	char pathRelativo[22] = "Metadata/Bitmap.bin";
	strcat(path, pathRelativo);
	if (access(path, F_OK) == -1) {
		log_error(fileSystem_log, "No se encontró el archivo bitmap");
		//printf("No se encontró el archivo bitmap del FS\n");
		exit(-1);
	}

	int fd = open(path, O_RDWR);
	ftruncate(fd, divisionRoundUp(configFS->cantBloques, 8)); //Tiene que tener este tamaño en bytes (pone los bytes en 0?)
	struct stat mystat;
	if (fstat(fd, &mystat) < 0) {
		log_error(fileSystem_log, "Error al establecer fstat");
		//printf("Error al establecer fstat\n");
		close(fd);
		exit(-1);
	}
	char *bitmap = (char *) mmap(0, mystat.st_size, PROT_READ | PROT_WRITE,	MAP_SHARED, fd, 0);
	if (bitmap == MAP_FAILED) {
		log_error(fileSystem_log, "Error al mapear a memoria");
		//printf("Error al mapear a memoria\n");
		close(fd);
		exit(-1);
	}
	bitarray = bitarray_create_with_mode(bitmap,divisionRoundUp(configFS->cantBloques, 8), LSB_FIRST);
	// log_info(fileSystem_log,"El tamano del bitarray es de : %d\n", bitarray_get_max_bit(bitarray));
	close(fd);
}

int tamanioArchivo(char* path) {
	struct stat st;
	if (stat(path, &st) == 0)
		return st.st_size;
	return -1;
}

int primerBloqueVacio() {
	int i, bloque = -1;
	for (i = 0; (bloque == -1) && (i < configFS->cantBloques); i++) {
		if (bitarray_test_bit(bitarray, i) == 0)
			bloque = i;
	}
	return bloque;
}

void crearDirectorio(char* path) {
	/*int i = strlen(path) - 1;
	while (path[i] != '/')
		i--; //i termina teniendo la posición de la última barra
	char* directorio = strndup(path, i + 1); //Se copia hasta la posición de la última barra inclusive
	mkdir(directorio, 0700);
	free(directorio);*/

	const size_t len = strlen(path);
	char _path[200];
	char *p;
	/* Copy string so its mutable */
	strcpy(_path, path);
	/* Iterate the string */
	for (p = _path + 1; *p; p++) {
		if (*p == '/') {
			/* Temporarily truncate */
			*p = '\0';
			mkdir(_path, S_IRWXU);
			*p = '/';
		}
	}
}

void crearArchivo(char* pathRelativo) {
	if(primerBloqueVacio() == -1){
		//DEBERÍA ENVIAR UN MENSAJE DE ERROR A KERNEL SI NO HAY MÁS BLOQUES (PENDIENTE!!)
		//TAMBIÉN HABRÍA QUE AGREGAR UN MENSAJE DE OK, EN ESE CASO
		return;
	}
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path, config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path, pathArchivos);
	strcat(path, pathRelativo);
	FILE * archivo = fopen(path, "a");//Creo el bloque si no existe
	if (!archivo) { //Si los directorios todavía no están creados, hay que hacerlo
		crearDirectorio(path);
		log_info(fileSystem_log,"Creados directorios necesarios");
		archivo = fopen(path, "w");
		if (!archivo) {
			log_error(fileSystem_log, "Error al crear archivo despúes de crear el directorio");
			//printf("Error al crear archivo despúes de crear el directorio\n");
			exit(-1);
		}
	}
	fclose(archivo);
	if (tamanioArchivo(path)==0){//Si el archivo está vacío, creo File Metadata
		t_config *fileMetadata = config_create(path);
		char keyTamanio[8] = "TAMANIO";
		char valorTamanio[2] = "0";
		config_set_value(fileMetadata, keyTamanio, valorTamanio);
		char keyBloques[8] = "BLOQUES";
		char valorBloques[10];
		int bloqueAAsignar = primerBloqueVacio();
		snprintf(valorBloques, 10, "[%d]", bloqueAAsignar);
		config_set_value(fileMetadata, keyBloques, valorBloques);
		bitarray_set_bit(bitarray, bloqueAAsignar);
		config_save(fileMetadata);
		config_destroy(fileMetadata);
	}
}

void borrarArchivo(char *pathRelativo) {
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path, config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path, pathArchivos);
	strcat(path, pathRelativo);

	t_config *archivo = config_create(path);
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	int cantBloques;
	if (tamArchivo == 0)
		cantBloques = 1;
	else
		cantBloques = divisionRoundUp(tamArchivo, configFS->tamBloque);
	char **bloquesArchivo = malloc(sizeof(char) * 10 * cantBloques); //Supongo que un bloque no puede tener más de 10 dígitos
	bloquesArchivo = config_get_array_value(archivo, "BLOQUES"); //Devuelve una array de c-strings
	int i;
	//Liberar bloques en el Bitmap
	for (i = 0; i < cantBloques; i++) {
		bitarray_clean_bit(bitarray, atoi(bloquesArchivo[i]));
	}
	config_destroy(archivo);
	//Borrar File Metadata
	remove(path);
	free(bloquesArchivo);
}

char* obtenerDatos(char* pathRelativo, int offset, int size) {
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path, config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path, pathArchivos);
	strcat(path, pathRelativo);

	t_config *archivo = config_create(path);
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	if (offset >= tamArchivo) {
		log_error(fileSystem_log, "Se está intentando leer más allá del fin del archivo");
		//printf("Se está intentando leer más allá del fin del archivo\n");
		exit(-1); //SE DEBERIÁ DEVOLVER UN MENSAJE AL KERNEL
	}
	int cantBloques;
	if (tamArchivo == 0)
		cantBloques = 1;
	else
		cantBloques = divisionRoundUp(tamArchivo, configFS->tamBloque);
	char **bloquesArchivo = config_get_array_value(archivo, "BLOQUES"); //Devuelve una array de c-strings

	int bloqueInicial = offset / configFS->tamBloque;
	int offsetEnBloque = offset % configFS->tamBloque;
	int sizeRestante = 0;

	if (offsetEnBloque + size > configFS->tamBloque) { //Si se pasa del bloque
		int proximoBloque = bloqueInicial + 1;
		if (proximoBloque == cantBloques) {
			log_error(fileSystem_log, "Se está intentando leer más allá del fin del archivo");
			//printf("Se está intentando leer más allá del fin del archivo\n");
			exit(-1); //EN REALIDAD DEBERÍA RETORNAR UN MENSAJE AL QUE PIDIÓ LA LECTURA
		}
		sizeRestante = size - (configFS->tamBloque - offsetEnBloque); //Si hay otro bloque, me guardo el tamaño restante
	}

	char pathBloque[100];
	strcpy(pathBloque, config->puntoMontaje);
	char pathRelativoBloque[22] = "Bloques/";
	strcat(pathBloque, pathRelativoBloque);
	char nombreBloque[10];
	snprintf(nombreBloque, 10, "%s.bin", bloquesArchivo[bloqueInicial]);
	strcat(pathBloque, nombreBloque);
	FILE * bloque = fopen(pathBloque, "r");
	char *bytesLeidos = reservarMemoria(size+1);//Sumo uno para evitar SEGFAULT al agregar el último '\0'
	fread(bytesLeidos, size - sizeRestante, sizeof(char), bloque); //En caso de que haya que leer más de otra pág
	bytesLeidos[size - sizeRestante] = '\0';
	fclose(bloque);

	if (sizeRestante) { //Hay que leer el resto de otro bloque
		char *bytesRestantesLeidos = obtenerDatos(pathRelativo,offset + (size - sizeRestante), sizeRestante);
		//Lee lo que esta en el bloque que sigue
		//SI obtenerDatos ME MANDA UN ERROR, LO TENDRÍA QUE TRATAR (POR AHORA HAY UN EXIT)
		int i = strlen(bytesLeidos);
		int j = 0;
		while (i < size) {
			bytesLeidos[i] = bytesRestantesLeidos[j];
			bytesLeidos[i + 1] = '\0';
			i++;
			j++;
		}
		free(bytesRestantesLeidos);
	}

	config_destroy(archivo);
	free(bloquesArchivo);

	return bytesLeidos;	//Habría que liberar la memoria en la función que la llame
}

int cantidadDeBloquesLibres() {
	int i, cantBloquesLibres = 0;
	for (i = 0; i < configFS->cantBloques; i++) {
		if (bitarray_test_bit(bitarray, i) == 0)
			cantBloquesLibres++;
	}
	return cantBloquesLibres;
}

void agregarBloques(t_config *archivo, int cantBloquesAAgregar) {
	if (cantBloquesAAgregar > cantidadDeBloquesLibres()) {
		log_error(fileSystem_log, "No hay suficientes bloques para asignar al archivo");
		//printf("No hay suficientes bloques para asignar al archivo\n");
		exit(-1);		//EN REALIDAD DEBERÍA RETORNAR UN MENSAJE              (PENDIENTE!!)
	}
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	int cantBloques;
	if (tamArchivo == 0)
		cantBloques = 1;
	else
		cantBloques = divisionRoundUp(tamArchivo, configFS->tamBloque);
	char **bloquesArchivo = config_get_array_value(archivo, "BLOQUES");	//Devuelve una array de c-strings
	//Aplano string
	int i;
	char *strBloquesArchivoPlano = string_new();
	string_append(&strBloquesArchivoPlano, "[");
	//Agrego elementos antiguos
	for (i = 0; i < (cantBloques); i++) {
		string_append(&strBloquesArchivoPlano, bloquesArchivo[i]);
		string_append(&strBloquesArchivoPlano, ",");
	}
	//Agrego elementos nuevos
	while (cantBloquesAAgregar != 0) {
		char primerBloqueLibre[10];
		snprintf(primerBloqueLibre, 10, "%d", primerBloqueVacio());
		string_append(&strBloquesArchivoPlano, primerBloqueLibre);
		string_append(&strBloquesArchivoPlano, ",");
		bitarray_set_bit(bitarray, primerBloqueVacio());//Marco bloque como ocupado en bitarray
		cantBloques++;
		cantBloquesAAgregar--;
	}
	strBloquesArchivoPlano[strlen(strBloquesArchivoPlano) - 1] = ']';

	config_set_value(archivo, "BLOQUES", strBloquesArchivoPlano);
	config_save(archivo);
	free(strBloquesArchivoPlano);
	free(bloquesArchivo);
}

void guardarDatos(char* pathRelativo, int offset, int size, char* buffer) {
	char path[200]; //Tamaño arbitrario (podría llegar a ser una limitación)
	strcpy(path, config->puntoMontaje);
	char pathArchivos[10] = "Archivos/";
	strcat(path, pathArchivos);
	strcat(path, pathRelativo);

	t_config *archivo = config_create(path);
	int tamArchivo = config_get_int_value(archivo, "TAMANIO");
	int cantBloques;
	if (tamArchivo == 0)
		cantBloques = 1;
	else
		cantBloques = divisionRoundUp(tamArchivo, configFS->tamBloque);

	//Asigno bloques extra (de ser necesario)
	int cantBloquesNecesaria = divisionRoundUp((offset + size), configFS->tamBloque);
	if (cantBloquesNecesaria > cantBloques) {
		agregarBloques(archivo, cantBloquesNecesaria - cantBloques);
		//SI agregarBloques ME MANDA UN ERROR, LO TENDRÍA QUE TRATAR (POR AHORA HAY UN EXIT)      (PENDIENTE!!)
		config_destroy(archivo);
		archivo = config_create(path); //Necesito cargarlo de nuevo para poder ver los cambios
	}

	//Actualizo tamaño del archivo
	char tamArchivoFinal[10];
	snprintf(tamArchivoFinal, 10, "%d", max(tamArchivo, offset + size));
	config_set_value(archivo, "TAMANIO", tamArchivoFinal);
	config_save(archivo);
	config_destroy(archivo);
	archivo = config_create(path); //Necesito cargarlo de nuevo para poder ver los cambios

	int bloqueInicial = offset / configFS->tamBloque;
	int offsetEnBloque = offset % configFS->tamBloque;

	if (offsetEnBloque + size > configFS->tamBloque) { //Si se pasa del bloque
		int sizeRestante = size - (configFS->tamBloque - offsetEnBloque);
		guardarDatos(pathRelativo, offset + (size - sizeRestante), sizeRestante, &buffer[size - sizeRestante]);
		//Le paso la posición del buffer desde la que tiene que seguir escribiendo
	}

	//Escribo el bloque
	char **bloquesArchivo = config_get_array_value(archivo, "BLOQUES");
	char pathBloque[100];
	strcpy(pathBloque, config->puntoMontaje);
	char pathRelativoBloque[22] = "Bloques/";
	strcat(pathBloque, pathRelativoBloque);
	char nombreBloque[10];
	snprintf(nombreBloque, 10, "%s.bin", bloquesArchivo[bloqueInicial]);
	strcat(pathBloque, nombreBloque);
	FILE * bloque = fopen(pathBloque, "r+");
	fwrite(buffer, sizeof(char), configFS->tamBloque - offsetEnBloque, bloque);
	fclose(bloque);

	config_destroy(archivo);
	free(bloquesArchivo);
}

void enviarSenialAKernel() {
	char senial[2] = "a";
	if (send(clienteKernel, senial, 2, 0) == -1) {
		log_error(fileSystem_log, "Error al enviar la senial");
		//printf("Error al enviar la senial\n");
	}
}

int accionPedidaPorKernel() {
	int accionPedida;
	if (recv(clienteKernel, &accionPedida, sizeof(int), 0) == -1) {
		log_error(fileSystem_log, "Error recibiendo la accion pedida");
		//printf("Error recibiendo la accion pedida\n");
		exit(-1);
	}
	return accionPedida;
}

char *recibirPath(){
	int tamPath;
	char *path;
	if (recv(clienteKernel, &tamPath, sizeof(int), 0) == -1) {
		log_error(fileSystem_log, "Error recibiendo el tamanio del Path");
		//printf("Error recibiendo el tamanio del Path\n");
		exit(-1);
	}
	path = reservarMemoria(tamPath*sizeof(char));
	enviarSenialAKernel();
	if (recv(clienteKernel, path, tamPath, 0) == -1) {
		log_error(fileSystem_log, "Error recibiendo el Path");
		//printf("Error recibiendo el tamanio del Path\n");
		exit(-1);
	}
	return path;
}


void atenderKernel() {
	while (1) {
		char* path;
		int accionPedida = accionPedidaPorKernel();
		log_info(fileSystem_log, "Me llego la accion %d\n", accionPedida);
		//printf("Me llego la accion %d\n", accionPedida);
		enviarSenialAKernel();
		switch (accionPedida) {
		case k_fs_validar_archivo:
		{
			path = recibirPath();
			log_info(fileSystem_log, "Valido que exista archivo %s\n", path);
			//printf("Valido que exista archivo %s\n", path);
			bool existe = validarArchivo(path);
			if(existe) log_info(fileSystem_log, "El archivo existe\n", path);
			else log_info(fileSystem_log, "El archivo no existe\n", path);
			if (send(clienteKernel, &existe, sizeof(bool), 0) == -1) {
				log_error(fileSystem_log, "Error enviando si el archivo existe");
				//printf("Error enviando si el archivo existe\n");
				exit(-1);
			}
			free(path);
			break;
		}
		case k_fs_crear_archivo:
			path = recibirPath();
			log_info(fileSystem_log, "Creo archivo en el path %s\n", path);
			//printf("Creo archivo en el path %s\n", path);
			crearArchivo(path);
			//NO ENVÍA CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
			break;
		case k_fs_borrar_archivo:
			path = recibirPath();
			log_info(fileSystem_log, "Borro archivo en el path %s\n", path);
			//printf("Borro archivo en el path %s\n", path);
			borrarArchivo(path);
			//NO ENVÍA CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
			break;
		case k_fs_leer_archivo:
		{
			path = recibirPath();
			int offset, size;
			offset = accionPedidaPorKernel();//Uso accionPedidaPorKernel para recibir parámetro
			enviarSenialAKernel();
			size = accionPedidaPorKernel();//Uso accionPedidaPorKernel para recibir parámetro
			enviarSenialAKernel();
			log_info(fileSystem_log, "LeerArchivo(%s,%d,%d)\n", path, offset, size);
			//printf("LeerArchivo(%s,%d,%d)\n", path, offset, size);
			char* bytesLeidos = obtenerDatos(path, offset, size);
			if (send(clienteKernel, bytesLeidos, size, 0) == -1) {
				log_error(fileSystem_log, "Error enviando el archivo leido");
				//printf("Error enviando el archivo leido\n");
				exit(-1);
			}
			free(bytesLeidos);
			break;
		}
		case k_fs_escribir_archivo:
		{
			path = recibirPath();
			int offset, size;
			offset = accionPedidaPorKernel();//Uso accionPedidaPorKernel para recibir parámetro
			enviarSenialAKernel();
			size = accionPedidaPorKernel();//Uso accionPedidaPorKernel para recibir parámetro
			enviarSenialAKernel();
			char* buffer = reservarMemoria(size);
			if (recv(clienteKernel, buffer, size, 0) == -1) {
				log_error(fileSystem_log, "Error recibiendo los bytes a escribir");
				//printf("Error recibiendo los bytes a escribir\n");
				exit(-1);
			}
			log_info(fileSystem_log, "escribirArchivo(%s,%d,%d,%.*s)\n", path, offset, size, size, buffer);
			//printf("escribirArchivo(%s,%d,%d,%.*s)\n", path, offset, size, size, buffer);//FORMA DE IMPRIMIR STRING CON LONGITUD!!
			guardarDatos(path,offset,size,buffer);
			//NO ENVÍA CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
			break;
		}
		default:
			break;
		}
	}
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

	inicializarLog();
//	fileSystem_log = log_create("/home/utnso/git/tp-2017-1c-C-digo-Facilito/FileSystem/Debug/FileSystem.log", "CódigoFacilito-FS\n", true, LOG_LEVEL_TRACE);
	log_info(fileSystem_log,"Iniciando FileSystem\n");

	leerArchivo();

	free(rutaArchivo);

	leerArchivoConfiguracionFS();
	crearBloques();
	inicializarBitmap();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(config->puerto);

	int servidor;

	esperarConexion(&servidor, &direccionServidor);
	aceptarConexion(&servidor, &clienteKernel);

	int procesoConectado = handshake(&clienteKernel, file_system);
	switch (procesoConectado) {
	case kernel:
		msjConexionCon("Kernel");
		atenderKernel();
		break;
	default:
		log_error(fileSystem_log, "No me puedo conectar con vos");
		//printf("No me puedo conectar con vos.\n");
		exit(-1);
		break;
	}

	bitarray_destroy(bitarray);

	close(clienteKernel);
	log_destroy(fileSystem_log);
	return 0;
}
