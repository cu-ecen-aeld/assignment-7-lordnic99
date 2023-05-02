all: writer
writer: 
	$(CC) $(CCFLAGS) $(LDFLAGS) -static writer.c -o writer
clean:
	rm -rf writer.o writer
# CROSS_COMPILE:
# 	aarch64-none-linux-gnu-gcc -static writer.c -o writer
