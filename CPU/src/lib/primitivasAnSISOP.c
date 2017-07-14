#include <ctype.h>
#include "primitivasAnSISOP.h"

#define TAM_VARIABLE 4

bool terminoPrograma;
int codigoError;

//VALORES A INICIALIZAR
t_pcb* pcbAEjecutar;
int stackSize;
int tamPag;
int serv_kernel;
int serv_memoria;

//FUNCIONES EXTRA

void inicializarPrimitivasANSISOP(t_pcb* _pcbAEjecutar, int _stackSize, int _tamPag, int _serv_kernel, int _serv_memoria){
	pcbAEjecutar = _pcbAEjecutar;
	stackSize = _stackSize;
	tamPag = _tamPag;
	serv_kernel = _serv_kernel;
	serv_memoria = _serv_memoria;

	terminoPrograma = false;
	codigoError = 0;
}

bool esArgumento(t_nombre_variable identificador_variable){
	return isdigit(identificador_variable);
}

bool terminoElPrograma(void){
	return terminoPrograma;
}

int hayError(){
	return codigoError;
}

void esperarSenialDeMemoria() {
	char senial[2] = "a";
	if (recv(serv_memoria, senial, 2, 0) == -1) {
		printf("Error al recibir senial antes de enviar paginas\n");
	}
}

