/*
 * accionesDeKernel.h
 *
 *  Created on: 31/5/2017
 *      Author: utnso
 */

#ifndef ACCIONESDEKERNEL_H_
#define ACCIONESDEKERNEL_H_

#include <stdio.h>
#define FALSE 0
#define TRUE 1

int recibirAccionDe(int *cliente) {
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

void escribirArchivo(char *bufferArchivo, u_int32_t fsize) {
	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	validarAperturaArchivo(archivo);
	fwrite(bufferArchivo, 1, fsize, archivo);
	fclose(archivo);
}

void esperarSenialDeMemoria() {
	char senial[2] = "a";
	if (recv(servMemoria, senial, 2, 0) == -1) {
		printf("Error al recibir senial antes de enviar paginas\n");
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

void finalizarUnProceso(int process_id) {
	int aux = process_id;
	if (send(servMemoria, &aux, sizeof(int), 0) == -1) {
		printf("Error enviando el process_id\n");
		exit(-1);
	} //TODAVIA NO LO PROBAMOS, ES MAS, MEMORIA NO RECIBE EL ERROR TODAVIA JEJE.
	list_remove(listaPCBs_NEW, process_id);

}

void tomarAccionSegunConfirmacion(u_int32_t confirmacion, int process_id) {
	if (confirmacion == noHayPaginas) {
		finalizarUnProceso(process_id);
	} else {
		printf("Hay paginas suficientes!\n");
	}
}

void avisarAccionAMemoria(int accion) {
	int aux = accion;
	if (send(servMemoria, &aux, sizeof(int), 0) == -1) {
		printf("Error enviando la accion.\n");
		exit(-1);
	}
	esperarSenialDeMemoria();

}

void proced_script(int *unCliente, int *unaCPU) {

	u_int32_t fsize = recibirTamArchivo(unCliente);
	char *bufferArchivo = reservarMemoria(fsize + 1);

	recibirArchivoDe(unCliente, bufferArchivo, fsize);
	printf("%s\n\n", bufferArchivo);
	escribirArchivo(bufferArchivo, fsize);

	t_pcb *pcb = crearPCB();
	list_add(listaPCBs_NEW, pcb);
	//list_add(listaPCBs_NEW, crearPCB());

	//PARA MEMORIA

	//Debería enviarse un enum que le indique que va a recibir

	avisarAccionAMemoria(asignarPaginas);
	enviarArchivoAMemoria(bufferArchivo, fsize);
	u_int32_t cant_pags = (divisionRoundUp(fsize, tamanioPagMemoria))
			+ config->STACK_SIZE;
	kernel_mem_start_process(&(pcb->id_proceso), &cant_pags);
	u_int32_t confirmacion = confirmacionMemoria();
	tomarAccionSegunConfirmacion(confirmacion, pcb->id_proceso);

	//PARA CPU

	//Serializo el PCB y lo envio a CPU
	//abstraer la asquerosidad de abajo
	void * serialized_pcb = NULL;
	int serialized_buffer_index = 0;
	serializar_pcb(pcb, &serialized_pcb, &serialized_buffer_index);

	if (send((*unaCPU), &serialized_buffer_index, (size_t) sizeof(int), 0)
			< 0) {
		printf("Send serialized_buffer_length to CPU failed\n");
		exit(-1);
	}
	if (send((*unaCPU), serialized_pcb, (size_t) serialized_buffer_index, 0)
			< 0) {
		printf("Send serialized_pcb to CPU failed\n");
		exit(-1);
	}
	printf("PCB enviado a CPU exitosamente\n");
	free(bufferArchivo);
}

void atenderAConsola(int *unaConsola, int *unaCPU) {
	int accion = recibirAccionDe(unaConsola);
	switch (accion) { //ACA VAN TODOS LOS CASES DE LAS DIFERENTES ACCIONES QUE PUEDE SOLICITAR CONSOLA A KERNEL
	case startProgram:
		proced_script(unaConsola, unaCPU);
		break;
	}
}

#endif /* ACCIONESDEKERNEL_H_ */
