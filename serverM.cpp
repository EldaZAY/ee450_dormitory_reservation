
#include "server_utils.h"

// #define DEBUG

struct LoginStatus {
    std::string username;
    bool loggedIn;
    bool isMember;
};

class MainServer {
private:

    std::map<std::string, int> allRoomData;
    std::map<std::string, addrinfo *> backendServers;
    std::map<std::string, std::string> memberData;
    std::map<int, struct LoginStatus> loginStatuses;

    std::string hostAddress;
    std::string port_UDP, port_TCP; // port numbers
    int sockfd_UDP, sockfd_TCP; // socket file descripters


    // reused code from Beej's Guide 6.1
    static void sigchld_handler(int s)
    {
        // waitpid() might overwrite errno, so we save and restore it:
        int saved_errno = errno;

        while(waitpid(-1, NULL, WNOHANG) > 0);

        errno = saved_errno;
    }


    /**
     * decrypt the string by offsetting each character and/or digit by -3.
     * @param input encrypted string
     * @return decrypted string
     */
    static std::string decrypt_offset(const std::string& input) {
        std::string decrypted;
        for (char c : input) {
            if (isdigit(c)) {
                // For digits
                int digit = c - '0';
                digit = (digit - 3) % 10; // Apply cyclic offset for digits
                decrypted += (digit + '0');
            } else if (isalpha(c)) {
                // For alphabets
                char base = isupper(c) ? 'A' : 'a';
                char decryptedChar = ((c - base - 3) % 26) + base; // Apply cyclic offset for alphabets
                decrypted += decryptedChar;
            } else {
                // For other characters, append as it is
                decrypted += c;
            }
        }
        return decrypted;
    }


    /**
     * Try to login with given encrypted username and password, and get login result
     * @param username encrypted username
     * @param password encrypted password
     * @return login result: "LI_0"/"LI_1"/"LI_2"/"LI_3"/"LI_4"
     */
    void tryLogin(const int& childSockfd, const std::string& username, const std::string& password) {
        std::regex pattern_username("[a-z]{5,50}");
        std::regex pattern_password(".{5,50}");
        std::string loginRes, decrypted_username;
        decrypted_username = decrypt_offset(username); // for printing on-screen messages

        // empty password: guest login
        if (password.empty()) {
            std::cout << "The main server received the guest request for " << decrypted_username <<
                " using TCP over port " << port_TCP << "." << std::endl;
            loginStatuses[childSockfd].loggedIn = true;
            loginStatuses[childSockfd].username = decrypted_username;
            std::cout << "The main server accepts " << decrypted_username << " as a guest." << std::endl;
            loginRes = MSG_LOGIN_GUEST;

            // send guest response to client
            if (send(childSockfd, loginRes.c_str(), loginRes.length(), 0) == -1) {
                perror("Child socket: send");
                exit(1);
            }
            std::cout << "The main server sent the guest response to the client." << std::endl;
            return;
        }

        // member login
        std::cout << "The main server received the authentication for " << decrypted_username << " using TCP over port "
            << port_TCP << "." << std::endl;
        if (!std::regex_match(username, pattern_username)) { // check username validity
            loginRes = MSG_LOGIN_INVALID_USERNAME;
        } else if (!std::regex_match(password, pattern_password)) { // check password validity
            loginRes = MSG_LOGIN_INVALID_PASSWORD;
        } else if (memberData.find(username) == memberData.end()) { // username not found
            loginRes = MSG_LOGIN_NOTFOUND;
        } else { // compare with memberData
            if (memberData[username] == password) {
                // successful login
                loginStatuses[childSockfd].loggedIn = true;
                loginStatuses[childSockfd].isMember = true;
                loginStatuses[childSockfd].username = decrypted_username;
                loginRes = MSG_LOGIN_MEMBER;
            } else { // incorrect password
                loginRes = MSG_LOGIN_FAIL;
            }
        }
        // send authentication result to client.
        if (send(childSockfd, loginRes.c_str(), loginRes.length(), 0) == -1) {
            perror("Child socket: send");
            exit(1);
        }
        std::cout << "The main server sent the authentication result to the client." << std::endl;
    }


