
SBU_REV= "0.1"


GCC = gcc

CFLAGS = -Wall -DVERSION_NUMBER=${SBU_REV}
OBJECTS = zbFwStart.o ../zbSocDriver/zbSocCmd.o
INCLUDE = -I../zbSocDriver/
DEFS =

APP_NAME=zbFwStart.bin

.PHONY: all, clean

${APP_NAME}: ${OBJECTS}
	$(GCC) $(CFLAGS) $(OBJECTS) -o ${APP_NAME}

%.o: %.c
	${GCC} ${CFLAGS} ${INCLUDE} ${DEFS} -c -o $@ $<

all: ${APP_NAME}

clean:
	rm -rf *.o ${APP_NAME}
	rm -rf ../zbSocDriver/*.o ${APP_NAME}
