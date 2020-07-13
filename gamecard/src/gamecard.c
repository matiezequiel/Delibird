#include "gamecard.h"
int main(/*int argc, char* argv[]*/) {

	//ID_PROCESO = argv[1];

	char* conf = "/home/utnso/tp-2020-1c-NN/gamecard/src/gamecard.config";

	logger = log_create("/home/utnso/log_gamecard.txt", "Gamecard", 1, LOG_LEVEL_INFO);
	config = config_create(conf);

	char* montajeFS= config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS");
	FILE* f= fopen(montajeFS, "r");

	if(f==NULL){
		fs(config,blockSize,cantBlocks);
	}else{
		fclose(f);
	}
	free(montajeFS);

	ip = config_get_string_value(config,"IP_BROKER");
	puerto = config_get_string_value(config,"PUERTO_BROKER");

	tiempoReconexion = config_get_int_value(config,"TIEMPO_DE_REINTENTO_CONEXION");

	tiempoDeReintento = config_get_int_value(config,"TIEMPO_DE_REINTENTO_OPERACION");

	tiempoDeRetardo = config_get_int_value(config,"TIEMPO_RETARDO_OPERACION");

	log_info(logger,"Lei la IP %s y puerto %s", ip, puerto);


	pthread_mutex_init(&mxArchivo, NULL);

//HILOS DE CONEXIONES

	pthread_t conexionGet;
	pthread_create(&conexionGet, NULL,(void*)funcionGet,NULL);

	pthread_t conexionNew;
	pthread_create(&conexionNew, NULL,(void*)funcionNew,NULL);

	pthread_t conexionCatch;
	pthread_create(&conexionCatch, NULL,(void*)funcionCatch, NULL);

	pthread_t conexionGameboy;
	pthread_create(&conexionGameboy, NULL,(void*) conexion_gameboy, NULL);

	pthread_join(conexionGameboy,NULL);
	pthread_join(conexionGet,NULL);
	pthread_join(conexionNew,NULL);
	pthread_join(conexionCatch,NULL);

	pthread_exit(&conexionGet);
	pthread_exit(&conexionNew);
	pthread_exit(&conexionCatch);
	pthread_exit(&conexionGameboy);

	free(mntPokemon);
	free(mntBlocks);
	config_destroy(config);

	return EXIT_SUCCESS;
}
//PARA EL GET
void buscarPokemon(t_mensaje* mensaje){

	FILE * f;
	t_buffer* buffer;

	if(existePokemon(mensaje)){

		char** arrayBloques = agarrarBlocks(mensaje);

		char* montaje = montarPoke(mensaje);
		string_append(&montaje, "/Metadata.bin");
		esperaOpen(montaje);

		//falta malloc

		char* datosArchivoSinFormato = string_new();

		int i = 0;
		//Con este while recorro cada .bin
		while(arrayBloques[i] != NULL){
			char* montajeBlocks=montarBlocks(arrayBloques,i);
			f= fopen(montajeBlocks, "r");

			if(f == NULL){
	            log_info(logger,"El bloque %s no existe", arrayBloques[i]);
	        }else{
	        	int c;
	        	do {
	        	      c = fgetc(f);
	        	      if( feof(f) ) {
	        	         break ;
	        	      }
	        	      string_append_with_format(&datosArchivoSinFormato,"%c",c);
	        	   } while(1);
	        	//esperar por consigna antes de cerrar archivo
	        	sleep(tiempoDeRetardo);
	        	fclose(f);
	        }
	        free(montajeBlocks);
	        i++;
	        }

	    freeDoblePuntero(arrayBloques);


	    char** arrayDatos= string_split(datosArchivoSinFormato, "\n");

	    free(datosArchivoSinFormato);

	    int socketLoc = crear_conexion(ip, puerto);

	    if(socketLoc ==-1){
	    	log_info(logger,"No se pudo conectar con el broker.");

	    }else{

	    	t_mensaje_get* mensajeGet = malloc(sizeof(t_mensaje_get));
	    	mensajeGet->pokemon = mensaje->pokemon;
	    	mensajeGet->pokemon_length = mensaje->pokemon_length;
	    	char* posiciones = string_new();
	    	mensajeGet->id_mensaje = 0;
			mensajeGet->id_mensaje_correlativo = mensaje->id_mensaje;
			mensajeGet->cantidad = 0;
			int j = 0;

	    	while(arrayDatos[j] != NULL){
	    		char** partirPosicionesYCantidad;

	    		partirPosicionesYCantidad = string_split(arrayDatos[j],"=");

	    		mensajeGet->cantidad++;

	    		if(j==0){
	    			string_append(&posiciones,partirPosicionesYCantidad[0]);
	    		}else{
	    			string_append_with_format(&posiciones,".%s",partirPosicionesYCantidad[0]);
	    		}

	    		freeDoblePuntero(partirPosicionesYCantidad);
	    		j++;
	    	}

	    	freeDoblePuntero(arrayDatos);

	    	mensajeGet->posiciones_length = strlen(posiciones)+1;
	    	mensajeGet->posiciones = malloc(mensajeGet->posiciones_length);
	    	strcpy(mensajeGet->posiciones,posiciones);

	    	buffer = serializar_mensaje_struct_get(mensajeGet);
	    	enviar_mensaje_struct(buffer,socketLoc,LOCALIZED_POKEMON);
	    	free(posiciones);
	    	free(buffer->stream);
	    	free(buffer);
	    }
	    free(montaje);
	}else{

		int socketLoc = crear_conexion(ip, puerto);

    	t_mensaje_get* mensajeGet = malloc(sizeof(t_mensaje_get));
    	mensajeGet->pokemon = mensaje->pokemon;
    	mensajeGet->pokemon_length = mensaje->pokemon_length;
    	mensajeGet->posiciones = string_new();
    	mensajeGet->posiciones_length = sizeof(mensajeGet->posiciones);
    	mensajeGet->id_mensaje = 0;
		mensajeGet->id_mensaje_correlativo = mensaje->id_mensaje;
		mensajeGet->cantidad = 0;
		buffer = serializar_mensaje_struct_get(mensajeGet);
    	enviar_mensaje_struct(buffer,socketLoc,LOCALIZED_POKEMON);
    	free(buffer->stream);
    	free(buffer);
	}
	free(mensaje);
}


