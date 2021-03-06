#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <commons/config.h>
#include <unistd.h>
#include "libreriaSockets.h"
#include "conexionesSelect.h"
#include "lib/CapaFS.h"

char *rutaArchivo;// = "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Kernel/src/ConfigKernel.txt";

u_int32_t planificacionActivada = 1;

void inicializarSemaforos(t_config *archivo_Modelo){
	config->SEM_IDS = config_get_array_value(archivo_Modelo, "SEM_IDS");
	char **strSEM_INIT = config_get_array_value(archivo_Modelo, "SEM_INIT");//Hay que convertir los strings en ints
	int i=0, cantSemaforos = 0;
	while (strSEM_INIT[i]!=NULL){
		cantSemaforos++;
		i++;
	}
	config->SEM_INIT = reservarMemoria(cantSemaforos * sizeof(int));
	for(i=0;i<cantSemaforos;i++) config->SEM_INIT[i] = atoi(strSEM_INIT[i]);
}

void inicializarVariablesCompartidas(t_config *archivo_Modelo){
	config->SHARED_VARS = config_get_array_value(archivo_Modelo, "SHARED_VARS");
	int i =0, cantVariables = 0;
	while (config->SHARED_VARS[i]!=NULL){
		cantVariables++;
		i++;
	}
	config->SHARED_VARS_VALUES = reservarMemoria(cantVariables * sizeof(int));
	for(i=0;i<cantVariables;i++) config->SHARED_VARS_VALUES[i] = 0;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->PUERTO_PROG = config_get_int_value(archivo_Modelo, "PUERTO_PROG");
	config->PUERTO_CPU = config_get_int_value(archivo_Modelo, "PUERTO_CPU");
	char *aux = config_get_string_value(archivo_Modelo, "IP_MEMORIA");
	strcpy(config->IP_MEMORIA, aux);
	config->PUERTO_MEMORIA = config_get_int_value(archivo_Modelo,"PUERTO_MEMORIA");
	aux = config_get_string_value(archivo_Modelo, "IP_FS");
	strcpy(config->IP_FS, aux);
	config->PUERTO_FS = config_get_int_value(archivo_Modelo, "PUERTO_FS");
	config->QUANTUM = config_get_int_value(archivo_Modelo, "QUANTUM");
	config->QUANTUM_SLEEP = config_get_int_value(archivo_Modelo,"QUANTUM_SLEEP");
	aux = config_get_string_value(archivo_Modelo, "ALGORITMO");
	strcpy(config->ALGORITMO, aux);
	config->GRADO_MULTIPROG = config_get_int_value(archivo_Modelo,"GRADO_MULTIPROG");
	config->STACK_SIZE = config_get_int_value(archivo_Modelo, "STACK_SIZE");
	inicializarSemaforos(archivo_Modelo);
	inicializarVariablesCompartidas(archivo_Modelo);
}

