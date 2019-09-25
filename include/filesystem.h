#ifndef __filesystem__
#define __filesystem__

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>


/** Cria diretórios recursivamente */
int mkdir_recursive(char *dir);

/** Cria o diretório com o nome do usuário caso não exista */
int create_user_dir(char *user);

/** Cria o diretório local caso não exista */
int create_local_dir();


#endif
