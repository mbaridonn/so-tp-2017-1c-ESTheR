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

u_int32_t recibirTamArchivo(int *unCliente){
	u_int32_t fsize;
	if (recv((*unCliente), &fsize, sizeof(u_int32_t), 0) == -1) {
			printf("Error recibiendo longitud del archivo\n");
			exit(-1);
		}
	return fsize;
}

void recibirArchivoDe(int *unCliente,char *bufferArchivo, u_int32_t fsize ) {

	if (recv((*unCliente), bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		exit(-1);
	}
}

void validarAperturaArchivo(FILE *archivo){
	if (archivo == NULL) {
			printf("No se pudo escribir el archivo\n");
			exit(-1);
	}
}

void enviarArchivoAMemoria(char *buffer,u_int32_t tamBuffer){

	if (send(servMemoria, &tamBuffer, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send(servMemoria, buffer, tamBuffer+1, 0) == -1) {
		printf("Error enviando archivo\n");
		exit(-1);
	}
	printf("El archivo se envió correctamente\n");
}

void escribirArchivo(char *bufferArchivo, u_int32_t fsize){
	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	validarAperturaArchivo(archivo);
	fwrite(bufferArchivo, 1, fsize, archivo);
	fclose(archivo);
}

void *proced_script(t_list *listaPCBs_NEW, int *unCliente,
		int *unaCPU) {

	u_int32_t fsize = recibirTamArchivo(unCliente);
	char *bufferArchivo = reservarMemoria(fsize+1);

	recibirArchivoDe(unCliente,bufferArchivo,fsize);
	printf("%s\n\n", bufferArchivo);
	escribirArchivo(bufferArchivo, fsize);

	t_pcb *pcb = crearPCB();
	list_add(listaPCBs_NEW, pcb);
	//list_add(listaPCBs_NEW, crearPCB());

	//PARA MEMORIA

	//Debería enviarse un enum que le indique que va a recibir

	enviarArchivoAMemoria(bufferArchivo,fsize);

	//PARA CPU
	int num = pcb->id_proceso;
	if (send((*unaCPU), &num, sizeof(int), 0) == -1) {
		printf("Error enviando algo a la CPU\n");
		exit(-1);
	}

	free(bufferArchivo);
}

void atenderAConsola(t_list *listaPCBs_NEW, int *unaConsola,
		int *unaCPU) {
	int accion = recibirAccionDe(unaConsola);
	switch (accion) { //ACA VAN TODOS LOS CASES DE LAS DIFERENTES ACCIONES QUE PUEDE SOLICITAR CONSOLA A KERNEL
	case startProgram:
		proced_script(listaPCBs_NEW, unaConsola, unaCPU);
		break;
	}
}

#endif /* ACCIONESDEKERNEL_H_ */
