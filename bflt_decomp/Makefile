
all: bflt_decomp

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

bflt_decomp: bflt_decomp.o
	$(CC) -lz -o $@ $^

clean:
	rm *.o bflt_decomp