void leerArchivoConfig() {
	if (access(rutaArchivo, F_OK) == -1) {
		log_error(kernel_log, "No se encontró el Archivo ");
		//printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(rutaArchivo);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	log_info(kernel_log, "Leí el archivo y extraje el puerto: %d\n", config->PUERTO_PROG);
	//printf("Leí el archivo y extraje el puerto: %d\n", config->PUERTO_PROG);
}

void faltaDeParametros(int argc) {
	if (argc == 1) {
		//log_error(kernel_log, "Te falto el parametro");
		printf("Te falto el parametro  \n");
	}
	exit(-1);
}

void conectarseConMemoria(int *servMemoria,	struct sockaddr_in *direccionServidor2) {
	conectar(servMemoria, direccionServidor2);
	handshake(servMemoria, kernel);
	msjConexionCon("una Memoria");
}

void conectarseConFS(int *servFS, struct sockaddr_in *direccionServidorFS) {
	conectar(servFS, direccionServidorFS);
	handshake(servFS, kernel);
	msjConexionCon("un FileSystem");
}

int obtenerTamanioDePagina(int *servMemoria) {
	int tamPaginaMemoria;
	printf("Estoy Pansado el tamanio");
	if (recv((*servMemoria), &tamPaginaMemoria, sizeof(int), 0) == -1) {
		log_error(kernel_log, "Error recibiendo longitud del archivo");
		//printf("Error recibiendo longitud del archivo\n");
		return -1;
	}
	log_info(kernel_log, "El tamanio recibido es: %d\n", tamPaginaMemoria);
	//printf("El tamanio recibido es: %d\n", tamPaginaMemoria);
	return tamPaginaMemoria;
}

cliente_CPU *crearClienteCPU(int *c_cpu) {
	cliente_CPU *nuevaCPU;
	nuevaCPU = reservarMemoria(sizeof(cliente_CPU));
	nuevaCPU->clie_CPU = (*c_cpu);
	nuevaCPU->libre = 1;
	return nuevaCPU;
}

bool hayProcesosReady() {
	return !list_is_empty(listaPCBs_READY);
}

bool hayProcesosNew() {
	return !list_is_empty(listaPCBs_NEW);
}

int cantidad_procesos_en_memoria(){
	return cant_historica_procesos_memoria - cant_procesos_finalizados - cant_procesos_detenidos;
}

void enviar_un_PCB_a_CPU(t_pcb *pcb, int *unaCPU) {
	//PARA CPU
	//Serializo el PCB y lo envio a CPU
	//abstraer la asquerosidad de abajo

	void * serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb, &serialized_pcb, &serialized_buffer_index);

	if (send((*unaCPU), &serialized_buffer_index, (size_t) sizeof(int), 0)< 0) {
		log_error(kernel_log, "Send serialized_buffer_length to CPU failed");
		//printf("Send serialized_buffer_length to CPU failed\n");
		exit(-1);
	}
	esperarSenialDeCPU(unaCPU);
	if (send((*unaCPU), serialized_pcb, (size_t) serialized_buffer_index, MSG_WAITALL)< 0) {
		log_error(kernel_log, "Send serialized_pcb to CPU failed");
		//printf("Send serialized_pcb to CPU failed\n");
		exit(-1);
	}
	log_info(kernel_log, "PCB enviado a CPU exitosamente");
	//printf("PCB enviado a CPU exitosamente\n");
}

void cliente_CPU_destroy(cliente_CPU *puntero){
	free(puntero);
}

void liberarSiEraCPU(int proceso){
	if(proceso==cpu){
		bool esElClienteActual(cliente_CPU *unaCPU) {
				return unaCPU->clie_CPU == sd;
			}
		list_remove_and_destroy_by_condition(listaCPUs,(void*) esElClienteActual, (void*) cliente_CPU_destroy);
	}
}

int hay_CPUs_Conectadas() {
	return !list_is_empty(listaCPUs);
}

cliente_CPU *obtenerCPUDesocupada() {
	bool estaDesocupada(cliente_CPU *unaCPU) {
				return unaCPU->libre == 1;
	}
	return list_find(listaCPUs, (void*) estaDesocupada);
}

/*void asignarProcesosSegunRoundRobin() {
	printf("************ Planificando segun RR *****************\n");
	cliente_CPU *cpuDesocupada;
		t_pcb *pcb;
	while (1) {
			if (hayProcesosReady() && planificacionActivada) {
				cpuDesocupada = obtenerCPUDesocupada();
				if(cpuDesocupada==NULL)break;
				cpuDesocupada->libre = 0;
				pcb = list_get(listaPCBs_READY, 0);
				transicion_colas_proceso(listaPCBs_READY,listaPCBs_EXEC,pcb);
				enviar_un_PCB_a_CPU(pcb, &cpuDesocupada->clie_CPU);
			}
	}
}*/

bool str_compare(char vec1[],char vec2[]){
	int i = 0;
	if(strlen(vec1)!=strlen(vec2)){
		return false;
	}
	while(vec1[i]!='\0'){
		if(vec1[i]!=vec2[i]){
			return false;
		}
		i++;
	}
	return true;
}

void inicializarVariablesGlobales(){
	if(strcmp(rutaArchivo,"/home/utnso/git/tp-2017-1c-C-digo-Facilito/Kernel/src/configKernelCompleta") == 0){
		config->SEM_INIT[2]++;
	}
}

void mostrar_tipo_de_algoritmo_de_planificacion(){
	if (str_compare("FIFO", config->ALGORITMO)) {
			printf("*******************Planificando segun FIFO*******************\n");
	} else{
			printf("*******************Planificando segun Round Robin*******************\n");
	}
}

