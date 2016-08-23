TARGET  := hackrf_ook
WARN    := -Wall 
CFLAGS  := -O2 ${WARN} `pkg-config --cflags libhackrf`
LDFLAGS := `pkg-config --libs libhackrf` -lm
CC      := gcc


all: ${TARGET}

${TARGET}.o: ${TARGET}.c
${TARGET}: ${TARGET}.o

clean:
	rm -rf ${TARGET}.o ${TARGET}