//TODO
//PARA EL CATCH
void agarrarPokemon(t_mensaje* mensaje){

char* montaje= montarPoke(mensaje);

string_append(&montaje,"/Metadata.bin" );
t_buffer* buffer;

int socketCaugth = crear_conexion(ip, puerto);
	if(socketCaugth ==-1){
		log_info(logger,"No se pudo conectar con el broker");
	}else{
		if(existePokemon(mensaje)){
			esperaOpen(montaje);

			//necesito id pokemon posx posy

			FILE* f;
			char* x= string_itoa(mensaje->posx);
			char* y= string_itoa(mensaje->posy);
			int existe=0;

			char* posicionMensaje= string_new();
			string_append(&posicionMensaje, x);
			string_append_with_format(&posicionMensaje, "-%s",y);


			char* arrayTodo=string_new();


			arrayTodo=guardarDatosBins(mensaje);

			char** datosSeparados=string_split(arrayTodo, "\n");
			int j=0;

			while(datosSeparados!=NULL){
				//falta free
				char** posiciones= string_split(datosSeparados[j], "=");

				if(strcmp(posicionMensaje,posiciones[0])==0){
					//verificador q existe la pos
					existe=1;
					if(atoi(posiciones[1])<2){
						datosSeparados[j]= datosSeparados[j+1];

						if(datosSeparados[0]==NULL){
							// hay q clean el bloque y cambiar al meta sin bloques
							char** listaNull = NULL;
							escrituraDeMeta(f,mensaje,listaNull,montaje,"");
						}

					}else{
						//falta free
						char* nuevaLinea=string_new();

						int cantidad= atoi(posiciones[1])-1;
						//falta free
						char* nuevaCant= string_itoa(cantidad);

						string_append(&nuevaLinea, posiciones[0]);
						string_append_with_format(&nuevaLinea, "=%s", nuevaCant);
						datosSeparados[j]= nuevaLinea;
					}

					break;
				}
				j++;
			}


			free(posicionMensaje);
			free(x);
			free(y);


//si no existe informa error
			if(existe==0){
				log_info(logger,"ERROR. No existe esa posicion.");
				//manda a broker q no existe.
				mensaje->resultado= 0;
				mensaje->id_mensaje_correlativo= mensaje->id_mensaje;
				buffer = serializar_mensaje_struct(mensaje);
				enviar_mensaje_struct(buffer, socketCaugth, CAUGHT_POKEMON);
				free(buffer->stream);
				free(buffer);
			}else{
				if(datosSeparados[0]!= NULL){
					//escribir en los archivos
					char* datosJuntos= string_new();
					int i=0;

					while(datosSeparados[i]!= NULL){
						string_append_with_format(&datosJuntos, "%s\n", datosSeparados[i]);
						i++;
					}

				//falta clean bit en los bloques viejos

				off_t offsetVacio = primerBloqueDisponible();

				char* montajeBlocks = string_new();
				string_append(&montajeBlocks,mntBlocks);
				string_append_with_format(&montajeBlocks,"/%d.bin",offsetVacio);
				f = fopen(montajeBlocks,"w");
				char **listaBloquesUsados = agregarBloquesAPartirDeString(datosJuntos,f,offsetVacio);


				escrituraDeMeta(f,mensaje,listaBloquesUsados,montaje,datosJuntos);


				freeDoblePuntero(listaBloquesUsados);
				free(datosJuntos);
				free(montajeBlocks);



				//free(nuevaCant);
				}


	//mandar CAUGTH
			mensaje->resultado= 1;
			mensaje->id_mensaje_correlativo= mensaje->id_mensaje;
			buffer = serializar_mensaje_struct(mensaje);
			enviar_mensaje_struct(buffer, socketCaugth, CAUGHT_POKEMON);
			free(buffer->stream);
			free(buffer);
			}


			freeDoblePuntero(datosSeparados);
			free(arrayTodo);



		}else{
			log_info(logger,"ERROR. Pokemon No existente.");
			//mandar a broker q no existe
			mensaje->resultado= 0;
			mensaje->id_mensaje_correlativo= mensaje->id_mensaje;
			buffer = serializar_mensaje_struct(mensaje);
			enviar_mensaje_struct(buffer, socketCaugth, CAUGHT_POKEMON);
			free(buffer->stream);
			free(buffer);
		}




	}
	free(mensaje);
	free(montaje);
}




