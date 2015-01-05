# build helloworld executable when user executes "make"
ezedns-client: main.o ini.o
	$(CC) $(LDFLAGS) main.o ini.o -o ezedns-client -lcurl
main.o: main.c ini.h
	$(CC) $(CFLAGS) -c main.c -lcurl
ini.o: ini.c ini.h
	$(CC) $(CFLAGS) -c ini.c 

# remove object files and executable when user executes "make clean"
clean:
	rm *.o ezedns-client
