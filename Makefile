
CFLAGS = -Os
LDFLAGS = -pthread
DEFINES =  
INC = -I.
DEPS = 

#SOURCES = $(wildcard *.c)
SOURCES = rpmsg_test_01.c 

OBJECTS = $(SOURCES:.c=.o)
TARGET = rpmsg_test_01

all:	$(SOURCES) $(TARGET)

$(TARGET):    $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET)

%.o:    %.c $(DEPS)
	$(CC) -c $(CFLAGS) $(DEFINES) $(INC) $< -o $@

.PHONY:	clean

clean:
	rm -f $(OBJECTS) $(TARGET)