char* obtenerPokemonString(t_poke* pokemon){
	char* poke;
	char* x=string_new();
	char* y=string_new();
	char* cantPoke=string_new();

		poke= pokemon->nombre;
		x= pokemon->x;
		y= pokemon->y;
		cantPoke= pokemon->cant;


		strcat(poke, " ");
		strcat(poke, x);
		strcat(poke, " ");
		strcat(poke, y);
		strcat(poke, " ");
		strcat(poke, cantPoke);

	return poke;


}

int igualPosicion(FILE *fp, char* mensaje, t_poke* pokemon){

char* poke;
int posCant[3];

	poke=strtok(mensaje," ");
int i= 0;

	while(poke!= NULL){
		posCant[i]=atoi(poke);
		poke= strtok(NULL, " ");
		i++;
	}

	if(pokemon->x== posCant[0] && pokemon->y== posCant[1] && pokemon->cant== posCant[2]){
		return 1;
	}else{
		return 0;
	}

}

t_mensaje* obtenerDatosPokemon(FILE* fp, t_mensaje* mensaje){

uint8_t posx;
uint8_t posy;
uint8_t cant;

	if(fp==NULL){
		mensaje->posx= 0;
		mensaje->posy =0;
		mensaje->cantidad=0;
	}else{

		fscanf(fp,"%d-%d=%d", posx,posy,cant );

		mensaje->posx= posx;
		mensaje->posy=posy;
		mensaje->cantidad=cant;

	}
	return mensaje;
}

