#include "../include/filesystem.h"
#include "../include/communication.h"
/** 
 *  Cria um diretório de forma recursiva
 *  Retorno: 0 - Diretório Criado
 *           1 - Diretório já existente
 *          -1 - Erro na criação 
 * */
int mkdir_recursive(char *dir){
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
        if(*p == '/') {
            *p = 0;
            if(mkdir(tmp, S_IRWXU) < 0 && errno != 17)
                return -1;
            *p = '/';
        }
    if(mkdir(tmp, S_IRWXU) < 0){
        if(errno = 17){
            return 1;
        }else
            return -1;
    }

    return 0;
}

int create_user_dir(char *user){
    int n;
	char path[256];
	strcpy(path, "./user_data/");
	strcat(path, user);

    n = mkdir_recursive(path);

	if(n < 0){
		printf("Error mkdir_recursive %s, %d\n", path, errno);
		return -1;
	}else if(n == 0)
        printf("📂 User directory created at %s\n", path);

	return 0;
}

int create_local_dir(){
    int n;
	char path[256];
	strcpy(path, "./sync_dir");

	n = mkdir_recursive(path);

	if(n < 0){
		printf("Error mkdir_recursive %s, %d\n", path, errno);
		return -1;
	}else if(n == 0)
        printf("📂 Local directory created at %s\n", path);

	return 0;
}

int fileSize(FILE *toBeMeasured){
    int size;
    
    //Ao infinito e além para medir o tamanho do arquivo.
    fseek(toBeMeasured, 0L, SEEK_END);
    size = ftell(toBeMeasured);
    //Retorna ao inicio depois da longa viagem.
    rewind(toBeMeasured);
    return size;
}

int isOpened(FILE *sourceFile){
    return sourceFile != NULL;
}

int fileSizeInPackets(int fileSize){
    int totalPackets;

	if (fileSize == 0){
		return 1;
	}

    //O tamanho do arquivo em pacotes é acrescido de um pacote caso haja resto.
    totalPackets = (fileSize % DATA_LENGTH) == 0? (fileSize / DATA_LENGTH) : (fileSize / DATA_LENGTH) + 1;
    return totalPackets;
}

char **splitPath(char *name, int *size) {
	int i = 0, n = 0;
	char *nameCopy = malloc(strlen(name) + 1);
  	strcpy(nameCopy, name);
	// Conta ocorrencias
	if (strlen(nameCopy) > 0 && nameCopy[0] != '/')
		n++;
	while (nameCopy[i] != '\0') {
		if (nameCopy[i] == '/')
			n++;
		i++;
	}
	if (strlen(nameCopy) > 0 && nameCopy[strlen(nameCopy)-1] == '/') {
		nameCopy[strlen(nameCopy)-1] = '\0';
		n--;
	}
	
	*size = n;

	i = 0;
	char **strings;
	strings = (char**)malloc(sizeof(char)*n);
	char *substring;
	char *nameBuff = strdup(nameCopy); // Necessaroi para strsep
	while( (substring = strsep(&nameBuff,"/")) != NULL ) {
		if (strlen(substring) > 0) {
			strings[i] = (char*) malloc(sizeof(char)*FILE_NAME_SIZE);
			strcpy(strings[i], substring);
			i++;
		}
	}
	*size = n;
	if (*size == 1 && strings[0] == NULL) {
		*size = 0;
		free(strings);
		return NULL;
	}
	return strings;
}