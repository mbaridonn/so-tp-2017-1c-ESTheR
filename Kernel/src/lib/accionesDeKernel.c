#include "accionesDeKernel.h"
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

bool tiene_este_pcb(t_list *lista,t_pcb *pcb){
	bool esElPCB(t_pcb *unPCB){
		return unPCB->id_proceso == pcb->id_proceso;
	}
	return list_any_satisfy(lista,esElPCB);
}

t_list *lista_que_tiene_este_pcb(t_pcb *pcb){
	if(tiene_este_pcb(listaPCBs_NEW,pcb)){
		return listaPCBs_NEW;
	}
	if(tiene_este_pcb(listaPCBs_READY,pcb)){
		return listaPCBs_READY;
	}
	if(tiene_este_pcb(listaPCBs_EXEC,pcb)){
		return listaPCBs_EXEC;
	}
	if(tiene_este_pcb(listaPCBs_BLOCK,pcb)){
		return listaPCBs_BLOCK;
	}
	if(tiene_este_pcb(listaPCBs_EXIT,pcb)){
		return listaPCBs_EXIT;
	}
	return NULL;
}

void poner_proceso_en_EXIT(t_pcb *pcb){
	t_list *lista;
	lista = lista_que_tiene_este_pcb(pcb);
	transicion_colas_proceso(lista,listaPCBs_EXIT,pcb);
}

void finalizarUnProceso(t_pcb *pcb) {
	int process = pcb->id_proceso;
	avisarAccionAMemoria(k_mem_finalizar_programa);
	if (send(servMemoria, &process, sizeof(int), 0) == -1) {
		printf("Error enviando el process_id\n");
		exit(-1);
	}
	poner_proceso_en_EXIT(pcb); // Tendria que considerar si estaba en EXEC.
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
	while(config->SHARED_VARS[i]!=NULL && strcmp(nombreVariable,config->SHARED_VARS[i])!=0) i++;
	if (config->SHARED_VARS[i]==NULL) return 0; //ACÁ EN REALIDAD SE DEBERÍA PRODUCIR UN ERROR
	else return config->SHARED_VARS_VALUES[i];
}

void asignarValorAVariableCompartida(nombreVariable, valorVariable){
	int i=0;
	while(config->SHARED_VARS[i]!=NULL && strcmp(nombreVariable,config->SHARED_VARS[i])!=0) i++;
	if (config->SHARED_VARS[i]!=NULL) config->SHARED_VARS_VALUES[i]=valorVariable;
	//SI NO SE ENCUENTRA, EN REALIDAD SE DEBERÍA PRODUCIR UN ERROR
}

void proced_script(int *unCliente) {

	u_int32_t fsize = recibirTamArchivo(unCliente);
	char *bufferArchivo = reservarMemoria(fsize + 1);

	//Recibe archivo de consola
	recibirArchivoDe(unCliente, bufferArchivo, fsize);
	printf("%s\n\n", bufferArchivo);
	u_int32_t cant_pags_script = divisionRoundUp(fsize, tamanioPagMemoria);
	t_pcb *pcb = crearPCB(bufferArchivo,cant_pags_script,tamanioPagMemoria);
	list_add(listaPCBs_NEW, pcb);
	int pid = pcb->id_proceso;

	//Envia archivo a Memoria
	avisarAccionAMemoria(k_mem_inicializarPrograma);//FALTA SLEEP (PENDIENTE!!)
	u_int32_t cant_pags = cant_pags_script + config->STACK_SIZE;
	kernel_mem_start_process(&(pcb->id_proceso), &cant_pags);
	u_int32_t confirmacion = confirmacionMemoria();
	enviarArchivoAMemoria(bufferArchivo, fsize);
	esperarSenialDeMemoria();

	tomarAccionSegunConfirmacion(confirmacion, pcb);
	avisarAConsolaSegunConfirmacion(confirmacion, unCliente);

	enviarPIDaConsola(pid,unCliente);

	free(bufferArchivo);
}

void atenderAConsola(int *unaConsola) {
	int accion = recibirAccionDe(unaConsola);
	switch (accion) { //ACA VAN TODOS LOS CASES DE LAS DIFERENTES ACCIONES QUE PUEDE SOLICITAR CONSOLA A KERNEL
	case startProgram:
		proced_script(unaConsola);
		break;
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

int recibir_int_de(int cliente){
	int accion;
	if(recv(cliente, &accion, sizeof(int), 0) == -1){
		printf("Error recibiendo un int.\n");
	}
	return accion;
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

void actualizar_info_pcb(t_pcb *pcb){
	bool esElPCB(t_pcb *unPCB){
		return unPCB->id_proceso == pcb->id_proceso;
	}
	list_remove_and_destroy_by_condition(listaPCBs_EXEC,esElPCB,pcb_destroy);
	list_add(listaPCBs_EXEC,pcb);
}

void mover_pcb_segun_motivo(t_pcb *pcb,int motivo_liberacion){
	switch(motivo_liberacion){
	case mot_finalizo:
		finalizarUnProceso(pcb);
		break;
	case mot_error:
		finalizarUnProceso(pcb); // Esta bien no?
		break;
	case mot_quantum:
		transicion_colas_proceso(listaPCBs_EXEC,listaPCBs_READY,pcb);
		break;
	case mot_semaforo:
		//transicion_colas_proceso(listaPCBs_EXEC,listaPCBs_BLOCK,pcb); DEPENDE
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
	printf("stackPointer: %d \n", pcb_prueba->stackPointer);
	printf("cantElementosStack: %d \n",
			pcb_prueba->indice_stack->elements->elements_count);
	printf("etiquetas_size: %d \n", pcb_prueba->etiquetas_size);
	printf("indice_etiquetas: %c \n", *(pcb_prueba->indice_etiquetas));
	printf("exit_code: %d \n\n", pcb_prueba->exit_code);
}

void atenderACPU(cliente_CPU *unaCPU){
	enviarSenialACPU(&(unaCPU->clie_CPU));//Porque le envia una senial otra vez? Es decir, la ejecucion anterior a esto confirma la atencion ya (envia una senial)
	int accion = recibirAccionDe(&(unaCPU->clie_CPU));
	enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
	int PID = recibir_int_de(unaCPU->clie_CPU);
	switch(accion){
	case cpuLibre:
	{
		int motivo_liberacion = recibir_int_de(unaCPU->clie_CPU);
		t_pcb *pcb = recibir_pcb_de(unaCPU->clie_CPU);
		mostrarPcb(pcb);
		actualizar_info_pcb(pcb);
		unaCPU->libre = 1;
		mover_pcb_segun_motivo(pcb,motivo_liberacion);
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
		cerrarArchivo(PID,fd);
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		break;
	}
	case cpu_k_borrar_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibir_int_de(unaCPU->clie_CPU);
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
		char* bytesAEscribir;//HAY QUE RECIBIR EL TAMANIO PRIMERO
		if (recv(&(unaCPU->clie_CPU), bytesAEscribir, tamanio, 0) == -1) {
			printf("Error al recibir bytes a escribir de CPU\n");
		}
		int confirmacion = escribirArchivo(PID,fd,bytesAEscribir,tamanio);
		enviarIntACPU(&(unaCPU->clie_CPU), confirmacion);
		break;
	}
	default:
		break;
	}
}