    /**
     * recvfrom backend servers over the UDP port, and react accordingly.
     */
    void handleBackendServer() {
        struct sockaddr_in backend_server_address; // store sender's address
        socklen_t addr_len = sizeof(backend_server_address);

        int numbytes; // number of bytes of the received datagram
        char buf[MAXBUFLEN];
        std::string op; // store operation code

        numbytes = recvfrom(sockfd_UDP, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&backend_server_address, &addr_len);
        std::string serverName = portToServerName(LOCAL_HOST, std::to_string(ntohs(backend_server_address.sin_port)));

        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';
        std::istringstream iss(buf);
#ifdef DEBUG
        std::cout << "Received message from Server " << serverName << ": " << iss.str() << std::endl;
#endif
        getline(iss, op); // extract operation code from the 1st line


        if (op == MSG_INIT) { // do data initialization
            std::string line;
            while (getline(iss, line)) {
                addLineToMap(line, allRoomData);
            }
#ifdef DEBUG
            std::cout << "My data after INIT: \n" << dataToStr(allRoomData) << std::endl;
#endif
            std::cout << "The main server has received the room status from Server " << serverName <<
                " using UDP over port " << port_UDP << "." << std::endl;
        }
        else { // extract child sockfd info
            std::string line; // to temporarily store a line of string read from iss
            getline(iss, line);
            int childSockfd = std::stoi(line);
            if (op == MSG_RESERVE_SUCCEED) {
                std::cout << "The main server received the response and the updated room status from Server "
                << serverName << " using UDP over port " << port_UDP << "." << std::endl;
                // update the room status
                getline(iss, line);
                std::string roomcode;
                int newNumAvailable;
                getDataFromLine(line, roomcode, newNumAvailable);
                allRoomData[roomcode] = newNumAvailable;
                std::cout << "The room status of Room " << roomcode << " has been updated." << std::endl;
            }
            else {
                std::cout << "The main server received the response from Server " << serverName <<
                        " using UDP over port " << port_UDP << "." << std::endl;
            }

            // forward the same op code to the client
            if (send(childSockfd, op.c_str(), op.length(), 0) == -1) {
                perror(("Send to client: " + op).c_str());
                exit(1);
            }

            // print on-screen message
            if (op == MSG_CHECK_UNAVAILABLE || op == MSG_CHECK_AVAILABLE || op == MSG_CHECK_NOTFOUND) {
                std::cout << "The main server sent the availability information to the client." << std::endl;
            }
            else if (op == MSG_RESERVE_FAIL || op == MSG_RESERVE_SUCCEED || op == MSG_RESERVE_NOTFOUND ) {
                std::cout << "The main server sent the reservation result to the client." << std::endl;
            }


        }
    }


