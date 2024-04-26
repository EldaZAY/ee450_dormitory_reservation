#ifndef SERVERUTILS_H
#define SERVERUTILS_H


#include <iostream>
#include <cstdio>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sstream>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <map>
#include <regex>
#include <sys/wait.h>
#include <signal.h>
#include <set>
#include <thread>



// static information
#define LOCAL_HOST "127.0.0.1"
#define PORT_SS_UDP "41902"
#define PORT_SD_UDP "42902"
#define PORT_SU_UDP "43902"
#define PORT_SM_UDP "44902"
#define PORT_SM_TCP "45902"
#define MAXBUFLEN 1024
#define BACKLOG 10


// exchange messages' command/option
#define MSG_INIT "INIT"
#define MSG_CHECK_REQUEST "CH"
#define MSG_CHECK_UNAVAILABLE "CH_0"
#define MSG_CHECK_AVAILABLE "CH_1"
#define MSG_CHECK_NOTFOUND "CH_2"
#define MSG_RESERVE_REQUEST "RE"
#define MSG_RESERVE_FAIL "RE_0"
#define MSG_RESERVE_SUCCEED "RE_1"
#define MSG_RESERVE_NOTFOUND "RE_2"
#define MSG_RESERVE_DENIED "RE_3"
#define MSG_LOGIN_REQUEST "LI"
#define MSG_LOGIN_FAIL "LI_0"
#define MSG_LOGIN_MEMBER "LI_1"
#define MSG_LOGIN_GUEST "LI_2"
#define MSG_LOGIN_NOTFOUND "LI_3"
#define MSG_LOGIN_INVALID_USERNAME "LI_4"
#define MSG_LOGIN_INVALID_PASSWORD "LI_5"




/**
 * Read from input string of log in information (form: "username,password"),
 * get username and password
 * @param line: input string
 * @param username: to store the extracted username
 * @param password: to store the extracted password
 */
void getLoginInfoFromLine(const std::string& line, std::string& username, std::string& password);


/**
 * Read from input string of one room data entry (form: "roomcode,available number"),
 * get roomcode and new number of availability
 * @param line: input string sent over sockets
 * @param roomcode: to store the extracted roomcode
 * @param numAvailable: to store the extracted available number
 */
void getDataFromLine(const std::string& line, std::string& roomcode, int& numAvailable);


/**
 * Read from input string of one room data entry (form: "roomcode,available number"),
 * store data into map<string, int> (string roomcode: int available num)
 * @param line: input string sent over sockets
 * @param mymap: the map to add entries to; entries with the same key will be overwritten.
 */
void addLineToMap(const std::string& line, std::map<std::string, int>& mymap);


/**
 * turn data map<string, int> to a printable string, convert each entry to "key,value\n"
 * @param mymap
 * @return
 */
std::string dataToStr(std::map<std::string, int> const& mymap);


/**
 * Get the name of the server from the address & port
 * @param address IP address
 * @param port port number
 * @return A string representing the server's name
 */
std::string portToServerName(const std::string& address, const std::string& port);


// get sockaddr, IPv4 or IPv6; reused code from Beej's Guide 6.3
void *get_in_addr(struct sockaddr *sa);


class BackendServer {
private:
    std::map<std::string, int> roomData; // stores room availability data

    std::string hostAddress;
    std::string port_UDP; // port number
    int sockfd_UDP; // socket file descripter

    struct addrinfo * SMinfo{}; // store the main server's info
    std::string serverName; // the name of this backend server (S/D/U)



public:
    BackendServer(const std::string& serverName, const std::string& hostAddress, const std::string& UDPport);
    ~BackendServer();


    /**
     * Read from input file, store data into roomData
     * @param file: input file path + name
     */
    void initDataFromFile(const std::string& file);


    /**
     * Creat & bind a UDP socket
     * @return whether successful or not
     */
    bool bootup();


    /**
     * Stores main server information into SMinfo
     * @param hostAddress host address of the main server
     * @param UDPport the UDP port of the main server, over which the backend server communicate with the main server
     * @return whether successful or not
     */
    bool addMainServer(const std::string& hostAddress, const std::string& UDPport);


    /**
     * Send the room data with the "INIT\n" header to the main server
     * @return whether successful or not
     */
    bool sendInitDataToMainServer() const;

    /**
      * recvfrom the main server over the UDP port, and react accordingly.
      */
    void handleMainServer();


};



#endif //SERVERUTILS_H
