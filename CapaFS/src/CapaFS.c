#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#define CANT_PROC_TABLA_ARCH 100 //ES DEMASIADO? OCUPA MUCHA MEMORIA?
#define CANT_ARCH_TABLA_ARCH 200 //ES DEMASIADO? OCUPA MUCHA MEMORIA?

//DESDE ACÁ NO COPIAR

void *reservarMemoria(int tamanio) {
	void *puntero = malloc(tamanio);
	if (puntero == NULL) {
		printf("No hay más espacio\n");
		exit(-1);
	}
	return puntero;
}

typedef struct{
	bool lectura;
	bool escritura;
	bool creacion;
} t_banderas;

//HASTA ACÁ NO COPIAR

typedef struct{
	char* nombreArchivo;
	int contadorAperturas;
} entradaTablaArchivosGlobal;

typedef struct{
	char* flags;
	u_int32_t fdGlobal;
	int offset;
} entradaTablaArchivosDeProceso;

entradaTablaArchivosGlobal tablaArchivosGlobal[CANT_ARCH_TABLA_ARCH];
entradaTablaArchivosDeProceso tablasDeArchivosDeProcesos[CANT_PROC_TABLA_ARCH][CANT_ARCH_TABLA_ARCH];
//Contiene TODAS las tablas de archivos de procesos, se accede a cada tabla por PID

void inicializarTablasDeArchivos(){
	int i, j;
	for (i=0;i<CANT_ARCH_TABLA_ARCH;i++){
		tablaArchivosGlobal[i].nombreArchivo=NULL;//entradaTablaArchivosGlobal vacía : nombreArchivo = NULL
	}

	for (i=0;i<CANT_PROC_TABLA_ARCH;i++){
		for (j=0;j<CANT_ARCH_TABLA_ARCH;j++){
			tablasDeArchivosDeProcesos[i][j].flags=NULL;//entradaTablaArchivosDeProceso vacía : flags = NULL
		}
	}
}

void abrirArchivo(int PID, /*t_direccion_archivo*/char* direccion, t_banderas flags){
	char* strFlags = string_new();
	if(flags.creacion) string_append(&strFlags, "c");
	if(flags.lectura) string_append(&strFlags, "r");
	if(flags.escritura) string_append(&strFlags, "w");

	bool existeArchivo = true; //VALOR DE PRUEBA, DESPUÉS BORRAR
	//Enviar mensaje a FS: existeArchivo = validarArchivo(char* path);
	if(!existeArchivo){
		if(string_contains(strFlags,"c")){//El archivo fue abierto en modo creación
			//Enviar mensaje a FS: crearArchivo(direccion);
		} else {
			//Enviar mensaje a CPU: El programa intentó acceder a un archivo que no existe (Exit Code -2)
			return;
		}
	}

	if(PID>=CANT_PROC_TABLA_ARCH){
		printf("Es necesario incrementar CANT_PROC_TABLA_ARCH para poder ubicar al proceso %d\n", PID);
		exit(-1);
	}

	//Actualizo la tabla global de archivos
	//Chequeo si ya existe entrada de ese archivo en la tabla global de archivos
	bool esEntradaDeArchivo(entradaTablaArchivosGlobal* entrada){
		return (entrada->nombreArchivo!=NULL) && strcmp(entrada->nombreArchivo, direccion)==0;//Chequea si son iguales
	}
	int i=0;
	bool entradaEncontrada = false;
	while((!entradaEncontrada) && (i<CANT_ARCH_TABLA_ARCH)){
		if(esEntradaDeArchivo(&tablaArchivosGlobal[i])){
			entradaEncontrada = true;
			i--;//Para conservar la posición de la entrada (fdGlobal)
		}
		i++;
	}
	if(!entradaEncontrada){//Si no existe, creo una en la primera posición libre de la tabla
		i=0;
		while((tablaArchivosGlobal[i].nombreArchivo != NULL) && (i<CANT_ARCH_TABLA_ARCH)) i++;
		if(i==CANT_ARCH_TABLA_ARCH){
			printf("No hay más entradas disponibles en la tabla de archivos global\n");
			exit(-1);
		}
		tablaArchivosGlobal[i].nombreArchivo = string_duplicate(direccion);
		tablaArchivosGlobal[i].contadorAperturas = 0;
	}
	tablaArchivosGlobal[i].contadorAperturas++;
	//i es el fdGlobal

	//Creo directamente entrada en la tabla de archivos del proceso
	//Un mismo proceso puede abrir más de una vez a un mismo archivo, con diferentes o iguales permisos.
	int j=0;
	while((tablasDeArchivosDeProcesos[PID][j].flags!=NULL) && j<CANT_ARCH_TABLA_ARCH) j++;
	if(j==CANT_ARCH_TABLA_ARCH){
		printf("No hay más entradas disponibles en la tabla de archivos del proceso\n");
		exit(-1);
	}
	tablasDeArchivosDeProcesos[PID][j].flags = string_duplicate(strFlags);
	tablasDeArchivosDeProcesos[PID][j].fdGlobal = i;//No es necesario castear xq voy a estar trabajando con enteros chicos (!)
	tablasDeArchivosDeProcesos[PID][j].offset = 0;

	u_int32_t fileDescriptor = j+3;//Las primeras 3 posiciones de la tabla de archivos del proceso están reservadas (!)
	//Devolver FD a la CPU

	free(strFlags);
}

