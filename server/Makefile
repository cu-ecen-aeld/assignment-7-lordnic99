all: aesdsocket
aesdsocket:
	$(CC) $(CCFLAGS) $(LDFLAGS) -static socket.c -o aesdsocket
CROSS_COMPILE:
	$(CC) $(CCFLAGS) $(LDFLAGS) -static socket.c -o aesdsocket
clean:
	rm -f aesdsocket 
