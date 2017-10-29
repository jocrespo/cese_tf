CC=gcc 
CFLAGS= -std=gnu99 -pthread -lusb-1.0 -I.
DEPS = main.h usb_comm.h error.h msg_types.h bema.h
OBJ = main.o usb_comm.o bema.o  

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

printer: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)
