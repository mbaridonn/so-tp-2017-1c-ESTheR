#include <ctype.h>
#include <parser/parser.h>
#include "primitivasAnSISOP.h"
#include "pcb.h"

#define TAM_VARIABLE 4

bool terminoPrograma;

//VALORES A INICIALIZAR
t_pcb* pcbAEjecutar;
int stackSize;
int tamPag;

//FUNCIONES EXTRA

void inicializarPrimitivasANSISOP(t_pcb* _pcbAEjecutar, int _stackSize, int _tamPag){
	pcbAEjecutar = _pcbAEjecutar;
	stackSize = _stackSize;
	tamPag = _tamPag;
	terminoPrograma = false;
}

bool esArgumento(t_nombre_variable identificador_variable){
	return isdigit(identificador_variable);
}

t_stack_entry* stack_entry_create_and_initialize(){
	t_stack_entry* stackEntry = calloc(1, sizeof(t_stack_entry));
	stackEntry->args = list_create();
	stackEntry->vars = list_create();
	stackEntry->ret_pos = -1;
	stackEntry->ret_vars->offset = 0;
	stackEntry->ret_vars->page_number = 0;
	stackEntry->ret_vars->tamanio = 0;
	return stackEntry;
}

bool terminoElPrograma(void){
	return terminoPrograma;
}

/*t_puntero_instruccion busquedaEtiqueta(t_nombre_etiqueta etiqueta){
	for (int i = 0; i < pcbAEjecutar->etiquetas_size; ++i) {
		if(strcmp(pcbAEjecutar->indice_etiquetas[i].name, etiqueta) == 0){
			return pcbAEjecutar->indice_etiquetas[i].PC;
		} else {
			return -1;   //ESTÁ BIEN ESTO??
		}
	}
	return -1;
}*/

//PRIMITIVAS

t_puntero definirVariable(t_nombre_variable var_nombre){
	if((pcbAEjecutar->stackPointer+4) > (stackSize * tamPag)){
		printf("StackOverflow. Se finaliza el proceso\n");
		//huboStackOver = true; (!!)
		return -1;//DEBERÍA PRODUCIR UN ERROR
	}

	int pagina = pcbAEjecutar->stackPointer / tamPag;
	int offset = pcbAEjecutar->stackPointer % tamPag;

	t_stack_entry* lineaStack;
	if(list_is_empty(pcbAEjecutar->indice_stack->elements)){
		lineaStack = stack_entry_create_and_initialize();
		list_add(pcbAEjecutar->indice_stack->elements, lineaStack);
	}else{
		lineaStack = list_get(pcbAEjecutar->indice_stack->elements, list_size(pcbAEjecutar->indice_stack->elements) - 1);
	}

	if(!esArgumento(var_nombre)){ // Es una variable
		printf("ANSISOP_definirVariable %c \n", var_nombre);
		t_var* nuevaVar = malloc(sizeof(t_var));
		nuevaVar->var_id = var_nombre;
		nuevaVar->page_number = pagina;
		nuevaVar->offset = offset;
		nuevaVar->tamanio = TAM_VARIABLE;
		list_add(lineaStack->vars, nuevaVar);
	}
	else{ // Es un argumento.
		printf("ANSISOP_definirVariable (argumento) %c \n", var_nombre);
		t_arg* nuevoArg = malloc(sizeof(t_var));
		nuevoArg->page_number = pagina;
		nuevoArg->offset = offset;
		nuevoArg->tamanio = TAM_VARIABLE;
		list_add(lineaStack->args, nuevoArg);
	}

	int posAbsoluta = pcbAEjecutar->stackPointer;
	pcbAEjecutar->stackPointer += TAM_VARIABLE;
	printf("Posicion relativa de %c: %d %d %d \n", var_nombre, pagina, offset, TAM_VARIABLE);
	printf("Posicion absoluta de %c: %i \n", var_nombre, posAbsoluta);
	return posAbsoluta;
}

