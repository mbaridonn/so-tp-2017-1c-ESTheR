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

int recibirAccionDe(int *cliente){
	u_int32_t accion;
	recv((*cliente),&accion,sizeof(u_int32_t),0);
	return (int)accion;
}

void *proced_script(int *servMemoria,t_list *listaPCBs_NEW, int *unCliente, int *unaCPU) {

	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	if (archivo == NULL) {
		printf("No se pudo escribir el archivo\n");
		exit(-1);
	}
	u_int32_t fsize;
	if (recv((*unCliente), &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		exit(-1);
	}

	char *bufferArchivo = reservarMemoria(fsize + 1);
	if (recv((*unCliente), bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		exit(-1);
	}
	printf("%s\n\n", bufferArchivo);

	t_pcb *pcb = crearPCB();
	list_add(listaPCBs_NEW, pcb);
	//list_add(listaPCBs_NEW, crearPCB());

	fwrite(bufferArchivo, 1, fsize, archivo);

	free(bufferArchivo);
	fclose(archivo);

	//PARA MEMORIA

	FILE * archivo2 = fopen("prueba.txt", "rb");
	if (archivo == NULL) {
		printf("No se pudo leer el archivo\n");
		exit(-1);
	}
	fseek(archivo2, 0, SEEK_END);
	u_int32_t fsize2 = ftell(archivo2);
	fseek(archivo2, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize2 + 1);
	fread(buffer, fsize2, 1, archivo2);

	buffer[fsize2] = '\0';
	if (send((*servMemoria), &fsize2, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send((*servMemoria), buffer, fsize2 + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		exit(-1);
	}
	printf("El archivo se envió correctamente\n");
	free(buffer);
	fclose(archivo2);

	//PARA CPU
	int num = pcb->id_proceso;
	if (send((*unaCPU), &num, sizeof(int), 0) == -1) {
			printf("Error enviando algo a la CPU\n");
			exit(-1);
		}
}

void atenderAConsola(int *servMemoria,t_list *listaPCBs_NEW,int *unaConsola,int *unaCPU){
	int accion = recibirAccionDe(unaConsola);
	switch (accion){ //ACA VAN TODOS LOS CASES DE LAS DIFERENTES ACCIONES QUE PUEDE SOLICITAR CONSOLA A KERNEL
	case startProgram:
		proced_script(servMemoria,listaPCBs_NEW, unaConsola, unaCPU);
		break;
	}
}


#endif /* ACCIONESDEKERNEL_H_ */
