#include "CapaFS.h"
#include "accionesDeKernel.h"

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

void enviarPathAFS(char *path){
	int tamPath = strlen(path)+1;
	if (send(servFS, &tamPath, sizeof(int), 0) == -1) {
		printf("Error enviando la longitud del Path\n");
		exit(-1);
	}
	esperarSenialDeFS();
	if (send(servFS, path, tamPath, 0) == -1) {
		printf("Error enviando el Path\n");
		exit(-1);
	}
}

u_int32_t abrirArchivo(int PID, /*t_direccion_archivo*/char* direccion, t_banderas flags){
	char* strFlags = string_new();
	if(flags.creacion) string_append(&strFlags, "c");
	if(flags.lectura) string_append(&strFlags, "r");
	if(flags.escritura) string_append(&strFlags, "w");

	bool existeArchivo;
	//Enviar mensaje a FS: existeArchivo = validarArchivo(char* path);
	avisarAccionAFS(k_fs_validar_archivo);
	enviarPathAFS(direccion);
	if (recv(servFS, &existeArchivo, sizeof(bool), 0) == -1) {
		printf("Error recibiendo si el archivo existe\n");
		exit(-1);
	}
	if(!existeArchivo){
		if(string_contains(strFlags,"c")){//El archivo fue abierto en modo creación
			//Enviar mensaje a FS: crearArchivo(direccion);
			avisarAccionAFS(k_fs_crear_archivo);
			enviarPathAFS(direccion);
			//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		} else {
			//Enviar mensaje a CPU: El programa intentó acceder a un archivo que no existe (Exit Code -2)
			return 0;//En realidad 0 es un FD válido, pero como está reservado y no lo usamos lo uso para indicar el error
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

	free(strFlags);

	u_int32_t fileDescriptor = j+3;//Las primeras 3 posiciones de la tabla de archivos del proceso están reservadas (!)
	return fileDescriptor;
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

int borrarArchivo(int PID, u_int32_t fileDescriptor){
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
		avisarAccionAFS(k_fs_borrar_archivo);
		enviarPathAFS(tablaArchivosGlobal[fdGlobal].nombreArchivo);
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		cerrarArchivo(PID,fileDescriptor);
		return k_cpu_accion_OK;
	} else {
		printf("El archivo no puede ser borrado, ya que está abierto por otro proceso\n");
		return k_cpu_error;
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

char* leerArchivo(int PID, u_int32_t fileDescriptor, /*t_valor_variable*/int tamanio){
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
		avisarAccionAFS(k_fs_leer_archivo);
		enviarPathAFS(tablaArchivosGlobal[fdGlobal].nombreArchivo);
		avisarAccionAFS(tablasDeArchivosDeProcesos[PID][posicionReal].offset);//Uso avisarAccion para pasar parámetro
		avisarAccionAFS(tamanio);//Uso avisarAccion para pasar parámetro
		char* bytesLeidos;
		if (recv(servFS, bytesLeidos, tamanio, 0) == -1) {
			printf("Error recibiendo el archivo leido\n");
			exit(-1);
		}
		//Enviar mensaje a CPU con los bytesLeidos, para que lo almacene en su puntero informacion
		return bytesLeidos;

	} else {
		//Enviar mensaje a CPU: El programa intentó leer un archivo sin permisos (Exit Code -3)
		return NULL;
	}
}

int escribirArchivo(int PID, u_int32_t fileDescriptor, char* bytesAEscribir, /*t_valor_variable*/int tamanio){
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
		avisarAccionAFS(k_fs_escribir_archivo);
		enviarPathAFS(tablaArchivosGlobal[fdGlobal].nombreArchivo);
		avisarAccionAFS(tablasDeArchivosDeProcesos[PID][posicionReal].offset);//Uso avisarAccion para pasar parámetro
		avisarAccionAFS(tamanio);//Uso avisarAccion para pasar parámetro
		if (send(servFS, bytesAEscribir, tamanio, 0) == -1) {
			printf("Error enviando los bytes a escribir.\n");
			exit(-1);
		}
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		return k_cpu_accion_OK;
	} else {
		//Enviar mensaje a CPU: El programa intentó escribir un archivo sin permisos (Exit Code -4)
		return k_cpu_error;
	}
}
