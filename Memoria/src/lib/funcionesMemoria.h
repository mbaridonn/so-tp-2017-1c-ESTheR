#ifndef LIB_FUNCIONESMEMORIA_H_
#define LIB_FUNCIONESMEMORIA_H_

#include <commons/log.h>

void inicializarMemoriaPrincipal(int valorTamFrame, int valorCantFrames, int valorEntradasCache, int valorCacheXProceso, int* ptrRetardoMemoria, t_log *memoria_log);
void liberarMemoriaPrincipal();
void atenderComandos();
void atenderKernel();
void atenderCPU();
void inicializarPrograma(int PID, int cantPags);
char *leerPagina(int PID, int nroPag, int offset, int tamanio);
void escribirPagina(int PID, int nroPag, int offset, int tamanio, char *buffer);
void asignarPaginasAProceso(int PID, int pagsRequeridas);
void liberarPaginaDeProceso(int PID, int nroPag);
void finalizarPrograma(int PID);

int clienteKernel;
char *bufferArchivo;

void ejecutarOperaciones();// ES DE PRUEBA. BORRAR DESPUES

#endif /* LIB_FUNCIONESMEMORIA_H_ */
