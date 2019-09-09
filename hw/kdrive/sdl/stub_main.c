#include <stdlib.h>
#include <xorg-config.h>
#include "kdrive.h"
#include "dix.h"


int main(int argc, char* argv[])
{
	char * envp[] = { NULL };
	return dix_main(argc, argv, envp);
}
