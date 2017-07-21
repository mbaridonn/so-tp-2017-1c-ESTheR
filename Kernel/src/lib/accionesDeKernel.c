#include "accionesDeKernel.h"
#include "estructurasComunes.h"
#include "CapaFS.h"//OJO!! DEPENDENCIA CIRCULAR

void transicion_colas_proceso(t_list *listaActual,t_list *listaDestino,t_pcb *pcb){
	quitar_PCB_de_Lista(listaActual, pcb);
	list_add(listaDestino, pcb);
}

int recibirAccionDe(int *cliente){
	u_int32_t accion;
	recv((*cliente), &accion, sizeof(u_int32_t), 0);
	return (int) accion;
}

u_int32_t recibirTamArchivo(int *unCliente) {
	u_int32_t fsize;
	if (recv((*unCliente), &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		exit(-1);
	}
	return fsize;
}

void recibirArchivoDe(int *unCliente, char *bufferArchivo, u_int32_t fsize) {
	if (recv((*unCliente), bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		exit(-1);
	}
}

void validarAperturaArchivo(FILE *archivo) {
	if (archivo == NULL) {
		printf("No se pudo escribir el archivo\n");
		exit(-1);
	}
}

int divisionRoundUp(int dividendo, int divisor) {
	if (dividendo <= 0 || divisor <= 0) {
		printf("Esta division funciona unicamente con enteros positivos\n");
		exit(-1);
	}
	return 1 + ((dividendo - 1) / divisor);
}

void enviarSenialACPU(int *clieCPU) {
	char senial[2] = "a";
	if (send((*clieCPU), senial, 2, 0) == -1) {
		printf("Error al enviar la senial\n");
		exit(-1);
	}
}

void esperarSenialDeCPU(int *clieCPU) {
	char senial[2] = "a";
	if (recv((*clieCPU), senial, 2, 0) == -1) {
		printf("Error al recibir senial de CPU\n");
	}
}

char* recibirPathDeCPU(int *clieCPU){
	int tamPath;
	enviarSenialACPU(clieCPU);
	if (recv((*clieCPU), &tamPath, sizeof(int), 0) == -1) {
		printf("Error recibiendo la longitud del Path\n");
		exit(-1);
	}
	char* path = reservarMemoria(tamPath);
	enviarSenialACPU(clieCPU);
	if (recv((*clieCPU), path, tamPath, 0) == -1) {
		printf("Error recibiendo el Path\n");
		exit(-1);
	}
	return path;
}

t_banderas *recibirBanderasDeCPU(int *clieCPU){
	t_banderas *flags = reservarMemoria(sizeof(t_banderas));
	enviarSenialACPU(clieCPU);
	if (recv((*clieCPU), flags, sizeof(t_banderas), 0) == -1) {
		printf("Error recibiendo las banderas\n");
		exit(-1);
	}
	return flags;
}

void enviarIntACPU(int *clieCPU, int valor){
	if (send((*clieCPU), &valor, sizeof(int), 0) == -1) {
		printf("Error enviando mensaje a CPU\n");
		exit(-1);
	}
}

void enviarIntAMemoria(int valor){
	if (send(servMemoria, &valor, sizeof(int), 0) == -1) {
		printf("Error enviando mensaje a Memoria\n");
		exit(-1);
	}
}

void enviarBufferAMemoria(char *buffer, int tamanio) {
	if (send(servMemoria, &tamanio, sizeof(int), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	esperarSenialDeMemoria();
	if (send(servMemoria, buffer, tamanio, 0) == -1) {
		printf("Error enviando el buffer\n");
		exit(-1);
	}
}

void enviarArchivoAMemoria(char *buffer, u_int32_t tamBuffer) {
	if (send(servMemoria, &tamBuffer, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send(servMemoria, buffer, tamBuffer + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		exit(-1);
	}
	printf("El archivo se envió correctamente\n");
}

void esperarSenialDeMemoria() {
	char senial[2] = "a";
	if (recv(servMemoria, senial, 2, 0) == -1) {
		printf("Error al recibir senial antes de enviar paginas\n");
	}
}

void esperarSenialDeFS() {
	char senial[2] = "a";
	if (recv(servFS, senial, 2, 0) == -1) {
		printf("Error al recibir senial de FS\n");
	}
}

bool esta_ejecutandose(int pid){
	return tiene_este_pcb(listaPCBs_EXEC,pid);
}

void kernel_mem_start_process(int *process_id, u_int32_t *cant_pags) {
	esperarSenialDeMemoria();
	if (send(servMemoria, process_id, sizeof(int), 0) == -1) {
		printf("Error enviando el process_id\n");
		exit(-1);
	}
	if (send(servMemoria, cant_pags, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la cantidad de paginas\n");
		exit(-1);
	}
	printf("Envie el process_id: %d y Cantidad de Paginas: %d\n", *process_id,
			*cant_pags);
}

u_int32_t confirmacionMemoria() {
	u_int32_t confirmacion;
	if (recv(servMemoria, &confirmacion, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo la confirmacion\n");
		exit(-1);
	}
	return confirmacion;
}

void pcb_destroy(t_pcb *puntero) { //NO FALTA LIBERAR LAS ESTRUCTURAS INTERNAS??
	free(puntero);
}

void quitar_PCB_de_Lista(t_list *lista,t_pcb *pcb){
	bool esElPCB(t_pcb *unPCB){
			return unPCB->id_proceso == pcb->id_proceso;
		}
	list_remove_by_condition(lista,(void*) esElPCB);
}

void avisarAccionAMemoria(int accion) {
	u_int32_t aux = accion;
	if (send(servMemoria, &aux, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la accion.\n");
		exit(-1);
	}
	esperarSenialDeMemoria();
}

void avisarAccionAFS(int accion) {
	u_int32_t aux = accion;
	if (send(servFS, &aux, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la accion.\n");
		exit(-1);
	}
	esperarSenialDeFS();
}

t_pcb *obtener_PCB_segun_PID_en(t_list *lista,int pid){
	bool esElPCB(t_pcb *unPCB){
			return unPCB->id_proceso == pid;
	}
	return list_find(lista,(void*)esElPCB);
}

bool tiene_este_pcb(t_list *lista,int pid){
	bool esElPCB(t_pcb *unPCB){
		return unPCB->id_proceso == pid;
	}
	return list_any_satisfy(lista,(void*)esElPCB);
}

t_list *lista_que_tiene_este_pcb(int pid){
	if(tiene_este_pcb(listaPCBs_NEW,pid)){
		return listaPCBs_NEW;
	}
	if(tiene_este_pcb(listaPCBs_READY,pid)){
		return listaPCBs_READY;
	}
	if(tiene_este_pcb(listaPCBs_EXEC,pid)){
		return listaPCBs_EXEC;
	}
	if(tiene_este_pcb(listaPCBs_BLOCK,pid)){
		return listaPCBs_BLOCK;
	}
	if(tiene_este_pcb(listaPCBs_EXIT,pid)){
		return listaPCBs_EXIT;
	}
	return NULL;
}

void poner_proceso_en_EXIT(t_pcb *pcb){
	t_list *lista;
	lista = lista_que_tiene_este_pcb(pcb->id_proceso);
	transicion_colas_proceso(lista,listaPCBs_EXIT,pcb);
}

estadisticas_proceso *crear_estadisticas_para(int pid){
	estadisticas_proceso *estadisticas = reservarMemoria(sizeof(estadisticas_proceso));
	estadisticas->pid = pid;
	estadisticas->cant_bytes_alocados = 0;
	estadisticas->cant_bytes_liberados = 0;
	estadisticas->cant_op_alocar = 0;
	estadisticas->cant_op_liberar = 0;
	estadisticas->cant_rafagas_ejec = 0;
	estadisticas->cant_syscalls_ejec = 0;
	return estadisticas;
}

void proceso_por_cliente_destroy(proceso_por_cliente *proc){
	free(proc);
}

void eliminar_proc_por_cliente_segun_PID(int pid){
	bool es_el_proceso_de(proceso_por_cliente *proc){
			return proc->pid == pid;
	}
	list_remove_and_destroy_by_condition(lista_proceso_por_cliente,(void *)es_el_proceso_de, (void*)proceso_por_cliente_destroy);
}

int obtener_cliente_segun_PID(int pid){
	bool es_el_proc_por_clie(proceso_por_cliente *p_x_c){
		return p_x_c->pid == pid;
	}
	proceso_por_cliente *proc_por_clie = list_find(lista_proceso_por_cliente,(void*)es_el_proc_por_clie);
	return proc_por_clie->clie_consola;
}

void avisar_accion_a_consola(int consola_clie,int accion){
	int aux = accion;
	if(send(consola_clie,&aux,sizeof(int),0) == -1){
		printf("Error al enviar accion a consola\n");
	}
}

void avisar_finalizacion_proceso_a_consola(int pid){
	int consola_clie = obtener_cliente_segun_PID(pid);
	avisar_accion_a_consola(consola_clie,finalizo_proceso);
}

void cerrar_conexion_con(int clie_consola){
	setClienteActual(clie_consola);
	int i = obtener_posicion_de_cliente(clie_consola);
	liberar_posicion_client_socket(i);
	liberar_posicion_procesos_por_socket(i);
	cerrar_conexion_con_cliente_actual();
}

void finalizarUnProceso(t_pcb *pcb) {
	int process = pcb->id_proceso;
	avisarAccionAMemoria(k_mem_finalizar_programa);
	if (send(servMemoria, &process, sizeof(int), 0) == -1) {
		printf("Error enviando el process_id\n");
		exit(-1);
	}
	poner_proceso_en_EXIT(pcb);
	avisar_finalizacion_proceso_a_consola(pcb->id_proceso);
	cerrar_conexion_con(obtener_cliente_segun_PID(pcb->id_proceso));
	eliminar_proc_por_cliente_segun_PID(pcb->id_proceso);
	//FALTA: MEMORY LEAKS
    //Al finalizar un proceso, el Kernel deberá informar si un proceso liberó todas las estructuras en las páginas de Heap.

	//AL ELIMINAR UN PROCESO, HABRÍA QUE ELIMINAR SU TABLA DE ARCHIVOS DE tablasDeArchivosDeProcesos
} //ESTO NO ESTÁ PROBADO, PERO BUENO, HAY QUE TENER FE

void tomarAccionSegunConfirmacion(u_int32_t confirmacion, t_pcb *pcb) {
	if (confirmacion == noHayPaginas) {
		finalizarUnProceso(pcb);
	} else {
		printf("Hay paginas suficientes!\n");
		quitar_PCB_de_Lista(listaPCBs_NEW,pcb);
		list_add(listaPCBs_READY,pcb);
	}
}

void avisarAConsolaSegunConfirmacion(int confirmacion, int *consola) {
	u_int32_t conf = confirmacion;
	if (send((*consola), &conf, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la confirmacion a consola.\n");
		exit(-1);
	}
}

void enviarPIDaConsola(int pid, int *consola){
	u_int32_t p_id = pid;
	if (send((*consola), &p_id, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando el pid a consola.\n");
		exit(-1);
	}
	printf("PID: %d \n",p_id);
}

int obtenerValorDeVariableCompartida(char* nombreVariable){
	int i=0;
	while(config->SHARED_VARS[i]!=NULL && !str_compare(nombreVariable,config->SHARED_VARS[i])) i++;
	if (config->SHARED_VARS[i]==NULL) return 0; //ACÁ EN REALIDAD SE DEBERÍA PRODUCIR UN ERROR
	else return config->SHARED_VARS_VALUES[i];
}

void asignarValorAVariableCompartida(char *nombreVariable, int valorVariable){
	int i=0;
	while(config->SHARED_VARS[i]!=NULL && !str_compare(nombreVariable,config->SHARED_VARS[i])) i++;
	if (config->SHARED_VARS[i]!=NULL) config->SHARED_VARS_VALUES[i]=valorVariable;
	//SI NO SE ENCUENTRA, EN REALIDAD SE DEBERÍA PRODUCIR UN ERROR
}

pedido_script *crear_pedido_script(int clie_consola,int pid,char *bufferArchivo,int fsize){
	pedido_script *pedido = reservarMemoria(sizeof(pedido_script));
	pedido->clie_consola = clie_consola;
	pedido->pid = pid;
	pedido->bufferArchivo = bufferArchivo;
	pedido->fsize = fsize;
	return pedido;
}

proceso_por_cliente *crear_proceso_por_cliente(int pid, int clie_consola){
	proceso_por_cliente *proc_del_cliente = reservarMemoria(sizeof(proceso_por_cliente));
	proc_del_cliente->clie_consola = clie_consola;
	proc_del_cliente->pid = pid;
	return proc_del_cliente;
}

void proced_script(int *unCliente) {

	u_int32_t fsize = recibirTamArchivo(unCliente);
	char *bufferArchivo = reservarMemoria(fsize + 1);

	//Recibe archivo de consola
	recibirArchivoDe(unCliente, bufferArchivo, fsize);
	printf("%s\n\n", bufferArchivo);
	u_int32_t cant_pags_script = divisionRoundUp(fsize, tamanioPagMemoria);
	t_pcb *pcb = crearPCB(bufferArchivo,cant_pags_script,tamanioPagMemoria,config->STACK_SIZE);
	pedido_script *pedido = crear_pedido_script((*unCliente),pcb->id_proceso,bufferArchivo,fsize);
	list_add(lista_pedidos_script,pedido);
	list_add(listaPCBs_NEW, pcb);
	printf("El largo de la cola de New es: %d\n",list_size(listaPCBs_NEW));

	/*Envia archivo a Memoria
	avisarAccionAMemoria(k_mem_inicializarPrograma);//FALTA SLEEP (PENDIENTE!!)
	u_int32_t cant_pags = cant_pags_script + config->STACK_SIZE;
	kernel_mem_start_process(&(pcb->id_proceso), &cant_pags);
	u_int32_t confirmacion = confirmacionMemoria();
	enviarArchivoAMemoria(bufferArchivo, fsize);
	esperarSenialDeMemoria();

	tomarAccionSegunConfirmacion(confirmacion, pcb);
	avisarAConsolaSegunConfirmacion(confirmacion, unCliente);

	enviarPIDaConsola(pid,unCliente);

	free(bufferArchivo);*/
}

int recibir_int_de(int cliente){
	int accion;
	if(recv(cliente, &accion, sizeof(int), 0) == -1){
		printf("Error recibiendo un int.\n");
	}
	return accion;
}

void atenderAConsola(int *unaConsola) {
	int accion = recibirAccionDe(unaConsola);
	switch (accion) { //ACA VAN TODOS LOS CASES DE LAS DIFERENTES ACCIONES QUE PUEDE SOLICITAR CONSOLA A KERNEL
	case startProgram:
		proced_script(unaConsola);
		break;

	case endProgram:
	{
		int *id_proceso_a_detener = reservarMemoria(sizeof(int));
		*id_proceso_a_detener = recibir_int_de(*unaConsola);
		printf("ID del proceso a conocer: %d\n", *id_proceso_a_detener);
		list_add(lista_detenciones_pendientes,id_proceso_a_detener);
		if(esta_ejecutandose(*id_proceso_a_detener)){
			printf("El proceso esta ejecutandose, este finalizara cuando CPU lo libere.\n");
		}else{
			t_list *lista = lista_que_tiene_este_pcb(*id_proceso_a_detener);
			t_pcb *pcb = obtener_PCB_segun_PID_en(lista,*id_proceso_a_detener);
			finalizarUnProceso(pcb);
			eliminar_detencion(pcb);
		}
		break;
	}
	default:
		break;
	}
}

t_pcb *obtener_PCB_segun_PID(int PID){
	bool tieneEstePID(t_pcb *unPCB){
		return unPCB->id_proceso==PID;
	}
	return list_find(listaPCBs_EXEC, (void*) tieneEstePID);
}


t_pcb *recibir_pcb_de(int cliente) {
	//Recibo PCB y lo deserializo
	void* tmp_buff = calloc(1, sizeof(int));
	int pcb_size = 0, pcb_size_index = 0;
	recv(cliente, tmp_buff, sizeof(int), 0);
	deserializar_data(&pcb_size, sizeof(int), tmp_buff, &pcb_size_index);
	void *pcb_serializado = calloc(1, (size_t) pcb_size);
	recv(cliente, pcb_serializado, (size_t) pcb_size, 0);
	int pcb_serializado_index = 0;
	t_pcb* incomingPCB = calloc(1, sizeof(t_pcb));
	deserializar_pcb(&incomingPCB, pcb_serializado, &pcb_serializado_index);
	free(tmp_buff);
	free(pcb_serializado);
	return incomingPCB;
}

void actualizar_estadisticas_cant_rafagas_de(t_pcb *pcb){
	bool es_estadistica_de(estadisticas_proceso *est){
			return est->pid == pcb->id_proceso;
	}
	estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
	estadisticas->cant_rafagas_ejec++;
}

void actualizar_estadisticas_cant_op_liberar_de(int PID){
	bool es_estadistica_de(estadisticas_proceso *est){
				return est->pid == PID;
		}
		estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
		estadisticas->cant_op_liberar++;
}

void actualizar_estadisticas_cant_bytes_liberados_de(int PID, int espacio_liberado){
	bool es_estadistica_de(estadisticas_proceso *est){
					return est->pid == PID;
			}
	estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
	estadisticas->cant_bytes_liberados+=espacio_liberado;
}

void actualizar_cant_op_alocar_de(int PID){
	bool es_estadistica_de(estadisticas_proceso *est){
						return est->pid == PID;
				}
	estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
	estadisticas->cant_op_alocar++;
}

void actualizar_estadisticas_cant_bytes_alocados_de(int PID, int espacio_alocado){
	bool es_estadistica_de(estadisticas_proceso *est){
						return est->pid == PID;
				}
	estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
	estadisticas->cant_bytes_alocados+=espacio_alocado;
}

void actualizar_estadisticas_cant_syscalls_de(int PID){
	bool es_estadistica_de(estadisticas_proceso *est){
				return est->pid == PID;
		}
	estadisticas_proceso *estadisticas = list_find(lista_estadisticas_de_procesos,(void*)es_estadistica_de);
	estadisticas->cant_syscalls_ejec++;
}

void actualizar_info_pcb(t_pcb *pcb){
	bool esElPCB(t_pcb *unPCB){
		return unPCB->id_proceso == pcb->id_proceso;
	}
	list_remove_and_destroy_by_condition(listaPCBs_EXEC,(void*)esElPCB,(void*)pcb_destroy);
	list_add(listaPCBs_EXEC,pcb);
	actualizar_estadisticas_cant_rafagas_de(pcb);
}

bool hubo_detencion_forzosa(t_pcb *pcb){
	bool esElPID(int *pid){
			return *pid == pcb->id_proceso;
	}
	return list_any_satisfy(lista_detenciones_pendientes,(void*)esElPID);
}

void pid_destroyer(int *pid){
	free(pid);
}

void eliminar_detencion(t_pcb *pcb){
	bool esElPID(int *pid){
				return *pid == pcb->id_proceso;
	}
	cant_procesos_detenidos++;
	list_remove_and_destroy_by_condition(lista_detenciones_pendientes,(void*)esElPID,(void*)pid_destroyer);
}

bloqueo *crear_bloqueo(int pid, int pos_semaforo){
	bloqueo *un_bloqueo = reservarMemoria(sizeof(bloqueo));
	un_bloqueo->pid = pid;
	un_bloqueo->pos_semaforo = pos_semaforo;
	return un_bloqueo;
}

void mover_pcb_segun_motivo(t_pcb *pcb,int motivo_liberacion){
	if(hubo_detencion_forzosa(pcb)){
		finalizarUnProceso(pcb);
		eliminar_detencion(pcb);
		return;
	}
	switch(motivo_liberacion){
	case mot_finalizo:
		finalizarUnProceso(pcb);
		cant_procesos_finalizados++;
		break;
	case mot_error:
		finalizarUnProceso(pcb);
		cant_procesos_finalizados++;
		break;
	case mot_quantum:
		transicion_colas_proceso(listaPCBs_EXEC,listaPCBs_READY,pcb);
		break;
	case mot_bloqueado:
		transicion_colas_proceso(listaPCBs_EXEC,listaPCBs_BLOCK,pcb);
		break;
	}
}

void mostrarPcb(t_pcb *pcb_prueba) {
	printf("id_proceso: %d \n", pcb_prueba->id_proceso);
	printf("program_counter: %d \n", pcb_prueba->program_counter);
	printf("cant_instrucciones: %d \n", pcb_prueba->cant_instrucciones);
	printf("cant_paginas_de_codigo: %d \n", pcb_prueba->cant_paginas_de_codigo);
	printf("indice_codigo->start: %u \n", pcb_prueba->indice_codigo->start);
	printf("indice_codigo->offset: %u \n", pcb_prueba->indice_codigo->offset);
	printf("contadorPags: %u \n", pcb_prueba->contadorPags);
	printf("stackPointer: %d \n", pcb_prueba->stackPointer);
	printf("cantElementosStack: %d \n",	pcb_prueba->indice_stack->elements->elements_count);
	printf("etiquetas_size: %d \n", pcb_prueba->etiquetas_size);
	printf("indice_etiquetas: %c \n", *(pcb_prueba->indice_etiquetas));
	printf("exit_code: %d \n\n", pcb_prueba->exit_code);
}

void bloqueo_destroy(bloqueo *bloq){
	free(bloq);
}

void wake_up(int pos_semaforo){
	bool tiene_este_semaforo(bloqueo *bloq){
		return bloq->pos_semaforo == pos_semaforo;
	}
	bloqueo *un_bloqueo = list_find(lista_bloqueos,(void *)tiene_este_semaforo);
	int pid = un_bloqueo->pid;
	bool es_este_bloqueo(bloqueo *bloq){
		return bloq->pos_semaforo == pos_semaforo && bloq->pid==pid;
	}
	list_remove_and_destroy_by_condition(lista_bloqueos,(void *)es_este_bloqueo,(void *)bloqueo_destroy);
	t_pcb *un_pcb = obtener_PCB_segun_PID_en(listaPCBs_BLOCK,pid);
	transicion_colas_proceso(listaPCBs_BLOCK,listaPCBs_READY,un_pcb);
}

void atenderACPU(cliente_CPU *unaCPU){
	enviarSenialACPU(&(unaCPU->clie_CPU));//Porque le envia una senial otra vez? Es decir, la ejecucion anterior a esto confirma la atencion ya (envia una senial)
	int accion = recibirAccionDe(&(unaCPU->clie_CPU));
	enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
	int PID = recibir_int_de(unaCPU->clie_CPU);
	printf("Me llego el PID %d y la accion %d\n", PID, accion);
	switch(accion){
	case cpuLibre:
	{
		int motivo_liberacion = recibir_int_de(unaCPU->clie_CPU);
		t_pcb *pcb = recibir_pcb_de(unaCPU->clie_CPU);
		mostrarPcb(pcb);
		actualizar_info_pcb(pcb);
		printf("Llegue haca aca\n");
		mover_pcb_segun_motivo(pcb,motivo_liberacion);
		printf("Llegue haca aca2\n");
		unaCPU->libre = 1;
		break;
	}
	case cpu_k_obtener_valor_compartida:
	{
		char* nombreVariable = recibirPathDeCPU(&(unaCPU->clie_CPU));
		int valorVariable = obtenerValorDeVariableCompartida(nombreVariable);//SI NO EXISTE, DEVUELVE 0 (DEBERÍA PRODUCIR ERROR)
		enviarIntACPU(&(unaCPU->clie_CPU),valorVariable);
		free(nombreVariable);
		break;
	}
	case cpu_k_asignar_valor_compartida:
	{
		char* nombreVariable = recibirPathDeCPU(&(unaCPU->clie_CPU));
		int valorVariable = recibir_int_de(unaCPU->clie_CPU);
		asignarValorAVariableCompartida(nombreVariable, valorVariable);//SI NO EXISTE, NO HACE NADA (DEBERÍA PRODUCIR ERROR)
		free(nombreVariable);
		break;
	}
	case cpu_k_abrir_archivo:
	{
		char* path = recibirPathDeCPU(&(unaCPU->clie_CPU));
		t_banderas *flags = recibirBanderasDeCPU(&(unaCPU->clie_CPU));
		printf("Me llego el path %s \n", path);
		u_int32_t fd = abrirArchivo(PID,path,*flags);
		enviarIntACPU(&(unaCPU->clie_CPU), fd);
		free(path);
		free(flags);
		break;
	}
	case cpu_k_cerrar_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibir_int_de(unaCPU->clie_CPU);
		printf("cerrarArchivo(%d, %d)\n", PID, fd);
		cerrarArchivo(PID,fd);
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		break;
	}
	case cpu_k_borrar_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibir_int_de(unaCPU->clie_CPU);
		printf("borrarArchivo(%d, %d)\n", PID, fd);
		int confirmacion = borrarArchivo(PID,fd);//Recibe si se pudo completar o si se produjo error
		enviarIntACPU(&(unaCPU->clie_CPU),confirmacion);
		break;
	}
	case cpu_k_mover_cursor_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibir_int_de(unaCPU->clie_CPU);
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int posicion = recibir_int_de(unaCPU->clie_CPU);
		printf("moverCursorArchivo(%d, %d, %d)\n", PID, fd, posicion);
		moverCursorArchivo(PID,fd,posicion);
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		break;
	}
	case cpu_k_leer_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibir_int_de(unaCPU->clie_CPU);
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int tamanio = recibir_int_de(unaCPU->clie_CPU);
		printf("leerArchivo(%d, %d, %d)\n", PID, fd, tamanio);
		char* bytesLeidos = leerArchivo(PID,fd,tamanio);
		if(bytesLeidos != NULL){
			enviarIntACPU(&(unaCPU->clie_CPU), tamanio);
			esperarSenialDeCPU(&(unaCPU->clie_CPU));
			if (send(&(unaCPU->clie_CPU), bytesLeidos, tamanio, 0) == -1) {
				printf("Error enviando los bytes leidos\n");
				exit(-1);
			}
		} else {
			enviarIntACPU(&(unaCPU->clie_CPU), k_cpu_error);
		}
		break;
	}
	case cpu_k_escribir_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibir_int_de(unaCPU->clie_CPU);
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int tamanio = recibir_int_de(unaCPU->clie_CPU);
		char* bytesAEscribir = reservarMemoria(tamanio);
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		if (recv((unaCPU->clie_CPU), bytesAEscribir, tamanio, 0) == -1) {
			printf("Error al recibir bytes a escribir de CPU\n");
			exit(-1);
		}
		printf("escribirArchivo(%d, %d, %d)\n", PID, fd, tamanio);
		int confirmacion = escribirArchivo(PID,fd,bytesAEscribir,tamanio);
		enviarIntACPU(&(unaCPU->clie_CPU), confirmacion);
		free(bytesAEscribir);
		break;
	}
	case cpu_k_reservar:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int contador_paginas = recibir_int_de(unaCPU->clie_CPU);
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int espacio_requerido = recibir_int_de(unaCPU->clie_CPU);
		printf("Me llego el contPags %d y el esp req %d\n", contador_paginas, espacio_requerido);
		inicializar_pid_tamPag_clieCPU_y_contador_paginas(PID,tamanioPagMemoria, unaCPU->clie_CPU, contador_paginas);
		u_int32_t direccion = reservarMemoriaDinamica(espacio_requerido);
		actualizar_cant_op_alocar_de(PID);
		if(direccion!=0) actualizar_estadisticas_cant_bytes_alocados_de(PID,espacio_requerido);
		enviarIntACPU(&(unaCPU->clie_CPU),direccion);
		break;
	}
	case cpu_k_liberar:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t direccion = recibir_int_de(unaCPU->clie_CPU);
		printf("Me llego la direccion %d\n", direccion);
		inicializar_pid_tamPag_clieCPU_y_contador_paginas(PID,tamanioPagMemoria, unaCPU->clie_CPU, 0/*No es necesario*/);
		liberarMemoriaDinamica(direccion);
		actualizar_estadisticas_cant_op_liberar_de(PID); // Se actualiza siempre, se haya podido liberar o no.
		break;
	}
	case cpu_k_wait:
	{
		char *identificador_semaforo = recibirPathDeCPU(&(unaCPU->clie_CPU));
		int i=0;
		while(config->SEM_IDS[i]!=NULL && !str_compare(identificador_semaforo,config->SEM_IDS[i])) i++;
		if (config->SEM_IDS[i]!=NULL) config->SEM_INIT[i]--;
		if(config->SEM_INIT[i] < 0){
			bloqueo *un_bloqueo = crear_bloqueo(PID,i);
			list_add(lista_bloqueos,un_bloqueo);
			int accion_a_enviar = k_cpu_bloquear;
			enviarIntACPU(&(unaCPU->clie_CPU),accion_a_enviar);
			printf("Le dije a CPU que se bloquee\n");
		}else{
			enviarIntACPU(&(unaCPU->clie_CPU),k_cpu_continuar);
			printf("Le dije a CPU que continue\n");
		}
		free(identificador_semaforo);
		break;
	}
	case cpu_k_signal:
	{
		char *identificador_semaforo = recibirPathDeCPU(&(unaCPU->clie_CPU));
		int i=0;
		while(config->SEM_IDS[i]!=NULL && !str_compare(identificador_semaforo,config->SEM_IDS[i])) i++;
		if (config->SEM_IDS[i]!=NULL) config->SEM_INIT[i]++;
		if(config->SEM_INIT[i] <= 0){
			wake_up(i);
			printf("Desbloquee un proceso!\n");
		}
		free(identificador_semaforo);
		break;
	}
	default:
		printf("Recibi un comando invalido!\n");
		exit(-1);
		break;
	}
	if(accion != cpuLibre){
		actualizar_estadisticas_cant_syscalls_de(PID);
	}
}
