#include "broker.h"
#include "suscripciones.h"
#include <signal.h>
#include <time.h>

int main(void) {

	signal(SIGUSR1, dump_cache);

	char* ip;
	char* puerto;
	int i=0;

	fflush(stdout);

	char* conf = "/home/utnso/tp-2020-1c-NN/broker/src/broker.config";

	logger = log_create("/home/utnso/broker.txt", "Broker", 1, LOG_LEVEL_INFO);

	config=config_create(conf);

	ip= config_get_string_value(config,"IP_BROKER");
	puerto= config_get_string_value(config,"PUERTO_BROKER");

	log_info(logger,"Servidor con IP %s y puerto %s", ip, puerto);

	contador_de_id = 0;
	pthread_mutex_init(&mx_id_mensaje, NULL);

	//INIT MEMORY

		pthread_mutex_init(&mx_memoria, NULL);
		pthread_mutex_init(&mx_mostrar, NULL);
		pthread_mutex_init(&mx_lru, NULL);

		frecuencia_compactacion = config_get_int_value(config,"FRECUENCIA_COMPACTACION");
		tamanio_minimo = config_get_int_value(config,"TAMANO_MINIMO_PARTICION");
		memory_size = config_get_int_value(config,"TAMANO_MEMORIA");
		algoritmo_memoria = config_get_string_value(config,"ALGORITMO_MEMORIA");
		algoritmo_particion_libre = config_get_string_value(config,"ALGORITMO_PARTICION_LIBRE");
		algoritmo_reemplazo = config_get_string_value(config,"ALGORITMO_REEMPLAZO");

		memoria = malloc(memory_size);
		contador_id_particiones = 0;
		tiempo_lru = 0;
		particiones = list_create();
		if(strcmp(algoritmo_memoria, "PARTICIONES") == 0){
			particiones_libres = list_create();
			ParticionLibre *primera_particion_libre = malloc(sizeof(ParticionLibre));
			primera_particion_libre->size = memory_size;
			primera_particion_libre->offset_init = 0;
			list_add(particiones_libres, primera_particion_libre);
		}

		if(strcmp(algoritmo_memoria, "BS") == 0){
			contador_id_buddy = 0;
			buddies = list_create();
			Buddy* primer_buddy = malloc(sizeof(Buddy));
			primer_buddy->size = memory_size;
			primer_buddy->id = contador_id_buddy;
			primer_buddy->id_padre = -1;
			primer_buddy->id_buddy = -1;
			primer_buddy->id_hijo1 = -1;
			primer_buddy->id_hijo2 = -1;
			primer_buddy->esta_libre = 1;
			primer_buddy->offset_init = 0;
			contador_id_buddy++;
			list_add(buddies, primer_buddy);
		}


	/* EJEMPLO PARA TESTEAR LA COMPACTACION
	  t_mensaje *msg = malloc(sizeof(t_mensaje));
	msg->pokemon = malloc(sizeof(char));
	msg->pokemon = "h";

	ParticionLibre* l1 = list_get(particiones_libres, 0 );
	log_info(logger, "cantidad libres antes %d  cantidad particiones %d size libre %d offset %d", list_size(particiones_libres), list_size(particiones), l1->size, l1->offset_init);
	almacenar_particion(msg, "GET");
		log_info(logger, "cantidad libres despues %d   cantidad particiones %d size libre %d offset %d", list_size(particiones_libres), list_size(particiones), l1->size, l1->offset_init);

		log_info(logger, "cantidad libres antes %d  cantidad particiones %d ", list_size(particiones_libres), list_size(particiones));
			almacenar_particion(msg, "GET");
				log_info(logger, "cantidad libres despues %d   cantidad particiones %d size libre %d offset %d", list_size(particiones_libres), list_size(particiones), l1->size, l1->offset_init);

				log_info(logger, "cantidad libres antes %d  cantidad particiones %d ", list_size(particiones_libres), list_size(particiones));
				msg->pokemon = "m";
				almacenar_particion(msg, "GET");
						log_info(logger, "cantidad libres despues %d   cantidad particiones %d size libre %d offset %d", list_size(particiones_libres), list_size(particiones), l1->size, l1->offset_init);

						msg->pokemon = "hahahahah";
						almacenar_particion(msg, "GET");
			log_info(logger, "cantidad libres despues %d   cantidad particiones %d ", list_size(particiones_libres), list_size(particiones));



			msg_get* mostrar = malloc(sizeof(msg_get));
						mostrar->nombre_pokemon = malloc(sizeof(char));
						Particion *p = list_get(particiones, 0);
									memcpy(&mostrar->longitud_nombre, memoria + p->offset_init, sizeof(uint32_t));
									memcpy(mostrar->nombre_pokemon, memoria + p->offset_init + sizeof(uint32_t), mostrar->longitud_nombre);

							log_info(logger,"nombre pokemon( %s )  longitud(%d)", mostrar->nombre_pokemon, mostrar->longitud_nombre);
*/



	int socketero[100];
	int socket_servidor = iniciar_servidor(ip,puerto);

	Colas *colas = malloc(sizeof(Colas));
	colas->SUSCRITOS_GET = list_create();

	colas->cant_suscritos_appeared = 0;
	colas->cant_suscritos_localized = 0;
	colas->cant_suscritos_caught =0;
	colas->cant_suscritos_catch =0;
	colas->cant_suscritos_new =0;


    while(1){
    	colas->socket_cliente = esperar_cliente(socket_servidor);

    	socketero[i]= colas->socket_cliente;
    	log_info(logger,"socketero: %d", socketero[i]);
    	i++;

    	/*pthread_t cliente_thread;
    	pthread_create(&cliente_thread, NULL,(void*) process_request,colas);*/

    	process_request(colas);
    }


	return EXIT_SUCCESS;
}



