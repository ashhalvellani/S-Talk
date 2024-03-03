#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "list.h"

#define BUFLEN 1024

const char* myPort;
const char* remoteHostname;
const char* remotePort;

List* list;

// define all 4 threads
static pthread_t inputThread;
static pthread_t outputThread;
static pthread_t receiverThread;
static pthread_t senderThread;

// list mutex
static pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;

// send and rec mutexes and conds
static pthread_mutex_t sendMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sendCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t recMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t recCond = PTHREAD_COND_INITIALIZER;

// send and rec messages and sockets to free and close, respectively 
// when finished
static char* messageToSend;
static int sockfdSend;

static char* messageToRec;
static int sockfdRec;

static void* keyboardInputLoop(void* args){
    while(1){
        char* msg;
        char buffer[BUFLEN];
        int size;
        do {
            // zero out buffer
            bzero(buffer, BUFLEN);

            // record size of message and save it in buffer
            size = read(0, buffer, BUFLEN);

            // copy buffer to malloc'd string of exact size to
            // add to list
            msg = (char*) malloc(sizeof(char) * (size + 1));
            strncpy(msg, buffer, size);
            msg[size] = '\0';

            // lock list and prepend message to send
            pthread_mutex_lock(&listMutex);
            List_prepend(list, msg);
            pthread_mutex_unlock(&listMutex);

            // if message was a single '!', terminate chat and 
            // cancel threads
            if (!strcmp(msg,"!\n")){
                pthread_mutex_lock(&sendMutex);
                pthread_cond_signal(&sendCond);
                pthread_mutex_unlock(&sendMutex);

                pthread_cancel(outputThread);
                pthread_cancel(receiverThread);
                return NULL;
            }
        } while (buffer[size-1] != '\n'); // stop when enter pressed

        // signal send thread to continue as message has been 
        // added and list is accessible
        pthread_mutex_lock(&sendMutex);
        pthread_cond_signal(&sendCond);
        pthread_mutex_unlock(&sendMutex);
    }
    return NULL;
}

static void* sendMessageLoop(void* args) {
    // Adapted from Beej's Guide to Network Programming
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    int val = getaddrinfo(remoteHostname, remotePort, &hints, &servinfo);  
    if (val != 0 ){exit(-1);}

    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfdSend = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfdSend ==-1){continue;}
        break;
    }
    if(p == NULL){exit(-1);}
    
    while (1) {
        // wait until signalled by input thread to continue
        // i.e. message is in the list and ready to send
        pthread_mutex_lock(&sendMutex);
        pthread_cond_wait(&sendCond, &sendMutex);
        pthread_mutex_unlock(&sendMutex);

        do {
            // lock list and trim message to send
            pthread_mutex_lock(&listMutex);
            messageToSend = List_trim(list);
            pthread_mutex_unlock(&listMutex);

            // send message and assert success
            int size = sendto(sockfdSend, messageToSend, strlen(messageToSend), 0, p->ai_addr, p->ai_addrlen);
            if(size == -1){exit(-1);}

            // if sent message was a single '!' output chat terminated and exit
            if(!strcmp(messageToSend,"!\n")) {
                free(messageToSend);
                messageToSend = NULL;

                char* endMessage = "Chat terminated\n";
                write(1, endMessage, strlen(endMessage));
                
                freeaddrinfo(servinfo);
                return NULL;
            }
            // free message
            free(messageToSend);
            messageToSend = NULL;

        } while (List_count(list) != 0); // send till list empty
    }
    freeaddrinfo(servinfo);
    return NULL;
}

