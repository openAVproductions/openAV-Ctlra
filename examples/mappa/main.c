#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "mappa.h"

static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

int main(int argc, char **argv)
{
	signal(SIGINT, sighndlr);

	/* create mappa context */
	struct mappa_t *m = mappa_create(NULL);
	if(!m)
		return -1;

	while(!done) {
		//mappa_run(m);
		usleep(10 * 1000);
	}

	return 0;
}
