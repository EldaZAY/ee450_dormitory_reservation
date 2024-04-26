#include "server_utils.h"

// #define DEBUG


/**
 * Read from input string of log in information (form: "username,password"),
 * get username and password
 * @param line: input string
 * @param username: to store the extracted username
 * @param password: to store the extracted password
 */
void getLoginInfoFromLine(const std::string& line, std::string& username, std::string& password) {
    std::regex re("([^,]*),(.*)");
    std::smatch match;
    if (regex_search(line, match, re)){
        username = match[1].str();
        password = match[2].str();
    }
}


/**
 * Read from input string of one room data entry (form: "roomcode,available number"),
 * get roomcode and new number of availability
 * @param line: input string sent over sockets
 * @param roomcode: to store the extracted roomcode
 * @param numAvailable: to store the extracted available number
 */
void getDataFromLine(const std::string& line, std::string& roomcode, int& numAvailable) {
    std::regex re("([^,]+),\\s*(\\d+)");
    std::smatch match;
    if (regex_search(line, match, re)){
        roomcode = match[1].str();
        numAvailable =  stoi(match[2].str());
    }
}


/**
 * Read from input string of one room data entry (form: "roomcode,available number"),
 * store data into map<string, int> (string roomcode: int available num)
 * @param line: input string sent over sockets
 * @param mymap: the map to add entries to; entries with the same key will be overwritten.
 */
void addLineToMap(const std::string& line, std::map<std::string, int>& mymap) {
    std::regex re("([^,]+),\\s*(\\d+)");
    std::smatch match;
    if (regex_search(line, match, re)){
        std::string roomcode = match[1].str();
        int numAvailable =  stoi(match[2].str());
        mymap[roomcode] = numAvailable;
    }
}


/**
 * turn data map<string, int> to a printable string, convert each entry to "key,value\n"
 * @param mymap
 * @return
 */
std::string dataToStr(std::map<std::string, int> const& mymap) {
    std::string res;
    for (const auto& pair: mymap) {
        res += pair.first + "," + std::to_string(pair.second) + "\n";
    }
    return res;
}


/**
 * Get the name of the server from the address & port
 * @param address IP address
 * @param port port number
 * @return A string representing the server's name
 */
std::string portToServerName(const std::string& address, const std::string& port) {
    if (address == LOCAL_HOST) {
        if (port == PORT_SS_UDP) {
            return "S";
        }
        if (port == PORT_SD_UDP) {
            return "D";
        }
        if (port == PORT_SU_UDP) {
            return "U";
        }
        if (port == PORT_SM_UDP || port == PORT_SM_TCP) {
            return "M";
        }
    }
    std::cout << "No matching server found" << std::endl;
    return "";
}


// get sockaddr, IPv4 or IPv6; reused code from Beej's Guide 6.3
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



// class BackendServer implementation


BackendServer::BackendServer(const std::string& serverName, const std::string& hostAddress, const std::string& UDPport) {
    this->serverName = serverName;
    this->hostAddress = hostAddress;
    this->port_UDP = UDPport;
    this->sockfd_UDP = -1;
}


BackendServer::~BackendServer() {
    if (sockfd_UDP != -1) {
        close(sockfd_UDP);
    }
}


/**
 * Read from input file, store data into roomData
 * @param file: input file path + name
 */
void BackendServer::initDataFromFile(const std::string& file) {
    std::ifstream inFile(file);
    std::regex re("([^,]+),\\s*(\\d+)");
    std::smatch match;
    std::string line;
    std::string roomcode;
    int numAvailable;
    while (getline(inFile, line)) {
        if (regex_search(line, match, re)) {
            roomcode = match[1].str();
            numAvailable =  stoi(match[2].str());
        }
        roomData[roomcode] = numAvailable;
    }
}


/**
 * Creat & bind a UDP socket
 * @return whether successful or not
 */
bool BackendServer::bootup() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_DGRAM; // use UDP

    struct addrinfo *serverInfo;

    // Create a UDP socket and bind to the designated port
    if (getaddrinfo(hostAddress.c_str(), port_UDP.c_str(), &hints, &serverInfo) != 0) {
        perror(("Server" + serverName + ": getaddrinfo").c_str());
        return false;
    }
    sockfd_UDP = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
    if (sockfd_UDP == -1) {
        perror(("Server" + serverName + ": socket").c_str());
        return false;
    }
    if (bind(sockfd_UDP, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
        close(sockfd_UDP);
        perror(("Server" + serverName + ": bind").c_str());
        return false;
    }
    freeaddrinfo(serverInfo);

    std::cout << "The Server " << serverName << " is up and running using UDP on port " << port_UDP << "." << std::endl;

    return true;
}


