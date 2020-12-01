/*  
    netmonitor.cpp

    S. Wang
    March 22, 2019
    Assignment 1 for UNX511

    ------------------------------------------------------

    The code in this file is based on Shichao's lab7 - server.cpp
    Modified for Assignment 1
    There is no header file for netmonitor.cpp
*/
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <sstream>
#include <sys/time.h>
#include <vector>
#include <thread>
using namespace std;
#define BUFLEN 4096 //buffer size

const char SOCKET_PATH[] = "/tmp/net_monitor"; //local socket file path

bool isRunning = false;
bool isChild = false;

//forward declaration

//SIGINT(^C) handler
void sigintHandler(int x);

//child function. used for communicate with interface monitor.
void child(int);


int main(int argc, char **argv)
{

    signal(SIGINT, sigintHandler);
    int numIntfs = 0; //number of monitoring network interface.
    int MAX_INTFS = 0;
    cout << "Please enter the number of interface you want to monitor: ";
    while(!(cin >> MAX_INTFS)){
        cout << "Please enter the number of interface you want to monitor: ";
        if(cin.fail()){//clearing input stream.
            cin.clear();
            cin.ignore(__INT_MAX__, '\n');
        }
    }
    string intfs[MAX_INTFS]; //network interface names
    for(size_t i = 0; i < MAX_INTFS; i++)
    {
        cout << "Interface " << i + 1 << ": ";
        cin >> intfs[i]; 
    }
    

    sockaddr_un addr;
    int fd, max_fd, ret;
    fd_set activefds, readfds;

    char buffer[BUFLEN];

    memset(buffer, 0, BUFLEN);
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, 17);

    FD_ZERO(&activefds);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        cout << "cannot create socket" << endl;
		cout << strerror(errno) << endl;
        exit(3);
    }

    if (bind(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        cout << "cannot bind" << endl;
        cout << strerror(errno) << endl;
        exit(4);
    }
    cout << "listening for clients" << endl;
    if (listen(fd, MAX_INTFS) < 0)
    {
        cout << "cannot listen" << endl;
		cout << strerror(errno) << endl;
        exit(5);
    }

    FD_SET(fd, &activefds);
    max_fd = fd;
    isRunning = true;

    for(int i = 0; i < MAX_INTFS; i++)
    {
        if(fork() == 0){ //create new childs to execute interface monitor.
            isChild = true;
            system((string("./intfmonitor ") + intfs[i]).c_str());
            exit(0);
        }
    }

    if(!isChild){
		
        while (isRunning)
        {   
            if(numIntfs == MAX_INTFS){//wait for ^C to exit if number of network interface reach max interface limit. 
                sleep(1);
            }else{//detect new connection every 1 sec if number of network interface not reach max interface limit.
                timeval tv;
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                readfds = activefds;
                ret = select(max_fd + 1, &readfds, NULL, NULL, &tv);
            
                if (ret == -1)
                {
                    cout << "error while calling select()" << endl;
                    break;
                }
                else
                {
                    if (FD_ISSET(fd, &readfds))//check if fd is non-block for reading.
                    {
						//accept new connection request. fork a new child to communicate with the interface monitor.
                        int mcl = accept(fd, NULL, NULL);
                        numIntfs++;
                        if (fork() == 0) 
                        {
                            isChild = true;
                            child(mcl);
                        }
                        if (isChild == true)
                            break;
                    }
                }
            }
        }
    }

	if (!isChild) { //clearing before quit.
		close(fd);
		unlink(SOCKET_PATH);
	}
    return 0;
}

void sigintHandler(int x)
{   
    if(!isChild)
        cout << "Ctrl-C detected, Please wait for quiting." << endl;
    if(isRunning){
        isRunning = false;
    }else{
        exit(0);
    }
}

void child(int fd)
{
    bool isReady = false;
    char buf[BUFLEN];
    while (isRunning)
    {
        read(fd, buf, BUFLEN);
        if (string(buf) == string("Ready"))
        {
            isReady = true;
            write(fd, "Monitor", 8);
            continue;
        }

        if (isReady)
        {
            if (string(buf) == string("Done"))
            {
                sleep(1);
                write(fd, "Monitor", 8);
                continue;
            }
            else if (string(buf) == string("LinkDown"))
            {
                write(fd, "SetLinkUp", 10);
                sleep(1);
                continue;
            }
        }else { cout << "Error: Monitor is not ready!" << endl; }
        sleep(1);
    }
	
	//tell interface monitor to quit.
    write(fd, "shutdown", 9);
    close(fd);
}