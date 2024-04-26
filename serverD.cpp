#include "server_utils.h"
using namespace std;


// #define DEBUG


int main(){
    // Create a BackendServer with a given name, a host address and a UDP port number.
    BackendServer serverD("D", LOCAL_HOST, PORT_SD_UDP);

    // Initialize room data from input file
    serverD.initDataFromFile("double.txt");

    // Bootup: create and bind a UDP socket
    if (!serverD.bootup()) {
        return 1;
    }
    // Add main server address & UDP port info
    serverD.addMainServer(LOCAL_HOST, PORT_SM_UDP);
    // Send initial roomData to ServerM.
    if (!serverD.sendInitDataToMainServer()) {
        return 1;
    }

    // Main loop: receive from ServerM, send replies to Server M.
    while (true) {
        serverD.handleMainServer();
    }
    return 0;
}