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
/*
t_valor_variable dereferenciar(t_puntero direccion_variable) {
   //if(status_check() != EXECUTING)
    //   return ERROR;
	printf("print1\n");
    //Hacemos el request a la UMC con el codigo 2
    t_list *pedidos = NULL;
    construir_operaciones_lectura(&pedidos, direccion_variable);
    printf("print2\n");
    void * variable_buffer = calloc(1, ANSISOP_VAR_SIZE), *tmp_buffer = NULL;
    printf("print3\n");
    int variable_buffer_index = 0;
    char response_status;
    printf("print4\n");
    logical_addr * current_address = NULL;
    printf("print5\n");
    while(list_size(pedidos) > 0) {
    	printf("print6\n");
        current_address  = list_remove(pedidos, 0);
//      asprintf(&tmp_buffer, "3%04d%04d%04d", current_address->page_number,
//                 current_address->offset, current_address->tamanio);
        printf("print7\n");
        int tmp_buffer_index = 0;
        printf("print8\n");
        char operation = PEDIDO_BYTES;
        printf("print9\n");
        serializar_data(&operation, sizeof(char), &tmp_buffer, &tmp_buffer_index);
        serializar_data(&current_address->page_number, sizeof(int), &tmp_buffer, &tmp_buffer_index);
        serializar_data(&current_address->offset, sizeof(int), &tmp_buffer, &tmp_buffer_index);
        serializar_data(&current_address->tamanio, sizeof(int), &tmp_buffer, &tmp_buffer_index);
        printf("print10\n");
        //log_info(cpu_log, "sending request : op:%c (%d,%d,%d) for direccion variable %d",operation, current_address->page_number,
        //         current_address->offset, current_address->tamanio, direccion_variable);
		//        log_info(cpu_log, "sending request : %s for direccion variable %d", &tmp_buffer, direccion_variable);
        printf("print11\n");
        variable_buffer_index += current_address->tamanio;
        printf("print12\n");
    }

	free(tmp_buffer);
	printf("print\n");
    //log_info(cpu_log, "Valor variable obtained : %d", *(int*)variable_buffer);
    //return (t_valor_variable) *(t_valor_variable*)variable_buffer;
	printf("Dereferenciar %d y su valor es: %d\n", direccion_variable, CONTENIDO_VARIABLE);
	return CONTENIDO_VARIABLE;
}*/

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
/*
t_list *armarDireccionesLogicasList(t_intructions *original_instruction) {

    t_intructions* actual_instruction = calloc(1, sizeof(t_intructions));
    actual_instruction->start = original_instruction->start;
    actual_instruction->offset = original_instruction->offset;

    t_list *address_list = list_create();
    logical_addr *addr = (logical_addr*) calloc(1,sizeof(logical_addr));
    int actual_page = 0;
    //log_info(cpu_log, "Obtaining logic addresses for start:%d offset:%d",actual_instruction->start, actual_instruction->offset );
    //First calculation
    addr->page_number =  (int) actual_instruction->start / 10 ;		//setup->PAGE_SIZE
    actual_page = addr->page_number;
    addr->offset = actual_instruction->start % 10 ;					//setup->PAGE_SIZE
    if(actual_instruction->offset > 10  - addr->offset ) {			//setup->PAGE_SIZE
        addr->tamanio = 10  - addr->offset;							//setup->PAGE_SIZE
    } else {
        addr->tamanio = actual_instruction->offset;
    }
    list_add(address_list, addr);
    actual_instruction->offset = actual_instruction->offset - addr->tamanio;

    //ojo que actual_instruction->offset es un size_t
    //cuando se lo decremente por debajo de 0, asumira su valor maximo
    while (actual_instruction->offset != 0) {
        addr = (logical_addr*) calloc(1,sizeof(logical_addr));
        addr->page_number = ++actual_page;
        addr->offset = 0;
        //addr->tamanio = setup->PAGE_SIZE - addr->offset;
        if(actual_instruction->offset > 10  - addr->offset ) {		//setup->PAGE_SIZE
            addr->tamanio = 10 - addr->offset;						//setup->PAGE_SIZE
        } else {
            addr->tamanio = actual_instruction->offset;
        }
        actual_instruction->offset = actual_instruction->offset - addr->tamanio;
        list_add(address_list, addr);
    }
    free(actual_instruction);
    return address_list;
}

void construir_operaciones_lectura(t_list **pedidos, t_puntero posicion_variable) {
    t_intructions instruccion_a_buscar;
    instruccion_a_buscar.start = (t_puntero_instruccion) posicion_variable;
    instruccion_a_buscar.offset = (size_t) ANSISOP_VAR_SIZE;
    *pedidos = armarDireccionesLogicasList(&instruccion_a_buscar);
}

*/