void avisarAccionAMemoria(int accion) {
	u_int32_t aux = accion;
	if (send(serv_memoria, &aux, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando la accion.\n");
		exit(-1);
	}
	esperarSenialDeMemoria();
}

void enviarBufferAMemoria(char *buffer, int tamanio) {
	if (send(serv_memoria, &tamanio, sizeof(int), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	esperarSenialDeMemoria();
	if (send(serv_memoria, buffer, tamanio, 0) == -1) {
		printf("Error enviando el buffer\n");
		exit(-1);
	}
}

void enviarIntAMemoria(int valor){
	if (send(serv_memoria, &valor, sizeof(int), 0) == -1) {
		printf("Error enviando mensaje a Memoria\n");
		exit(-1);
	}
}

char * conseguirDatosDeLaMemoria(int PID, int nroPag, int offset, int tamanio) {
	char* instruccion = reservarMemoria(tamanio);
	avisarAccionAMemoria(cpu_mem_leer);
	//Uso la misma función, aunque estoy pasando parámetros
	avisarAccionAMemoria(PID);
	avisarAccionAMemoria(nroPag);
	avisarAccionAMemoria(offset);
	avisarAccionAMemoria(tamanio);
	if (recv(serv_memoria, instruccion, tamanio, 0) == -1) {
		printf("Error al recibir instrucción de Memoria\n");
	}
	printf("Instruccion recibida: %s\n", instruccion);
	return instruccion;
}

void solicitarEscrituraAMemoria(int pid, int nroPagina, int offset, int tamanio,
		char *bytesAEscribir) {
	avisarAccionAMemoria(cpu_mem_escribir);
	enviarIntAMemoria(pid);
	enviarIntAMemoria(nroPagina);
	enviarIntAMemoria(offset);
	enviarBufferAMemoria(bytesAEscribir,tamanio);

}

void solicitarA(int *cliente, char *nombreCli) {
	char a[2] = "a";
	send((*cliente), a, 2, 0);
	printf("Esperando atencion de %s..\n", nombreCli);
	recv((*cliente), a, 2, 0);
}

void esperarSenialDeKernel() {
	char senial[2] = "a";
	if (recv(serv_kernel, senial, 2, 0) == -1) {
		printf("Error al recibir senial de Kernel\n");
	}
}

void enviarSenialAKernel() {
	char senial[2] = "a";
	if (send(serv_kernel, senial, 2, 0) == -1) {
		printf("Error al enviar la senial\n");
		exit(-1);
	}
}

void enviarIntAKernel(int mensaje){
	esperarSenialDeKernel();
	if(send(serv_kernel,&mensaje,sizeof(int),0)==-1){
		printf("Error enviando mensaje a Kernel.\n");
	}
}

void enviarPathAKernel(char *path){
	int tamPath = strlen(path)+1;
	esperarSenialDeKernel();
	if (send(serv_kernel, &tamPath, sizeof(int), 0) == -1) {
		printf("Error enviando la longitud del Path\n");
		exit(-1);
	}
	esperarSenialDeKernel();
	if (send(serv_kernel, path, tamPath, 0) == -1) {
		printf("Error enviando el Path\n");
		exit(-1);
	}
}

void enviarBanderasAKernel(t_banderas flags){
	esperarSenialDeKernel();
	if (send(serv_kernel, &flags, sizeof(flags), 0) == -1) {
		printf("Error enviando las banderas\n");
		exit(-1);
	}
}

u_int32_t recibirUIntDeKernel() {
	u_int32_t valor;
	if (recv(serv_kernel, &valor, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo u_int32_t de Kernel\n");
		exit(-1);
	}
	return valor;
}

//PRIMITIVAS

t_puntero definirVariable(t_nombre_variable var_nombre){
	if((pcbAEjecutar->stackPointer+4) > (stackSize * tamPag)){
		printf("StackOverflow. Se finaliza el proceso\n");
		codigoError = -10;//Defino nuevo Exit Code -10: stackOverflow (!!)
		return -1;//DEBERÍA PRODUCIR UN ERROR?
	}

	int pagina = pcbAEjecutar->stackPointer / tamPag;
	int offset = pcbAEjecutar->stackPointer % tamPag;

	t_stack_entry* lineaStack;
	if(list_is_empty(pcbAEjecutar->indice_stack->elements)){
		lineaStack = stack_entry_create();
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
		add_var(&lineaStack, nuevaVar);
	}
	else{ // Es un argumento
		printf("ANSISOP_definirVariable (argumento) %c \n", var_nombre);
		t_arg* nuevoArg = malloc(sizeof(t_var));
		nuevoArg->page_number = pagina;
		nuevoArg->offset = offset;
		nuevoArg->tamanio = TAM_VARIABLE;
		add_arg(&lineaStack, nuevoArg);
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
		return EXIT_FAILURE;//DEBERÍA PRODUCIR UN ERROR   (PENDIENTE!!)
	}

	t_puntero posicionAbsoluta;
	t_stack_entry* contexto = list_get(pcbAEjecutar->indice_stack->elements, list_size(pcbAEjecutar->indice_stack->elements) - 1);

	if(!esArgumento(var_nombre)){ //Es una variable
		t_var* var_local;
		bool notFound = true;
		int i;
		for(i=0; i < contexto->cant_vars/*list_size(contexto->vars)*/; i++){
			var_local = &contexto->vars[i];
			if(var_local->var_id == var_nombre){
				notFound = false;
				break;
			}
		}
		if(notFound){
			printf("No se encontro la variable %c en el stack \n", var_nombre);
			return EXIT_FAILURE;//DEBERÍA PRODUCIR UN ERROR  (PENDIENTE!!)
		}
		else{
			posicionAbsoluta = var_local->page_number * tamPag + var_local->offset;
		}
	} else { //Es un argumento
		if((var_nombre-'0') > contexto->cant_args/*list_size(contexto->args)*/){
			return EXIT_FAILURE;//DEBERÍA PRODUCIR UN ERROR  (PENDIENTE!!)
		}else{
			t_arg* argumento =  &contexto->args[var_nombre-'0'];
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

	solicitarEscrituraAMemoria(pcbAEjecutar->id_proceso, pagina, offset, tamanio, (char*)&valor);
	//DEBERÍA RECIBIR UNA CONFIRMACIÓN??
}

void irAlLabel(t_nombre_etiqueta nombre_etiqueta){
	printf("ANSISOP_irALabel %s\n", nombre_etiqueta);
	t_puntero_instruccion numeroInstr = metadata_buscar_etiqueta(nombre_etiqueta, pcbAEjecutar->indice_etiquetas, pcbAEjecutar->etiquetas_size);
	//busquedaEtiqueta(nombre_etiqueta); NECESARIO??
	printf("Numero de instruccion: %d", numeroInstr);
	if(numeroInstr == -1){
		printf("No se encontro la etiqueta\n");
		return;
	}
	pcbAEjecutar->program_counter = numeroInstr - 1; //ES NECESARIO EL -1 ??
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){
	printf("ANSISOP_llamarSinRetorno %s\n", etiqueta);
	t_stack_entry* nuevaLineaStack = stack_entry_create();
	nuevaLineaStack->ret_pos = pcbAEjecutar->program_counter;
	list_add(pcbAEjecutar->indice_stack->elements, nuevaLineaStack);
	irAlLabel(etiqueta);
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	printf("ANSISOP_llamarConRetorno (etiqueta: %s, retornar: %d)\n", etiqueta, donde_retornar);
	t_stack_entry* nuevaLineaStackEjecucionActual;
	nuevaLineaStackEjecucionActual = stack_entry_create();
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
	//int i;
	if(contexto != NULL){
		pcbAEjecutar->stackPointer -= TAM_VARIABLE * (contexto->cant_args + contexto->cant_vars); // Disminuyo stackPointer del pcb
		if(pcbAEjecutar->stackPointer >= 0){
			/*for(i=0; i<contexto->cant_args; i++){ // Limpio lista de argumentos del contexto   NECESARIO??
				free(list_remove(contexto->args,i));
			}
			for(i=0; i<contexto->cant_vars; i++){
				free(list_remove(contexto->vars, i));
			}*/
		}
		free(contexto->args);//list_destroy(contexto->args);
		free(contexto->vars);//list_destroy(contexto->vars);
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
	//Appendeo el '!'
	char *nombreVariable = string_new();
	string_append(&nombreVariable, "!");
	string_append(&nombreVariable, var_compartida_nombre);
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_obtener_valor_compartida);
	enviarPathAKernel(nombreVariable);//Asumo que el nombre termina con un \0
	int valor = recibirUIntDeKernel(); //EN REALIDAD, PODRÍA FALLAR Y SE DEBERÍA PRODUCIR UN ERROR
	free(nombreVariable);
	return valor;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida var_compartida_nombre, t_valor_variable var_compartida_valor){
	//Appendeo el '!'
	char *nombreVariable = string_new();
	string_append(&nombreVariable, "!");
	string_append(&nombreVariable, var_compartida_nombre);
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_asignar_valor_compartida);
	enviarPathAKernel(nombreVariable);//Asumo que el nombre termina con un \0
	enviarIntAKernel(var_compartida_valor);
	//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE. EN REALIDAD, PODRÍA FALLAR Y SE DEBERÍA PRODUCIR UN ERROR
	free(nombreVariable);
	return var_compartida_valor;
}

//OPERACIONES DE KERNEL

void wait(t_nombre_semaforo identificador_semaforo){

}

void signal(t_nombre_semaforo identificador_semaforo){

}

t_puntero reservar(t_valor_variable espacio){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_reservar);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	//enviarIntAKernel(pcbAEjecutar->) FALTA ENVIAR CONTADOR_PAGINAS (HASTA QUE NO SE AGREGUE VA A FALLAR, XQ KERNEL LO ESPERA!!)
	enviarIntAKernel(espacio);
	int confirmacion = recibirUIntDeKernel();
	u_int32_t direccion = recibirUIntDeKernel();
	if (confirmacion==noSePudoReservarMemoria){
		printf("El espacio requerido supera el tamaño máximo reservable por petición (Exit Code -8)\n");
		codigoError = -8;
	} else if (confirmacion==noHayPaginas){
		printf("No se pueden asignar mas paginas al proceso. No hay mas páginas en Memoria (Exit Code -9)\n");
		codigoError = -9;
	} else {
		printf("Se pudo reservar memoria, direccion: %u\n", direccion);
		//INCREMENTAR PCB->CONTADOR_DE_PAGINAS (PENDIENTE!!!!)
	}
	return direccion;//OJO! DEVUELVE 0 SI NO SE PUDO RESERVAR
}

void liberar(t_puntero puntero){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_liberar);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarIntAKernel(puntero);
	int confirmacion = recibirUIntDeKernel();
	if (confirmacion == noSePudoLiberarMemoria){
		printf("No se pudo liberar Memoria, dicha memoria no está asignada al proceso (Exit Code -11)\n");
		codigoError = -11;//Defino nuevo Exit Code -11: error al liberar Memoria (!!)
	} else if (confirmacion==exitoLiberacionPagina){
		printf("Se pudo liberar Memoria, y tambien la pagina\n");
		//DECREMENTAR PCB->CONTADOR_DE_PAGINAS (PENDIENTE!!!!)
	} else /*confirmacion==falloLiberacionPagina || confirmacion==sePudoLiberarMemoria*/{
		printf("Se pudo liberar Memoria\n");
	}
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_abrir_archivo);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarPathAKernel(direccion);
	enviarBanderasAKernel(flags);
	t_descriptor_archivo fd = recibirUIntDeKernel();
	if(fd != 0){
		printf("FD del archivo: %u\n", fd);
		return fd;
	} else { //En realidad 0 es un FD válido, pero como está reservado y no lo usamos lo uso para indicar el error
		printf("El programa intentó acceder a un archivo que no existe (Exit Code -2)\n");
		codigoError = -2;
		return 0; //NO!! SOLO LO PUSE PARA QUE NO ROMPA, NO SÉ QUÉ TIENE QUE DEVOLVER !!
	}
}

void borrar(t_descriptor_archivo descriptor_archivo){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_borrar_archivo);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarIntAKernel(descriptor_archivo);
	int confirmacion = recibirUIntDeKernel();
	if(confirmacion == k_cpu_error){
		//ERROR      PENDIENTE!!!
	}
}

void cerrar(t_descriptor_archivo descriptor_archivo){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_cerrar_archivo);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarIntAKernel(descriptor_archivo);
	//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_mover_cursor_archivo);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarIntAKernel(descriptor_archivo);
	enviarIntAKernel(posicion);
	//NO RECIBE CONFIRMACIÓN, ASUMO QUE SE REALIZA CORRECTAMENTE
}

