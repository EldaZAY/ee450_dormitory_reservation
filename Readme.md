## Socket Programming Project: Dormitory Reservation System — EE450 2024 spring

### 0 Personal Infomation
Name: Aiyu Zhang \
Student ID: 8524183902

### 1 Project Abstract
The dormitory reservation systems contains one main server, three backend servers, and multiple clients. The backend servers each takes charge of one type of room layout and communicates only with the main server over UDP. The main server accepts client connections over TCP, handles the login statuses of clients (as members or guests), forwards requests of clients to backend servers, and forward responses from backend servers to clients. 

The encryption of the client login credentials is by offsetting. The optional part of more advanced encryption is not completed. 

### 2 Code Files Description
#### 2.1 server_utils:
Contains constants (designated port numbers, operation codes that are sent in messages, etc.), class BackendServer and other common server utility functions.

class BackendServer: 
Stores room availability data in a map, stores socket related info of itself and the main server. Implements all methods that are needed for booting up and running a backend server. 

#### 2.2 Server<S/D/U>: 
Creates an instance of class BackendServer, loads data from the input file, and sends initialization data to the main server. The main loop keeps handling main server messages and sending responses.

#### 2.3 ServerM:
Contains class MainServer and runs the Server M. 

class MainServer: 
Stores all roomdata (corresponding to the data from backend servers) in a map, stores client login status and member status, stores socket related info of itself and the backend servers. Implements all methods that deal with clients and backend servers. 

main: Creates an instance of class MainServer, boots up and adds backend servers' info. Uses threading to run two loops simultaneously, one for handling clients, one for handling backend servers. 

#### 2.4 client:
Contains class Client and runs a client.

class Client: 
Stores socket info of itself and the main server. Implements methods that deal with on-screem prompts, login process including encryption, and communication with the main server.

main: Creates an instance of class Client, boots up, handles log in, and handles requests from the user. 


### 3 Exchanged Message Format
- All exchanged messages start with one line of operation code, and may be followed by necessary data.
- In the following description, "(roomcode)" stands for the room layout code.
- Each entry in "(room_data_entries)" is in the form of "(roomcode),(num_available)".
- "(childsockfd)" is used to identify the client that originates the requests.

#### 3.1 Backend servers to Server M:
Standard form of INIT:
> "(op code)\n(room_data_entries)"

Standard form of all messages except INIT:
> "(op code)\n(childsockfd)" + (optional)"\n(room_data_entry)"

Message Table:

| Exchanged Message                      | Description                                                                             |
|:---------------------------------------|:----------------------------------------------------------------------------------------|
| INIT\n(room_data_entries)              | Send the initial room status entries, each entry is in the form of "XXXX,num_available" |
| CH_0\n(childsockfd)                    | check availability - Room not available                                                 |
| CH_1\n(childsockfd)                    | check availability - Room available                                                     |
| CH_2\n(childsockfd)                    | check availability - Room not found                                                     |
| RE_0\n(childsockfd)                    | reserve - Room reservation failed                                                       |
| RE_1\n(childsockfd)\n(room_data_entry) | reserve - Room reservation succeeded, update Room XXXX's availability to num_available  |
| RE_2\n(childsockfd)                    | reserve - Room not found                                                                |

#### 3.2 Server M to backend servers:
Standard form: 
> "(op code)\n(childsockfd)\n(roomcode)"

Message Table:

| Exchanged Message              | Description                           |
|:-------------------------------|:--------------------------------------|
| CH\n(childsockfd)\n(roomcode)  | check availability of Room (roomcode) |
| RE\n(childsockfd)\n(roomcode)  | reserve one Room (roomcode)           |

#### 3.3 Server M to client:
Standard form:
> "(op code)" + (optional)"\n(roomcode)"

Message Table:

| Exchanged Message | Description                                |
|:------------------|:-------------------------------------------|
| LI_0              | log in result - failed, incorrect password |
| LI_1              | log in result - logged in as a member      |
| LI_2              | log in result - logged in as a guest       |
| LI_3              | log in result - username not found         |
| LI_4              | log in result - invalid username           |
| LI_5              | log in result - invalid password           |
| CH_0              | check availability - Room not available    |
| CH_1              | check availability - Room available        |
| CH_2              | check availability - Room not found        |
| RE_0              | reserve - Room reservation failed          |
| RE_1              | reserve - Room reservation succeeded       |
| RE_2              | reserve - Room not found                   |
| RE_3              | reserve - guest client, permission denied  |

#### 3.4 Client to Server M:
Standard form:
> "(op code)" + (optional)("\n(roomcode)" or "\n(encrypted_username,encrypted_password)"

Message Table:

| Exchanged Message                           | Description                                 |
|:--------------------------------------------|:--------------------------------------------|
| LI\n(encrypted_username,encrypted_password) | log in with encrypted username and password |
| CH\n(room_code)                             | check availability of (roomcode)            |
| RE\n(room_code)                             | reserve one Room of (roomcode)              |



### 4 Project Idiosyncrasy
- The validity of a guest's username is not checked (for the project's on-screem message requirement doesn't include this situation)
- In the provided member information file, some of the usernames is less than 5 chars long, which do not satisfy the described validity requirement (5-50 chars). Using the original member file may result in login failure.

### 5 Reused Code
Some of the codes referred to Beej's Guide and are marked with comments. There is no other reference or reused code. 