static void* receiveMessageLoop(void* args) {
    // Adapted from Beej's Guide to Network Programming
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in remoteAddress;
    socklen_t addressLen;
    
    char buffer[BUFLEN];
    char* msg;
    int size;

    memset(&hints, 0 ,sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    int val = getaddrinfo(NULL, myPort, &hints, &servinfo);
    if (val != 0){exit(-1);}

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfdRec = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfdRec ==-1){continue;}

        int bindVal = bind(sockfdRec, p->ai_addr, p->ai_addrlen);
        if(bindVal == -1)
        {
            close(sockfdRec);
            continue;
        }
        break;
    }
    if(p == NULL){exit(-1);}
    freeaddrinfo(servinfo);

    while (1){
        do {
            // receive message from remote address and store in buffer
            addressLen = sizeof(remoteAddress);
            size = recvfrom(sockfdRec, buffer, BUFLEN, 0, (struct sockaddr*) &remoteAddress, &addressLen);
            if(size == -1){exit(-1);}

            // copy buffer to malloc'd string of exact size to
            // add to list
            msg = (char*)malloc(sizeof(char)*(size+1));
            strncpy(msg, buffer, size);
            msg[size] = '\0';

            // lock list and prepend message to receive
            pthread_mutex_lock(&listMutex);
            List_prepend(list, msg);
            pthread_mutex_unlock(&listMutex);

            // if message was a single '!', terminate chat and 
            // cancel threads
            if(!strcmp(msg, "!\n"))
            {
                pthread_mutex_lock(&recMutex);
                pthread_cond_signal(&recCond);
                pthread_mutex_unlock(&recMutex);
                
                pthread_cancel(inputThread);
                pthread_cancel(senderThread);
                
                return NULL;
            }
        } while (buffer[size-1]!='\n'); // stop when new line found
        
        // signal output thread to continue as message has been 
        // added and list is accessible
        pthread_mutex_lock(&recMutex);
        pthread_cond_signal(&recCond);
        pthread_mutex_unlock(&recMutex);
    }
    return NULL;
}

static void* screenOutputLoop(void* args) {
    while (1){
        // wait until signalled by receiver thread to continue
        // i.e. message is in the list and ready to output
        pthread_mutex_lock(&recMutex);
        pthread_cond_wait(&recCond, &recMutex);
        pthread_mutex_unlock(&recMutex);

        do {
            // lock list and trim message to output
            pthread_mutex_lock(&listMutex);
            messageToRec = List_trim(list);
            pthread_mutex_unlock(&listMutex);

            // add prefix to differentiate local and remote
            // messages
            char* remotePrefix = "Remote: ";
            write(1, remotePrefix, strlen(remotePrefix));

            // write message and assert success
            int writeVal = write(1, messageToRec, strlen(messageToRec));
            if(writeVal == -1) {exit(-1);}

            // if received message is a single '!' output chat terminated and exit
            if(!strcmp(messageToRec, "!\n")) {
                free(messageToRec);
                messageToRec = NULL;

                char* endMessage = "Chat terminated\n";
                write(1,endMessage, strlen(endMessage));

                return NULL;
            }
            // free message
            free(messageToRec);
            messageToRec = NULL;

        } while (List_count(list) != 0); // output till list empty
    }
    return NULL;
}

int main(int argc, char const *argv[]) {
    if (argc!=4) {
        printf("Invalid arguments.\n");
        return -1;
    }

    // store arguments
    myPort = argv[1];
    remoteHostname = argv[2];
    remotePort = argv[3];

    list = List_create();

    // initiate threads
    pthread_create(&inputThread, NULL, keyboardInputLoop, NULL);
    pthread_create(&senderThread, NULL, sendMessageLoop, NULL);
    pthread_create(&receiverThread, NULL, receiveMessageLoop, NULL);
    pthread_create(&outputThread, NULL, screenOutputLoop, NULL);

    // terminate threads
    pthread_join(inputThread, NULL);
    pthread_join(senderThread, NULL); 
    pthread_join(receiverThread, NULL);
    pthread_join(outputThread, NULL);

    // close sockets and free memory
    close(sockfdSend);
    free(messageToSend);
    messageToSend = NULL;

    close(sockfdRec);
    free(messageToRec);
    messageToRec = NULL;

    // free list
    List_free(list, free);

    return 0;
}
