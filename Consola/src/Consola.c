#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include "libreriaSockets.h"

#define RUTA_CARPETA_SCRIPTS "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/scripts/"

char *rutaArchivo;// = "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt";

pthread_mutex_t mutexDesconexionMasiva = PTHREAD_MUTEX_INITIALIZER;

enum notificacionesConsolaKernel {
	finalizo_proceso, print, finalizacion_forzosa, confirmacion_de_memoria
};

enum motivos_finalizacion{
	desconexion_consola, consola_de_consola, consola_de_kernel
};

t_list *lista_hilos_por_PID;
t_list *lista_futuras_desconexiones;

struct sockaddr_in direccionServidor;

typedef struct {
	char* ipKernel;
	int puerto;
} t_configuracion;
t_configuracion *config;

typedef struct {
	int hora;
	int minuto;
	int segundo;
	char *fecha;
} tiempo_proceso;

typedef struct {
	pthread_t hilo;
	int PID;
	int serv_kernel;
	char *nombre_script;
	int cantImpresiones;
	tiempo_proceso *tiempo_inicio;
	tiempo_proceso *tiempo_fin;
} hilo_por_programa;

typedef struct{
	int pid;
	int motivo;
}futura_desconexion_consola;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum acciones {
	startProgram, endProgram, futura_desconexion
};

enum confirmacionMem {
	noHayPaginas, hayPaginas
};

void llenarSocket() {
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(config->ipKernel);
	direccionServidor.sin_port = htons(config->puerto);
}

void informarAccion(int *cliente, int *accion) {
	u_int32_t acc = (*accion);
	send((*cliente), &acc, sizeof(u_int32_t), 0);
}

void solicitarA(int *cliente, char *nombreCli) {
	char a[2] = "a";
	send((*cliente), a, 2, 0);
	log_info(consola_log, "Esperando atencion de %s..\n", nombreCli);
	recv((*cliente), a, 2, 0);
}

void msjConexionCon(char *s) {
	log_info(consola_log,
			"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			s);
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		log_error(consola_log, "No hay más espacio");
		exit(-1);
	}
	return puntero;
}

hilo_por_programa *obtener_hilo_por_programa_segun_pid(int pid) {
	bool es_este_pid(hilo_por_programa *hiloPorPrograma) {
		return hiloPorPrograma->PID == pid;
	}
	return list_find(lista_hilos_por_PID, (void*) es_este_pid);
}

hilo_por_programa *crear_hilo_por_programa() {
	hilo_por_programa *hilo_por_PID = reservarMemoria(
			sizeof(hilo_por_programa));
	hilo_por_PID->tiempo_inicio = reservarMemoria(sizeof(tiempo_proceso));
	hilo_por_PID->tiempo_fin = reservarMemoria(sizeof(tiempo_proceso));
	hilo_por_PID->tiempo_inicio->fecha = reservarMemoria(50);
	hilo_por_PID->tiempo_fin->fecha = reservarMemoria(50);
	hilo_por_PID->PID = -1;
	return hilo_por_PID;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->ipKernel = strdup(
			config_get_string_value(archivo_Modelo, "IP_KERNEL"));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
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
		log_error(consola_log, "No se encontró el Archivo");
		//printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(rutaArchivo);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	log_info(consola_log, "Leí el archivo y extraje el puerto: %d ",
			config->puerto);
}

void mostrarConfirmacion(int confirmacion) {
	u_int32_t conf = confirmacion;
	if (conf == hayPaginas) {
		log_info(consola_log,
				"Paginas suficientes - El proceso se almaceno exitosamente");
	} else {
		log_error(consola_log,
				"Paginas insuficientes - El proceso no pudo almacenarse en MP");
	}
}

void esperarConfirmacionDeKernel(int *kernel) {
	u_int32_t confirmacion;
	log_info(consola_log, "Esperando la confirmacion de Kernel..");
	if (recv((*kernel), &confirmacion, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log,
				"Error recibiendo la confirmacion de parte de Kernel");
		exit(-1);
	}
	mostrarConfirmacion(confirmacion);
}

void liberar_hilo_por_programa(hilo_por_programa *un_hilo_por_programa){
	free(un_hilo_por_programa->nombre_script);
	free(un_hilo_por_programa->tiempo_inicio->fecha);
	free(un_hilo_por_programa->tiempo_inicio);
	free(un_hilo_por_programa->tiempo_fin->fecha);
	free(un_hilo_por_programa->tiempo_fin);
	free(un_hilo_por_programa);
}

