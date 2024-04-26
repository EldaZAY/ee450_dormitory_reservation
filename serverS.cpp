#include "server_utils.h"
using namespace std;


// #define DEBUG


int main(){
    // Create a BackendServer with a given name, a host address and a UDP port number.
    BackendServer serverS("S", LOCAL_HOST, PORT_SS_UDP);

    // Initialize room data from input file
    serverS.initDataFromFile("single.txt");

    // Bootup: create and bind a UDP socket
    if (!serverS.bootup()) {
        return 1;
    }
    // Add main server address & UDP port info
    serverS.addMainServer(LOCAL_HOST, PORT_SM_UDP);
    // Send initial roomData to ServerM.
    if (!serverS.sendInitDataToMainServer()) {
        return 1;
    }

    // Main loop: receive from ServerM, send replies to Server M.
    while (true) {
        serverS.handleMainServer();
    }
    return 0;
}