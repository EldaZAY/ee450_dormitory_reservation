#include "server_utils.h"
using namespace std;


// #define DEBUG


int main(){
    // Create a BackendServer with a given name, a host address and a UDP port number.
    BackendServer serverU("U", LOCAL_HOST, PORT_SU_UDP);

    // Initialize room data from input file
    serverU.initDataFromFile("suite.txt");

    // Bootup: create and bind a UDP socket
    if (!serverU.bootup()) {
        return 1;
    }
    // Add main server address & UDP port info
    serverU.addMainServer(LOCAL_HOST, PORT_SM_UDP);
    // Send initial roomData to ServerM.
    if (!serverU.sendInitDataToMainServer()) {
        return 1;
    }

    // Main loop: receive from ServerM, send replies to Server M.
    while (true) {
        serverU.handleMainServer();
    }
    return 0;
}