t_puntero obtenerPosicionVariable(t_nombre_variable var_nombre){
	printf("ANSISOP_obtenerPosicion %c \n", var_nombre);
	if(list_is_empty(pcbAEjecutar->indice_stack->elements)){
		printf("En indice de stack se encuentra vacio \n");
		return EXIT_FAILURE;//DEBERÍA PRODUCIR UN ERROR
	}

	t_puntero posicionAbsoluta;
	t_stack_entry* contexto = list_get(pcbAEjecutar->indice_stack->elements, list_size(pcbAEjecutar->indice_stack->elements) - 1);

	if(!esArgumento(var_nombre)){ //Es una variable
		t_var* var_local;
		bool notFound = true;
		int i;
		for(i=0; i < contexto->cant_vars/*list_size(contexto->vars)*/; i++){
			var_local = list_get(contexto->vars, i);
			if(var_local->var_id == var_nombre){
				notFound = false;
				break;
			}
		}
		if(notFound){
			printf("No se encontro la variable %c en el stack \n", var_nombre);
			return EXIT_FAILURE;//DEBERÍA PRODUCIR UN ERROR
		}
		else{
			posicionAbsoluta = var_local->page_number * tamPag + var_local->offset;
		}
	} else { //Es un argumento
		if((var_nombre-'0') > contexto->cant_args/*list_size(contexto->args)*/){
			return EXIT_FAILURE;//DEBERÍA PRODUCIR UN ERROR
		}else{
			t_var* argumento = list_get(contexto->args, var_nombre-'0');
			//EL ARGUMENTO X TIENE QUE ESTAR NECESARIAMENTE EN LA POSICIÓN X??
			posicionAbsoluta = argumento->page_number * tamPag + argumento->offset;
		}
	}
	printf("Posicion absoluta de %c: %d \n", var_nombre, posicionAbsoluta);
	return posicionAbsoluta;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){
	printf("ANSISOP_dereferenciar posicion: %d \n", direccion_variable);

	int pagina = (direccion_variable / tamPag) + pcbAEjecutar->cant_paginas_de_codigo;
	int offset = direccion_variable % tamPag;
	int tamanio = TAM_VARIABLE;

	t_valor_variable* ptrValor = (int*)conseguirDatosDeLaMemoria(pcbAEjecutar->id_proceso, pagina, offset, tamanio);
		//conseguirDatosDeLaMemoria ESTÁ EN MEMORIA.C
		//SE PUEDE CASTEAR UN ARRAY DE 4 CHARS A INT ASÍ??
		//CAPAZ NO SE OBTENGA BIEN EL VALOR: VER ENDIANNESS
	//DEBERÍA RECIBIR UNA CONFIRMACIÓN??
	t_valor_variable valor = *ptrValor;
	free(ptrValor);
	printf("Se obtuvo el valor: %d \n", valor);
	return valor;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	printf("ANSISOP_asignar (valor: %d, direccion_variable: %d\n)", valor, direccion_variable);
	int pagina, offset, tamanio;
	pagina = (direccion_variable / tamPag) + pcbAEjecutar->cant_paginas_de_codigo;
	offset = direccion_variable % tamPag;
	tamanio = TAM_VARIABLE;

	//Enviar mensaje a Memoria: escribirPagina(pcbAEjecutar->id_proceso, pagina, offset, tamanio, (char*)&valor);
	//DEBERÍA RECIBIR UNA CONFIRMACIÓN??
}

void irAlLabel(t_nombre_etiqueta nombre_etiqueta){
	printf("ANSISOP_irALabel %s\n", nombre_etiqueta);
	t_puntero_instruccion numeroInstr; //= busquedaEtiqueta(nombre_etiqueta);//PENDIENTE
	printf("Numero de instruccion: %d", numeroInstr);
	if(numeroInstr == -1){
		printf("No se encontro la etiqueta\n");
		return;
	}
	pcbAEjecutar->program_counter = numeroInstr - 1; //ES NECESARIO EL -1 ??
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){
	printf("ANSISOP_llamarSinRetorno %s\n", etiqueta);
	t_stack_entry* nuevaLineaStack = stack_entry_create_and_initialize();
	nuevaLineaStack->ret_pos = pcbAEjecutar->program_counter;
	list_add(pcbAEjecutar->indice_stack->elements, nuevaLineaStack);
	irAlLabel(etiqueta);
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	printf("ANSISOP_llamarConRetorno (etiqueta: %s, retornar: %d)\n", etiqueta, donde_retornar);
	t_stack_entry* nuevaLineaStackEjecucionActual;
	nuevaLineaStackEjecucionActual = stack_entry_create_and_initialize();
	nuevaLineaStackEjecucionActual->ret_vars->page_number = donde_retornar / tamPag;
	nuevaLineaStackEjecucionActual->ret_vars->offset = donde_retornar % tamPag;
	nuevaLineaStackEjecucionActual->ret_vars->tamanio = TAM_VARIABLE;
	nuevaLineaStackEjecucionActual->ret_pos = pcbAEjecutar->program_counter;
	list_add(pcbAEjecutar->indice_stack->elements, nuevaLineaStackEjecucionActual);
	irAlLabel(etiqueta);
}

void finalizar(void){
	printf("ANSISOP_finalizar\n");
	//Obtengo contexto quitado de la lista y lo limpio.
	t_stack_entry* contexto = list_remove(pcbAEjecutar->indice_stack->elements, list_size(pcbAEjecutar->indice_stack->elements) - 1);
	int i;
	if(contexto != NULL){
		pcbAEjecutar->stackPointer -= TAM_VARIABLE * (list_size(contexto->args) + list_size(contexto->vars)); // Disminuyo stackPointer del pcb
		if(pcbAEjecutar->stackPointer >= 0){
			for(i=0; i<list_size(contexto->args); i++){ // Limpio lista de argumentos del contexto
				free(list_remove(contexto->args,i));
			}
			for(i=0; i<list_size(contexto->vars); i++){
				free(list_remove(contexto->vars, i));
			}
		}
		list_destroy(contexto->args);
		list_destroy(contexto->vars);
	}
	if(list_is_empty(pcbAEjecutar->indice_stack->elements)){
		terminoPrograma = true;
		printf("Finalizó la ejecucion del programa\n");
		//finalizarPor(OP_CPU_PROGRAM_END);
	}else{
		pcbAEjecutar->program_counter = contexto->ret_pos;
	}
	free(contexto);
}

void retornar(t_valor_variable var_retorno){
	printf("ANSISOP_retornar\n");
	// Tomo contexto actual:
	t_stack_entry* registroActual = list_get(pcbAEjecutar->indice_stack->elements, list_size(pcbAEjecutar->indice_stack->elements) - 1
			/*pcbActual->stack->elements_count -1 ES LO MISMO??*/);
	// Calculo la dirección de retorno a partir de retVar:
	t_puntero offset_absoluto = (registroActual->ret_vars->page_number * tamPag) + registroActual->ret_vars->offset;
	asignar(offset_absoluto, var_retorno);
	finalizar();
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida var_compartida_nombre){

}

t_valor_variable asignarValorCompartida(t_nombre_compartida var_compartida_nombre, t_valor_variable var_compartida_valor){

}

//OPERACIONES DE KERNEL

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