bool hay_alguna_cpu_disponible(){
	bool estaDesocupada(cliente_CPU *unaCPU) {
			return unaCPU->libre == 1;
	}
	return list_any_satisfy(listaCPUs, (void*) estaDesocupada);
}

bool proceso_new_puede_pasar_a_READY(){
	return hayProcesosNew() && cantidad_procesos_en_memoria() < config->GRADO_MULTIPROG;
}

bool proceso_ready_puede_pasar_a_EXEC(){
	return hayProcesosReady() && hay_alguna_cpu_disponible();
}

pedido_script *obtener_pedido_segun_PID(int pid){
	bool tieneEstePID(pedido_script *pedido){
			return pedido->pid==pid;
	}
	return list_find(lista_pedidos_script, (void*) tieneEstePID);
}

int enviar_programa_a_memoria(t_pcb *pcb){
	avisarAccionAMemoria(k_mem_inicializarPrograma);//FALTA SLEEP (PENDIENTE!!)
	u_int32_t cant_pags = pcb->cant_paginas_de_codigo + config->STACK_SIZE;
	kernel_mem_start_process(&(pcb->id_proceso), &cant_pags);
	u_int32_t confirmacion = confirmacionMemoria();
	pedido_script *pedido = obtener_pedido_segun_PID(pcb->id_proceso);
	enviarArchivoAMemoria(pedido->bufferArchivo, pedido->fsize);
	esperarSenialDeMemoria();
	free(pedido->bufferArchivo);
	return confirmacion;
}

void pedido_destroy(pedido_script *pedido){
	free(pedido);
}

void eliminar_pedido(pedido_script *pedido){
	bool tieneEstePID(pedido_script *unPedido){
				return pedido->pid==unPedido->pid;
	}
	list_remove_and_destroy_by_condition(lista_pedidos_script,tieneEstePID,pedido_destroy);
}

void avisar_a_consola_si_hubo_exito(int confirmacion, t_pcb *pcb){
	pedido_script *pedido = obtener_pedido_segun_PID(pcb->id_proceso);
	avisar_accion_a_consola(pedido->clie_consola,confirmacion_de_memoria);
	//esperarSenialDeCPU(&(pedido->clie_consola)); // Al esperar una senial y al estar el select esperando movimiento hay quilombo.
	avisarAConsolaSegunConfirmacion(confirmacion, &(pedido->clie_consola));
	proceso_por_cliente *proc_del_cliente = crear_proceso_por_cliente(pcb->id_proceso, pedido->clie_consola);
	list_add(lista_proceso_por_cliente, proc_del_cliente);
	eliminar_pedido(pedido);
}

void planificar() {
	cliente_CPU *cpuDesocupada;
	t_pcb *pcb;
	mostrar_tipo_de_algoritmo_de_planificacion();
		printf("Voy a planificar\n");
		if(planificacionActivada){
			while(proceso_new_puede_pasar_a_READY() || proceso_ready_puede_pasar_a_EXEC()){
				if(proceso_new_puede_pasar_a_READY()){
					pcb = list_get(listaPCBs_NEW,0);
					int confirmacion = enviar_programa_a_memoria(pcb);
					tomarAccionSegunConfirmacion(confirmacion, pcb);
					avisar_a_consola_si_hubo_exito(confirmacion,pcb);
					cant_historica_procesos_memoria++;
					estadisticas_proceso *estadisticas = crear_estadisticas_para(pcb->id_proceso);
					list_add(lista_estadisticas_de_procesos,estadisticas);
				}
				if(proceso_ready_puede_pasar_a_EXEC()){
					cpuDesocupada = obtenerCPUDesocupada();
					cpuDesocupada->libre = 0;
					pcb = list_get(listaPCBs_READY, 0);
					transicion_colas_proceso(listaPCBs_READY,listaPCBs_EXEC,pcb);
					enviar_un_PCB_a_CPU(pcb, &cpuDesocupada->clie_CPU);
				}
			}
		}

}

void abrirHiloPlanificador() {
	pthread_t hilo_comandos;
	if (pthread_create(&hilo_comandos, NULL, planificar, NULL)) {
		log_error(kernel_log, "Error al crear el thread de planificación");
		//printf("Error al crear el thread de planificación.\n");
		exit(-1);
	}
}

