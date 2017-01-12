#include <string.h>
#include "tests.h"
#include "tests-common.h"

int
main(int argc, char **argv)
{
    run_test(fixes_test);

#ifdef RES_TESTS
    run_test(hashtabletest_test);
#endif

    run_test(input_test);
    run_test(list_test);
    run_test(misc_test);
    run_test(signal_logging_test);
    run_test(string_test);
    run_test(touch_test);
    run_test(xfree86_test);
    run_test(xkb_test);
    run_test(xtest_test);

    return 0;
}
