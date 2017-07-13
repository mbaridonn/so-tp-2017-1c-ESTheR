#include "accionesDeKernel.h"
#include "CapaFS.h"//OJO!! DEPENDENCIA CIRCULAR

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

void enviarIntACPU(int *clieCPU, int valor){//PENDIENTE
	if (send((*clieCPU), &valor, sizeof(int), 0) == -1) {
		printf("Error enviando mensaje a CPU\n");
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

void finalizarUnProceso(t_pcb *pcb) {
	int process = pcb->id_proceso;
	avisarAccionAMemoria(finalizarProceso);
	if (send(servMemoria, &process, sizeof(int), 0) == -1) {
		printf("Error enviando el process_id\n");
		exit(-1);
	}
	quitar_PCB_de_Lista(listaPCBs_NEW,pcb);
	list_add(listaPCBs_EXIT, pcb);
	if(false);
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
	avisarAccionAMemoria(asignarPaginas);//FALTA SLEEP (PENDIENTE!!)
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

void transicion_colas_proceso(t_list *listaActual,t_list *listaDestino,t_pcb *pcb){
	quitar_PCB_de_Lista(listaActual, pcb);
	list_add(listaDestino, pcb);
}

t_pcb *obtener_PCB_segun_PID(int PID){
	bool tieneEstePID(t_pcb *unPCB){
		return unPCB->id_proceso==PID;
	}
	return list_find(listaPCBs_EXEC, (void*) tieneEstePID);
}

void atenderACPU(cliente_CPU *unaCPU){
	enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
	int accion = recibirAccionDe(&(unaCPU->clie_CPU));
	enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
	int PID = recibirAccionDe(&(unaCPU->clie_CPU));
	switch(accion){
	case cpuLibre:
		unaCPU->libre = 1;
		t_pcb *un_pcb = obtener_PCB_segun_PID(PID);
		transicion_colas_proceso(listaPCBs_EXEC,listaPCBs_READY,un_pcb);
		break;
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
		int valorVariable = recibirAccionDe(&(unaCPU->clie_CPU));
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
		u_int32_t fd = recibirAccionDe(&(unaCPU->clie_CPU));
		cerrarArchivo(PID,fd);
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		break;
	}
	case cpu_k_borrar_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibirAccionDe(&(unaCPU->clie_CPU));
		int confirmacion = borrarArchivo(PID,fd);//Recibe si se pudo completar o si se produjo error
		enviarIntACPU(&(unaCPU->clie_CPU),confirmacion);
		break;
	}
	case cpu_k_mover_cursor_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibirAccionDe(&(unaCPU->clie_CPU));
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int posicion = recibirAccionDe(&(unaCPU->clie_CPU));
		moverCursorArchivo(PID,fd,posicion);
		//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
		break;
	}
	case cpu_k_leer_archivo:
	{
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		u_int32_t fd = recibirAccionDe(&(unaCPU->clie_CPU));
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int tamanio = recibirAccionDe(&(unaCPU->clie_CPU));
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
		u_int32_t fd = recibirAccionDe(&(unaCPU->clie_CPU));
		enviarSenialACPU(&(unaCPU->clie_CPU));//LO QUERÍA AGREGAR EN recibirAccionDe, PERO NO SABÍA SI IBA A ROMPER LO ANTERIOR
		int tamanio = recibirAccionDe(&(unaCPU->clie_CPU));
		char* bytesAEscribir;
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