void eliminarLinea(FILE* fp){





}

void conexion_gameboy(){
	char *ip = "127.0.0.3";
	char *puerto = "5001";
	int socket_gamecard = iniciar_servidor(ip,puerto);

    while(1){
    	int socket_cliente = esperar_cliente(socket_gamecard);

    	process_request(socket_cliente);
    }
}

void funcionNew(int socket){

    int conexionNew = crear_conexion(ip,puerto);

    if(conexionNew == -1){
    	log_info(logger,"Reintenando reconectar cada %d segundos",tiempoReconexion);
        conexionNew = reintentar_conexion(ip,puerto,tiempoReconexion);
    }

    //enviar_mensaje(ID_PROCESO,conexionNew, SUS_NEW);

    //para pruebas con debug
    enviar_mensaje("suscribime",conexionNew, SUS_NEW);

    log_info(logger,"Me suscribi a la cola NEW!");


    while(conexionNew != -1){
    	process_request(conexionNew);
    }
}
void funcionCatch(int socket){

    int conexionCatch =	crear_conexion(ip,puerto);

    if(conexionCatch == -1){
    	log_info(logger,"Reintenando reconectar cada %d segundos",tiempoReconexion);
        conexionCatch = reintentar_conexion(ip,puerto,tiempoReconexion);
    }


    //enviar_mensaje(ID_PROCESO,conexionCatch, SUS_CATCH);

    //para pruebas con debug
    enviar_mensaje("suscribime",conexionCatch, SUS_CATCH);

    log_info(logger,"Me suscribi a la cola CATCH!");


    while(conexionCatch != -1){
    	process_request(conexionCatch);
    }

}
void funcionGet(){
	int conexionGet = crear_conexion(ip,puerto);

	if(conexionGet == -1){
		log_info(logger,"Reintenando reconectar cada %d segundos",tiempoReconexion);
        conexionGet = reintentar_conexion(ip,puerto,tiempoReconexion);
	}

//	enviar_mensaje(ID_PROCESO,conexionGet, SUS_GET);

	//para pruebas con debug
	enviar_mensaje("suscribime",conexionGet, SUS_GET);

	log_info(logger,"Me suscribi a la cola GET!");


	while(conexionGet != -1){
		process_request(conexionGet);
	}

}
void funcionACK(int id_mensaje){
	int conexionRespuesta;
	conexionRespuesta = crear_conexion(ip,puerto);
	char* ack = string_new();
	string_append_with_format(&ack,"%s-",ID_PROCESO);
	string_append_with_format(&ack,"%d",id_mensaje);
	enviar_mensaje(ack,conexionRespuesta,ACK);
	free(ack);
}

void process_request(int socket){
	int cod_op;
	t_mensaje* mensaje;

	if(recv(socket, &cod_op, sizeof(op_code), MSG_WAITALL) == -1)
			cod_op = -1;

	switch (cod_op){
	case GET_POKEMON:
		mensaje = recibir_mensaje_struct(socket);
		funcionACK(mensaje->id_mensaje);
		log_info(logger,"Recibi mensaje de contenido pokemon %s y envie confirmacion de su recepcion",mensaje->pokemon);
		pthread_t solicitudGet;
		pthread_create(&solicitudGet, NULL,(void *) buscarPokemon, mensaje);
		pthread_detach(solicitudGet);
		//free(mensaje->pokemon);
		//free(mensaje->resultado);
		//free(mensaje);
		break;
	case NEW_POKEMON:
		mensaje = recibir_mensaje_struct(socket);
		funcionACK(mensaje->id_mensaje);
		log_info(logger,"Recibi mensaje de contenido pokemon %s y envie confirmacion de su recepcion",mensaje->pokemon);
		pthread_t solicitudNew;
		pthread_create(&solicitudNew, NULL,(void *) nuevoPokemon, mensaje);
		pthread_detach(solicitudNew);
		//free(mensaje->pokemon);
		//free(mensaje->resultado);
		//free(mensaje);
		break;
	case CATCH_POKEMON:
		mensaje = recibir_mensaje_struct(socket);
		funcionACK(mensaje->id_mensaje);
		log_info(logger,"Recibi mensaje de contenido pokemon %s y envie confirmacion de su recepcion",mensaje->pokemon);
		pthread_t solicitud;
		pthread_create(&solicitud, NULL,(void*) agarrarPokemon,(gamecard*) mensaje);
		pthread_detach(solicitud);
		//free(mensaje->pokemon);
		//free(mensaje->resultado);
		free(mensaje);
		break;
	case ACK:
		log_info(logger,"hola");
		break;
	case -1:
		//ip = config_get_string_value(config,"IP_GAMECARD");
		//puerto = config_get_string_value(config,"PUERTO_GAMECARD");
		//socket = iniciar_servidor(ip,puerto);
		socket = crear_conexion(ip,puerto);
		log_info(logger,"Codigo de operacion invalido, iniciando servidor gamecard");
		break;
	default:
		exit(1);
		break;
	}
}