void quitar_hilo_por_programa(hilo_por_programa *un_hilo_por_programa){
	bool es_el_hilo_por_pid(hilo_por_programa *h){
		return h->PID == un_hilo_por_programa->PID;
	}
	list_remove_and_destroy_by_condition(lista_hilos_por_PID,(void*)es_el_hilo_por_pid,(void*)liberar_hilo_por_programa);
}

void matar_hilo(hilo_por_programa *un_hilo_por_programa, int motivo) {
	close(un_hilo_por_programa->serv_kernel);
	pthread_t thread = un_hilo_por_programa->hilo;
	quitar_hilo_por_programa(un_hilo_por_programa);
	if(motivo == desconexion_consola) pthread_mutex_unlock(&mutexDesconexionMasiva);
	pthread_cancel(thread);
}

void mostrarDiferenciaInicioFinEjecucion(tiempo_proceso *tiempoInicio,
		tiempo_proceso *tiempoFin) {
	int difHoras, difMinutos, difSegundos;
	int segInicio = tiempoInicio->segundo;
	int minInicio = tiempoInicio->minuto;
	int horaInicio = tiempoInicio->hora;
	int segFin = tiempoFin->segundo;
	int minFin = tiempoFin->minuto;
	int horaFin = tiempoFin->hora;

	if (segInicio > segFin) {
		--minFin;
		segFin += 60;
	}

	difSegundos = segFin - segInicio;
	if (minInicio > minFin) {
		--horaFin;
		minInicio += 60;
	}

	difMinutos = minFin - minInicio;
	difHoras = horaFin - horaInicio;

	log_info(consola_log, "Diferencia: Horas:%d Minutos:%d Segundos:%d",
			difHoras, difMinutos, difSegundos);
}

void mostrarTiempoInicioFinDiferencia(tiempo_proceso *tiempoInicio,
		tiempo_proceso *tiempoFin) {
	log_info(consola_log, "Fecha Inicio:%s", tiempoInicio->fecha);
	log_info(consola_log, "Fecha Fin:%s", tiempoFin->fecha);
	mostrarDiferenciaInicioFinEjecucion(tiempoInicio, tiempoFin);
}

void guardarFechaHoraEjecucion(tiempo_proceso *tiempo) {
	struct tm *tm;
	time_t tiempoEnSegundos = time(NULL);
	tm = localtime(&tiempoEnSegundos);
	strcpy(tiempo->fecha, asctime(tm));
	tiempo->hora = tm->tm_hour;
	tiempo->minuto = tm->tm_min;
	tiempo->segundo = tm->tm_sec;
}

u_int32_t recibirPID(int *cliente) {
	u_int32_t pid;
	if (recv((*cliente), &pid, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log, "Error recibiendo el PID");
		exit(-1);
	}
	log_info(consola_log, "PID: %d asignado a ese programa\n", pid);
	return pid;
}

void conectarse_con_kernel(int *serv_kernel) {
	conectar(serv_kernel, &direccionServidor);
	handshake(serv_kernel, consola);
	msjConexionCon("Kernel");
}

void enviarSenialAKernel(int serv_kernel) {
	char senial[2] = "a";
	if (send(serv_kernel, senial, 2, 0) == -1) {
		printf("Error al enviar la senial\n");
		exit(-1);
	}
}

char *obtener_un_mensaje(int serv_kernel) {
	int tamanio;
	char *buffer;
	enviarSenialAKernel(serv_kernel);
	if (recv(serv_kernel, &tamanio, sizeof(int), 0) == -1) {
		printf("Error recibiendo el tamanio del mensaje\n");
	}
	buffer = reservarMemoria(tamanio + 1);
	if (recv(serv_kernel, buffer, tamanio, 0) == -1) {
		printf("Error recibiendo el mensaje\n");
	}
	buffer[tamanio] = '\0';
	enviarSenialAKernel(serv_kernel);
	return buffer;
}

int recibir_accion_de_kernel(int serv_kernel) {
	int accion;
	if (recv(serv_kernel, &accion, sizeof(int), 0) == -1) {
		printf("Error al recibir accion por parte de Kernel\n");
	}
	return accion;
}

void mostrarCantidadImpresiones(int cantImpresiones) {
	log_info(consola_log, "Cantidad de impresiones: %d", cantImpresiones);
}

void mostrar_exit_code_segun_motivo(int motivo){
	switch(motivo){
		case desconexion_consola:
		{
			printf("Exit_Code: -6\n");
			break;
		}
		case consola_de_consola:
		{
			printf("Exit_Code: -7\n");
			break;
		}
		case consola_de_kernel:
		{
			printf("Exit_Code: -13\n");
			break;
		}
	}
}

