#include "fs.h"
t_log* logger;
t_config* config;

pthread_mutex_t mxArchivo;

char* mntPokemon = "/home/utnso/Escritorio/TALL_GRASS/Files/";
char* mntBlocks = "/home/utnso/Escritorio/TALL_GRASS/Blocks";
char* ip;
char* puerto;
int tiempoReconexion,tiempoDeRetardo,tiempoDeReintento;

int contadorBloques = 0;

int sizeBlock = 24;



typedef struct{
	char** array;
	char* pokemon;
}t_array;



typedef struct{
	int x;
	int y;
	int cant;
	char* nombre;
}t_poke;

typedef struct{
    char ** blocksRenovados;
    char * blockARenovar;
}t_block;


typedef struct{
	int socket;
	char* msg;
}gamecard;



void buscarPokemon(t_mensaje* mensaje);
void nuevoPokemon(t_mensaje* mensaje);
void agarrarPokemon(t_mensaje* mensaje);
void funcionACK();
void funcionGet();
void funcionNew();
void funcionCatch();

char* obtenerPokemonString(t_poke* pokemon);
int igualPosicion(FILE *fp, char* mensaje, t_poke* pokemon);
t_mensaje* obtenerDatosPokemon(FILE* fp, t_mensaje* mensaje);



void crearBloquesYMetadata(t_mensaje* mensaje,char* montaje);
void escribirMeta(FILE* f,t_mensaje* mensaje,char* contador);
void cambiar_meta_blocks(char* montaje,t_mensaje* mensaje);
t_block* buscarCoincidencias(FILE* fblocks, t_mensaje* mensaje);
void recrearBlock(FILE* fblocks,char** blockRenovado,char* montajeBlocks);
void verificarTamBlockYActualizarlo(int size,t_block* block, FILE* fblocks, char* montajeBlocks, char* montaje, char** nroBloque);
void reescribirMeta(char* montaje, char*nuevoBloque,char**nroBloque);

char** copiarBlocksMenosElQueSuperaElSize(FILE * fblocks, char* montajeBlocks,t_block* block);
void buscarBinEnDondeEntreElBlockARenovarYRenovarlo(char** nroBloque,int size,FILE * fblocks,t_block* block, char* montaje);


void freeDoblePuntero(char** doblePuntero);
char* montarBlocks(char** arrayBloques, int i);
char** agarrarBlocks(t_mensaje* mensaje);

void process_request(int socket_cliente);
void conexion_gameboy();




