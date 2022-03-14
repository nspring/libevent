/*
 * Copyright (c) 2002-2007 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "../util-internal.h"
#include "event2/event-config.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef EVENT__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <event.h>
#include <evutil.h>

int hup_fd, err_fd = -1;

static void
hup_cb(int fd, short event, void *arg)
{
  if(event == EV_TIMEOUT) {
    printf("%s: timeout (%x) event received, fd %d\n", __func__, event, fd);
    return;
  }
  hup_fd = fd;
  printf("%s: hup (%x) event received, fd %d\n", __func__, event, fd);
  close(fd); /* else, HUP event will continue to be delivered */
}

static void
err_cb(int fd, short event, void *arg)
{
  if(event == EV_TIMEOUT) {
    printf("%s: timeout (%x) event received, fd %d\n", __func__, event, fd);
    return;
  }
  err_fd = fd;
  printf("%s: error (%x) event received, fd %d\n", __func__, event, fd);
}

struct timeval timeout = {2, 0};

#ifndef SHUT_WR
#define SHUT_WR 1
#endif
#ifndef SHUT_RD
#define SHUT_RD 2
#endif

int
main (int argc, char **argv)
{
	struct event *eva, *evb;
	const char *test = "test string";
	int pair[2];
	int pass = 1;
	struct event_base *base;
	struct event_config *cfg;

	/* Initialize the library and check if the backend
	   supports EV_FEATURE_HUP_ERR */
	cfg = event_config_new();
	event_config_require_features(cfg, EV_FEATURE_HUP_ERR);
	base = event_base_new_with_config(cfg);
	event_config_free(cfg);
	if (!base) {
		/* Backend doesn't support EV_FEATURE_HUP_ERR */
		return 0;
	}

	if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == -1)
		return (1);
        
	evutil_make_socket_nonblocking(pair[0]);
	evutil_make_socket_nonblocking(pair[1]);
	eva = event_new(base, pair[0], EV_ERROR, err_cb, NULL);
	evb = event_new(base, pair[1], EV_HUP, hup_cb, NULL);
	event_add(eva, &timeout);
	event_add(evb, &timeout);

	send(pair[0], test, strlen(test)+1, 0);
	shutdown(pair[0], SHUT_WR);
	shutdown(pair[1], SHUT_RD | SHUT_WR); /* generate HUP on writing socket */

	event_dispatch();

	if(hup_fd != pair[1]) {
		fprintf(stderr, "failed to get EV_HUP; ");
		pass = 0;
	}
	if(err_fd != pair[0]) {
		fprintf(stderr, "failed to get EV_ERROR; ");
		pass = 0;
	}

	if(pass) {
		exit(EXIT_SUCCESS);
	} else {
		fprintf(stderr, "\n");
		exit(EXIT_FAILURE);
	}
}
