#include <iostream>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>
#include <climits>
#define SERVER_PORT 8465
#define MAX_LINE 256
#define STDIN 0

using namespace std;


void to_lower_buf(char* buf, int num_chars)
{
    for (int i = 0; i < num_chars; ++i)
    {
        buf[i] = tolower(buf[i]);
    }
}

int main(int argc, char * argv[]) {

    fd_set master;   
    fd_set read_fds; 
    int fdmax;       

    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s;

    bool quitting = false;
    bool dont_lowercase = false;
    bool sending = false;
    bool skip_quit = false;


    FD_ZERO(&master);    
    FD_ZERO(&read_fds);

    // make sure right number of arguments
    if (argc != 2)
    {
        cout << "usage: client <Server IP>" << endl;
        exit(1);
    }


    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
		perror("select client: socket");
		exit(1);
    }

    // build address data structure 
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr  = inet_addr(argv[1]);
    sin.sin_port = htons (SERVER_PORT);

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    {
		perror("select client: connect");
		close(s);
		exit(1);
    }

    // add the STDIN to the master set
    FD_SET(STDIN, &master);

    // add the listener to the master set
    FD_SET(s, &master);

    // keep track of the biggest file descriptor
    fdmax = s; 

    // display welcome message
    cout << "List of Commands:" << endl;
	cout << "#1: Type 'add', then hit enter. Then, enter the 'first name' 'last name' and 'phone number' to add an employee. " << endl;
	cout << "#2: Type 'delete', then hit enter. Then, enter an 'id' to delete an employee." << endl;
	cout << "#3: Type 'list' to list all employees." << endl;
	cout << "#4: Type 'shutdown' to shut down server. " << endl;
	cout << "#5: Type 'quit' to disconnect from server. " << endl;
    cout << "#6: Type 'login 'username' 'password'' to login to server." << endl;
    cout << "#7: Type 'logout' to logout of the server" << endl;
    cout << "#8: Type 'who' to find out who is connected to server." << endl;
    cout << "#9: Type 'lookup' to find out specific records." << endl;


    cout << "c: ";

    //manurally flush buffer
    cout.flush();

    // main loop; get and send lines of text 
    while (1) 
    {
        read_fds = master; 

        // wait for server to send something or user to input something
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) 
        {
            perror("select");
            exit(1);
        }

        // if user input is ready 
        if (FD_ISSET(STDIN, &read_fds)) 
        {

            // handle the user input
            if (fgets(buf, sizeof(buf), stdin))
            {
                buf[MAX_LINE -1] = '\0';

                len = strlen(buf) + 1;
                
                //lower case all characters in buf
                if (!dont_lowercase)
                {
                    to_lower_buf(buf, strlen(buf));
                }

                else
                {
                    dont_lowercase = false;
                    skip_quit = true;
                }

                //see if message is send command
                string converted_buf(buf);
                converted_buf = converted_buf.substr(0, 4);

                //test to see if quitting
                if ((strcmp(buf, "quit\n") == 0) && !skip_quit)
                {
                    quitting = true;
                } 

                else
                {
                    skip_quit = false;
                }

                // if message is msgstore
                if (strcmp(buf, "msgstore\n") == 0 || converted_buf == "send")
                {
                    dont_lowercase  = true;
                }

                //send user's message to the server
                send(s, buf, len, 0);
            } 

            else 
            {
                break;
            }
        }
        
        // if there is a message from the server
        if (FD_ISSET(s, &read_fds)) 
        {
            recv(s, buf, sizeof(buf), 0);
            
            // get code of message
            string message_code(buf);
            message_code = message_code.substr(0, 3);

            // Quit
            if (quitting)
            {
                break;
            } 

            // Shutdown              
            if (message_code == "210")
            {
                cout << "\ns: " << buf;

                // fall out of while loop
                break;
            }

            //dont lowercase next input from client
            else if(message_code == "401" ||
                    message_code == "499" ||
                    message_code == "420" ||
                    message_code == "403" ||
                    message_code == "300")
            {
                dont_lowercase = false;
            }
            
            string converted_buf(buf);
            converted_buf = converted_buf.substr(0, 34);

            if(converted_buf == "200 OK you have a new message from")
            {
                cout << endl;
            }

            // display what the server sent
            cout << "s: " << buf;
            
            if (strcmp(buf, "200 OK") != 0)
            { 
                cout << "c: ";
            }

            // flush output buffer
            cout.flush();
        }    
    }

    close(s);
    return 0;
}