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

void *proced_script(struct sockaddr_in *direccionServidor2, t_list *listaPCBs_NEW, int *unCliente) {

	FILE *archivo;
	archivo = fopen("prueba.txt", "w");
	if (archivo == NULL) {
		printf("No se pudo escribir el archivo\n");
		return NULL;
	}
	u_int32_t fsize;
	if (recv((*unCliente), &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo longitud del archivo\n");
		return NULL;
	}

	char *bufferArchivo = reservarMemoria(fsize + 1);
	if (recv((*unCliente), bufferArchivo, fsize + 1, 0) == -1) {
		printf("Error recibiendo el archivo\n");
		return NULL;
	}
	printf("%s\n\n", bufferArchivo);

	list_add(listaPCBs_NEW, crearPCB());

	fwrite(bufferArchivo, 1, fsize, archivo);

	free(bufferArchivo);
	fclose(archivo);

	//PARA MEMORIA

	conectar(&cliente2, direccionServidor2);
	handshake(&cliente2, kernel);
	msjConexionCon("una Memoria");

	FILE * archivo2 = fopen("prueba.txt", "rb");
	if (archivo == NULL) {
		printf("No se pudo leer el archivo\n");
		return NULL;
	}
	fseek(archivo2, 0, SEEK_END);
	u_int32_t fsize2 = ftell(archivo2);
	fseek(archivo2, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize2 + 1);
	fread(buffer, fsize2, 1, archivo2);

	buffer[fsize2] = '\0';
	if (send(cliente2, &fsize2, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		return NULL;
	}
	if (send(cliente2, buffer, fsize2 + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		return NULL;
	}
	printf("El archivo se envió correctamente\n");
	free(buffer);
	fclose(archivo2);
	return NULL;
}

void atenderAConsola(struct sockaddr_in *direccionServidor2,t_list *listaPCBs_NEW,int *unaConsola){
	int accion = recibirAccionDe(unaConsola);
	switch (accion){ //ACA VAN TODOS LOS CASES DE LAS DIFERENTES ACCIONES QUE PUEDE SOLICITAR CONSOLA A KERNEL
	case startProgram:
		proced_script(direccionServidor2, listaPCBs_NEW, unaConsola);
		break;
	}
}


#endif /* ACCIONESDEKERNEL_H_ */