void avisar_a_kernel_motivo_futura_desconexion(int serv_kernel,int motivo){
	int mot = motivo;
	if(send(serv_kernel,&mot,sizeof(int),0)<0){
		printf("Error al enviar motivo de futura desconexion a Kernel\n");
		exit(-1);
	}
}

void esperar_senial_de_kernel(int serv_kernel){
	char senial[2] = "a";
	if (recv(serv_kernel, senial, 2, 0) == -1) {
		printf("Error al recibir senial de Kernel\n");
	}
}

futura_desconexion_consola *crear_futura_desconexion_consola(int pid,int motivo){
	futura_desconexion_consola *fut_desc = reservarMemoria(sizeof(futura_desconexion_consola));
	fut_desc->pid=pid;
	fut_desc->motivo=motivo;
	return fut_desc;
}

void finalizar_programa_segun_PID(int pid, int motivo) {
	hilo_por_programa *unHiloPorPrograma = NULL;
	unHiloPorPrograma = obtener_hilo_por_programa_segun_pid(pid);

	/*guardarFechaHoraEjecucion(tiempoFin);
	 mostrarTiempoInicioFinDiferencia(tiempoInicio, tiempoFin);
	 mostrarCantidadImpresiones(unHiloPorPrograma->cantImpresiones);*/ // HAY QUE MOSTRAR ESTOS DATOS EH
	if(unHiloPorPrograma == NULL){
		printf("El proceso %d no pertenece a esta consola\n\n", pid);
	}else{
	futura_desconexion_consola *fut_desc = crear_futura_desconexion_consola(pid,motivo);
	int acc = futura_desconexion;
	if(motivo == consola_de_consola || motivo == desconexion_consola){
		printf("*****LLEGUE HASTA ACAAAAAAAA*******\n");
		list_add(lista_futuras_desconexiones,fut_desc);
		char senial[2] = "a";
		send(unHiloPorPrograma->serv_kernel,senial,2,0);
		printf("*****LLEGUE HASTA ACAAAAAAAA2*******\n");
	}else{
		free(fut_desc);
		solicitarA(&(unHiloPorPrograma->serv_kernel),"Kernel");
		informarAccion(&(unHiloPorPrograma->serv_kernel),&acc);
		avisar_a_kernel_motivo_futura_desconexion(unHiloPorPrograma->serv_kernel,motivo);
		esperar_senial_de_kernel(unHiloPorPrograma->serv_kernel);
		guardarFechaHoraEjecucion(unHiloPorPrograma->tiempo_fin);
		mostrarTiempoInicioFinDiferencia(unHiloPorPrograma->tiempo_inicio, unHiloPorPrograma->tiempo_fin);
		mostrarCantidadImpresiones(unHiloPorPrograma->cantImpresiones);
		mostrar_exit_code_segun_motivo(motivo);
		matar_hilo(unHiloPorPrograma,motivo);
		printf("Fue ANIQUILADOX exitosamente el proceso de PID: %d\n\n", pid);
	}
	}
}

int recibir_exit_code(int serv_kernel){
	int exit_code;
	if(recv(serv_kernel,&exit_code,sizeof(int),0)<0){
		printf("Error al recibir el exit_code de parte de kernel\n");
		exit(-1);
	}
	return exit_code;
}

int obtener_motivo_futura_desconexion_segun_PID(int pid){
	bool es_esa_fut_desc(futura_desconexion_consola *f){
		return f->pid == pid;
	}
	futura_desconexion_consola *f_desc = list_find(lista_futuras_desconexiones,(void*)es_esa_fut_desc);
	return f_desc->motivo;
}

void futura_desconexion_consola_destroyer(futura_desconexion_consola *f){
	free(f);
}

void liberar_futura_desconexion_segun_PID(int pid){
	bool es_esa_fut_desc(futura_desconexion_consola *f){
			return f->pid == pid;
	}
	list_remove_and_destroy_by_condition(lista_futuras_desconexiones,(void*)es_esa_fut_desc,(void*)futura_desconexion_consola_destroyer);
}

