#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>
#include <iterator>
#include <iomanip>

#define SERVER_PORT 8465
#define MAX_LINE 256
#define MAX_SIZE 10

using namespace std;

fd_set master;
int listener; 
int fdmax;
int ListSize = 0;

// Client Info
string clientList[MAX_SIZE];
map<int, string> client_ips;
map<int, string> client_ids;

// Usernames/Passwords
string user_names[4];
string passwords[4];
int idnum = 1000;

// Check for spaces in input
bool has_space(string str)
{
    for (int i = 0; i < str.size(); ++i)
    {
        if (isspace(str.at(i)))
        {
            return true;
        }
    }

    return false;
}


void* ClientNode(void *newfd) 
{
    // vars
    char buf[MAX_LINE];
    int nbytes;
    int i, j;
    long clientSocket = (long) newfd;
    int len = 0;
    bool add_check = false;
    bool del_check = false;
    int user_pos = -1;
    bool send_msg = false;
    int recv_soc = -1;
    string space = " ";
    fstream file, temp;
    string fixed, newid;

    while(1) 
    {
        // handle data from a client
        if ((nbytes = recv(clientSocket, buf, sizeof(buf), 0)) <= 0) 
        {
            // got error or connection closed by client
            if (nbytes == 0) 
            {
                
                cout << "Socket " << clientSocket <<" terminated" << endl;
            }
            
            else 
            {
                perror("recv");
            }

            // remove client's ip address and id and close socket, terminate thread
            client_ips.erase(clientSocket);
            client_ids.erase(clientSocket);
            close(clientSocket); 
            FD_CLR(clientSocket, &master);
            pthread_exit(0);
        } 
       
        //recv from client
        else
        {
            // print message that was sent to server
            cout << "Socket " << clientSocket << ": "  << buf;

            string first_word(buf);     
            int num_spaces = 0;

            if (has_space(first_word) && !add_check)
            {
                for (int i = 0; i < first_word.size(); ++i)
                {
                    if (isspace(first_word.at(i)))
                    {
                        ++num_spaces;
                    }
                }

                --num_spaces; 

                first_word = first_word.substr(0, first_word.find(' '));
            }

            // Add Function
            if (add_check)
            { 

                idnum++;
                string tempbuf(buf);
                ofstream write_message("output.txt", ios_base::app);
                write_message << idnum << space << tempbuf;
                write_message.close();

                // Puts new employee at next spot
                clientList[ListSize - 1] = tempbuf;
                cout << "Saving\n\t" << clientList[ListSize - 1] << "to the list." << endl;

                strcpy(buf, "200 OK\n");
                len = strlen(buf) + 1;
                send(clientSocket, buf, len, 0);

                add_check = false;
            }

            // List Function
            else if (strcmp(buf, "list\n") == 0)
            {
                string line;
				file.open("output.txt");
				getline(file, line);
                string message_to_client = "200 OK";
                strcpy(buf, message_to_client.c_str());
			
            // check if line is not empty
				if (!line.empty()) {
					string one, two, three, four, total;
					cout << "200 OK\n";
					file.close();
					file.open("output.txt");

            // output to text file, add to buffer
					while (file >> one >> two >> three >> four) {
						total = "\n" + one + space + two + space + three + space + four + "\n";
						cout << total << endl;
                        message_to_client += total;
                        strcpy(buf, message_to_client.c_str());
					}
					file.close();
				}
				else {
					strcpy(buf, "The list is empty!\n");
                    len = strlen(buf) + 1;
					send(clientSocket, buf, len, 0);
				}
                len = strlen(buf) + 1;
                send(clientSocket, buf, len, 0);
                
            }
            
            // Delete Function
            else if (del_check)
            {
                string tempid(buf);
                string one, two, three, four, total;
                temp.open("temp.txt");
                file.open("output.txt");

                while (file >> one >> two >> three >> four) {
                    if (tempid != one){
                        total = "\n" + one + space + two + space + three + space + four + "\n";
                        temp << total << endl;
                    } 
					}   
                file.close();
                temp.close();
                
                remove("output.txt");
                rename("temp.txt", "output.txt");
                
                strcpy(buf, "200 OK\n");
               
                len = strlen(buf) + 1;
               
                send(clientSocket, buf, len, 0);

                del_check = false;

            }

            // if we get the quit command
            else if (strcmp(buf, "quit\n") == 0)
            {
                strcpy(buf, "200 OK\n");
                len = strlen(buf) + 1;
                send(clientSocket, buf, len, 0);
                close(clientSocket);
                cout << "Socket " << clientSocket <<" terminated" << endl;

                FD_CLR(clientSocket, &master);
                client_ips.erase(clientSocket);
                client_ids.erase(clientSocket);

                pthread_exit(0);
            }

            // Add check
            else if (strcmp(buf, "add\n") == 0)
            {
                // if user is NOT Logged in
                if (user_pos == -1) 
                {
                    strcpy(buf, "401 You are not currently logged in, login first.\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                }

                // if array is full
                else if (ListSize == MAX_SIZE)
                {
                    strcpy(buf, "403 Cannot hold any more messages.\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                }
						
                // if user is authorized
                else 
                {
                    ++ListSize;
                    strcpy(buf, "200 OK\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);

                    add_check = true;

                    continue;											
                }		    
            }

            // Delete Check
            else if (strcmp(buf, "delete\n") == 0){
                
                // if user is NOT Logged in
                if (user_pos == -1) 
                {
                    strcpy(buf, "401 You are not currently logged in, login first.\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                }

                    // if user is authorized
                else 
                {
                    strcpy(buf, "200 OK\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);

                    del_check = true;

                    continue;											
                }

            }

            // Login Check
            else if (first_word == "login" && num_spaces == 2)
            {
                string converted_buf(buf);
                string user_name;
                string password;

                int pos_first_space = 0;
                int pos_second_space = 0;
                bool user_name_found = false;
                bool pass_word_found = false;

                // test to see if user is already logged in 
                if (user_pos != -1)
                {
                    strcpy(buf, "411 Already logged in logout first\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                    continue;
                }

                // find spaces 
                pos_first_space = converted_buf.find(' ');
                pos_second_space = converted_buf.find(' ', pos_first_space + 1);       

                // get user name and password
                user_name = converted_buf.substr(
                                                  pos_first_space + 1,
                                                  pos_second_space - pos_first_space - 1);

                password = converted_buf.substr(
                                                 pos_second_space + 1,
                                                 converted_buf.size() - pos_second_space - 2);

                // see if user name is valid
                for (int i = 0; i < 4; ++i)
                {
                    if (user_names[i] == user_name)
                    {
                        user_name_found = true;
                        user_pos = i;
                        break;
                    }
                }

                // Invalid username
                if (!user_name_found)
                {
                    strcpy(buf, "410 Wrong UserID or Password\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                    continue;

                 }

                 // see if given password is correct
                 if (passwords[user_pos] == password)
                 {
                     client_ids[clientSocket] = user_name;
                     strcpy(buf, "200 OK\n");
                     len = strlen(buf) + 1;
                     send(clientSocket, buf, len, 0);

                 }

                 // wrong password 
                 else
                 {
                     strcpy(buf, "410 Wrong UserID or Password\n");
                     len = strlen(buf) + 1;
                     send(clientSocket, buf, len, 0);
                     user_pos = -1;
                }
            }

            // Logout Function
            else if (strcmp(buf, "logout\n") == 0)
            {
                // if user is not logged in 
                if (user_pos == -1)
                {
                    strcpy(buf, "412 Not logged in\n");  
                    len = strlen(buf) + 1; 
                    send(clientSocket, buf, len, 0);
                } 

                else
                {
                    // mark client as anonymous
                    client_ids[clientSocket] = "anonymous";

                    strcpy(buf, "200 OK\n");  
                    len = strlen(buf) + 1; 
                    send(clientSocket, buf, len, 0);

                    // set to -1 as user is no longer logged in 
                    user_pos = -1;
                } 
            }

            // Who function
            else if (strcmp(buf, "who\n") == 0)
            {
                string message = "200 OK \nThe list of the active users: \n";
                string filler = "             ";

                for (int i = 0; i < fdmax + 1; ++i)
                {
                    if (FD_ISSET(i, &master) && i != listener)
                    {
                        message += filler.replace(0, client_ids[i].size(), client_ids[i]) + client_ips[i] + "\n";       
                        filler = "             ";
                    }
                }

                // c.str turns string into character array
                strcpy(buf, message.c_str());
                len = strlen(buf) + 1;
                send(clientSocket, buf, len, 0);
            }

            // if message is shutdown
            else if (strcmp(buf, "shutdown\n") == 0)
            {
                // if the user is not the root
                if (user_pos != 0)
                {
                    strcpy(buf, "402 User not allowed to execute this command.\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                }

                // if the user is the root
                else
                {    
                    // close listener
                    close(listener);
                    FD_CLR(listener, &master); 
                    FD_CLR(clientSocket, &master);

                    strcpy(buf, "210 the server is shutting down.\n");
                    len = strlen(buf) + 1;

                    for (int i = 0; i < fdmax + 1; ++i)
                    {
                        if (FD_ISSET(i, &master))
                        {
                            send(i, buf, len, 0);
                        }
                    }

                    // send 200 OK to client that sent shutdown
                    strcpy(buf, "200 OK");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);

                    usleep(5 * 1000);

                    // send last message to client that sent shutdown
                    strcpy(buf, "210 the server is about to shutdown ......\n");
                    len = strlen(buf) + 1;
                    send(clientSocket, buf, len, 0);
                   
                    // close clientSocket
                    close(clientSocket);
                    cout << "Socket " << clientSocket <<" terminated" << endl;
 
                    // put thread to sleep
                    usleep(5 * 1000);
                    exit(0);
                }
            }

            // Lookup function
            else if (first_word == "lookup" && num_spaces == 2){
                

                /*
                string converted_buf(buf);
                string finder;
                lenfw = strlen(first_word);
                second_word = first_word.substr(lenfw + 2, first_word.find(' '));
                file.open("output.txt");

                // Checking for first names
                if (second_char == '1'){
                    while (getline(file, fixed)) {
                        for (i = 0; i < sizeof(clientList))
                        if (converted_buf == client){
                            finder += clientList[i]
                            strcpy(buf, clientList[i]); 
                            len = strlen(buf) + 1;
                            send(clientSocket, buf, len, 0);
                        }
                        
                }
                // Checking for last names
                if (second_char == '2'){
                    while (getline(file, fixed)) {
                        for (i = 0; i < sizeof(clientList))
                        if (converted_buf == client){
                            finder += clientList[i]
                            strcpy(buf, clientList[i]); 
                            len = strlen(buf) + 1;
                            send(clientSocket, buf, len, 0);
                        }
                }
                //  Checking for Phone number
                while (getline(file, fixed)) {
                    int phonenum = atoi(convertedbuf)
                        for (i = 0; i < sizeof(clientList))
                        if (phonenum == clientList[i])
                            cout << phonenum << endl;
                            strcpy(buf, clientList[i]); 
                            len = strlen(buf) + 1;
                            send(clientSocket, buf, len, 0);
                        }
                */
               continue;

            }

            // if we get invalid input
            else
            {
                strcpy(buf, "300 message format error\n");
                len = strlen(buf) + 1;
                send(clientSocket, buf, len, 0); 
            }
        }
    }
}

int main(void)
{
    //vars
    struct sockaddr_in myaddr;     
    struct sockaddr_in remoteaddr; 
    int newfd;        
    int yes=1;        
    socklen_t addrlen;        
    pthread_t cThread;

    // read data in file into internal data structure
    fstream outFile;
    outFile.open("output.txt");

    // make sure opening file was successful 
    if (!outFile.is_open())
    {
        cout << "Failed to open output.txt" << endl;
        exit(1);
    }

    // after while loop, ListSize will equal the number of messages and all of the messages will by in the array
    while(outFile.peek() != EOF)
    {
        getline(outFile, clientList[ListSize]);
        ++ListSize;
    }
 
    cout << endl;

    for (int i = 0; i < ListSize; ++i)
    {
        cout << "Loading message: " << clientList[i] << endl;

        /* append new line character to message */
        clientList[i] += '\n';
    }

    cout << "Num messages: " << ListSize << endl;
    cout << endl;

    outFile.close();  

    // initialize user names and passwords
    user_names[0] = "root";
    user_names[1] = "john";
    user_names[2] = "david";
    user_names[3] = "mary";

    passwords[0] = "root01";
    passwords[1] = "john01";
    passwords[2] = "david01";
    passwords[3] = "mary01";

    // clear the master set
    FD_ZERO(&master);    

    // get the listener
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket");
        exit(1);
    }

    // lose the pesky "address already in use" error message
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
    {
        perror("setsockopt");
        exit(1);
    }

    // bind
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    myaddr.sin_port = htons(SERVER_PORT);
    memset(&(myaddr.sin_zero), '\0', 8);

    if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) 
    {
        perror("bind");
        exit(1);
    }

    // listen
    if (listen(listener, 10) == -1) 
    {
        perror("listen");
        exit(1);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; 

    addrlen = sizeof(remoteaddr);

    // main loop
    for(;;) 
    {
        // handle new connections
        if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) 
        {
            perror("accept");
	    exit(1);
        }
       
        // if someone connects 
        else
        {
            // add to master set
            FD_SET(newfd, &master); 

            cout << "multiThreadServer: new connection from "
                 << inet_ntoa(remoteaddr.sin_addr)
                 << " socket " << newfd << endl;
            
            // keep track of the maximum
            if (newfd > fdmax) 
            {    
                fdmax = newfd;
            }

            // store user ip and id
            client_ips[newfd] = inet_ntoa(remoteaddr.sin_addr);
            client_ids[newfd] = "anonymous";

	    if (pthread_create(&cThread, NULL, ClientNode, (void *) newfd) < 0) 
            {
                perror("pthread_create");
                exit(1);
            }
        }
    }
    
    return 0;
}