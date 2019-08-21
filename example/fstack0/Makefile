CC   = gcc
SRC  = $(wildcard *.c)
OBJ  = $(SRC:.c=.o)
BIN  = fstack0
FLG  = -Wall -Wno-unused-function -g
DPDK = dpdk-19.05-debug
INC  = -msse4.2 -I/opt/lib64/$(DPDK)/usr/local/include -I/opt/lib64/$(DPDK)/usr/local/include/dpdk
INC += -I/opt/lib64/f-stack/include
LIB  = -Wl,-rpath,/opt/lib64/$(DPDK)/usr/local/lib -L/opt/lib64/$(DPDK)/usr/local/lib -ldpdk
LIB += -L/opt/lib64/f-stack/lib -lfstack

%.o: %.c
	$(CC) -c $(INC) $(FLG) $< -o $@

$(BIN): $(OBJ)
	gcc -o $(BIN) $(OBJ) $(LIB)

clean:
	rm -fr $(OBJ) $(BIN) __pycache__