    /**
     * recv from a client over the TCP port, and react accordingly.
     * @param childSockfd child socket file descripter
     * @return false iff connection is closed
     */
    bool handleClient(int childSockfd) {
        int numbytes; // number of bytes of the received message
        char buf[MAXBUFLEN];
        std::string op; // store operation code

        numbytes = recv(childSockfd, buf, MAXBUFLEN-1, 0);
        if (numbytes == -1) {
            perror("recv");
            return false;
        }
        if (numbytes == 0) {
            return false;
        }
        buf[numbytes] = '\0';
        std::istringstream iss(buf);
#ifdef DEBUG
        std::cout << "Received message from a client: " << iss.str() << std::endl;
#endif
        getline(iss, op); // extract operation code from the 1st line

        if (op == MSG_LOGIN_REQUEST) {
            std::string login_info, username, password, loginRes;
            getline(iss, login_info); // extract login info from the second line
            getLoginInfoFromLine(login_info, username, password);
            tryLogin(childSockfd, username, password);
        }
        else if (loginStatuses[childSockfd].loggedIn) {
            std::string roomcode, backendServerName, msg;

            getline(iss, roomcode); // extract roomcode from the second line
            backendServerName = roomcode.substr(0, 1);
#ifdef DEBUG
            std::cout << "Roomcode: " << roomcode  <<
                "Extracted roomtype: " << backendServerName << std::endl;
#endif

            if (op == MSG_CHECK_REQUEST) {
                std::cout << "The main server has received the availability request on Room " << roomcode << " from "
                << loginStatuses[childSockfd].username << " using TCP over port " << port_TCP << "." << std::endl;
            } else if (op == MSG_RESERVE_REQUEST) {
                std::cout << "The main server has received the reservation request on Room " << roomcode << " from "
                << loginStatuses[childSockfd].username << " using TCP over port " << port_TCP << "." << std::endl;

                if (!loginStatuses[childSockfd].isMember) {
                    std::cout << loginStatuses[childSockfd].username << " cannot make a reservation." << std::endl;
                    msg = MSG_RESERVE_DENIED;
                    if (send(childSockfd, msg.c_str(), msg.length(), 0) == -1) {
                        perror(("Send to client: " + msg).c_str());
                        exit(1);
                    }
                    std::cout << "The main server sent the error message to the client." << std::endl;
                    return true;
                }
            }

            // In other cases, forward request to backend servers if the corresponding backend server exists.
            if (backendServers.find(backendServerName) == backendServers.end()) {
                // incorrect input roomcode (doesn't start with "S"/"D"/"U")
                std::string msg_onscreen;
                if (op == MSG_CHECK_REQUEST) {
                    msg = MSG_CHECK_NOTFOUND;
                    msg_onscreen =  "The main server sent the availability information to the client.";
                }
                else if (op == MSG_RESERVE_REQUEST) {
                    msg = MSG_RESERVE_NOTFOUND;
                    msg_onscreen =  "The main server sent the reservation result to the client.";
                }
                // directly send reply to client
                if (send(childSockfd, msg.c_str(), msg.length(), 0) == -1) {
                    perror(("Send to client: " + msg).c_str());
                    exit(1);
                }
                std::cout << msg_onscreen << std::endl;
            } else { // forward request to a backend server
                msg = op + "\n" + std::to_string(childSockfd) + "\n" + roomcode;
                if (sendto(sockfd_UDP, msg.c_str(), msg.length(), 0,
                    backendServers[backendServerName]->ai_addr, backendServers[backendServerName]->ai_addrlen) == -1) {
                    perror("Server M: sendto");
                    exit(1);
                }
                std::cout << "The main server sent a request to Server " << backendServerName << "." << std::endl;
            }
        }
        return true;
    }


public:
    MainServer(const std::string& hostAddress, const std::string& UDPport, const std::string& TCPport) {
        this->hostAddress = hostAddress;
        this->port_UDP = UDPport;
        this->port_TCP = TCPport;
        this->sockfd_UDP = -1;
        this->sockfd_TCP = -1;
    }

    ~MainServer() {
        for (const auto& pair : backendServers) {
            freeaddrinfo(pair.second);
        }
        if (sockfd_UDP != -1) {
            close(sockfd_UDP);
        }
        if (sockfd_TCP != -1) {
            close(sockfd_TCP);
        }
    }


    /**
     * Creat & bind a UDP socket and a TCP socket
     * @return whether successful or not
     */
    bool bootup() {

        struct addrinfo hints_UDP, hints_TCP;
        memset(&hints_UDP, 0, sizeof hints_UDP);
        memset(&hints_TCP, 0, sizeof hints_TCP);
        hints_UDP.ai_family = AF_INET; // use IPv4
        hints_TCP.ai_family = AF_INET; // use IPv4
        hints_UDP.ai_socktype = SOCK_DGRAM; // use UDP
        hints_TCP.ai_socktype = SOCK_STREAM; // use TCP

        struct addrinfo *SMInfo_UDP, *SMInfo_TCP;
        struct sigaction sa;


        // Create a UDP socket and bind to the designated port
        if (getaddrinfo(hostAddress.c_str(), port_UDP.c_str(), &hints_UDP, &SMInfo_UDP) != 0) {
            perror("ServerM UDP: getaddrinfo");
            return false;
        }
        sockfd_UDP = socket(SMInfo_UDP->ai_family, SMInfo_UDP->ai_socktype, SMInfo_UDP->ai_protocol);
        if (sockfd_UDP == -1) {
            perror("ServerM UDP: socket");
            return false;
        }
        if (bind(sockfd_UDP, SMInfo_UDP->ai_addr, SMInfo_UDP->ai_addrlen) == -1) {
            close(sockfd_UDP);
            perror("ServerM UDP: bind");
            return false;
        }
        freeaddrinfo(SMInfo_UDP);

        // Create a TCP socket and bind to the designated port, start listening.
        if (getaddrinfo(hostAddress.c_str(), port_TCP.c_str(), &hints_TCP, &SMInfo_TCP) != 0) {
            perror("ServerM TCP: getaddrinfo");
            return false;
        }
        sockfd_TCP = socket(SMInfo_TCP->ai_family, SMInfo_TCP->ai_socktype, SMInfo_TCP->ai_protocol);
        if (sockfd_TCP == -1) {
            perror("ServerM TCP: socket");
            return false;
        }
        if (bind(sockfd_TCP, SMInfo_TCP->ai_addr, SMInfo_TCP->ai_addrlen) == -1) {
            close(sockfd_TCP);
            perror("ServerM TCP: bind");
            return false;
        }
        freeaddrinfo(SMInfo_TCP);
        if (listen(sockfd_TCP, BACKLOG) == -1) {
            perror("listen");
            return false;
        }


        // reused code from Beej's Guide 6.1
        sa.sa_handler = sigchld_handler; // reap all dead processes
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            return false;
        }


