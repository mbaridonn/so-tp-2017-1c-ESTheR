#ifndef CAPAMEMORIA_H_
#define CAPAMEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include "estructurasComunes.h"
#include <commons/collections/list.h>

void inicializar_pid_tamPag_clieCPU_y_contador_paginas(int pid, int tamanioPag, int clienteCPU, int contPags);
void inicializar_tablaHeap();
void liberar_tablaHeap();
u_int32_t reservarMemoriaDinamica(int espacioRequerido);
void liberarMemoriaDinamica(u_int32_t puntero);

#endif /* CAPAMEMORIA_H_ */
