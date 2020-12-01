Authors: Shichao Wang, Brandon Rychlowski

Use "make all" to build the network monitor.

Run "./netmonitor" to start the program.

The program contains the function that will automatically bring a network interface up if the network interface is down.
	To make this function work, super user authority may be needed. 

The program passed the test on ubuntu 18.04.1 LTS(Linux version 4.15.0-45-generic (buildd@lgw01-amd64-031) (gcc version 7.3.0 (Ubuntu 7.3.0-16ubuntu3)) #48-Ubuntu SMP Tue Jan 29 16:28:13 UTC 2019, Virtual Machine). 

The program did not pass the test on WSL(Windows Subsystem for Linux, ubuntu 18.04.1 LTS, Linux version 4.4.0-17763-Microsoft (Microsoft@Microsoft.com) (gcc version 5.4.0 (GCC) ) #253-Microsoft Mon Dec 31 17:49:00 PST 2018, Windows 10 1809).