        std::cout << "The main server is up and running." << std::endl;
        return true;
    }

    
    bool addBackendServers(const std::string& serverName, const std::string& hostAddress, const std::string& UDPport) {
        struct addrinfo hints_UDP;
        memset(&hints_UDP, 0, sizeof hints_UDP);
        hints_UDP.ai_family = AF_INET; // use IPv4
        hints_UDP.ai_socktype = SOCK_DGRAM; // use UDP

        struct addrinfo *serverInfo;
        if (getaddrinfo(hostAddress.c_str(), UDPport.c_str(), &hints_UDP, &serverInfo) != 0) { // Server S
            perror(("Server"+ serverName +": getaddrinfo").c_str());
            return false;
        }
        backendServers[serverName] = serverInfo;
        return true;
    }


    /**
     *  * Read from input file, store member data into memberData
     * @param file input file path + name
     */
    void initMemberDataFromFile(const std::string& file) {
        std::ifstream inFile(file);
        std::regex re("([^,]{5,50}),\\s(.{5,50})");
        std::smatch match;
        std::string line;
        std::string username, password;
        while (getline(inFile, line)) {
            if (regex_search(line, match, re)) {
                username = match[1].str();
                password = match[2].str();
            }
            memberData[username] = password;
        }
    }


    /**
     * Keep accepting new client connections. Deal with clients.
     */
    void handleMultipleClients() { // reused code from Beej's Guide 6.1
        int new_fd; // child socket filedescripter
        struct sockaddr_storage their_addr; // connector's address information
        socklen_t sin_size;

        // Main loop
        while (true) {
            sin_size = sizeof their_addr;
            new_fd = accept(this->sockfd_TCP, (struct sockaddr *)&their_addr, &sin_size);
            if (new_fd == -1) {
                perror("accept");
                continue;
            }
            struct LoginStatus defaultLoginStat= {"", false, false};
            loginStatuses[new_fd] = defaultLoginStat;

            pid_t pid = fork();

            if (pid == -1) {
                // Error occurred
                std::cerr << "Error in fork()" << std::endl;
                exit(1);
            }
            if (pid == 0) { // this is the child process
                close(this->sockfd_TCP); // child doesn't need the listener
                while (true) {
                    if (!this->handleClient(new_fd)) {
                        break;
                    }
                }
                close(new_fd);
                exit(0);
            }
            // close(new_fd);  // parent doesn't need this

        }
    }


    /**
     * Deal will all backend servers. Keep receiving and reacting accordingly.
     */
    void handleAllBackendServers() {
        while (true) {
            handleBackendServer();
        }
    }


};


int main(){
    MainServer serverS(LOCAL_HOST, PORT_SM_UDP, PORT_SM_TCP);
    if(!serverS.bootup()) {
        return 1;
    }
    serverS.addBackendServers("S", LOCAL_HOST, PORT_SS_UDP);
    serverS.addBackendServers("D", LOCAL_HOST, PORT_SD_UDP);
    serverS.addBackendServers("U", LOCAL_HOST, PORT_SU_UDP);
    serverS.initMemberDataFromFile("member.txt");

    // one thread for handling backend servers over UDP socket
    // another thread for handling clients over TCP socket
    std::thread t1(&MainServer::handleAllBackendServers, &serverS);
    std::thread t2(&MainServer::handleMultipleClients, &serverS);

    t1.join();
    t2.join();

    return 0;
}
