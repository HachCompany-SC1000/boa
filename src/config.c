/*
 *  Boa, an http server
 *  Copyright (C) 1999 Larry Doolittle <ldoolitt@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* $Id: config.c,v 1.23 2000/04/10 19:46:15 jon Exp $*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include "y.tab.h"
#include "parse.h"
#include "boa.h"

extern FILE *yyin;
int yyparse(void);              /* Better match the output of lex */

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

char *chroot_path;
int server_port;
uid_t server_uid;
gid_t server_gid;
char *server_root;
char *server_name;
char *server_admin;
char *server_ip = NULL;
int virtualhost = 0;

char *document_root;
char *user_dir;
char *directory_index;
char *default_type;
char *dirmaker;
char *cachedir;
char *pidfile;

int ka_timeout;
int ka_max;

/* These came from log.c */
char *error_log_name;
char *access_log_name;
char *cgi_log_name;

int use_localtime = 0;

/* These come from boa_grammar.y */
void add_alias(char *fakename, char *realname, int script);
void add_mime_type(char *extension, char *type);

/* These are new */
void c_set_user(char *v1, char *v2, void *t);
void c_set_group(char *v1, char *v2, void *t);
void c_set_string(char *v1, char *v2, void *t);
void c_set_int(char *v1, char *v2, void *t);
void c_set_unity(char *v1, char *v2, void *t);
void c_add_type(char *v1, char *v2, void *t);
void c_add_alias(char *v1, char *v2, void *t);

/* Fakery to keep the value passed to action() a void *,
   see usage in table and c_add_alias() below */
int script_number = SCRIPTALIAS;
int redirect_number = REDIRECT;
int alias_number = ALIAS;

/* Help keep the table below compact */
#define S0A STMT_NO_ARGS
#define S1A STMT_ONE_ARG
#define S2A STMT_TWO_ARGS

struct ccommand clist[] = {
    {"ChrootPath", S1A, c_set_string, &chroot_path},
    {"Port", S1A, c_set_int, &server_port},
    {"PidFile", S1A, c_set_string, &pidfile},
    {"Listen", S1A, c_set_string, &server_ip},
    {"BackLog", S1A, c_set_int, &backlog},
    {"User", S1A, c_set_user, NULL},
    {"Group", S1A, c_set_group, NULL},
    {"ServerAdmin", S1A, c_set_string, &server_admin},
    {"ServerRoot", S1A, c_set_string, &server_root},
    {"ErrorLog", S1A, c_set_string, &error_log_name},
    {"AccessLog", S1A, c_set_string, &access_log_name},
    {"UseLocaltime", S0A, c_set_unity, &use_localtime},
    {"CgiLog", S1A, c_set_string, &cgi_log_name},
    {"VerboseCGILogs", S0A, c_set_unity, &verbose_cgi_logs},
    {"ServerName", S1A, c_set_string, &server_name},
    {"VirtualHost", S0A, c_set_unity, &virtualhost},
    {"DocumentRoot", S1A, c_set_string, &document_root},
    {"UserDir", S1A, c_set_string, &user_dir},
    {"DirectoryIndex", S1A, c_set_string, &directory_index},
    {"DirectoryMaker", S1A, c_set_string, &dirmaker},
    {"DirectoryCache", S1A, c_set_string, &cachedir},
    {"KeepAliveMax", S1A, c_set_int, &ka_max},
    {"KeepAliveTimeout", S1A, c_set_int, &ka_timeout},
    {"MimeTypes", S1A, c_set_string, &mime_types},
    {"DefaultType", S1A, c_set_string, &default_type},
    {"AddType", S2A, c_add_type, NULL},
    {"ScriptAlias", S2A, c_add_alias, &script_number},
    {"Redirect", S2A, c_add_alias, &redirect_number},
    {"Alias", S2A, c_add_alias, &alias_number}
};

