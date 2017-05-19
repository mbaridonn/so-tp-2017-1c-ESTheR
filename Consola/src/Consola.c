#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "

typedef struct {
	//char ipKernel[10]; FALTA IMPLEMENTAR
	int puerto;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

void msjConexionCon(char *s) {
	printf(
			"\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",
			s);
} //Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio \n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	//config->ipKernel = config_get_string_value(archivo_Modelo,"IP_KERNEL");
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
}
void mostrarArchivoConfig() {
	FILE *f;

	f = fopen(RUTAARCHIVO, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	printf("\n");
	printf("------------------------------------------\n");

}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	//mostrarArchivoConfig();
	printf("Leí el archivo y extraje el puerto: %d \n\n", config->puerto);
}

void mandarArchivo(int *cliente) {

	char *lineaIngresada, *comando, *nombreArchivo;
	FILE *archivo;

	printf("\nIngrese mandarArchivo + nombre de su archivo\n");
	printf("Ejemplo: mandarArchivo prueba.txt\n\n");

	lineaIngresada = reservarMemoria(100);

	fgets(lineaIngresada, 100, stdin);

	comando = strtok(lineaIngresada, " ");
	nombreArchivo = strtok(NULL, "\n");

	if (strcmp("mandarArchivo", comando) != 0) {
		printf("El comando ingresado no existe");
	} else {
		archivo = fopen(nombreArchivo, "rb");
		if (archivo == NULL) {
			printf("No se pudo leer el archivo\n");
			exit(-1);
		}
	}

	fseek(archivo, 0, SEEK_END);
	u_int32_t fsize = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize + 1);
	fread(buffer, fsize, 1, archivo);
	fclose(archivo);
	buffer[fsize] = '\0';
	if (send(*cliente, &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send(*cliente, buffer, fsize + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		exit(-1); //no deberia darle la opcion de volver a intentar en vez de hacer exit? Lo mismo con los exit de arriba
	}
	printf("El archivo se envió correctamente\n\n");

	free(lineaIngresada);
	free(buffer);

}

void iniciarPrograma() {
	/*Iniciar Programa: Este comando iniciará un nuevo Programa AnSISOP, recibiendo por
	 parámetro el path del script AnSISOP a ejecutar. Una vez iniciado el programa la consola
	 quedará a la espera de nuevos comandos, pudiendo ser el iniciar nuevos Programas AnSISOP
	 o algunas de las siguientes opciones. Quedará a decisión del grupo utilizar paths absolutos o
	 relativos y deberán fundamentar su elección.*/

	char *lineaIngresada, *comando, *nombreScript;

	printf("\nIngrese iniciarPrograma + nombre del script AnSISOP. \n");
	printf("Ejemplo: iniciarPrograma script123\n\n");

	lineaIngresada = reservarMemoria(100);

	fgets(lineaIngresada, 100, stdin);

	comando = strtok(lineaIngresada, " ");
	nombreScript = strtok(NULL, "\n");

	if (strcmp("iniciarPrograma", comando) != 0) {
		printf("El comando ingresado no existe");
	} else {
		//error si el script no se encuentra/no existe
		}

}

void finalizarPrograma() {
	/*Finalizar Programa: Como su nombre lo indica este comando finalizará un Programa
	 AnSISOP, terminando el thread correspondiente al PID que se desee finalizar.*/
}

void desconectarConsola() {
	/*Desconectar Consola: Este comando finalizará la conexión de todos los threads de la consola
	 con el kernel, dando por muertos todos los programas de manera abortiva.*/
	printf("\nConsola desconectada. \n\n");
	exit(-1);
}

void limpiarMensajes() {
	system("clear");
	printf("Consola limpiada! \n\n");
}

void elegirComando(int *cliente) {
	char *opcionIngresada;
	int seguirAbierto = 1; /*Si se va a cerrar sólo en una de las opciones, tendría que ser
	 directamente la "opcionIngresada" la condición del do-while. Por ahora la dejo así*/

	do {
		printf("Los siguientes comandos estan disponibles para ejecutar:\n");
		printf("1-iniciarPrograma\n");
		printf("2-desconectarConsola\n");
		printf("3-limpiarMensajes\n");
		printf("4-mandarArchivo\n\n");

		printf("Ingrese el numero de comando para ejecutarlo:\n");

		opcionIngresada = reservarMemoria(10);
		fgets(opcionIngresada, 10, stdin);

		switch (*opcionIngresada) {
		case '1':
			iniciarPrograma();
			break;
		case '2':
			desconectarConsola();
			break;
		case '3':
			limpiarMensajes();
			break;
		case '4':
			mandarArchivo(cliente);
			break;
		default:
			printf("\nOpcion invalida. Vuelva a elegir una opcion \n\n");
			break;
		}

		free(opcionIngresada);
	} while (seguirAbierto == 1);
}

int main(void) {

	leerArchivo();

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1"); //En realidad se le debería pasar config->ipKernel
	direccionServidor.sin_port = htons(config->puerto);

	int cliente;

	conectar(&cliente, &direccionServidor);
	int procesoConectado = handshake(&cliente, consola);

	switch (procesoConectado) {
	case kernel:
		msjConexionCon("Kernel");
		elegirComando(&cliente);

		break;

	default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(cliente);

	return 0;
}
