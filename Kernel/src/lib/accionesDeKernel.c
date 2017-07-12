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

void enviarSenialACPU(int *unaCPU) {
	char senial[2] = "a";
	if (send((*unaCPU), senial, 2, 0) == -1) {
		printf("Error al enviar la senial\n");
	}
}

char* recibirPathDeCPU(int *unaCPU){
	int tamPath;
	if (recv((*unaCPU), &tamPath, sizeof(int), 0) == -1) {
		printf("Error recibiendo la longitud del Path\n");
		exit(-1);
	}
	char* path = reservarMemoria(tamPath);
	enviarSenialACPU(unaCPU);
	if (recv((*unaCPU), path, tamPath, 0) == -1) {
		printf("Error recibiendo el Path\n");
		exit(-1);
	}
	return path;
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

void atenderACPU(cliente_CPU *unaCPU){
	int accion = recibirAccionDe(&(unaCPU->clie_CPU));
	int PID = recibirAccionDe(&(unaCPU->clie_CPU));
	switch(accion){
	case cpuLibre:
		unaCPU->libre = 1;
		break;
	case cpu_k_abrir_archivo:
	{
		char* path = recibirPathDeCPU(&(unaCPU->clie_CPU));
		//recibirBanderasDeCPU(&(unaCPU->clie_CPU));
		//abrirArchivo(PID,path,flags);
		free(path);
		break;
	}
	case cpu_k_cerrar_archivo:
		//cerrarArchivo(PID,fd);
		break;
	case cpu_k_borrar_archivo:
		//borrarArchivo(PID,fd);
		break;
	case cpu_k_mover_cursor_archivo:
		//moverCursorArchivo(PID,fd,posicion);
		break;
	case cpu_k_leer_archivo:
		//leerArchivo(PID,fd,tamanio);
		break;
	case cpu_k_escribir_archivo:
		//escribirArchivo(PID,fd,bytesAEscribir,tamanio);
		break;
	default:
		break;
	}
}
