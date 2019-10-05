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
	strcpy(path, SERVER_DIR);
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
	strcpy(path, LOCAL_DIR);

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


int get_dir_status(char *dir_path, DIR_ENTRY **entries){
    DIR *d;
    int i = 0;
    struct dirent *entry;

    d = opendir(dir_path);

    if (d == NULL) {
        fprintf (stderr, "Cannot open directory '%s': %s\n",
                 dir_path, strerror (errno));
        return -1;
    }

    while (1) {
        char file_path[256] = "";
        struct stat file_stat;
        
        entry = readdir(d);
        if (entry == NULL)
            break;
        
        if((strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) != 0){
            i++;

            strcat(file_path, dir_path);
            strcat(file_path, entry->d_name);
            
            if(stat(file_path, &file_stat) < 0){
                fprintf (stderr, "Cannot stat file '%s': %s\n", file_path, strerror (errno));
                return -1;
            }

            *entries = realloc(*entries, i * sizeof(DIR_ENTRY));

            strcpy((*entries)[i-1].name, entry->d_name);
            (*entries)[i-1].size = file_stat.st_size;
            (*entries)[i-1].last_modified = file_stat.st_mtime;
			(*entries)[i-1].last_status = file_stat.st_ctime;
			(*entries)[i-1].last_access = file_stat.st_atime;
        }
    }

    /* Close the directory. */
    if (closedir (d) < 0) {
        fprintf (stderr, "Could not close '%s': %s\n", dir_path, strerror (errno));
        return -1;
    }

    return i;
}

void compare_entry_diff(DIR_ENTRY *server_entries, DIR_ENTRY *client_entries, int n_server_ent, int n_client_ent, SYNC_LIST *list){
	char *downloadList = malloc(MAX_NAME_LENGTH);
	char *uploadList = malloc(MAX_NAME_LENGTH);
	char *deleteList = malloc(MAX_NAME_LENGTH);
	int down_count = 0, up_count = 0, del_count = 0, exists_in_client, exists_in_server;
	int i, j;
	char temp_name[MAX_NAME_LENGTH];
	char *name_tok;

	for(i=0; i < n_server_ent; i++){
		exists_in_client = 0;
		for(j=0; j < n_client_ent; j++){
			strcpy(temp_name, server_entries[i].name);
			name_tok = strtok(temp_name, "~");

			// Caso exista um sinalizador de DELETE
			if((server_entries[i].name)[0] == '~' && !strcmp(name_tok, client_entries[j].name)){
				exists_in_client = 1;		
				if(server_entries[i].last_modified > client_entries[j].last_status){
					// Caso o marcador no servidor seja mais recente, adiciona a lista de deletes
					del_count++;
					deleteList = realloc(deleteList, MAX_NAME_LENGTH * del_count);
					strcpy((char*)(deleteList + (del_count - 1) * MAX_NAME_LENGTH), client_entries[j].name);
				} else {
					// Caso o arquivo no cliente seja mais recente, adiciona à lista de uploads
					up_count++;
					uploadList = realloc(uploadList, MAX_NAME_LENGTH * up_count);
					strcpy((char*)(uploadList + (up_count - 1) * MAX_NAME_LENGTH), client_entries[j].name);
				}
			} else if(!strcmp(server_entries[i].name, client_entries[j].name)){
				exists_in_client = 1;
				if(server_entries[i].last_modified < client_entries[j].last_status){
					// Caso o arquivo no cliente seja mais recente, adiciona à lista de uploads
					up_count++;
					uploadList = realloc(uploadList, MAX_NAME_LENGTH * up_count);
					strcpy((char*)(uploadList + (up_count - 1) * MAX_NAME_LENGTH), server_entries[i].name);
				}
			}
		}
		if((server_entries[i].name)[0] != '~' && !exists_in_client){
			// Caso o arquivo não exista do lado do cliente, adiciona à lista de downloads
			down_count++;
			downloadList = realloc(downloadList, MAX_NAME_LENGTH * down_count);
			strcpy((char*)(downloadList + (down_count-1) * MAX_NAME_LENGTH), server_entries[i].name);
		}
	}

	for(i=0; i < n_client_ent; i++){
		exists_in_server = 0;
		for(j=0; j < n_server_ent; j++){
			strcpy(temp_name, server_entries[j].name);
			name_tok = strtok(temp_name, "~");

			if(!strcmp(server_entries[j].name, client_entries[i].name)){
				exists_in_server = 1;
				if(server_entries[j].last_modified > client_entries[i].last_status){
					// Caso o arquivo no servidor seja mais recente, adiciona à lista de downloads
					down_count++;
					downloadList = realloc(downloadList, MAX_NAME_LENGTH * down_count);
					strcpy((char*)(downloadList + (down_count-1) * MAX_NAME_LENGTH), server_entries[j].name);
				}
			} else if(server_entries[j].name[0] == '~' && !strcmp(name_tok, client_entries[i].name))
				exists_in_server = 1;
		}
		if(!exists_in_server){
			// Caso o arquivonão exista no servidor, adiciona à lista de uploads
			up_count++;
			uploadList = realloc(uploadList, MAX_NAME_LENGTH * up_count);
			strcpy((char*)(uploadList + (up_count - 1) * MAX_NAME_LENGTH), client_entries[i].name);
		}
	}

	// Preenche a sctruct com a lista de sincronização
	list->n_downloads = down_count;
	list->n_uploads = up_count;
	list->n_deletes = del_count;
	list->list = malloc((up_count + down_count + del_count) * MAX_NAME_LENGTH);
	memcpy(list->list, downloadList, down_count * MAX_NAME_LENGTH);
	memcpy(list->list + down_count * MAX_NAME_LENGTH, uploadList, up_count * MAX_NAME_LENGTH);
	memcpy(list->list + (down_count + up_count) * MAX_NAME_LENGTH, deleteList, del_count * MAX_NAME_LENGTH);

	free(downloadList);
	free(uploadList);
	free(deleteList);
}

void print_dir_status(DIR_ENTRY **entries, int n){
    int i;

    printf("\n");
    for(i=0; i<n; i++){
        printf("-> %s\n", (*entries)[i].name);
        printf("    size: %ldB\n", (*entries)[i].size);
        printf("    last modified(mtime): %s", ctime(&(*entries)[i].last_modified));
		printf("    last accessed(atime): %s", ctime(&(*entries)[i].last_access));
		printf("    last status change(ctime): %s", ctime(&(*entries)[i].last_status));
		printf("\n");
    }
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
