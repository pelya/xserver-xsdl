#include <stdlib.h>
#include <xorg-config.h>
#include "kdrive.h"
#include "dix.h"


extern int android_main( int argc, char *argv[], char *envp[] );

int android_main( int argc, char *argv[], char *envp[] )
{
    return dix_main(argc, argv, envp);
}
