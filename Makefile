CC="g++"
CFLAGS="-I"
CFLAGS+="-Wall"

netmonitor: .
	$(CC) $(CFLAGS) -o netmonitor netmonitor.cpp
intfmonitor: .
	$(CC) $(CFLAGS) -o intfmonitor intfmonitor.cpp
clean: 
	rm -f netmonitor intfmonitor
all: netmonitor intfmonitor
