#include "server_utils.h"

// #define DEBUG

class Client {
private:
    // bool loginStatus;
    std::string username;

    int sockfd;
    std::string serverAddress;
    std::string serverPort;
    std::string clientPort;


    /**
     * encrypt the string by offsetting each character and/or digit by 3.
     * @param input original string
     * @return encrypted string
     */
    static std::string encrypt_offset(const std::string& input) {
        std::string encrypted;
        for (char c : input) {
            if (isdigit(c)) {
                // For digits
                int digit = c - '0';
                digit = (digit + 3) % 10; // Apply cyclic offset for digits
                encrypted += (digit + '0');
            } else if (isalpha(c)) {
                // For alphabets
                char base = isupper(c) ? 'A' : 'a';
                char encryptedChar = ((c - base + 3) % 26) + base; // Apply cyclic offset for alphabets
                encrypted += encryptedChar;
            } else {
                // For other characters, append as it is
                encrypted += c;
            }
        }
        return encrypted;
    }


    /**
     * recv() and store into istringstream
     * @return istringstream storing message received over a TCP socket
     */
    std::istringstream recvTCP() {
        int numbytes; // number of bytes of the received message
        char buf[MAXBUFLEN];

        numbytes = recv(sockfd, buf, MAXBUFLEN-1, 0);
        if (numbytes == -1) {
            perror("recv");
            exit(1);
        }
        buf[numbytes] = '\0';
        std::istringstream iss(buf);
#ifdef DEBUG
        std::cout << "Received message over TCP socket: " << iss.str() << std::endl;
#endif
        return iss;
    }

    /**
     * send message over a TCP socket to the server
     */
    void sendTCP(const std::string& msg) const {
        if (send(sockfd, msg.c_str(), msg.length(), 0) == -1) {
            perror(("Send to server: " + msg).c_str());
        }
#ifdef DEBUG
        std::cout << "Send message over TCP socket: " << msg << std::endl;
#endif
    }




public:
    Client(const std::string& serverAddress, const std::string& serverPort) {
        sockfd = -1;
        // loginStatus = false;
        username = "";
        this->serverAddress = serverAddress;
        this->serverPort = serverPort;
    }


    /**
     * Creat & bind a TCP socket, connect to server M.
     * @return whether successful or not
     */
    bool bootup() {
        struct addrinfo hints_TCP;
        memset(&hints_TCP, 0, sizeof hints_TCP);
        hints_TCP.ai_family = AF_INET; // use IPv4
        hints_TCP.ai_socktype = SOCK_STREAM; // use TCP

        struct addrinfo *serverInfo, *clientInfo;
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        // get serverInfo
        if (getaddrinfo(serverAddress.c_str(), serverPort.c_str(), &hints_TCP, &serverInfo) != 0) {
            perror("server: getaddrinfo");
            return false;
        }

        // Create a TCP socket
        if (getaddrinfo(LOCAL_HOST, nullptr, &hints_TCP, &clientInfo) != 0) {
            perror("Client: getaddrinfo");
            return false;
        }
        sockfd = socket(clientInfo->ai_family, clientInfo->ai_socktype, clientInfo->ai_protocol);
        if (sockfd == -1) {
            perror("Client: socket");
            return false;
        }
        if (bind(sockfd, clientInfo->ai_addr, clientInfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("Client: bind");
            return false;
        }
        // Get the dynamically assigned port
        if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &addrLen) == -1) {
            perror("Client: getsockname");
            return false;
        }
        clientPort = std::to_string(ntohs(clientAddr.sin_port));
#ifdef DEBUG
        std::cout << "Client's port: " << clientPort << std::endl;
#endif
        freeaddrinfo(clientInfo);

        if (connect(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            return false;
        }
        std::cout << "Client is up and running." << std::endl;
        return true;
    }


