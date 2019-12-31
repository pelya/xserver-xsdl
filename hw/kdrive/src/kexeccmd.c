#include <stdio.h>
#include <pthread.h>

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include "kdrive.h"
#include "opaque.h"
#ifdef __ANDROID__
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "XSDL", __VA_ARGS__)
#endif

extern char *kdExecuteCommand;

static void *child_command(void *unused)
{
	FILE *cmd;
	char buf[512];
	sprintf (buf, ":%s", display);
	printf ("setenv DISPLAY=%s\n", buf);
	setenv ("DISPLAY", buf, 1);
	setenv ("PULSE_SERVER", "tcp:127.0.0.1:4713", 1);
	printf ("Starting child command: %s\n", kdExecuteCommand);
	cmd = popen (kdExecuteCommand, "r");
	if (!cmd) {
		printf ("Error while starting child command: %s\n", kdExecuteCommand);
		return NULL;
	}
	while (fgets (buf, sizeof(buf), cmd)) {
		printf ("> %s", buf);
	}
	printf ("Child command returned with status %d\n", pclose (cmd));
	return NULL;
}

void KdExecuteChildCommand(void)
{
	pthread_t thread_id;
	if (!kdExecuteCommand)
		return;
	pthread_create(&thread_id, NULL, &child_command, NULL);
}