void dump_cache(int n){

	FILE* f;

	int contador = 0;

	f=fopen("dump.txt","w+");

	fprintf(f, "-----------------------------------------------------------------------------------------------------------------\n");

	time_t tiempo = time(0);
	struct tm *tlocal = localtime(&tiempo);
	char output[128];
	strftime(output, 128, "%d/%m/%y %H:%M:%S", tlocal);

	char* dump = string_new();
	string_append_with_format(&dump, "Dump: %s ",output);
	fprintf(f, "%s\n", dump);



	for(int i= 0; i<list_size(particiones);i++){
	char* particion=string_new();
	Particion* p= list_get(particiones,i);
			string_append_with_format(&particion, "Particion %s: ",string_itoa(contador));
			string_append_with_format(&particion, "%#05X - ", p->offset_init);
			string_append_with_format(&particion, "#%05X.", p->offset_end);
			string_append_with_format(&particion, "	[X] Size: %sb ",string_itoa(p->size) );
			string_append_with_format(&particion, "LRU: %s ", string_itoa(p->tiempo_lru));
			string_append_with_format(&particion, "Cola: %s", p->cola);
			string_append_with_format(&particion, "	 ID: %s", string_itoa(p->id_mensaje));
			fprintf(f, "%s\n\n",particion );
			contador++;
			free(particion);
		}

	for(int i=0; i<list_size(particiones_libres); i++){
		char* particion = string_new();
		ParticionLibre* libre = list_get(particiones_libres, i);
		string_append_with_format(&particion, "Particion %s: ",string_itoa(contador));
		string_append_with_format(&particion, "%#05X - ", libre->offset_init);
		string_append_with_format(&particion, "%#05X.", libre->offset_init + libre->size - 1);
		string_append_with_format(&particion, "	[L] Size: %sb ",string_itoa(libre->size) );
		fprintf(f, "%s\n",particion );
		contador++;
		free(particion);
	}


	fprintf(f, "-----------------------------------------------------------------------------------------------------------------\n");
	fclose(f);

	log_info(logger, "<CACHE> Actualize el dump de la cache!");
}


