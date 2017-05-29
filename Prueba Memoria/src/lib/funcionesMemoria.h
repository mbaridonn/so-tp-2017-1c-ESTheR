#ifndef LIB_FUNCIONESMEMORIA_H_
#define LIB_FUNCIONESMEMORIA_H_

/*typedef struct {
	int puerto;
	int cantFrames;
	int tamFrame;
	int entradasCache;
	int cacheXProceso;
	//int reemplazoCache CONVENDRÁ USAR ENUM PARA EL ALGORITMO DE REEMPLAZO?
	int retardoMemoria;
} t_configuracion;
extern t_configuracion *config;*/ //PENDIENTE!!!!

void inicializarMemoriaPrincipal(int valorTamFrame, int valorCantFrames);
void liberarMemoriaPrincipal();
void atenderComandos();
void inicializarPrograma(int PID, int cantPags);
char *leerPagina(int PID, int nroPag, int offset, int tamanio);
void escribirPagina(int PID, int nroPag, int offset, int tamanio, char *buffer);
void asignarPaginasAProceso(int PID, int pagsRequeridas);
void finalizarPrograma(int PID);

void ejecutarOperaciones();// ES DE PRUEBA. BORRAR DESPUES

#endif /* LIB_FUNCIONESMEMORIA_H_ */
