## Socket Programming Project: Dormitory Reservation System — EE450 2024 spring

### 0 Personal Infomation
Name: Aiyu Zhang \
Student ID: 8524183902

### 1 Project Abstract


### 2 Code Files Description
#### 2.1 server_utils:
#### 2.2 Server<S/D/U>:
#### 2.3 ServerM:
#### 2.4 client:


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

### 5 Reused Code