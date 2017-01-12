#ifndef TESTS_H
#define TESTS_H

int protocol_xchangedevicecontrol_test(void);

int protocol_xiqueryversion_test(void);
int protocol_xiquerydevice_test(void);
int protocol_xiselectevents_test(void);
int protocol_xigetselectedevents_test(void);
int protocol_xisetclientpointer_test(void);
int protocol_xigetclientpointer_test(void);
int protocol_xipassivegrabdevice_test(void);
int protocol_xiquerypointer_test(void);
int protocol_xiwarppointer_test(void);
int protocol_eventconvert_test(void);
int xi2_test(void);

#ifndef INSIDE_PROTOCOL_COMMON

extern int enable_XISetEventMask_wrap;
extern int enable_GrabButton_wrap;

#endif /* INSIDE_PROTOCOL_COMMON */

#endif /* TESTS_H */

