
#include <strings.h>
#include <linux/time.h>

#define __FDS_BITS(p) ((p)->fds_bits)
#define FNONBLOCK O_NONBLOCK
#define FNDELAY O_NDELAY

typedef unsigned long fd_mask;