/**
 * Stores main server information into SMinfo
 * @param hostAddress host address of the main server
 * @param UDPport the UDP port of the main server, over which the backend server communicate with the main server
 * @return whether successful or not
 */
bool BackendServer::addMainServer(const std::string& hostAddress, const std::string& UDPport) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4
    hints.ai_socktype = SOCK_DGRAM; // use UDP

    if (getaddrinfo(hostAddress.c_str(), UDPport.c_str(), &hints, &SMinfo) != 0) {
        perror("ServerM: getaddrinfo");
        return false;
    }
    return true;
}


/**
 * Send the room data with the "INIT\n" header to the main server
 * @return whether successful or not
 */
bool BackendServer::sendInitDataToMainServer() const {
    std::string roomDataToSend = MSG_INIT;
    roomDataToSend += "\n" + dataToStr(roomData);
    if (sendto(sockfd_UDP, roomDataToSend.c_str(), roomDataToSend.length(), 0, SMinfo->ai_addr, SMinfo->ai_addrlen) == -1) {
        perror(("Server" + serverName + ": sendto").c_str());
        return false;
    }
    std::cout << "The Server " << serverName << " has sent the room status to the main server." << std::endl;
    return true;
}


/**
  * recvfrom the main server over the UDP port, and react accordingly.
  */
void BackendServer::handleMainServer() {
    int numbytes; // number of bytes of the received datagram
    char buf[MAXBUFLEN];
    std::string op, childSockfdStr, roomcode, replyMsg;

    numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, SMinfo->ai_addr, &(SMinfo->ai_addrlen));
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }
    buf[numbytes] = '\0';
    std::istringstream iss(buf);
#ifdef DEBUG
    std::cout << "Received message from the main server: " << iss.str() << std::endl;
#endif
    getline(iss, op); // extract operation code from the 1st line
    getline(iss, childSockfdStr); // extract childSockfd as a string from the 1st line
    getline(iss, roomcode); // extract childSockfd as a string from the 1st line

    if (op == MSG_CHECK_REQUEST) {
        std::cout << "The Server " << serverName << " received an availability request from the main server." << std::endl;
        if (roomData.find(roomcode) != roomData.end()) {
            if (roomData[roomcode] > 0) {
                std::cout << "Room " << roomcode << " is available." << std::endl;
                replyMsg = MSG_CHECK_AVAILABLE;
            }
            else {
                std::cout << "Room " << roomcode << " is not available." << std::endl;
                replyMsg = MSG_CHECK_UNAVAILABLE;
            }
        }
        else {
            std::cout << "Not able to find the room layout." << std::endl;
            replyMsg = MSG_CHECK_NOTFOUND;
        }
        replyMsg += "\n" + childSockfdStr;
    }
    else if (op == MSG_RESERVE_REQUEST) {
        std::cout << "The Server " << serverName << " received a reservation request from the main server." << std::endl;
        if (roomData.find(roomcode) != roomData.end()) {
            if (roomData[roomcode] > 0) {
                roomData[roomcode] -= 1;
                std::cout << "Successful reservation. The count of Room <roomcode> is now <updated_count>." << std::endl;
                replyMsg = MSG_RESERVE_SUCCEED;
                replyMsg += "\n" + childSockfdStr + "\n" + roomcode + "," + std::to_string(roomData[roomcode]);
            }
            else {
                std::cout << "Cannot make a reservation. Room " << roomcode << " is not available." << std::endl;
                replyMsg = MSG_RESERVE_FAIL;
                replyMsg += "\n" + childSockfdStr;
            }
        }
        else {
            std::cout << "Cannot make a reservation. Not able to find the room layout." << std::endl;
            replyMsg = MSG_RESERVE_NOTFOUND;
            replyMsg += "\n" + childSockfdStr;
        }
    }
    // send response to Server M
    if (sendto(sockfd_UDP, replyMsg.c_str(), replyMsg.length(), 0, SMinfo->ai_addr, SMinfo->ai_addrlen) == -1) {
        perror(("Server" + serverName + ": sendto").c_str());
        exit(1);
    }
    std::cout << "The Server " << serverName << " finished sending the response to the main server." << std::endl;
}
