/*  
    intfmonitor.cpp

    S. Wang, B. Rychlowski
    March 21, 2019
    Assignment 1 for UNX511

    ---------------------------------------------

    Parts of code in this file is from Shichao's lab 6 - client.cpp
    There is no header file for monitor.cpp
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
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <sys/time.h>
#include <vector>
#include <thread>
using namespace std;
#define BUFLEN 4096


const char *SOCKET_PATH = "/tmp/net_monitor";
ifstream in;


//forward declaration

//bring network up. takes interface name, returns ture on success, returns false on fail. errno is set on fail.
//"sudo" may be required for this part.
bool bringNetworkUp(const char* interface_name);

//read an integer from given file path. returns -1 on error.
int readInt(string path);

//read an string from given file path. returns "unknown" or empty string on error.
string readStr(string path);



class Data
{
private:
    string INTF_PATH;
    string operstate_path;
    string rx_bytes_path;
    string rx_dropped_path;
    string rx_errors_path;
    string rx_packets_path;
    string tx_bytes_path;
    string tx_dropped_path;
    string tx_errors_path;
    string tx_packets_path;



    string operstate, interface;
    int rx_bytes, rx_dropped, rx_errors, rx_packets;
    int tx_bytes, tx_dropped, tx_errors, tx_packets;
    int up_count = 0;
    int down_count = 0;

    void addUpCount(){
        up_count++;
    }
    void addDownCount(){
        down_count++;
    }


public:
    //constructor, takes interface name to create paths for reading.
    Data(const char* interface_name){
        this->interface = interface_name;
        INTF_PATH = string("/sys/class/net/") + interface_name + '/';
        operstate_path = string(INTF_PATH + "operstate");
        rx_bytes_path = string(INTF_PATH + "statistics/rx_bytes");
        rx_dropped_path = string(INTF_PATH + "statistics/rx_dropped");
        rx_errors_path = string(INTF_PATH + "statistics/rx_errors");
        rx_packets_path = string(INTF_PATH + "statistics/rx_packets");
        tx_bytes_path = string(INTF_PATH + "statistics/tx_bytes");
        tx_dropped_path = string(INTF_PATH + "statistics/rx_dropped");
        tx_errors_path = string(INTF_PATH + "statistics/rx_errors");
        tx_packets_path = string(INTF_PATH + "statistics/rx_packets");
    }

	//isDown query, returns true if the network interface is down.
    bool isDown() const{
        return operstate == "down";
    }

    //this function returns true if operstate is "up"
    //this function may not be used in this assignment
    /*
    bool isReady(){
        update();
        if (this->operstate != "up") {
            return false;
        }
        return true;        
    }
    */

    // update network interface statistics
    void update(){
        string str;
        if(operstate == string()){ //first time reading operstate.
            operstate = readStr(operstate_path);
        }else{ 
            //store operstate for checking if it changed
            str = readStr(operstate_path); 

            if(str != operstate){ //operstate changed
                if(str == "up"){ //from down to up
                    addUpCount();
                }else if(str == "down"){ //from up to down
                    cout << "down" << endl;
                    addDownCount();
                }
            }
            operstate = str;
        }        
        
        //updating values
        rx_bytes = readInt(rx_bytes_path);
        rx_dropped = readInt(rx_dropped_path);
        rx_errors = readInt(rx_errors_path);
        rx_packets = readInt(rx_packets_path);
        tx_bytes = readInt(tx_bytes_path);
        tx_dropped = readInt(tx_dropped_path);
        tx_errors = readInt(tx_errors_path);
        tx_packets = readInt(tx_packets_path);
    }
	//get interface name
    const string getName() const{
        return this-> interface;
    }


    friend ostream& operator<<(ostream& os, Data & data){
        os << "Interface: " << data.interface << " state: " << data.operstate << " up_count: " << data.up_count << " down_count: " << data.down_count
        << endl << "rx_bytes: " << data.rx_bytes << " rx_dropped: " << data.rx_dropped << " rx_ errors: " << data.rx_errors << " rx_packets: " << data.rx_packets
        << endl << "tx_bytes: " << data.tx_bytes << " tx_dropped: " << data.tx_dropped << " tx_ errors: " << data.tx_errors << " tx_packets: " << data.tx_packets
        << endl;
    }
};



int main(int argc, char** argv){
    //code copied from lab 6 client.cpp, modified
    if(argc != 2){
        exit(1);
    }
    struct sockaddr_un socket_address;
    char buffer[BUFLEN];
    int socketfd;
    bool isRunning = true;
    memset(&socket_address, 0, sizeof(socket_address));
    if((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ){
        std::cout << "cannot create socket";
        return -1;
    }

    socket_address.sun_family = AF_UNIX;
    strncpy(socket_address.sun_path, SOCKET_PATH, 17);
    if(connect(socketfd, (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0){
        std::cout << "cannot connect" << std::endl;
        return -1;
    }else{
        std::cout << "Connected" << std::endl;
    }    
    //!code copied from lab 6 client.cpp, modified 

    Data d(argv[1]);
    d.update();

    write(socketfd, "Ready", 6);
    while(isRunning){
        memset(buffer, 0, BUFSIZ);
        read(socketfd, buffer, BUFSIZ);
        if(string("Monitor") == buffer){
            d.update();
            cout << d << endl;
            if(d.isDown()){
                write(socketfd, "LinkDown", 9);
                cout << "Network " << d.getName() << " is down." << endl;
            }else{
                write(socketfd, "Done", 5);
            }
            sleep(1);
        }else if(string("SetLinkUp") == buffer){
            cout << "trying to bring " << d.getName() << " up... " << endl;
            if(!bringNetworkUp(d.getName().c_str())){
                cout << "Cannot bring this network up" << endl
                << "you may run this program using 'sudo'" << endl
                << "    error: " << strerror(errno) << endl;
            }
            write(socketfd, "Done", 5);
            sleep(1);
        }else if(string("shutdown") == buffer){ //clearning before quit.
            close(socketfd);
            isRunning = false;
        }
    }
	return 0;
}

bool bringNetworkUp(const char* interface_name){
    if(interface_name == nullptr) return false;
    
    struct ifreq ifr;
    int fd = -1;

    memset(&ifr, 0, sizeof ifr);

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
    ifr.ifr_flags |= IFF_UP;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;

    if(ioctl(fd, SIOCSIFFLAGS, &ifr) != -1){
        return true;
    }else{
        return false;
    }
    
}

int readInt(string path){
    int a = -1;
    in.close();
    in.open(path);
    if(in.is_open()){
        in >> a;
    }
    in.close();
    return a;
}   

string readStr(string path){
    string str = "unknown";
    in.close();
    in.open(path);
    if(in.is_open()){
        in >> str;
    }
    in.close();
    return str;
}



