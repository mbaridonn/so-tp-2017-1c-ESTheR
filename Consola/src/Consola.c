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

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "

enum notificacionesConsolaKernel{
	finalizo_proceso, print
};

t_list *lista_hilos_por_PID;

struct sockaddr_in direccionServidor;

int serv_kernel;

typedef struct {
	char* ipKernel;
	int puerto;
} t_configuracion;
t_configuracion *config;

typedef struct{
	int hora;
	int minuto;
	int segundo;
	char *fecha;
} tiempo_proceso;

typedef struct{
	pthread_t *hilo;
	int PID;
	char *nombre_script;
} hilo_por_programa;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum acciones {
	startProgram, endProgram
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
	log_info(consola_log, "\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n", s);
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		log_error(consola_log, "No hay más espacio");
		exit(-1);
	}
	return puntero;
}

hilo_por_programa *crear_hilo_por_programa(){
	hilo_por_programa *hilo_por_PID = reservarMemoria(sizeof(hilo_por_programa));
	return hilo_por_PID;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->ipKernel = strdup(config_get_string_value(archivo_Modelo, "IP_KERNEL"));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
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
		log_error(consola_log, "No se encontró el Archivo");
		//printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	log_info(consola_log, "Leí el archivo y extraje el puerto: %d ", config->puerto);
}

void mostrarConfirmacion(int confirmacion) {
	u_int32_t conf = confirmacion;
	if (conf == hayPaginas) {
		log_info(consola_log, "Paginas suficientes - El proceso se almaceno exitosamente");
	} else {
		log_error(consola_log, "Paginas insuficientes - El proceso no pudo almacenarse en MP");
	}
}

void esperarConfirmacionDeKernel(int *kernel) {
	u_int32_t confirmacion;
	log_info(consola_log, "Esperando la confirmacion de Kernel..");
	if (recv((*kernel), &confirmacion, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log, "Error recibiendo la confirmacion de parte de Kernel");
		exit(-1);
	}
	mostrarConfirmacion(confirmacion);
}

void mostrarDiferenciaInicioFinEjecucion(tiempo_proceso *tiempoInicio, tiempo_proceso *tiempoFin) {
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

	log_info(consola_log, "Diferencia: Horas:%d Minutos:%d Segundos:%d", difHoras,difMinutos,difSegundos);
}

void mostrarTiempoInicioFinDiferencia(tiempo_proceso *tiempoInicio, tiempo_proceso  *tiempoFin){
	printf("Fecha Inicio:%s\n",tiempoInicio->fecha);
	printf("Fecha Fin:%s\n",tiempoFin->fecha);
	mostrarDiferenciaInicioFinEjecucion(tiempoInicio,tiempoFin);
}

void guardarFechaHoraEjecucion(tiempo_proceso *tiempo){
	struct tm *tm;
	time_t tiempoEnSegundos = time(NULL);
	tm = localtime(&tiempoEnSegundos);
	strcpy(tiempo->fecha,asctime(tm));
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
	log_info(consola_log, "PID: %d asignado a ese programa\n",pid);
	return pid;
}

void conectarse_con_kernel(){
	conectar(&serv_kernel, &direccionServidor);
	int procesoConectado = handshake(&serv_kernel, consola);
	msjConexionCon("Kernel");
}

char *obtener_un_mensaje(){
	u_int32_t tamanio;
	char *buffer;
	if(recv(serv_kernel,&tamanio,sizeof(u_int32_t),0) == -1){
		printf("Error recibiendo el tamanio del mensaje\n");
	}
	buffer = reservarMemoria(tamanio);
	if(recv(serv_kernel,buffer,tamanio,0) == -1){
		printf("Error recibiendo el mensaje\n");
	}
	return buffer;
}

int recibir_accion_de_kernel(){
	int accion;
	if(recv(serv_kernel,&accion,sizeof(int),0) == -1){
		printf("Error al recibir accion por parte de Kernel\n");
	}
	return accion;
}

void matar_hilo(pthread_t *hilo){
	pthread_cancel(*hilo);
}

void recibir_y_mostrar_mensajes(hilo_por_programa *un_hilo_por_programa,tiempo_proceso *tiempoInicio,tiempo_proceso *tiempoFin){
	while(1){
		int accion = recibir_accion_de_kernel();
		switch(accion){
		case finalizo_proceso:
			printf("El programa con PID: %d ha finalizado\n",un_hilo_por_programa->PID);
			guardarFechaHoraEjecucion(tiempoFin);
			mostrarTiempoInicioFinDiferencia(tiempoInicio,tiempoFin);
			matar_hilo(un_hilo_por_programa->hilo);
			break;
		case print:
		{
			char *mensaje = obtener_un_mensaje();
			printf("%s",mensaje);
			free(mensaje);
			break;
		}
		default:
			printf("Accion recibida por Kernel invalida.\n");
		}
	}
}

hilo_por_programa *obtener_hilo_por_programa_segun_hilo(pthread_t *hilo){
	bool es_este_hilo(hilo_por_programa *hiloPorPrograma){
		return *(hiloPorPrograma->hilo) == *hilo;
	}
	return list_find(lista_hilos_por_PID,(void*) es_este_hilo);
}