    /**
     * Keep prompting for username & password input and send request to server
     * until successfully logged in as a member/guest.
     */
    void login() {
        std::string input_username, input_password;
        bool loginStatus = false;
        while (!loginStatus) {
            input_username = "";
            input_password = "";
            std::cout << "Please enter the username: ";
            std::getline(std::cin, input_username);
            std::cout << "Please enter the password (Press “Enter” to skip for guest): ";
            std::getline(std::cin, input_password);

            // send login request to server M
            std::string msg = MSG_LOGIN_REQUEST;
            msg += "\n" + encrypt_offset(input_username) + "," + encrypt_offset(input_password);
            sendTCP(msg);
            if (input_password.empty()) {
                std::cout << input_username << " sent a guest request to the main server using TCP over port "
                << clientPort << "." << std::endl;
            }
            else {
                std::cout << input_username << " sent an authentication request to the main server." << std::endl;
            }

            // get login result from server M
            std::istringstream iss = recvTCP();
            std::string op; // store the operation code
            getline(iss, op); // extract operation code from the 1st line
            if (op == MSG_LOGIN_GUEST) {
                loginStatus = true;
                username = input_username;
                std::cout << "Welcome guest " << username << "!" << std::endl;
            } else if (op == MSG_LOGIN_MEMBER) {
                loginStatus = true;
                username = input_username;
                std::cout << "Welcome member " << username << "!" << std::endl;
            } else if (op == MSG_LOGIN_FAIL) {
                std::cout << "Failed login. Password does not match." << std::endl;
            } else if (op == MSG_LOGIN_NOTFOUND) {
                std::cout << "Failed login. Username does not exist." << std::endl;
            } else if (op == MSG_LOGIN_INVALID_USERNAME) {
                std::cout << "Failed login. Invalid username" << std::endl;
            } else if (op == MSG_LOGIN_INVALID_PASSWORD) {
                std::cout << "Failed login. Invalid password" << std::endl;
            }
        }
    }


    void handleRequests() {
        while (true) {
            std::string roomcode, input_op, msg;
            std::cout << "Please enter the room layout code: ";
            std::getline(std::cin, roomcode);
            std::cout << "Would you like to search for the availability or make a reservation? "
            << "(Enter “Availability” to search for the availability or Enter “Reservation” to make a reservation ): ";
            std::getline(std::cin, input_op);
#ifdef DEBUG
            std::cout << "input_op: " << input_op << std::endl;
#endif

            if (input_op == "Availability") {
                msg = MSG_CHECK_REQUEST;
                msg += "\n" + roomcode;
                sendTCP(msg);
                // send(sockfd, msg.c_str(), msg.length(), 0);

                std::cout << username << " sent an availability request to the main server." << std::endl;
            } else if (input_op == "Reservation") {
                msg = MSG_RESERVE_REQUEST;
                msg += "\n" + roomcode;
                sendTCP(msg);
                std::cout << username << " sent a reservation request to the main server." << std::endl;
            }
            else {
                continue;
            }

            // get response
            std::string op;
            std::istringstream iss = recvTCP();
            std::cout << "The client received the response from the main server using TCP over port " << clientPort << "." << std::endl;
            getline(iss, op);
            if (op == MSG_CHECK_AVAILABLE) {
                std::cout << "The requested room is available." << std::endl;
            } else if (op == MSG_CHECK_UNAVAILABLE) {
                std::cout << "The requested room is not available." << std::endl;
            } else if (op == MSG_CHECK_NOTFOUND) {
                std::cout << "Not able to find the room layout." << std::endl;
            } else if (op == MSG_RESERVE_SUCCEED) {
                std::cout << "Congratulation! The reservation for Room " << roomcode << " has been made." << std::endl;
            } else if (op == MSG_RESERVE_FAIL) {
                std::cout << "Sorry! The requested room is not available." << std::endl;
            } else if (op == MSG_RESERVE_NOTFOUND) {
                std::cout << "Oops! Not able to find the room." << std::endl;
            } else if (op == MSG_RESERVE_DENIED) {
                std::cout << "Permission denied: Guest cannot make a reservation." << std::endl;
            }

            std::cout << std::endl << "-----Start a new request-----" << std::endl;
        }
    }

};







int main() {
    Client client(LOCAL_HOST, PORT_SM_TCP);
    client.bootup();
    client.login();
    client.handleRequests();



    return 0;
}