void entradaTablaArchivosDeProceso_limpiar(entradaTablaArchivosDeProceso *entrada){
	free(entrada->flags);
	entrada->flags=NULL;
}

void entradaTablaArchivosGlobal_limpiar(entradaTablaArchivosGlobal *entrada){
	free(entrada->nombreArchivo);
	entrada->nombreArchivo=NULL;
}

void cerrarArchivo(int PID, u_int32_t fileDescriptor){
	if(PID>=CANT_PROC_TABLA_ARCH){
		printf("Es necesario incrementar CANT_PROC_TABLA_ARCH para poder ubicar al proceso %d\n", PID);
		exit(-1);
	}
	if(fileDescriptor>=CANT_ARCH_TABLA_ARCH){
		printf("Es necesario incrementar CANT_ARCH_TABLA_ARCH para poder ubicar al archivo %u\n", fileDescriptor);
		exit(-1);
	}

	//Actualizo tabla de archivos del proceso
	int posicionReal = fileDescriptor - 3;//(!)
	if(tablasDeArchivosDeProcesos[PID][posicionReal].flags==NULL){
		printf("No existe la entrada en la tabla de archivos del proceso\n");
		exit(-1);
	}
	u_int32_t fdGlobal = tablasDeArchivosDeProcesos[PID][posicionReal].fdGlobal;
	entradaTablaArchivosDeProceso_limpiar(&tablasDeArchivosDeProcesos[PID][posicionReal]);

	//Actualizo tabla de archivos global
	tablaArchivosGlobal[fdGlobal].contadorAperturas--;
	if(tablaArchivosGlobal[fdGlobal].contadorAperturas<=0){
		entradaTablaArchivosGlobal_limpiar(&tablaArchivosGlobal[fdGlobal]);
	}
}

void borrarArchivo(int PID, u_int32_t fileDescriptor){
	if(PID>=CANT_PROC_TABLA_ARCH){
		printf("Es necesario incrementar CANT_PROC_TABLA_ARCH para poder ubicar al proceso %d\n", PID);
		exit(-1);
	}
	if(fileDescriptor>=CANT_ARCH_TABLA_ARCH){
		printf("Es necesario incrementar CANT_ARCH_TABLA_ARCH para poder ubicar al archivo %u\n", fileDescriptor);
		exit(-1);
	}

	int posicionReal = fileDescriptor - 3;//(!)
	if(tablasDeArchivosDeProcesos[PID][posicionReal].flags==NULL){
		printf("No existe la entrada en la tabla de archivos del proceso\n");
		exit(-1);
	}
	u_int32_t fdGlobal = tablasDeArchivosDeProcesos[PID][posicionReal].fdGlobal;

	if(tablaArchivosGlobal[fdGlobal].contadorAperturas==1){
		//Enviar mensaje a FS: borrarArchivo(tablaArchivosGlobal[fdGlobal].nombreArchivo);
		cerrarArchivo(PID,fileDescriptor);
	} else {
		printf("El archivo no puede ser borrado, ya que está abierto por otro proceso\n");
		exit(-1); //SE DEBERÍA ENVIAR UN MENSAJE AL CPU
	}
}

