/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@jlab.org>
 *  Some changes Copyright (C) 1996-99 Jon Nelson <jnelson@boa.org>
 *  Some changes Copyright (C) 1997 Alain Magloire <alain.magloire@rcsm.ee.mcgill.ca>
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

/* $Id: signals.c,v 1.26 2000/04/10 19:48:17 jon Exp $*/

#include "boa.h"
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>           /* wait */
#endif
#include <signal.h>             /* signal */

void sigsegv(int);
void sigbus(int);
void sigterm(int);
void sighup(int);
void sigint(int);
void sigchld(int);
void sigalrm(int);

/*
 * Name: init_signals
 * Description: Sets up signal handlers for all our friends.
 */

void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGBUS);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigaddset(&sa.sa_mask, SIGALRM);

    sa.sa_handler = sigsegv;
    sigaction(SIGSEGV, &sa, NULL);

    sa.sa_handler = sigbus;
    sigaction(SIGBUS, &sa, NULL);

    sa.sa_handler = sigterm;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sighup;
    sigaction(SIGHUP, &sa, NULL);

    sa.sa_handler = sigint;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_handler = sigchld;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = sigalrm;
    sigaction(SIGALRM, &sa, NULL);
}

void sigsegv(int dummy)
{
    log_error_time();
    fputs("caught SIGSEGV, dumping core in /tmp\n", stderr);
    fclose(stderr);
    chdir("/tmp");
    abort();
}

void sigbus(int dummy)
{
    log_error_time();
    fputs("caught SIGBUS, dumping core in /tmp\n", stderr);
    fclose(stderr);
    chdir("/tmp");
    abort();
}

void sigterm(int dummy)
{
    lame_duck_mode = 1;
}

void lame_duck_mode_run(int server_s)
{
    log_error_time();
    fputs("caught SIGTERM, starting shutdown\n", stderr);
    FD_CLR(server_s, &block_read_fdset);
    close(server_s);
    lame_duck_mode = 2;
}

void sighup(int dummy)
{
    sighup_flag = 1;
}

void sighup_run(void)
{
    sighup_flag = 0;
    log_error_time();
    fputs("caught SIGHUP, restarting\n", stderr);

    /* Philosophy change for 0.92: don't close and attempt reopen of logfiles,
     * since usual permission structure prevents such reopening.
     */

    dump_mime();
    dump_passwd();
    dump_alias();
    free_requests();

    log_error_time();
    fputs("re-reading configuration files\n", stderr);
    read_config_files();

    log_error_time();
    fputs("successful restart\n", stderr);
}

void sigint(int dummy)
{
    log_error_time();
    fputs("caught SIGINT: shutting down\n", stderr);
    fclose(stderr);
    exit(1);
}

void sigchld(int dummy)
{
    sigchld_flag = 1;
}

void sigchld_run(void)
{
    int status;
    pid_t pid;

    sigchld_flag = 0;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        if (verbose_cgi_logs) {
            log_error_time();
            fprintf(stderr, "reaping child %d: status %d\n", pid, status);
        }
    return;
}

void sigalrm(int dummy)
{
    log_error_time();
    fprintf(stderr, "%ld requests, %ld errors\n",
            status.requests, status.errors);
    alarm(60 * 60);
}
