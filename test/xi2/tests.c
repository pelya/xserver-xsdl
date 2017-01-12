#include "tests.h"
#include "tests-common.h"
#include "protocol-common.h"

int
main(int argc, char **argv)
{
    run_test(protocol_xiqueryversion_test);
    run_test(protocol_xiquerydevice_test);
    run_test(protocol_xiselectevents_test);
    run_test(protocol_xigetselectedevents_test);
    run_test(protocol_xisetclientpointer_test);
    run_test(protocol_xigetclientpointer_test);
    run_test(protocol_xipassivegrabdevice_test);
    run_test(protocol_xiquerypointer_test);
    run_test(protocol_xiwarppointer_test);
    run_test(protocol_eventconvert_test);
    run_test(xi2_test);

    return 0;
}