void limpiarMensajes() {
	system("clear");
	//log_info(kernel_log, "Consola limpiada!\n");
	printf("Consola limpiada!\n");
}

void mostrar_info_de(int PID){
	bool es_estadistica_de(estadisticas_proceso *est){
				return est->pid == PID;
	}
	estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
	log_info(kernel_log, "Cantidad de rafagas ejecutadas: %d",estadisticas->cant_rafagas_ejec);
	//printf("Cantidad de rafagas ejecutadas: %d\n",estadisticas->cant_rafagas_ejec);
	log_info(kernel_log, "Cantidad de operaciones privilegiadas ejecutadas: %d",estadisticas->cant_syscalls_ejec);
	//printf("Cantidad de operaciones privilegiadas ejecutadas: %d\n",estadisticas->cant_syscalls_ejec);
	log_info(kernel_log, "Cantidad de acciones alocar: %d y cantidad de bytes alocados: %d",estadisticas->cant_op_alocar,estadisticas->cant_bytes_alocados);
	//printf("Cantidad de acciones alocar: %d y cantidad de bytes alocados: %d\n",estadisticas->cant_op_alocar,estadisticas->cant_bytes_alocados);
	log_info(kernel_log, "Cantidad de acciones liberar: %d y cantidad de bytes liberados: %d",estadisticas->cant_op_liberar,estadisticas->cant_bytes_liberados);
	//printf("Cantidad de acciones liberar: %d y cantidad de bytes liberados: %d\n",estadisticas->cant_op_liberar,estadisticas->cant_bytes_liberados);
	int *lista_de_FDs;
	lista_de_FDs = obtener_FDs_de_proceso(PID);
	int i = 0;
	printf("File Descriptors del proceso: ");
	while(lista_de_FDs[i]!=0){
		printf("%d ",lista_de_FDs[i]);
		i++;
	}
	if(lista_de_FDs[0]==0){
		//log_error(kernel_log, "El proceso no tiene abierto ningun archivo.");
		printf("El proceso no tiene abierto ningun archivo.\n");
	}
	printf("\n");
	free(lista_de_FDs);
}

void list_read_id(t_list *listaProcesos){
	int i = 0;
	t_pcb *pcb;
	if(list_size(listaProcesos)==0){
		log_info(kernel_log, "Actualmente no hay\n");
		//printf("Actualmente no hay\n");
	}
	while(i < list_size(listaProcesos)){
		pcb = list_get(listaProcesos,i);
		//log_info(kernel_log, "%d ",pcb->id_proceso);
		printf("%d ",pcb->id_proceso);
		i++;
	}
	printf("\n\n");
}

cliente_CPU *obtenerClienteCPUSegunFD(int fd){
	bool tieneEsteFD(cliente_CPU *c){
		 return c->clie_CPU == fd;
	}
	return list_find(listaCPUs, (void*) tieneEsteFD);
}

void finalizar_ejecucion_de_proceso(int *pid){
	list_add(lista_detenciones_pendientes,pid);
	if(esta_ejecutandose(*pid)){
				printf("El proceso esta ejecutandose, este finalizara cuando CPU lo libere.\n");
	}else{
		t_list *lista = lista_que_tiene_este_pcb(*pid);
		t_pcb *pcb = obtener_PCB_segun_PID_en(lista,*pid);
		finalizarUnProceso(pcb);
		eliminar_detencion(pcb);
	}
}

void enviar_accion_a_consola(int clie_consola,int accion){
	int una_accion = accion;
	if(send(clie_consola,&una_accion,sizeof(int),0) <0){
		log_error(kernel_log, "Error enviando accion a consola");
		//printf("Error enviando accion a consola\n");
		exit(-1);
	}
}

void realizar_finalizacion_forsoza(int pid){
	int clie_consola = obtener_cliente_segun_PID(pid);
	enviar_accion_a_consola(clie_consola,finalizacion_forzosa);
}

