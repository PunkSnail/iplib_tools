Target = qqwry2txt

CC = gcc -std=c11
CFLAG = -Wall -Wconversion

Inc = ./
Src = ./

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    LIBS = -liconv
endif

Obj = $(patsubst %.c, %.o, $(wildcard $(Src)*.c))

$(Target):$(Obj)
	$(CC) $^ -o $@ $(CFLAG) $(LIBS) 

%.o:%.c
	$(CC) $^ -c -o $@ -I$(Inc) $(CFLAG)

clean:
	rm -f $(Target) $(Obj)
