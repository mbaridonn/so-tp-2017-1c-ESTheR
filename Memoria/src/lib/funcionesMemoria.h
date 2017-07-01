#ifndef LIB_FUNCIONESMEMORIA_H_
#define LIB_FUNCIONESMEMORIA_H_

void inicializarMemoriaPrincipal(int valorTamFrame, int valorCantFrames, int valorEntradasCache, int valorCacheXProceso);
void liberarMemoriaPrincipal();
void atenderComandos();
void inicializarPrograma(int PID, int cantPags);
char *leerPagina(int PID, int nroPag, int offset, int tamanio);
void escribirPagina(int PID, int nroPag, int offset, int tamanio, char *buffer);
void asignarPaginasAProceso(int PID, int pagsRequeridas);
void finalizarPrograma(int PID);

void ejecutarOperaciones();// ES DE PRUEBA. BORRAR DESPUES

#endif /* LIB_FUNCIONESMEMORIA_H_ */