void habilitarConsolaKernel() {
	char* lineaIngresada;
	char* subcomando;
	int seguirAbierto = 1;
	do {
		lineaIngresada = reservarMemoria(100);
		subcomando = reservarMemoria(100);
	printf("Comandos: (l)istado procesos, (i)nfo proceso, (t)abla global, \n "
		             "(g)rado multiprogramación, (f)inalizar proceso, (d)etener planificación\n"
		             "(c)lear\n");

		char comando;
		fgets(lineaIngresada, 100, stdin);
		comando = lineaIngresada[0];
		switch (comando) {
		case 'l':
		{
			printf("Listado de procesos:\n"
							"-System: Todos los del Sistema\n"
							"-New: Cola de Nuevos\n"
							"-Ready: Cola de Listos\n"
							"-Exec: Cola de Ejecución\n"
							"-Block: Cola de Bloqueados\n"
							"-Exit: Cola de Terminados\n");
			fgets(subcomando, 100, stdin);
			subcomando = strtok(subcomando, "\n");
			if (strcmp("System", subcomando) == 0) {
				printf("Procesos del Sistema:\n");
				printf("Lista de procesos NEW:\n");
				list_read_id(listaPCBs_NEW);
				printf("Lista de procesos READY:\n");
				list_read_id(listaPCBs_READY);
				printf("Lista de procesos EXEC:\n");
				list_read_id(listaPCBs_EXEC);
				printf("Lista de procesos BLOCK:\n");
				list_read_id(listaPCBs_BLOCK);
				printf("Lista de procesos EXIT:\n");
				list_read_id(listaPCBs_EXIT);

			} else if (strcmp("New", subcomando) == 0) {
				printf("Lista de procesos Nuevos:\n");
				list_read_id(listaPCBs_NEW);

			} else if (strcmp("Ready", subcomando) == 0) {
				printf("Lista de procesos Listos:\n");
				list_read_id(listaPCBs_READY);

			} else if (strcmp("Exec", subcomando) == 0) {
				printf("Lista de procesos Ejecución:\n");
				list_read_id(listaPCBs_EXEC);

			} else if (strcmp("Block", subcomando) == 0) {
				printf("Lista de procesos Bloqueados:\n");
				list_read_id(listaPCBs_BLOCK);

			} else if (strcmp("Exit", subcomando) == 0) {
				printf("Lista de procesos Terminados:\n");
				list_read_id(listaPCBs_EXIT);

			} else {
				printf("Comando invalido\n");
			}

			break;
		}
		case 'i':
		{
			printf("Info");
			printf("Ingrese el ID del proceso para obtener info: ");
			char *opcion = reservarMemoria(100);
			fgets(opcion, 100, stdin);
			mostrar_info_de(atoi(opcion));
			t_list *lista = lista_que_tiene_este_pcb(atoi(opcion));
			t_pcb *pcb = obtener_PCB_segun_PID_en(lista,atoi(opcion));
			printf("El exit code del proceso es: %d\n",pcb->exit_code);
			//printf("EL FAMOSO EXIT_CODE ES: %d\n",pcb->exit_code);
			free(opcion);
			break;
		}
		case 't':
		{
			printf("Tabla Global:");
			int i = 0, j=0;
				for(i=0;i<CANT_PROC_TABLA_ARCH;i++){
					for(j=0;j<CANT_ARCH_TABLA_ARCH;j++){
						printf("%d ",tablasDeArchivosDeProcesos[i][j].fdGlobal);
					}
					printf("\n");
				}
			break;
		}
		case 'g':
		{
			printf("Ingrese el nuevo grado de multiprogramación: ");
			char *opcion = reservarMemoria(100);
			fgets(opcion, 100, stdin);
			int viejo_grado_multiprog = config->GRADO_MULTIPROG;
			config->GRADO_MULTIPROG = atoi(opcion);
			/*if(config->GRADO_MULTIPROG > viejo_grado_multiprog){
				int i;
				for(i=0;i<(config->GRADO_MULTIPROG - viejo_grado_multiprog);i++){
					printf("Es verdad es mayor\n");
					signal();
				}
			}*/ //Esta era la idea principal, se ve que el signal no se "acumula"
			if(config->GRADO_MULTIPROG > viejo_grado_multiprog)planificar();
			log_info(kernel_log, "El nuevo grado de multiprogramación es: %s\n", opcion);
			//printf("El nuevo grado de multiprogramación es: %s\n", opcion);
			free(opcion);
			break;
		}
		case 'f':
		{
			printf("Ingrese el ID del proceso a finalizar: ");
			char *opcion = reservarMemoria(100);
			fgets(opcion, 100, stdin);
			int id_proceso_a_detener = atoi(opcion);
			int consola = obtener_cliente_segun_PID(id_proceso_a_detener);
			pid_nuevo_exit_code *p_n_e_c = crear_pid_nuevo_exit_code(id_proceso_a_detener,-13);
			list_add(lista_pids_con_nuevos_exit_code,p_n_e_c);
			finalizar_programas_de(consola);
			free(opcion);
			break;

		}
		case 'd':
		{
			planificacionActivada = 0;
			printf("Planificación detenida\n");
				break;
		}
		case 'c':
		{
			limpiarMensajes();
				break;
		}
		default:
		{
			log_error(kernel_log, "'%c' no es un comando valido\n", comando);
			//printf("'%c' no es un comando valido\n", comando);
			break;
		}
		}
		free(lineaIngresada);
		free(subcomando);
	} 	while(seguirAbierto);
}