void hacer_muchas_cosas(hilo_por_programa *un_hilo_por_programa){
	tiempo_proceso *tiempoInicio = reservarMemoria(sizeof(tiempo_proceso));
	tiempo_proceso *tiempoFin = reservarMemoria(sizeof(tiempo_proceso));
	tiempoInicio->fecha = reservarMemoria(50);
	tiempoFin->fecha = reservarMemoria(50);

	int accion;
	FILE *archivo;
	printf("El nombre del script es: %s\n",un_hilo_por_programa->nombre_script);
	archivo = fopen(un_hilo_por_programa->nombre_script, "rb"); //USAR PATH ABSOLUTO?
	if (archivo == NULL) {
		log_error(consola_log, "No se pudo leer el archivo");
		exit(-1);
	}

	fseek(archivo, 0, SEEK_END);
	u_int32_t fsize = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize + 1);
	fread(buffer, fsize, 1, archivo);
	fclose(archivo);
	buffer[fsize] = '\0';
	solicitarA(&serv_kernel, "Kernel");
	accion = startProgram;
	informarAccion(&serv_kernel, &accion);

	guardarFechaHoraEjecucion(tiempoInicio);

	if (send(serv_kernel, &fsize, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log, "Error enviando longitud del archivo");
		exit(-1);
	}
	if (send(serv_kernel, buffer, fsize + 1, 0) == -1) {
		log_error(consola_log, "Error enviando archivo");
		exit(-1);
	}
	free(buffer);

	log_info(consola_log, "El archivo se envió correctamente\n");

	esperarConfirmacionDeKernel(&serv_kernel);
	u_int32_t pid = recibirPID(&serv_kernel);

	un_hilo_por_programa->PID = pid;

	recibir_y_mostrar_mensajes(un_hilo_por_programa,tiempoInicio,tiempoFin);

	free(tiempoInicio->fecha);
	free(tiempoInicio);
	free(tiempoFin->fecha);
	free(tiempoFin);
}

void cormillot(char *lineaIngresada){
	int i = 0;
	while(lineaIngresada[i]!='\n'){
		i++;
	}
	lineaIngresada[i]='\0';
}

void iniciarPrograma() {
	conectarse_con_kernel();
	/*Iniciar Programa: Este comando iniciará un nuevo Programa AnSISOP, recibiendo por
	 parámetro el path del script AnSISOP a ejecutar. Una vez iniciado el programa la consola
	 quedará a la espera de nuevos comandos, pudiendo ser el iniciar nuevos Programas AnSISOP
	 o algunas de las siguientes opciones. Quedará a decisión del grupo utilizar paths absolutos o
	 relativos y deberán fundamentar su elección.*/

	char *lineaIngresada, *comando, *nombreScript;
	nombreScript = reservarMemoria(100);

	lineaIngresada = reservarMemoria(100);

	printf("\nIngrese nombre del script AnSISOP. \n");
	printf("Ejemplo: script.ansisop\n\n");
	fgets(lineaIngresada, 100, stdin);
	cormillot(lineaIngresada);
	strcpy(nombreScript,lineaIngresada);
	free(lineaIngresada);

	hilo_por_programa *un_hilo_por_programa = crear_hilo_por_programa();
	pthread_t *hilo_programa;
	un_hilo_por_programa->hilo = hilo_programa;
	un_hilo_por_programa->nombre_script = nombreScript;
	list_add(lista_hilos_por_PID,un_hilo_por_programa);
	printf("El nombre del script es: %s\n",un_hilo_por_programa->nombre_script);
		if(pthread_create(hilo_programa,NULL,hacer_muchas_cosas,un_hilo_por_programa)){
		printf("Error al crear el thread de iniciar programa.\n");
		exit(-1);
	}
}

void finalizarPrograma(int *cliente) {
	/*Finalizar Programa: Como su nombre lo indica este comando finalizará un Programa
		 AnSISOP, terminando el thread correspondiente al PID que se desee finalizar.*/
	int accion;
	char *opcion = reservarMemoria(100);
	int id_proceso_a_detener;

	solicitarA(cliente, "Kernel");
	accion = endProgram;
	informarAccion(cliente, &accion);

	printf("Ingrese el ID del proceso a finalizar: ");
	fgets(opcion, 100, stdin);
	id_proceso_a_detener = atoi(opcion);

	send(*cliente, &id_proceso_a_detener, sizeof(int), 0);

	free(opcion);
}


void desconectarConsola() {
	/*Desconectar Consola: Este comando finalizará la conexión de todos los threads de la consola
	 con el kernel, dando por muertos todos los programas de manera abortiva.*/
	log_info(consola_log, "\nConsola desconectada. \n");
	exit(-1);
}

void limpiarMensajes() {
	system("clear");
	log_info(consola_log,"Consola limpiada! \n");
}

void hilo_iniciar_programa(){
	pthread_t *hilo_iniciar_programa;
	if(pthread_create(hilo_iniciar_programa,NULL,iniciarPrograma,hilo_iniciar_programa)){
		printf("Error al crear el thread de iniciar programa.\n");
		exit(-1);
	}
	hilo_por_programa *hilo_por_PID = crear_hilo_por_programa();
	hilo_por_PID->hilo = hilo_iniciar_programa;
	list_add(lista_hilos_por_PID,hilo_por_PID);
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
			finalizarPrograma(&serv_kernel);
			break;
		case '4':
			limpiarMensajes();
			break;
		default:
			log_error(consola_log, "\nOpcion invalida. Vuelva a elegir una opcion \n");
			break;
		}

		free(opcionIngresada);
	} while (seguirAbierto);
}

int main(void) {

	lista_hilos_por_PID = list_create();
	inicializarLog();

	leerArchivo();
	llenarSocket();
	elegirComando();

	log_destroy(consola_log);
	return 0;
}