void c_set_user(char *v1, char *v2, void *t)
{
    struct passwd *passwdbuf;
    char *endptr;
    int i;
    DBG(printf("User %s = ", v1);
        )
        i = strtol(v1, &endptr, 0);
    if (*v1 != '\0' && *endptr == '\0') {
        server_uid = i;
    } else {
        passwdbuf = getpwnam(v1);
        if (!passwdbuf) {
            fprintf(stderr, "No such user: %s\n", v1);
            exit(1);
        }
        server_uid = passwdbuf->pw_uid;
    }
    DBG(printf("%d\n", server_uid);
        )
}

void c_set_group(char *v1, char *v2, void *t)
{
    struct group *groupbuf;
    char *endptr;
    int i;
    DBG(printf("Group %s = ", v1);
        )
        i = strtol(v1, &endptr, 0);
    if (*v1 != '\0' && *endptr == '\0') {
        server_gid = i;
    } else {
        groupbuf = getgrnam(v1);
        if (!groupbuf) {
            fprintf(stderr, "No such group: %s\n", v1);
            exit(1);
        }
        server_gid = groupbuf->gr_gid;
    }
    DBG(printf("%d\n", server_gid);
        )
}

void c_set_string(char *v1, char *v2, void *t)
{
    char *s;
    DBG(printf("Setting pointer %p to string %s ..", t, v1);
        )
        if (t) {
        s = *(char **) t;
        if (s)
            free(s);
        *(char **) t = strdup(v1);
        DBG(printf("done.\n");
            )
    } else {
        DBG(printf("skipped.\n");
            )
    }
}

void c_set_int(char *v1, char *v2, void *t)
{
    char *endptr;
    int i;
    DBG(printf("Setting pointer %p to integer string %s ..", t, v1);
        )
        if (t) {
        i = strtol(v1, &endptr, 0); /* Automatic base 10/16/8 switching */
        if (*v1 != '\0' && *endptr == '\0') {
            *(int *) t = i;
            DBG(printf(" Integer converted as %d, done\n", i);
                )
        } else {
            /* XXX should tell line number to user */
            fprintf(stderr, "Error: %s found where integer expected\n",
                    v1);
        }
    } else {
        DBG(printf("skipped.\n");
            )
    }
}

void c_set_unity(char *v1, char *v2, void *t)
{
    DBG(printf("Setting pointer %p to unity\n", t);
        )
        if (t)
        *(int *) t = 1;
}

void c_add_type(char *v1, char *v2, void *t)
{
    add_mime_type(v1, v2);
}

void c_add_alias(char *v1, char *v2, void *t)
{
    add_alias(v2, v1, *(int *) t);
}

struct ccommand *lookup_keyword(char *c)
{
    struct ccommand *p;
    DBG(printf("Checking string '%s' against keyword list\n", c);
        )
        for (p = clist;
             p < clist + (sizeof (clist) / sizeof (struct ccommand)); p++) {
        if (strcmp(c, p->name) == 0)
            return p;
    }
    return NULL;
}

/*
 * Name: read_config_files
 *
 * Description: Reads config files via yyparse, then makes sure that
 * all required variables were set properly.
 */
void read_config_files(void)
{
    yyin = fopen("boa.conf", "r");

    if (!yyin) {
        fputs("Could not open boa.conf for reading.\n", stderr);
        exit(1);
    }
    if (yyparse()) {
        fputs("Error parsing config files, exiting\n", stderr);
        exit(1);
    }

    if (!server_name) {
        struct hostent *he;
        char temp_name[100];

        if (gethostname(temp_name, 100) == -1) {
            perror("gethostname:");
            exit(1);
        }

        he = gethostbyname(temp_name);
        if (he == NULL) {
            perror("gethostbyname:");
            exit(1);
        }

        server_name = strdup(he->h_name);
        if (server_name == NULL) {
            perror("strdup:");
            exit(1);
        }
    }
}