void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_escribir_archivo);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarIntAKernel(descriptor_archivo);
	enviarIntAKernel(tamanio);
	esperarSenialDeKernel();
	if (send(serv_kernel, informacion, tamanio, 0) == -1) {
		printf("Error al enviar bytes a escribir\n");
		exit(-1);
	}
	int confirmacion = recibirUIntDeKernel();
	if (confirmacion == k_cpu_accion_OK){
		printf("Escritura realizada correctamente\n");
	} else {
		printf("El programa intentó escribir un archivo sin permisos (Exit Code -4)\n");
		codigoError = -4;
	}
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	solicitarA(&serv_kernel,"Kernel");
	enviarIntAKernel(cpu_k_leer_archivo);
	enviarIntAKernel(pcbAEjecutar->id_proceso);
	enviarIntAKernel(descriptor_archivo);
	enviarIntAKernel(tamanio);
	int respuesta = recibirUIntDeKernel();
	char* bytesLeidos;
	if (respuesta!=k_cpu_error){
		enviarSenialAKernel();
		if (recv(serv_kernel, bytesLeidos, tamanio, 0) == -1) {
			printf("Error al recibir bytes leidos de Kernel\n");
		}
	} else {
		printf("El programa intentó leer un archivo sin permisos (Exit Code -3)\n");
		codigoError = -3;
		return;
	}

	//Hay que escribir los bytesLeidos en la posicion de memoria que indica informacion
	int nroPag = informacion / tamPag;
	int offset = informacion % tamPag;

	solicitarEscrituraAMemoria(pcbAEjecutar->id_proceso, nroPag, offset, tamanio, bytesLeidos);
	//CREO QUE NO SE PUEDE USAR asignar(informacion, bytesLeidos) PORQUE EL TAMANIO PUEDE SER MAYOR AL DE UNA VARIABLE (4 BYTES)
}
