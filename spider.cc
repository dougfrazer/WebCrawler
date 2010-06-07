#include <stdio.h>
#include <string.h>
#include <queue>
#include <set>
#include <map>
#include "url.h"
#include <stdarg.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <resolv.h>

#define MAXBUF  1024
void PANIC(char *msg);
#define PANIC(msg)  {perror(msg); abort();}

using namespace std;

typedef struct {
    url_t url;
    int depth;
} urlStruct;

typedef struct {
    char data[MAXBUF+1];
    url_t url;
    int threadID;
    int depth;
    int size;
} dataStruct;

//function declarations
dataStruct retrieveWebPage(url_t url, int currentDepth, int thread_id);
void* fetcherThread(void* arg);
void* parserThread(void* arg);
void parseWebPage(dataStruct& dataToParse);
void addUrlToFetchQueue(urlStruct& url);

queue<urlStruct> urlQueue;
queue<dataStruct> dataQueue;
map<std::string, int> prevSearched;

//the mutexes and condition variables
pthread_mutex_t theLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t parserHasWork;
pthread_cond_t fetcherHasWork;
pthread_cond_t workDone;
pthread_mutex_t** threadMutexes;
pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;

int depth = 0;

//numberInQueue is incremented when a url is added to the fetcher queue
//and decremented when a url is pulled off of the parser queue
int numberInQueue = 0;