void abrirHiloConsolaKernel(){
	pthread_t hilo_consola_kernel;
	if(pthread_create(&hilo_consola_kernel,NULL,habilitarConsolaKernel,NULL)){
		log_error(kernel_log, "Error al crear el thread de Consola para Kernel.");
		//printf("Error al crear el thread de Consola para Kernel.\n");
		exit(-1);
	}
}

void enviar_planificacion(int cpu){
	int algoritmo = RR;
	if(str_compare("FIFO", config->ALGORITMO)){
		algoritmo = FIFO;
	}
	if(send(cpu,&algoritmo,sizeof(int),0) == -1){
		log_error(kernel_log, "Error al enviar el tipo de algoritmo a CPU.");
		//printf("Error al enviar el tipo de algoritmo a CPU.\n");
		exit(-1);
	}
}

void enviar_quantum(int cpu){
	int quantum = config->QUANTUM;
	if(str_compare("FIFO", config->ALGORITMO)){
		quantum = -1;
	}
	if(send(cpu,&quantum,sizeof(int),0) == -1){
		log_error(kernel_log, "Error al enviar el quantum a CPU.");
		//printf("Error al enviar el quantum a CPU.\n");
		exit(-1);
	}
}

void enviar_quantum_sleep(int cpu){
	if(send(cpu,&config->QUANTUM_SLEEP,sizeof(int),0) == -1){
		log_error(kernel_log, "Error al enviar el quantum a CPU.");
		//printf("Error al enviar el quantum a CPU.\n");
		exit(-1);
	}
}

void enviar_planificacion_y_quantum(int cpu){
	enviar_planificacion(cpu);
	enviar_quantum(cpu);
	enviar_quantum_sleep(cpu);
}

void inicializar_contadores_procesos(){
	cant_historica_procesos_memoria = 0;
	cant_procesos_finalizados = 0;
	cant_procesos_detenidos = 0;
}

/*void eliminar_procesos_por_cliente_de(int clie_consola){
	bool es_el_proceso_de(proceso_por_cliente *proc){
		return proc->clie_consola == clie_consola;
	}
	list_remove_and_destroy_by_condition(lista_proceso_por_cliente,(void *)es_el_proceso_de, (void*)proceso_por_cliente_destroy);
}*/

