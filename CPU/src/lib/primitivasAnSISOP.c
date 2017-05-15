#include "primitivasAnSISOP.h"

//DEFINICIONES DE PRUEBA
static const int CONTENIDO_VARIABLE = 20;
static const int POSICION_MEMORIA = 0x10;
bool termino = false;

/*//FUNCIONES EXTRA
bool esArgumento(t_nombre_variable identificador_variable){
	if(isdigit(identificador_variable)){
		return true;
	}else{
		return false;
	}
}*/

//PRIMITIVAS
t_puntero definirVariable(t_nombre_variable var_nombre){
	printf("definir la variable %c\n", var_nombre);
	return POSICION_MEMORIA;
}

t_puntero obtenerPosicionVariable(t_nombre_variable var_nombre){
	printf("Obtener posicion de %c\n", var_nombre);
	return POSICION_MEMORIA;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){
	printf("Dereferenciar %d y su valor es: %d\n", direccion_variable, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){

}

t_valor_variable obtenerValorCompartida(t_nombre_compartida var_compartida_nombre){

}

t_valor_variable asignarValorCompartida(t_nombre_compartida var_compartida_nombre, t_valor_variable var_compartida_valor){

}

void irAlLabel(t_nombre_etiqueta nombre_etiqueta){

}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

}

void finalizar(void){
	termino = true;
	printf("Finalizar\n");
}

bool terminoElPrograma(void){//FUNCION DE PRUEBA
	return termino;
}

void retornar(t_valor_variable var_retorno){

}



void wait(t_nombre_semaforo identificador_semaforo){

}

void signal(t_nombre_semaforo identificador_semaforo){

}

t_puntero reservar(t_valor_variable espacio){

}

void liberar(t_puntero puntero){

}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){

}

void borrar(t_descriptor_archivo direccion){

}

void cerrar(t_descriptor_archivo descriptor_archivo){

}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){

}

void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){

}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){

}
