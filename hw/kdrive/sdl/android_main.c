#ifdef __ANDROID__
#include <stdlib.h>

extern int android_main(int argc, char *argv[], char *envp[]);

int main(int argc, char* argv[])
{
	char * envp[] = { NULL };
	return android_main(argc, argv, envp);
}
#endif
