/*
 * conexionesSelect.h
 *
 *  Created on: 31/5/2017
 *      Author: utnso
 */

#ifndef CONEXIONESSELECT_H_
#define CONEXIONESSELECT_H_

#include <arpa/inet.h>
#include "lib/estructurasComunes.h"
#define FALSE 0
#define TRUE 1

int activity, master_socket, addrlen, sd, max_sd, cliente, valread;
char buffer[1025];

fd_set readfds;

int primerIndiceLibre(int client_socket[]) {
	int i;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (client_socket[i] == -1) {
			return i;
		}
	}
	printf("No hay indices libres\n");
	return -99;
}

void mostrarConexion(int cliente, struct sockaddr_in direccionServidor) {
	printf("Nueva conexion , socket fd: %d , ip: %s , puerto: %d \n", cliente,
			inet_ntoa(direccionServidor.sin_addr),
			ntohs(direccionServidor.sin_port));
}

void confirmarAtencionA(int *unCliente) {
	char a[2] = "a";
	send((*unCliente), a, 2, 0);
}

void msjConexionCon(char *s) {
	printf(
			"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			s);
}

void inicializarVec(int vec[]) {
	int i;
	for (i = 0; i < MAX_CLIENTS; i++) {
		vec[i] = -1;
	}
}

void esperarConexionDe(struct sockaddr_in *direccionServidor) {
	esperarConexion(&master_socket, direccionServidor);
	addrlen = sizeof((*direccionServidor)); /*.*/
	puts("Esperando conexiones...");
}

void prepararSockets(int client_socket[]) {
	FD_ZERO(&readfds);
	FD_SET(master_socket, &readfds);
	max_sd = master_socket;

	int i;
	for (i = 0; i < MAX_CLIENTS; i++) {
		sd = client_socket[i];

		if (sd > 0)
			FD_SET(sd, &readfds);

		if (sd > max_sd)
			max_sd = sd;
	}
}

void esperarActividad() {
	activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
	if (activity < 0) {
		printf("select error");
	}
}

int esConexionEntrante() {
	return FD_ISSET(master_socket, &readfds);
}

void manejarConexionEntrante(struct sockaddr_in *direccionServidor) {
	if ((cliente = accept(master_socket, (struct sockaddr *) direccionServidor,
			(socklen_t*) &addrlen)) < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
}

void mostrarNuevaConexion(struct sockaddr_in direccionServidor) {
	mostrarConexion(cliente, direccionServidor);
}

int realizarHandshake(int proceso) {
	return handshake(&cliente, proceso);
}

void setInformacionSockets(int client_socket[], int procesos_por_socket[],
		int proceso) {
	int indice = primerIndiceLibre(client_socket);
	procesos_por_socket[indice] = proceso;
	agregarSocket(client_socket, &cliente);
}

void setClienteActual(int socket) {
	sd = socket;
}

int clienteActualTuvoActividad() {
	return FD_ISSET(sd, &readfds);
}

int clienteActualSeDesconecto() {
	return (valread = read(sd, buffer, 1024)) == 0;
}

void cerrarConexionClienteActual(struct sockaddr_in *direccionServidor) {
	getpeername(sd, (struct sockaddr*) direccionServidor,
			(socklen_t*) &addrlen);
	printf("Cliente desconectado , ip %s , puerto %d \n",
			inet_ntoa((*direccionServidor).sin_addr),
			ntohs((*direccionServidor).sin_port));

	close(sd);
}

void liberarPosicion(int vec[], int pos) {
	vec[pos] = -1;
}

int socketQueTuvoActividad(int client_socket[]){
	int i;
	for(i=0;i<30;i++){
		setClienteActual(client_socket[i]);
		if(clienteActualTuvoActividad()){
			return client_socket[i];
		}
	}
	return -1;
}

int numeroSocketQueTuvoActividad(int client_socket[]){
	int i;
	for(i=0;i<MAX_CLIENTS;i++){
		setClienteActual(client_socket[i]);
		if(clienteActualTuvoActividad()){
			return i;
		}
	}
	return -1;
}
#endif /* CONEXIONESSELECT_H_ */
