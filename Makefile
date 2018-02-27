
CFLAGS = -Os
LDFLAGS = -pthread
DEFINES =  
INC = -I.
DEPS = 


all: rpmsg_test_01 rpmsg_test_02

rpmsg_test_01: rpmsg_test_01.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFINES) rpmsg_test_01.c -o rpmsg_test_01

rpmsg_test_02: rpmsg_test_02.c message_protocol.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFINES) rpmsg_test_02.c -o rpmsg_test_02

.PHONY:	clean

clean:
	rm -f rpmsg_test_01 rpmsg_test_02