void moverCursorArchivo(int PID, u_int32_t fileDescriptor, /*t_valor_variable*/int posicion){
	if(PID>=CANT_PROC_TABLA_ARCH){
		printf("Es necesario incrementar CANT_PROC_TABLA_ARCH para poder ubicar al proceso %d\n", PID);
		exit(-1);
	}
	if(fileDescriptor>=CANT_ARCH_TABLA_ARCH){
		printf("Es necesario incrementar CANT_ARCH_TABLA_ARCH para poder ubicar al archivo %u\n", fileDescriptor);
		exit(-1);
	}

	int posicionReal = fileDescriptor - 3;//(!)
	if(tablasDeArchivosDeProcesos[PID][posicionReal].flags==NULL){
		printf("No existe la entrada en la tabla de archivos del proceso\n");
		exit(-1);
	}

	tablasDeArchivosDeProcesos[PID][posicionReal].offset = posicion;
}

void leerArchivo(int PID, u_int32_t fileDescriptor,/*t_puntero*/u_int32_t informacion, /*t_valor_variable*/int tamanio){
	if(PID>=CANT_PROC_TABLA_ARCH){
		printf("Es necesario incrementar CANT_PROC_TABLA_ARCH para poder ubicar al proceso %d\n", PID);
		exit(-1);
	}
	if(fileDescriptor>=CANT_ARCH_TABLA_ARCH){
		printf("Es necesario incrementar CANT_ARCH_TABLA_ARCH para poder ubicar al archivo %u\n", fileDescriptor);
		exit(-1);
	}

	int posicionReal = fileDescriptor - 3;//(!)
	if(tablasDeArchivosDeProcesos[PID][posicionReal].flags==NULL){
		printf("No existe la entrada en la tabla de archivos del proceso\n");
		exit(-1);
	}

	if(string_contains(tablasDeArchivosDeProcesos[PID][posicionReal].flags,"r")){//El archivo fue abierto en modo lectura
		int fdGlobal = tablasDeArchivosDeProcesos[PID][posicionReal].fdGlobal;
		/*Enviar mensaje a FS: informacion(?) = obtenerDatos(tablaArchivosGlobal[fdGlobal].nombreArchivo,
		  			tablasDeArchivosDeProcesos[PID][posicionReal].offset, tamanio);*/
		//DÓNDE SE GUARDA LA INFORMACIÓN LEÍDA??
	} else {
		//Enviar mensaje a CPU: El programa intentó leer un archivo sin permisos (Exit Code -3)
	}
}

void escribirArchivo(int PID, u_int32_t fileDescriptor, /*char*?*/void* informacion, /*t_valor_variable*/int tamanio){
	if(PID>=CANT_PROC_TABLA_ARCH){
		printf("Es necesario incrementar CANT_PROC_TABLA_ARCH para poder ubicar al proceso %d\n", PID);
		exit(-1);
	}
	if(fileDescriptor>=CANT_ARCH_TABLA_ARCH){
		printf("Es necesario incrementar CANT_ARCH_TABLA_ARCH para poder ubicar al archivo %u\n", fileDescriptor);
		exit(-1);
	}

	int posicionReal = fileDescriptor - 3;//(!)
	if(tablasDeArchivosDeProcesos[PID][posicionReal].flags==NULL){
		printf("No existe la entrada en la tabla de archivos del proceso\n");
		exit(-1);
	}

	if(string_contains(tablasDeArchivosDeProcesos[PID][posicionReal].flags,"w")){//El archivo fue abierto en modo escritura
		int fdGlobal = tablasDeArchivosDeProcesos[PID][posicionReal].fdGlobal;
		/*Enviar mensaje a FS: guardarDatos(tablaArchivosGlobal[fdGlobal].nombreArchivo,
		  			tablasDeArchivosDeProcesos[PID][posicionReal].offset, tamanio, informacion);*/
	} else {
		//Enviar mensaje a CPU: El programa intentó escribir un archivo sin permisos (Exit Code -4)
	}
}

int main(void) {
	inicializarTablasDeArchivos();

	//PRUEBAS
	/*t_banderas flags;
	flags.creacion=false;
	flags.escritura=true;
	flags.lectura=true;
	abrirArchivo(90, "Pepin.bin", flags);
	cerrarArchivo(90,3);*/

	//AL ELIMINAR UN PROCESO, HABRÍA QUE ELIMINAR SU TABLA DE ARCHIVOS DE tablasDeArchivosDeProcesos

	return EXIT_SUCCESS;
}
