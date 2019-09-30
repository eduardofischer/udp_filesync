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
    //O tamanho do arquivo em pacotes é acrescido de um pacote caso haja resto.
    totalPackets = (fileSize % DATA_LENGTH) == 0? (fileSize / DATA_LENGTH) : (fileSize / DATA_LENGTH) + 1;
    return totalPackets;
}


int write_packet_to_the_file(PACKET *packet, FILE *file){
    int bytes_written;
    //Sets the pointer
    fseek(file,packet->header.seqn * DATA_LENGTH, SEEK_SET);
    //Writes the pointer to the file
    bytes_written = fwrite(packet->data,sizeof(char), packet->header.length,file);
    rewind(file);

    if (bytes_written == sizeof(char) * packet->header.length){
        return SUCCESS;
    }
    else{
        printf("There was a error writing to the file.");
        return ERR_WRITING_FILE;
    }

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
        }
    }

    /* Close the directory. */
    if (closedir (d) < 0) {
        fprintf (stderr, "Could not close '%s': %s\n", dir_path, strerror (errno));
        return -1;
    }

    return i;
}

void print_dir_status(DIR_ENTRY **entries, int n){
    int i;

    printf("\n");
    for(i=0; i<n; i++){
        printf("-> %s\n", (*entries)[i].name);
        printf("    size: %ld\n", (*entries)[i].size);
        printf("    last modified: %s", ctime(&(*entries)[i].last_modified));
    }
    printf("\n");
}