void recibir_y_mostrar_mensajes(hilo_por_programa *un_hilo_por_programa) {
	enviarSenialAKernel(un_hilo_por_programa->serv_kernel);
	while (1) {
		printf("Esperando accion de KernelSITO desde PID: %d\n",
				un_hilo_por_programa->PID);
		int accion = recibir_accion_de_kernel(
				un_hilo_por_programa->serv_kernel);
		printf("La accion fue: %d\n",accion);
		switch (accion) {
		case finalizo_proceso:
			printf("El programa con PID: %d ha finalizado\n",
					un_hilo_por_programa->PID);
			int exit_code = recibir_exit_code(un_hilo_por_programa->serv_kernel);
			guardarFechaHoraEjecucion(un_hilo_por_programa->tiempo_fin);
			mostrarTiempoInicioFinDiferencia(un_hilo_por_programa->tiempo_inicio, un_hilo_por_programa->tiempo_fin);
			mostrarCantidadImpresiones(un_hilo_por_programa->cantImpresiones);
			printf("Exit_Code: %d\n",exit_code);
			matar_hilo(un_hilo_por_programa,-1);
			break;
		case print: {
			char *mensaje = obtener_un_mensaje(
					un_hilo_por_programa->serv_kernel);
			(un_hilo_por_programa->cantImpresiones)++;
			printf("Mensaje de PID %d: %s\n", un_hilo_por_programa->PID,
					mensaje);
			free(mensaje);
			break;
		}
		case finalizacion_forzosa:
			finalizar_programa_segun_PID(un_hilo_por_programa->PID,consola_de_kernel);
			break;
		case confirmacion_de_memoria:
		{
			//enviarSenialAKernel(un_hilo_por_programa->serv_kernel); // Comentado por el quilombo que hacia un recv en simultaneo compitiendo con el select en kernel.
			esperarConfirmacionDeKernel(&(un_hilo_por_programa->serv_kernel));
			break;
		}
		case 97:
		{
			printf("Entre al case re loco\n");
			int acc = futura_desconexion;
			int motivo = obtener_motivo_futura_desconexion_segun_PID(un_hilo_por_programa->PID);
			informarAccion(&(un_hilo_por_programa->serv_kernel),&acc);
			avisar_a_kernel_motivo_futura_desconexion(un_hilo_por_programa->serv_kernel,motivo);
			esperar_senial_de_kernel(un_hilo_por_programa->serv_kernel);
			guardarFechaHoraEjecucion(un_hilo_por_programa->tiempo_fin);
			mostrarTiempoInicioFinDiferencia(un_hilo_por_programa->tiempo_inicio, un_hilo_por_programa->tiempo_fin);
			mostrarCantidadImpresiones(un_hilo_por_programa->cantImpresiones);
			mostrar_exit_code_segun_motivo(motivo);
			liberar_futura_desconexion_segun_PID(un_hilo_por_programa->PID);
			matar_hilo(un_hilo_por_programa,motivo);
			break;
		}
		default:
			printf("Accion recibida por Kernel invalida.\n");
		}
	}
}

void hacer_muchas_cosas(hilo_por_programa *un_hilo_por_programa) {
	//mostrar_datos_de_hilo_por_programa(un_hilo_por_programa);

	int accion;
	FILE *archivo;
	char *path = reservarMemoria(100);
	strcpy(path, RUTA_CARPETA_SCRIPTS);
	strcat(path, un_hilo_por_programa->nombre_script);

	archivo = fopen(path, "rb");
	if (archivo == NULL) {
		log_error(consola_log, "No se pudo leer el archivo");
		exit(-1);
	}
	free(path);

	fseek(archivo, 0, SEEK_END);
	u_int32_t fsize = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize + 1);
	fread(buffer, fsize, 1, archivo);
	fclose(archivo);
	buffer[fsize] = '\0';
	solicitarA(&(un_hilo_por_programa->serv_kernel), "Kernel");
	accion = startProgram;
	informarAccion(&(un_hilo_por_programa->serv_kernel), &accion);

	guardarFechaHoraEjecucion(un_hilo_por_programa->tiempo_inicio);

	if (send(un_hilo_por_programa->serv_kernel, &fsize, sizeof(u_int32_t), 0)
			== -1) {
		log_error(consola_log, "Error enviando longitud del archivo");
		exit(-1);
	}
	if (send(un_hilo_por_programa->serv_kernel, buffer, fsize + 1, 0) == -1) {
		log_error(consola_log, "Error enviando archivo");
		exit(-1);
	}
	free(buffer);

	log_info(consola_log, "El archivo se envió correctamente\n");

	u_int32_t pid = recibirPID(&(un_hilo_por_programa->serv_kernel));

	un_hilo_por_programa->PID = pid;
	un_hilo_por_programa->cantImpresiones = 0;

	recibir_y_mostrar_mensajes(un_hilo_por_programa);
}

void cormillot(char *lineaIngresada) {
	int i = 0;
	while (lineaIngresada[i] != '\n') {
		i++;
	}
	lineaIngresada[i] = '\0';
}

