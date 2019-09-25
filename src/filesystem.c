#include "../include/filesystem.h"

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
            return 0;
        }else
            return -1;
    }

    return 0;
}

int create_user_dir(char *user){
	char path[256];
	strcpy(path, "./user_data/");
	strcat(path, user);

	if(mkdir_recursive(path) < 0){
		printf("Error mkdir_recursive %s, %d\n", path, errno);
		return -1;
	}

    printf("📂 User directory created at %s\n", path);

	return 0;
}

int create_local_dir(){
	char path[256];
	strcpy(path, "./sync_dir");

	if(mkdir_recursive(path) < 0){
		printf("Error mkdir_recursive %s, %d\n", path, errno);
		return -1;
	}

    printf("📂 Local directory created at %s\n", path);

	return 0;
}