void nuevoPokemon(t_mensaje* mensaje){

	FILE * f;
	t_buffer* buffer;
	char* montaje= montarPoke(mensaje);
	pthread_mutex_lock(&mxArchivo);
	f = fopen(montaje,"r");

	//Si el fopen devuelve NULL significa que el archivo no existe entonces creamos uno de 0
	if(f==NULL){
		asignarBloqueYcrearMeta(mensaje,montaje);
	}
	pthread_mutex_unlock(&mxArchivo);
	if(f!=NULL){
			fclose(f);
			string_append(&montaje,"/Metadata.bin");
			f = fopen(montaje,"r");
			cambiar_meta_blocks(montaje,mensaje);
			fclose(f);

	}

	int socketAppeared = crear_conexion(ip, puerto);

	if(socketAppeared ==-1){
		log_info(logger,"No se pudo conectar con el broker.");
	}else{
		mensaje->id_mensaje_correlativo = mensaje->id_mensaje;
		buffer = serializar_mensaje_struct(mensaje);
		enviar_mensaje_struct(buffer, socketAppeared,APPEARED_POKEMON);
		free(buffer->stream);
		free(buffer);
	}
}

//-------------------------------------------------------------------------------------
off_t primerBloqueDisponible(){
	off_t offset = 1;

	while(bitarray_test_bit(bitmap,offset)){
		offset++;
	}
	if(offset>cantBlocks)
		log_info(logger,"todos los bloques estan llenos");

	return offset;
}

char* montarPoke(t_mensaje* mensaje){
	char* montaje= string_new();
	string_append(&montaje,mntPokemon);
	string_append(&montaje,mensaje->pokemon);

	return montaje;
}


