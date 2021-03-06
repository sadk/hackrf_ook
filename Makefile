TARGET  := hackrf_ook
WARN    := -Wall 
CFLAGS  := -O2 ${WARN} `pkg-config --cflags libhackrf glib-2.0`
LDFLAGS := `pkg-config --libs libhackrf glib-2.0` -lm
CC      := gcc


all: ${TARGET}

${TARGET}.o: ${TARGET}.c
${TARGET}: ${TARGET}.o

clean:
	rm -rf ${TARGET}.o ${TARGET}

mrproper: clean
	rm *~

