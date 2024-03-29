/*
 *  Copyright (C) 1997 Jon Nelson <jnelson@boa.org>
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

/* $Id: index_dir.c,v 1.24 2000/02/12 21:52:45 jon Exp $*/

#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>             /* for PATH_MAX */
#include <dirent.h>             /* for MAXNAMLEN */
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_FILE_LENGTH                         MAXNAMLEN
#define MAX_PATH_LENGTH                         PATH_MAX


#define INT_TO_HEX(x) \
  ((((x)-10)>=0)?('A'+((x)-10)):('0'+(x)))

#include "escape.h"

char *escape_string(char *inp, char *buf);
int select_files(const struct dirent *d);
int index_directory(char *dir, char *title);

/*
 * Name: escape_string
 * 
 * Description: escapes the string inp.  Uses variable buf.  If buf is
 *  NULL when the program starts, it will attempt to dynamically allocate
 *  the space that it needs, otherwise it will assume that the user 
 *  has already allocated enough space for the variable buf, which
 *  could be up to 3 times the size of inp.  If the routine dynamically
 *  allocates the space, the user is responsible for freeing it afterwords
 * Returns: NULL on error, pointer to string otherwise.
 */

char *escape_string(char *inp, char *buf)
{
    int max;
    char *index;
    unsigned char c;

    max = strlen(inp) * 3;

    if (buf == NULL && max)
        buf = malloc(sizeof (unsigned char) * (max + 1));

    if (buf == NULL)
        return NULL;

    index = buf;
    while ((c = *inp++)) {
        if (needs_escape((unsigned int) c)) {
            *index++ = '%';
            *index++ = INT_TO_HEX(c >> 4);
            *index++ = INT_TO_HEX(c & 15);
        } else
            *index++ = c;
    }
    *index = '\0';

    return buf;
}

void send_error(int error)
{
    char *the_error;

    switch (error) {

    case 1:
        the_error = "Not enough arguments were passed to the indexer.";
        break;
    case 2:
        the_error = "The Directory Sorter ran out of Memory";
        break;
    case 3:
        the_error =
            "The was a problem changing to the appropriate directory.";
        break;
    case 4:
        the_error = "There was an error escaping a string.";
    default:
        the_error = "An unknown error occurred producing the directory.";
        break;
    }
    printf("<html>\n<head>\n<title>\n%s\n</title>\n" \
           "<body>\n%s\n</body>\n</html>\n", the_error, the_error);
}

int select_files(const struct dirent *dirbuf)
{
    if (dirbuf->d_name[0] == '.')
        return 0;
    else
        return 1;
}

/*
 * Name: index_directory
 * Description: Called from get_dir_mapping if a directory html
 * has to be generated on the fly
 * If no_slash is true, prepend slashes to hrefs
 * returns -1 for problem, else 0
 */

int index_directory(char *dir, char *title)
{
    struct dirent *dirbuf;
    int numdir;
    struct dirent **array;
    struct stat statbuf;
    char filename[MAX_FILE_LENGTH * 3];
    time_t timep;
    int i;
    char *header;

    if (chdir(dir) == -1) {
        send_error(3);
        return -1;
    }
    numdir = scandir(".", &array, select_files, alphasort);
    if (numdir == -1) {
        send_error(2);
        return -1;
    }
    printf("<html>\n"
           "<head>\n<title>Index of %s</title>\n</head>\n\n"
           "<body bgcolor=\"#ffffff\">\n"
           "<H2>Index of %s</H2>\n"
           "<table>\n%s",
           title, title,
           (strcmp(title, "/") == 0 ? "" :
            "<tr><td colspan=3><h3>Directories</h3></td></tr>"
            "<tr><td colspan=3><a href=\"../\">Parent Directory</a></td></tr>\n"));

    for (i = 0; i < numdir; ++i) {
        dirbuf = array[i];

        if (stat(dirbuf->d_name, &statbuf) == -1)
            continue;

        if (!S_ISDIR(statbuf.st_mode))
            continue;

        if (escape_string(dirbuf->d_name, filename) == NULL) {
            send_error(4);
            return -1;
        }
        printf("<tr>"
               "<td width=\"40%%\"><a href=\"%s/\">%s/</a></td>"
               "<td align=right>%24s</td>"
               "<td align=right>%ld bytes</td>"
               "</tr>\n",
               filename, dirbuf->d_name,
               ctime(&statbuf.st_mtime), (long) statbuf.st_size);
    }

    printf
        ("<tr><td colspan=3>&nbsp;</td></tr>\n<tr><td colspan=3><h3>Files</h3></td></tr>\n");

    for (i = 0; i < numdir; ++i) {
        dirbuf = array[i];

        if (stat(dirbuf->d_name, &statbuf) == -1)
            continue;


        if (S_ISDIR(statbuf.st_mode))
            continue;

        if (escape_string(dirbuf->d_name, filename) == NULL) {
            send_error(4);
            return -1;
        }
        printf("<tr>"
               "<td width=\"40%%\"><a href=\"%s\">%s</a></td>"
               "<td align=right>%24s</td>"
               "<td align=right>%ld bytes</td>"
               "</tr>\n",
               filename, dirbuf->d_name,
               ctime(&statbuf.st_mtime), (long) statbuf.st_size);
    }

    time(&timep);
    printf("</table>\n<hr noshade>\nIndex generated %s\n "
           "<!-- This program is part of the Boa Webserver Copyright (C) 1991-1999 http://www.boa.org -->\n"
           "</body>\n</html>\n", ctime(&timep));

    return 0;                   /* success */
}

int main(int argc, char *argv[])
{
    int c;

    if (argc < 2) {
        send_error(1);
        return -1;
    }
    for (c = 1; c < argc; c += 2)
        if (argv[c + 1] == NULL)
            index_directory(argv[c], argv[c]);
        else
            index_directory(argv[c], argv[c + 1]);
    return 0;
}
