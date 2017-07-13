#ifndef CAPAMEMORIA_H_
#define CAPAMEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include "estructurasComunes.h"
#include <commons/collections/list.h>

void inicializar_pid_y_tamPag(int pid, int tamanioPag);
void inicializar_tablaHeap();
void liberar_tablaHeap();

#endif /* CAPAMEMORIA_H_ */
