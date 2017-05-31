/*
 * estructurasComunes.h
 *
 *  Created on: 31/5/2017
 *      Author: utnso
 */

#ifndef ESTRUCTURASCOMUNES_H_
#define ESTRUCTURASCOMUNES_H_
#define MAX_CLIENTS 30

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum acciones{
	startProgram
};

int cliente, cliente2;

#endif /* ESTRUCTURASCOMUNES_H_ */
