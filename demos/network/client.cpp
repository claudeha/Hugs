#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

using namespace std;

#define SERVERPORT "3000"
#define SERVERIP "127.0.0.1"

enum { MAX_BUFFER_SIZE = 1024 };

int main(int argc, char* argv[])
{
    struct addrinfo hints, *servinfo, *p;
    const char* msg = "Hello World";
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    getaddrinfo(SERVERIP, SERVERPORT, &hints, &servinfo);
    int sockfd;
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        cerr << "client: could not bind socket" << endl;
        return 2;
    }
    int sent = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen);
    cout << "sent " << sent << " bytes to " << SERVERIP << endl;
    char buffer[MAX_BUFFER_SIZE];
    int bytesReceived = recv( sockfd, buffer, MAX_BUFFER_SIZE-1, 0 );
    buffer[bytesReceived]= '\0';
    cout << "received back " << bytesReceived << " bytes: " << buffer << endl;
    freeaddrinfo(servinfo);
    close(sockfd);
    return 0;
}