void asignarBloqueYcrearMeta(t_mensaje* mensaje,char* montaje){

	FILE* f;
	off_t offset;
	char* bloques = string_new();
	char* escribirBloque = string_new();

	char* x = string_itoa(mensaje->posx);
	char* y = string_itoa(mensaje->posy);
	char* cant = string_itoa(mensaje->cantidad);

	//pthread_mutex_lock(&mxArchivo);
	offset = primerBloqueDisponible();

	string_append(&bloques,mntBlocks);
	string_append_with_format(&bloques,"/%d.bin",offset);
	log_info(logger,"El offset es %d",offset);

	f=fopen(bloques,"w");

	char** listaBloquesUsados;

	if(f!=NULL){
//Bloque
		//En escribirBloque se guarda formato 1-2=3
		string_append(&escribirBloque, x);
		string_append_with_format(&escribirBloque, "-%s",y);
		string_append_with_format(&escribirBloque, "=%s\n",cant);

		listaBloquesUsados = agregarBloquesAPartirDeString(escribirBloque,f,offset);
	}
	//pthread_mutex_unlock(&mxArchivo);
//Metadata

	mkdir(montaje,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	string_append(&montaje,"/Metadata.bin");

	escrituraDeMeta(f,mensaje,listaBloquesUsados,montaje,escribirBloque);

	free(bloques);
	free(escribirBloque);
	free(x);
	free(y);
	free(cant);

	freeDoblePuntero(listaBloquesUsados);

}

char** agregarBloquesAPartirDeString(char* escribirBloque,FILE* f,off_t offset){

	off_t desplazamiento = 0;
	char** bloquesUsados;
	char* contadores = string_new();
	string_append_with_format(&contadores,"%d",offset);
	log_info(logger,contadores);
	char* loQueNoEntraEnUnBloque;
	char* loQueEntraEnUnBloque = string_substring_until(escribirBloque,blockSize);
	if(strlen(escribirBloque)>blockSize)
		loQueNoEntraEnUnBloque = string_substring_from(escribirBloque,blockSize);
	else
		loQueNoEntraEnUnBloque = string_new();
	int cantidadDeBloques = calcularCantidadDeBLoques(escribirBloque);
	char* cantidadDeBloquesString = string_itoa(cantidadDeBloques);
	log_info(logger,cantidadDeBloquesString);
	int i = 1;

	fprintf(f,"%s",loQueEntraEnUnBloque);
	fclose(f);
	free(loQueEntraEnUnBloque);
	char* copia = malloc(strlen(loQueNoEntraEnUnBloque)+1);
	memcpy(copia,loQueNoEntraEnUnBloque,strlen(loQueNoEntraEnUnBloque)+1);
	free(loQueNoEntraEnUnBloque);
	bitarray_set_bit(bitmap,offset);

	while(i != cantidadDeBloques){

		char * loQueEntraEnUnBloque = string_substring_until(copia,blockSize);
		if(strlen(copia)>blockSize){
			char* loQueNoEntraEnUnBloque = string_substring_from(copia,blockSize);
			copia = realloc(copia,strlen(loQueNoEntraEnUnBloque)+1);
			memcpy(copia,loQueNoEntraEnUnBloque,strlen(loQueNoEntraEnUnBloque)+1);
			free(loQueNoEntraEnUnBloque);
		}

		desplazamiento = primerBloqueDisponible();
		string_append_with_format(&contadores,",%d",desplazamiento);
		log_info(logger,contadores);
		char* blocks = string_new();
		string_append(&blocks,mntBlocks);
		string_append_with_format(&blocks,"/%d.bin",desplazamiento);

		f=fopen(blocks,"w");
		fprintf(f,"%s",loQueEntraEnUnBloque);
		fclose(f);
		free(loQueEntraEnUnBloque);
		bitarray_set_bit(bitmap,desplazamiento);

		i++;

		free(blocks);
	}

	bloquesUsados = string_split(contadores,",");

	free(contadores);
	free(copia);
	free(cantidadDeBloquesString);

	return bloquesUsados;
}

int calcularCantidadDeBLoques(char* escribirBloque){
	int cantidadDeBloques;
	int cant = strlen(escribirBloque)/blockSize;
	int resto = strlen(escribirBloque)%blockSize;

	if(resto != 0)
		cantidadDeBloques = cant+1;
	else
		cantidadDeBloques = cant;

	return cantidadDeBloques;
}

int tamRestante(FILE* f){
	int tam;

	fseek(f,0L,SEEK_END);

	tam = blockSize - ftell(f);

	if(tam<0){
		log_info(logger,"tamaño restante menor al blockSize, osea wtf?");
		tam=0;
	}

	fseek(f,0L,SEEK_SET);

	return tam;
}

void recrearBlock(FILE* fblocks,char** blocksRenovados,char* montajeBlocks){
	fblocks= fopen(montajeBlocks, "w+");
	int i = 0;
	while(blocksRenovados[i] != NULL){
		fprintf(fblocks,"%s\n",blocksRenovados[i]);
		i++;
	}
	fclose(fblocks);
	free(montajeBlocks);
	sleep(tiempoDeRetardo);
}

void escrituraDeMeta(FILE* f,t_mensaje* mensaje,char** listaBloquesUsados,char* montaje,char* bloquesActualizados){
	f = fopen(montaje,"w+");
	escribirMeta(f,mensaje,listaBloquesUsados,bloquesActualizados);
	fprintf(f,"OPEN=Y");
	fclose(f);
	log_info(logger,"%d",tiempoDeRetardo);
	log_info(logger,"Empiezo a esperar");
	sleep(tiempoDeRetardo);
	f=fopen(montaje,"w+");
	escribirMeta(f,mensaje,listaBloquesUsados,bloquesActualizados);
	fprintf(f,"OPEN=N");
	fclose(f);
	log_info(logger,"Termino de esperar");

	free(montaje);
}

void escribirMeta(FILE* f,t_mensaje* mensaje,char** lista,char* bloquesActualizados){

	char* blocks = string_new();
	int i=0;
	int lengthBlocks = string_length(bloquesActualizados);
	string_append(&blocks,lista[i]);
	i++;
	while(lista[i] != NULL){
		string_append_with_format(&blocks,",%s",lista[i]);
		i++;
	}

	fprintf(f,"DIRECTORY=N\n");
	fprintf(f,"SIZE=%d\n",lengthBlocks);
	fprintf(f,"BLOCKS=[%s]\n",blocks);

	free(blocks);
}

void cambiar_meta_blocks(char* montaje,t_mensaje* mensaje){
    FILE* fblocks;

    char* datosBins;
    char** listaBloquesUsados;
    char* bloquesActualizados;

    esperaOpen(montaje);

    pthread_mutex_lock(&mxArchivo);
    datosBins = guardarDatosBins(mensaje);

    bloquesActualizados = verificarCoincidenciasYsumarCantidad(datosBins,mensaje);

    off_t offsetVacio = primerBloqueDisponible();

    char* montajeBlocks = string_new();
    string_append(&montajeBlocks,mntBlocks);
    string_append_with_format(&montajeBlocks,"/%d.bin",offsetVacio);
    fblocks = fopen(montajeBlocks,"w");
    listaBloquesUsados = agregarBloquesAPartirDeString(bloquesActualizados,fblocks,offsetVacio);

    escrituraDeMeta(fblocks,mensaje,listaBloquesUsados,montaje,bloquesActualizados);
    pthread_mutex_unlock(&mxArchivo);

    free(datosBins);
    free(bloquesActualizados);
    free(montajeBlocks);

    freeDoblePuntero(listaBloquesUsados);


}

char* verificarCoincidenciasYsumarCantidad(char* datosBins, t_mensaje* mensaje){

    char* mensajePosiciones = string_new();

	char* x = string_itoa(mensaje->posx);
	char* y = string_itoa(mensaje->posy);
	char* cant = string_itoa(mensaje->cantidad);

	string_append(&mensajePosiciones, x);
	string_append_with_format(&mensajePosiciones, "-%s",y);

	char** bloques = string_split(datosBins,"\n");
	char* bloquesActualizados = string_new();

	int i = 0;
	int contadorDeCoincidencias = 0;
	while(bloques[i]!=NULL){
		char** posicionDeBloque;
		posicionDeBloque = string_split(bloques[i],"=");
		if(strcmp(posicionDeBloque[0],mensajePosiciones) == 0){
			char** dividirPosicionCantidad = string_split(bloques[i],"=");
			int cantidadActualizada = mensaje->cantidad + atoi(dividirPosicionCantidad[1]);
			char* cantidadActualizadaString = string_itoa(cantidadActualizada);
			string_append_with_format(&mensajePosiciones, "=%s",cantidadActualizadaString);//Aca el mensajePosicones equivaldria a las posiciones con la cantidad actualizada
			string_append_with_format(&bloquesActualizados,"%s\n",mensajePosiciones);
			contadorDeCoincidencias++;
			freeDoblePuntero(dividirPosicionCantidad);
			free(cantidadActualizadaString);
		}else{
			string_append_with_format(&bloquesActualizados,"%s\n",bloques[i]);
		}
		i++;
		freeDoblePuntero(posicionDeBloque);
	}

	if(contadorDeCoincidencias == 0){
		string_append_with_format(&mensajePosiciones, "=%s",cant);//Aca el mensajePosicones equivaldria al nuevo mensaje que agregamos al final del char* bloquesActualizados
		string_append_with_format(&bloquesActualizados,"%s\n",mensajePosiciones);
	}

	free(mensajePosiciones);
	free(x);
	free(y);
	free(cant);

	freeDoblePuntero(bloques);

	return bloquesActualizados;
}

char* guardarDatosBins(t_mensaje* mensaje){
	char* datosBins = string_new();
	FILE* fblocks;
    int i = 0;
    char** arrayBloques = agarrarBlocks(mensaje);

    //Con este while recorro cada .bin
    while(arrayBloques[i] != NULL){
    	char* montajeBlocks = montarBlocks(arrayBloques, i);
    	fblocks= fopen(montajeBlocks, "r");
    	//Si fblocks es igual a NULL significa que el .bin que trato de abrir no esta en la carpeta Blocks
    	if(fblocks==NULL){
    		log_info(logger,"El bloque %s no existe", arrayBloques[i]);
    	}else{
    		int c;
    		do {
    			c = fgetc(fblocks);
    			if( feof(fblocks) ) {
    				break ;
    			}
    			string_append_with_format(&datosBins,"%c",c);
    		} while(1);

    		fclose(fblocks);

    		off_t offset = atoi(arrayBloques[i]);

    		bitarray_clean_bit(bitmap, offset);

    		log_info(logger,datosBins);
    	}
    	i++;
    	free(montajeBlocks);
    }

    freeDoblePuntero(arrayBloques);

    return datosBins;
}

//FUNCIONES AUXILIARES, MAS VALE QUE SE USAN PA, TE EMBELLECEN TODO EL CODIGO

char* montarBlocks(char** arrayBloques, int i){

	char* montajeBlocks = string_new();
	string_append(&montajeBlocks,mntBlocks);
	string_append_with_format(&montajeBlocks,"/%s.bin",arrayBloques[i]);

	return montajeBlocks;
}


char** agarrarBlocks(t_mensaje* mensaje){

	char* montaje = string_new();
	string_append(&montaje,mntPokemon);
	string_append_with_format(&montaje,"%s/Metadata.bin",mensaje->pokemon);

	char* bloques;
	t_config* configBloques = config_create(montaje);
	bloques = config_get_string_value(configBloques,"BLOCKS");

	char**basura = string_split(bloques,"[");
	char**nroBloque = string_split(basura[0],"]"); //En el nroBloques[0] se guardan los bin de esta forma "1,2,3"
	char**arrayBloques = string_split(nroBloque[0],","); //En la variable arrayBloques se guarda un array de tipo char** de cada .bin

	config_destroy(configBloques);
	free(montaje);
	freeDoblePuntero(basura);
	freeDoblePuntero(nroBloque);

	return arrayBloques;
}

void esperaOpen(char* montaje){
	char * valorOpen;
	t_config* configBloques;

	configBloques=config_create(montaje);

	valorOpen = config_get_string_value(configBloques,"OPEN");
	//reintento volver a abrir
	while(strcmp(valorOpen,"Y") == 0){
		log_info(logger,"Estoy esperando");
		log_info(logger,"%s",valorOpen);
		sleep(tiempoDeReintento);

		config_destroy(configBloques);
	    configBloques = config_create(montaje);

		valorOpen = config_get_string_value(configBloques,"OPEN");
	}
	config_destroy(configBloques);
}

int existePokemon(t_mensaje* mensaje){
	FILE* fp;

	char* montajePokemon = string_new();

	string_append(&montajePokemon, mntPokemon);
	string_append(&montajePokemon, mensaje->pokemon);

	fp= fopen(montajePokemon, "r");
	if(fp==NULL){
		free(montajePokemon);
		return 0;
	}else{
		fclose(fp);
		free(montajePokemon);
		return 1;
	}

}