int finalizar_programas_de(int clie_consola){
	int finalizo_algun_proceso = 0;
	int i;
	proceso_por_cliente *proc_por_clie;
	for(i=0; i<list_size(lista_proceso_por_cliente); i++){
		proc_por_clie = list_get(lista_proceso_por_cliente, i);
		if(proc_por_clie->clie_consola == clie_consola){
			finalizar_ejecucion_de_proceso(&(proc_por_clie->pid));
			finalizo_algun_proceso = 1;
		}
	}
	return finalizo_algun_proceso;
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
	inicializarTablasDeArchivos();
	inicializar_tablaHeap();
	inicializar_contadores_procesos();

	leerArchivoConfig();
	inicializarVariablesGlobales();
	free(rutaArchivo);

	int i, procesoConectado;

	listaPCBs_NEW = list_create();
	listaPCBs_READY = list_create();
	listaPCBs_EXEC = list_create();
	listaPCBs_BLOCK = list_create();
	listaPCBs_EXIT = list_create();
	listaCPUs = list_create();
	lista_pedidos_script = list_create();
	lista_detenciones_pendientes = list_create();
	lista_bloqueos = list_create();
	lista_estadisticas_de_procesos = list_create();
	lista_proceso_por_cliente = list_create();
	lista_futuras_desconexiones = list_create();
	lista_pids_con_nuevos_exit_code = list_create();

	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = INADDR_ANY;
	direccionServidor.sin_port = htons(config->PUERTO_PROG);

	//PARA MEMORIA
	struct sockaddr_in direccionServidor2;
	direccionServidor2.sin_family = AF_INET;
	direccionServidor2.sin_addr.s_addr = inet_addr(config->IP_MEMORIA);
	direccionServidor2.sin_port = htons(config->PUERTO_MEMORIA);

	//PARA FileSystem
	struct sockaddr_in direccionServidorFS;
	direccionServidorFS.sin_family = AF_INET;
	direccionServidorFS.sin_addr.s_addr = inet_addr(config->IP_FS);
	direccionServidorFS.sin_port = htons(config->PUERTO_FS);

	conectarseConMemoria(&servMemoria, &direccionServidor2);
	tamanioPagMemoria = obtenerTamanioDePagina(&servMemoria);

	conectarseConFS(&servFS,&direccionServidorFS);

	inicializarVec(client_socket);
	inicializarVec(procesos_por_socket);

	esperarConexionDe(&direccionServidor);

	abrirHiloConsolaKernel();

	while (1) {
		prepararSockets(client_socket);
		esperarActividad();
		if (esConexionEntrante()) {
			manejarConexionEntrante(&direccionServidor); //La vieja y confiable.
			mostrarNuevaConexion(direccionServidor);

			procesoConectado = realizarHandshake(kernel);

			switch (procesoConectado) {
			case consola:
				msjConexionCon("una Consola\n");
				setInformacionSockets(client_socket, procesos_por_socket, consola);
				break;

			case cpu:
				msjConexionCon("CPU");
				enviar_planificacion_y_quantum(cliente);
				enviarIntACPU(&cliente,config->STACK_SIZE);
				enviarIntACPU(&cliente,tamanioPagMemoria);
				cliente_CPU *nuevaCPU = crearClienteCPU(&cliente);
				list_add(listaCPUs, nuevaCPU);
				setInformacionSockets(client_socket, procesos_por_socket, cpu);
				planificar();
				break;

			case file_system:
				printf("Me conecte con File System!\n");
				break;

			case main_de_consola:
				printf("Me conecte con la consola de Consola!\n");
				setInformacionSockets(client_socket, procesos_por_socket, main_de_consola);
				break;

			default:
				printf("No me puedo conectar con vos.\n");
				break;
			}
		}

		setClienteActual(socketQueTuvoActividad(client_socket));
		i = numeroSocketQueTuvoActividad(client_socket);
		if (clienteActualSeDesconecto()) {
			int loFinalice = 0;
			if(procesos_por_socket[i] == consola){
				loFinalice = finalizar_programas_de(client_socket[i]);
			}
			liberarSiEraCPU(procesos_por_socket[i]);
			liberarPosicion(client_socket, i);
			liberarPosicion(procesos_por_socket, i);
			if(!loFinalice)
				cerrarConexionClienteActual(&direccionServidor);
		} else {
			int proceso = procesos_por_socket[i];
			switch (proceso) { // ACA VAN TODOS LOS CASES DE CUANDO HAY MOVIMIENTO EN UN SOCKET PORQUE SOLICITA ALGO
			case consola:
				printf("Hubo movimiento en una consola\n");
				confirmarAtencionA(&client_socket[i]);
				atenderAConsola(&client_socket[i]);
				break;
			case cpu:
				printf("Hubo movimiento en una CPU\n");
				cliente_CPU *cpu = obtenerClienteCPUSegunFD(client_socket[i]);
				confirmarAtencionA(&client_socket[i]);
				atenderACPU(cpu);
				break;
			case main_de_consola:
				printf("Hubo movimiento en una Consola de Consola\n");
				confirmarAtencionA(&client_socket[i]);
				atenderAConsolaDeConsola(&client_socket[i]);
				break;
			default:
				break;
			}
		}
	}

	log_destroy(kernel_log);
	liberar_tablaHeap();

	return 0;
}