int main(int argc, char** argv) {

    std::string inputSite(argv[1]);
    url_t output_url;
    output_url = parse_single_URL(inputSite);

    depth = atoi(argv[2]);
    int numOfThreads = atoi(argv[3]);


    urlStruct firstUrl;
    firstUrl.url = output_url;
    firstUrl.depth = 0;
    addUrlToFetchQueue(firstUrl);

    //the thread mutexes will be used to make the fetcher threads wait until
    //the parser thread has parsed their url
    threadMutexes = new pthread_mutex_t*[numOfThreads];
    pthread_t* fetcherThreadArray = new pthread_t[numOfThreads];
    pthread_t thread1;

    //limit the stack size of the threads
    pthread_attr_t attr;
    int iret1, iret2;

    iret2 = pthread_attr_setstacksize(&attr, 5*1024*1024);
    if (iret2) {
        cout << "pthread_attr_setstacksize returned " << iret2 << endl;
        exit(1);
    }

    iret2 = pthread_attr_init(&attr);
    if (iret2) {
        cout << "pthread_attr_init returned " << iret2 << endl;
        exit(1);
    }

    //start the parser thread
    iret1 = pthread_create( &thread1, &attr, parserThread,  NULL);

    //start the fetcher thread
    for(int i=0; i<numOfThreads ; i++){
        iret2 = pthread_create( &fetcherThreadArray[i], &attr, fetcherThread, (void*) i);
    }

    //wait until the work is completed
    pthread_mutex_lock(&theLock);
    //while(numberInQueue > 0) {
        pthread_cond_wait(&workDone, &theLock);
        pthread_mutex_unlock(&theLock);
    //}

    delete[] fetcherThreadArray;
    delete[] threadMutexes;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// Adds a url the the fetcher queue
///////////////////////////////////////////////////////////////////////////////////////
void addUrlToFetchQueue(urlStruct& url) {
    std::string theUrl = url.url.host + "/" + url.url.file;
    map<std::string, int>::iterator iter = prevSearched.find(theUrl);


    if(iter == prevSearched.end() && url.depth <= depth) {
        prevSearched.insert(make_pair(theUrl, url.depth));
        numberInQueue++;
        urlQueue.push(url);
        pthread_cond_broadcast(&fetcherHasWork);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// The fetcher thread
///////////////////////////////////////////////////////////////////////////////////////
void* fetcherThread(void* arg){
    int thread_id = (int)arg;
    pthread_mutex_t thisLock;
    threadMutexes[thread_id] = &thisLock;
    while(true) {
        //this mutex will be unlocked by the parser thread after this url has been parsed
        pthread_mutex_lock( &thisLock );
        pthread_mutex_lock( &theLock );

        //while there is nothing in the url queue, wait for things to be added to the queue
        while(urlQueue.empty()) {
            pthread_cond_wait(&fetcherHasWork, &theLock);
        }

        urlStruct nextUrl = urlQueue.front();
        urlQueue.pop();

        pthread_mutex_unlock( &theLock );

        dataStruct temp1 = retrieveWebPage(nextUrl.url, nextUrl.depth, thread_id);

        //insert the data into the parser queue
        pthread_mutex_lock( &theLock );
        dataQueue.push(temp1);
        pthread_mutex_unlock( &theLock );

        //signal the parser that there is work
        (void)pthread_cond_signal( &parserHasWork );
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// The parser thread
///////////////////////////////////////////////////////////////////////////////////////
void* parserThread(void* arg){

    while(true) {
        pthread_mutex_lock( &theLock );

        //wait until the parser has work
        while(dataQueue.empty()) {
        pthread_cond_wait(&parserHasWork, &theLock);
        }

        dataStruct nextData = dataQueue.front();
        dataQueue.pop();

        pthread_mutex_lock( &coutMutex );
        cout << "service requester " << nextData.threadID << " url " << nextData.url.host << "/" << nextData.url.file << endl;
        pthread_mutex_unlock( &coutMutex );

        //parse the url only if it is under the depth we are supposed to hit
        if(nextData.depth < depth){
            parseWebPage(nextData);
        }

        numberInQueue--;

        //if we are done signal the main thread
        if(numberInQueue == 0) {
            pthread_cond_signal(&workDone);
        }

        pthread_mutex_unlock( &theLock );
        //this is the lock that makes the fetcher thread wait, so unlock it
        pthread_mutex_unlock(threadMutexes[nextData.threadID]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////
// Parses the links from a url
///////////////////////////////////////////////////////////////////////////////////////
void parseWebPage(dataStruct& dataToParse){
    set<url_t, lturl, std::allocator<url_t> > urlSet;
    parse_URLs(dataToParse.data, dataToParse.size, urlSet);  
    int counter=0;
    int* depthOfSet = &dataToParse.depth;
    int actualDepth = *depthOfSet;

    set<url_t>::iterator dataPointer;
    for(dataPointer = urlSet.begin(); dataPointer != urlSet.end(); dataPointer++){
        urlStruct currentUrl;
        currentUrl.url = *dataPointer;
        currentUrl.depth = actualDepth + 1;

        addUrlToFetchQueue(currentUrl);
        counter++;
    }
}


///////////////////////////////////////////////////////////////////////////////////////
// Retrieves data from a url
///////////////////////////////////////////////////////////////////////////////////////
dataStruct retrieveWebPage(url_t url, int currentDepth, int thread_id) {
    int size = 0;
    dataStruct temp1;
    int sockfd, bytes_read, total_bytes = 0;
    struct sockaddr_in dest;
    char buffer[MAXBUF];
    struct hostent *hostEntity;

    /*---Should probably check arguments here---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        PANIC("Socket");

    /*---Initialize server address/port struct---*/
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(80); /*default HTTP Server port */
    hostEntity = gethostbyname(url.host.c_str());
    if( hostEntity == NULL) {
        printf("Failed to look up hostname %s\n", url.host.c_str());
        exit(1);
    }
    memcpy(&dest.sin_addr.s_addr, hostEntity->h_addr, hostEntity->h_length);

    /*---Connect to server---*/
    if ( connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 )
        PANIC("Connect");
    
    snprintf(buffer, MAXBUF, "GET /%s HTTP/1.1\r\nHost: %s\r\n", url.file.c_str(), url.host.c_str());
    snprintf(buffer+strlen(buffer), MAXBUF, "Connection: close\r\n");
    snprintf(buffer+strlen(buffer), MAXBUF, "User-Agent: Mozilla/5.0\r\n");
    snprintf(buffer+strlen(buffer), MAXBUF, "Cache-Control: max-age=0\r\n");
    snprintf(buffer+strlen(buffer), MAXBUF, "Accept: text/*;q=0.3, text/html;q=0.7, text/html;level=1\r\n");
    snprintf(buffer+strlen(buffer), MAXBUF, "Accept-Language: en-US,en;q=0.8\r\n");
    snprintf(buffer+strlen(buffer), MAXBUF, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n");
    snprintf(buffer+strlen(buffer), MAXBUF, "\r\n");

    send(sockfd, buffer, sizeof(buffer), 0);
    
    printf("succesfully wrote the buffers, need to read now");
    /*---While there's data, read and print it---*/
    do {
        bzero(buffer, sizeof(buffer));
        bytes_read = recv(sockfd, buffer, sizeof(buffer), 0);
        if ( bytes_read > 0 ) {
            sprintf(temp1.data+total_bytes, "%s", buffer);
            total_bytes += bytes_read;
//            printf("%s", buffer);
        }
    } while ( bytes_read > 0 );

    temp1.depth = currentDepth;
    temp1.size = total_bytes;
    temp1.url.file.clear();
    temp1.url.file.append(url.file);
    temp1.url.host.clear();
    temp1.url.host.append(url.host);
    temp1.url.type.clear();
    temp1.url.type.append(url.type);
    temp1.threadID = thread_id;

    pthread_mutex_lock( &coutMutex );
    cout << "requester " << thread_id  << " url " << url.host << "/" << url.file << endl;
    pthread_mutex_unlock( &coutMutex );

    /*---Clean up---*/
    close(sockfd);
    
    return temp1;
}