void iniciarPrograma() {
	int serv_kernel;
	conectarse_con_kernel(&serv_kernel);
	/*Iniciar Programa: Este comando iniciará un nuevo Programa AnSISOP, recibiendo por
	 parámetro el path del script AnSISOP a ejecutar. Una vez iniciado el programa la consola
	 quedará a la espera de nuevos comandos, pudiendo ser el iniciar nuevos Programas AnSISOP
	 o algunas de las siguientes opciones. Quedará a decisión del grupo utilizar paths absolutos o
	 relativos y deberán fundamentar su elección.*/

	int existeArchivo;
	char *lineaIngresada, *nombreScript, *path;

	do {
		printf("\nIngrese nombre del script AnSISOP. \n");
		printf("Ejemplo: script.ansisop\n\n");

		nombreScript = reservarMemoria(100);
		lineaIngresada = reservarMemoria(100);
		path = reservarMemoria(100);

		fgets(lineaIngresada, 100, stdin);
		cormillot(lineaIngresada);
		strcpy(nombreScript, lineaIngresada);
		free(lineaIngresada);
		strcpy(path, RUTA_CARPETA_SCRIPTS);
		strcat(path, nombreScript);
		if (access(path, F_OK) != -1) {
			existeArchivo = 1;
		} else {
			existeArchivo = 0;
			printf("No existe el archivo ingresado :(. Vuelva a intentarlo.\n");
			free(nombreScript);
		}
		free(path);
	} while (!existeArchivo);

	hilo_por_programa *un_hilo_por_programa = crear_hilo_por_programa();
	pthread_t hilo_programa;
	un_hilo_por_programa->nombre_script = nombreScript;
	un_hilo_por_programa->serv_kernel = serv_kernel;
	list_add(lista_hilos_por_PID, un_hilo_por_programa);
	if (pthread_create(&hilo_programa, NULL, hacer_muchas_cosas,
			un_hilo_por_programa)) {
		printf("Error al crear el thread de iniciar programa.\n");
		exit(-1);
	}
	un_hilo_por_programa->hilo = hilo_programa;
}

void finalizarPrograma() {
	/*Finalizar Programa: Como su nombre lo indica este comando finalizará un Programa
	 AnSISOP, terminando el thread correspondiente al PID que se desee finalizar.*/
	char *opcion = reservarMemoria(100);
	int id_proceso_a_detener;

	printf("Ingrese el ID del proceso a finalizar:\n");
	fgets(opcion, 100, stdin);
	id_proceso_a_detener = atoi(opcion);

	finalizar_programa_segun_PID(id_proceso_a_detener,consola_de_consola);
	free(opcion);
}

void matar_todos_los_procesos(){
	while(!list_is_empty(lista_hilos_por_PID)){
		hilo_por_programa *un_hilo_por_programa = list_get(lista_hilos_por_PID,0);
		finalizar_programa_segun_PID(un_hilo_por_programa->PID,desconexion_consola);
		pthread_mutex_lock(&mutexDesconexionMasiva);
	}
}

void desconectarConsola() {
	/*Desconectar Consola: Este comando finalizará la conexión de todos los threads de la consola
	 con el kernel, dando por muertos todos los programas de manera abortiva.*/
	matar_todos_los_procesos();
	log_info(consola_log, "\nConsola desconectada. \n");
	exit(-1);
}

void limpiarMensajes() {
	system("clear");
	log_info(consola_log, "Consola limpiada! \n");
}

void elegirComando() {
	char *opcionIngresada;
	int seguirAbierto = 1;

	do {
		printf("Los siguientes comandos estan disponibles para ejecutar:\n");
		printf("1-iniciarPrograma\n");
		printf("2-desconectarConsola\n");
		printf("3-finalizarPrograma\n");
		printf("4-limpiarMensajes\n");

		printf("Ingrese el numero de comando para ejecutarlo:\n");

		opcionIngresada = reservarMemoria(sizeof(opcionIngresada));
		fgets(opcionIngresada, sizeof(opcionIngresada), stdin);

		switch (*opcionIngresada) {
		case '1':
			iniciarPrograma();
			break;
		case '2':
			desconectarConsola();
			break;
		case '3':
			finalizarPrograma();
			break;
		case '4':
			limpiarMensajes();
			break;
		default:
			log_error(consola_log,
					"\nOpcion invalida. Vuelva a elegir una opcion \n");
			break;
		}

		free(opcionIngresada);
	} while (seguirAbierto);
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

	lista_hilos_por_PID = list_create();
	lista_futuras_desconexiones = list_create();
	inicializarLog();

	leerArchivo();
	free(rutaArchivo);
	pthread_mutex_lock(&mutexDesconexionMasiva);
	llenarSocket();
	elegirComando();

	log_destroy(consola_log);
	return 0;
}
