#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <csse2310a4.h>
#include <FreeImage.h>
#include <csse2310_freeimage.h>
#include <semaphore.h>
#include <signal.h>

#define PORT "--port"
#define CONNECTIONS "--max"
#define MAX_CONNECTIONS 10000
#define MIN_CONNECTIONS 0
#define COMMAND_LINE_ERROR 15
#define FAILED_LISTEN 3
#define BASE10 10

#define PORT_MIN 1024
#define PORT_MAX 65535

#define GET "GET"
#define POST "POST"

#define METHOD_NOT_ALLOWED 405
#define NOT_FOUND 404
#define OK 200
#define BAD_REQUEST 400
#define PAYLOAD_TOO_LARGE 413
#define UNPROCESSABLE_CONTENT 422
#define FAILED_OPERATION 501

#define HTML_PATH "/local/courses/csse2310/resources/a4/home.html"
#define ROTATE "rotate"
#define FLIP "flip"
#define SCALE "scale"
#define ROTATE_MIN (-359)
#define ROTATE_MAX 359
#define HORIZONTAL "h"
#define VERTICAL "v"
#define SCALE_MIN 1
#define SCALE_MAX 10000
#define LAST_SCALE_ARGUMENT 3

#define EIGHT_MIB 8388608

/**
 * A struct to store information regarding the command line parameters
 */
typedef struct {
    char* port; // A string representing the port
    bool portGiven; // A boolean representing if the user specified a port
    int max; // An int representing the max number of connections
    bool maxGiven; // A boolean represnting if the user specified max conn
} CommandParameters;

/**
 * A struct to store information about the HTTP request to construct
 */
typedef struct {
    char* method; // A string to represent the method
    char* address; // A string to represent the address
    unsigned char* body; // An unsigned string to represent the body
    unsigned long len; // An integer to represent the size of the body (bytes)
    HttpHeader** headers; // A pointer to an array for storing headers
} HttpRequest;

/**
 * A struct to store information about the HTTP response constructed
 */
typedef struct {
    int status; // An integer representing the status code
    char* statusExplanation; // A string to represent the status explanation
    HttpHeader** headers; // A pointer to an array for storing headers
    const unsigned char* body; // An unsigned const string for storing the body
    unsigned long bodySize; // An integer representing the size of the body
    unsigned long len; // An integer representing the size of the message
} HttpResponse;

/**
 * A struct to store information about the operations
 */
typedef struct {
    char* operation; // A string representing the operation name
    int value1; // An integer representing the first value (if needed)
    int value2; // An integer representing the second value (if needed)
    char* direction; // A string representing the flip direction (if needed)
} Operation;

/**
 * A struct to store information about the statistics generated
 */
typedef struct {
    unsigned int connected; // The number of currently connected clients
    unsigned int serviced; // The total number of serviced clients
    unsigned int success; // The number of successful HTTP responses
    unsigned int unSuccess; // The number of unsuccessful HTTP responses
    unsigned int operations; // The number of successful performed operations
} Statistics;

/**
 * A struct to store information regarding the arguments passed to the client
 * thread
 */
typedef struct {
    sem_t* semaphore; // The semaphoore for client number limiting
    int fd; // The file descriptor for receiving requests
    bool maxGiven; // Whether a max client limit was specified or not
    Statistics* stats; // A pointer to the struct for generating statistics
    pthread_mutex_t* statsMutex; // A mutex for modifying Statistics data
} ThreadArgs;

/**
 * A struct to store informationr regarding the arguments passed to the signal
 * thread
 */
typedef struct {
    sigset_t set; // The sigset for setting up the signal
    Statistics* stats; // A pointer to the struct for generating statistics
    pthread_mutex_t* statsMutex; // A mutex for modifying Statistics data
} SignalArgs;

/*******************************DECLARATIONS***********************************/
void signal_pipe();

CommandParameters command_line_arguments(int argc, char** argv);
void check_empty_string(char* arg);
void check_out_of_bounds(int num, int bound);
void check_boolean(bool boolean);
int convert_to_int(char* intString, int min, int max);
void command_line_error();

void check_port(CommandParameters params);
int open_listen(CommandParameters params);
void connection_error(CommandParameters params);

void print_port_num(int fdServer, CommandParameters params);
void process_connections(int fdServer, CommandParameters params);

void* client_thread(void* arg);
void create_signal_thread(Statistics* stats, pthread_mutex_t* statsMutex);
void create_client_thread(sem_t* semaphore, int fd, CommandParameters params,
        Statistics* stats, pthread_mutex_t* statsMutex);
void* signal_thread(void* arg);
void sighup_statistics(Statistics* stats);

bool check_initial_validity(HttpRequest request, FILE* to, ThreadArgs args);

bool valid_method(HttpRequest request);
void invalid_method_response(FILE* to);
bool valid_get(HttpRequest request);

void invalid_get_response(FILE* to);
void home_page_response(FILE* to);

bool valid_operation(HttpRequest request);
void free_check_operations(char*** op, char** addresses, char*** operations);
bool check_operation(char* name);
void invalid_image(ThreadArgs* args, Operation** operations,
        HttpRequest* request, FILE* to);
bool valid_operation_value(char* intString, int min, int max);
void invalid_operation_response(FILE* to);

bool valid_image_size(HttpRequest request);
void invalid_size_response(FILE* to, HttpRequest request);

void invalid_image_response(FILE* to);
Operation* get_operations(HttpRequest request);

bool process_operations(
        FIBITMAP** image, Operation* operations, FILE* to, ThreadArgs args);
void failed_operation_response(FILE* to, Operation op);

void process_success(HttpRequest* request, Operation** operations,
        ThreadArgs* args, FIBITMAP** image, FILE* to);
void success_response(FILE* to, unsigned char* data, unsigned long numBytes);
void end_client_thread(ThreadArgs* args, FILE* from, FILE* to);

void free_operations(Operation** operations);
void free_request(HttpRequest* request);

/******************************************************************************/

/**
 * main()
 * -------------
 *  Executes main functionality of program. Constructs signal handler for
 *  ignoring SIGPIPE
 *
 *  int argc: the number of command line arguments
 *  char** argv: the command line arguments
 *
 *  Returns: 0 on successful exit
 */
int main(int argc, char** argv)
{
    CommandParameters params = command_line_arguments(argc, argv);
    check_port(params);
    int fdServer = open_listen(params);
    print_port_num(fdServer, params);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_pipe;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sa, 0);
    process_connections(fdServer, params);
    return 0;
}

/**
 * signal_pipe()
 * --------------
 *  Upon receiving SIGPIPE, ignore
 */
void signal_pipe()
{
}

/**
 * command_line_arguments()
 * -----------------------------------
 *  A function that parses the command line arguments specified by the user,
 *  and throws errors if required
 *
 *  int argc: the number of command line argument parameters
 *  char** argv: an array of strings with the command line argument parameters
 *
 *  Returns: a struct CommandParameters storing all information regarding the
 *  command line parameters
 */
CommandParameters command_line_arguments(int argc, char** argv)
{
    CommandParameters params = {"0", false, 0, false};
    for (int i = 1; i < argc; i++) {
        // Check if any argument is the empty string
        check_empty_string(argv[i]);
        if (strcmp(argv[i], PORT) == 0) {
            check_boolean(params.portGiven);
            check_out_of_bounds(i + 1, argc);
            check_empty_string(argv[i + 1]);
            params.port = argv[i + 1];
            params.portGiven = true;
            i++;
        } else if (strcmp(argv[i], CONNECTIONS) == 0) {
            check_boolean(params.maxGiven);
            check_out_of_bounds(i + 1, argc);
            check_empty_string(argv[i + 1]);
            params.max = convert_to_int(
                    argv[i + 1], MIN_CONNECTIONS, MAX_CONNECTIONS);
            params.maxGiven = true;
            i++;
        } else {
            command_line_error();
        }
    }
    return params;
}

/**
 * check_empty_string()
 * ------------------------
 *  Checks if a given string is the empty string, and throws a command line
 *  error if it is
 *
 *  char* arg: the string to check
 */
void check_empty_string(char* arg)
{
    if (strcmp(arg, "") == 0) {
        command_line_error();
    }
}

/**
 * check_out_of_bounds()
 * ------------------------
 *  Checks if a number is out of the specified bound, and run the command
 *  line error function if so
 *
 *  int num: the num to check if out of bounds
 *  int bound: the bound
 */
void check_out_of_bounds(int num, int bound)
{
    if (num >= bound) {
        command_line_error();
    }
}

/**
 * check_boolean()
 * --------------------
 *  A simple helper function that checks if a boolean value is true or false,
 *  and if true, runs a command line error
 *
 *  bool boolean: the boolean to check if true
 */
void check_boolean(bool boolean)
{
    if (boolean) {
        command_line_error();
    }
}

/**
 * convert_to_int()
 * --------------------
 * A helper function that takes an integer as a string, and uses strtol() to
 * convert the string representation into an integer, throwing errors if the
 * integer does not meet the appropriate bounds
 *
 * char* intString: a string representation of the integer to convert
 * int min: the minimum bound for the int
 * int max: the maxmimum bound for the int
 */
int convert_to_int(char* intString, int min, int max)
{
    char* endPtr;
    long int converted = strtol(intString, &endPtr, BASE10);
    if ((*endPtr != '\0') || (converted < min) || (converted > max)) {
        command_line_error();
    }
    return (int)converted;
}

/**
 * command_line_error()
 * -----------------------
 *  When a problem is encountered processing the command line arguments, print
 *  the appropriate message to stderr and exit with the appropriate code
 */
void command_line_error()
{
    fprintf(stderr, "Usage: uqimageproc [--port port] [--max connections]\n");
    exit(COMMAND_LINE_ERROR);
}

/**
 * check_port()
 * ---------------
 *  Check if the supplied port number is valid
 *
 *  CommandParameters params: the struct storing information regarding command
 *  line arguments
 */
void check_port(CommandParameters params)
{
    char* endPtr;
    long int converted = strtol(params.port, &endPtr, BASE10);
    if ((converted < PORT_MIN || converted > PORT_MAX) && (converted != 0)) {
        connection_error(params);
    }
}

/**
 * open_listen()
 * ------------------
 *  Given a port number, attempts to create a socket and listen on the
 *  server
 *
 *  CommandParameters params: the struct storing information regarding command
 *  line arguments
 *
 *  Returns: a fd for the listening on the socket
 */
int open_listen(CommandParameters params)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(NULL, params.port, &hints, &ai);
    if (err) {
        freeaddrinfo(ai);
        connection_error(params);
    }
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        freeaddrinfo(ai);
        connection_error(params);
    }
    int optVal = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int))
            < 0) {
        freeaddrinfo(ai);
        connection_error(params);
    }
    if (bind(listenfd, ai->ai_addr, sizeof(struct sockaddr)) < 0) {
        freeaddrinfo(ai);
        connection_error(params);
    }
    if (listen(listenfd, params.max) < 0) {
        freeaddrinfo(ai);
        connection_error(params);
    }
    freeaddrinfo(ai);
    return listenfd;
}

/**
 * connection_error()
 * --------------------------
 *  If an error occurs while trying to listen on the port, throw an error and
 *  exit accordingly
 *
 *  CommandParameters params: the struct storing information regarding command
 *  line arguments
 */
void connection_error(CommandParameters params)
{
    fprintf(stderr, "uqimageproc: cannot listen on port \"%s\"\n", params.port);
    exit(FAILED_LISTEN);
}

/**
 * print_port_num()
 * ---------------------
 *  Prints the port number associated with the server (the one that clients
 *  will use to send data)
 *
 *  int fdServer: the fd for the listen port
 *  CommandParameters params: the struct storing information regarding command
 *  line arguments
 *
 *  REF:
 * https://stackoverflow.com/questions/4046616/sockets-how-to-find-out-what-port-and-address-im-assigned
 *  Assisted in discovering which functions to use
 */
void print_port_num(int fdServer, CommandParameters params)
{
    if (strcmp(params.port, "0")) {
        fprintf(stderr, "%s\n", params.port);
    } else {
        char host[NI_MAXHOST], port[NI_MAXSERV];
        struct sockaddr_in fromAddr;
        socklen_t fromAddrSize = sizeof(fromAddr);
        getsockname(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        getnameinfo((struct sockaddr*)&fromAddr, fromAddrSize, host, NI_MAXHOST,
                port, NI_MAXSERV, 0);
        fprintf(stderr, "%s\n", port);
    }
}

/**
 * process_connections()
 * ------------------------
 * Processes incoming connections
 *
 * int fdServer: the fd for the listen port
 *
 * REF: man page for pthread_mutex_unlock to learn how to use pthread_mutex_t
 */
void process_connections(int fdServer, CommandParameters params)
{
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    sem_t semaphore;
    if (params.maxGiven) {
        sem_init(&semaphore, 0, params.max);
    }
    Statistics stats = {0, 0, 0, 0, 0};
    pthread_mutex_t statsMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_unlock(&statsMutex);
    create_signal_thread(&stats, &statsMutex);
    // Repeatedly accept connections
    while (1) {
        if (params.maxGiven) {
            sem_wait(&semaphore);
        }
        fromAddrSize = sizeof(struct sockaddr_in);
        // Block, waiting for a new connection
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        if (fd < 0) {
            fprintf(stderr, "Error Accepting Connection\n");
            exit(1);
        }
        char hostName[NI_MAXHOST];
        getnameinfo((struct sockaddr*)&fromAddr, fromAddrSize, hostName,
                NI_MAXHOST, NULL, 0, 0);
        create_client_thread(&semaphore, fd, params, &stats, &statsMutex);
    }
}

/**
 * create_signal_thread()
 * -------------------------
 * Creates the signal thread for handling SIGHUP
 *
 * Statistics* stats: a pointer to the statistics structure
 * pthread_mutex_t* statsMutex: a pointer to the mutex for stats
 *
 * REF:https://pubs.opengroup.org/onlinepubs/009604599/functions/pthread_sigmask.html
 * Used for basis of SIGHUP handling
 */
void create_signal_thread(Statistics* stats, pthread_mutex_t* statsMutex)
{
    SignalArgs* arg = malloc(sizeof(SignalArgs));
    arg->stats = stats;
    arg->statsMutex = statsMutex;
    sigemptyset(&(arg->set));
    sigaddset(&(arg->set), SIGHUP);
    pthread_sigmask(SIG_BLOCK, &(arg->set), NULL);
    pthread_t sigThread;
    pthread_create(&sigThread, NULL, signal_thread, arg);
}

/**
 * create_client_thread()
 * --------------------------
 *  Creates the client thread for processing HTTP requests
 *
 *  sem_t* semaphore: the semaphore for limiting connections
 *  int fd: the file descriptor for getting the request
 *  CommandParameters params: the command line parameters struct
 *  Statistics* stats: a pointer to the statistics structure
 *  pthread_mutex_t* statsMutex: a pointer to the mutex for stats
 */
void create_client_thread(sem_t* semaphore, int fd, CommandParameters params,
        Statistics* stats, pthread_mutex_t* statsMutex)
{
    ThreadArgs* threads = malloc(sizeof(ThreadArgs));
    threads->semaphore = semaphore;
    threads->fd = fd;
    threads->maxGiven = params.maxGiven;
    threads->stats = stats;
    threads->statsMutex = statsMutex;
    pthread_t threadID;
    pthread_create(&threadID, NULL, client_thread, threads);
    pthread_detach(threadID);
}

/**
 * client_thread()
 * ------------------
 *  Processes the HTTP requests sent by a client
 *
 *  void* arg: a void pointer to the data sent to the thread
 *
 *  Returns: Null pointer
 */
void* client_thread(void* arg)
{
    ThreadArgs args = *(ThreadArgs*)arg;
    free(arg);
    // Update Connected Clients
    pthread_mutex_lock(args.statsMutex);
    args.stats->connected++;
    pthread_mutex_unlock(args.statsMutex);
    int fd2 = dup(args.fd);
    FILE* from = fdopen(args.fd, "r");
    FILE* to = fdopen(fd2, "w");
    while (1) {
        HttpRequest request;
        if (get_HTTP_request(from, &request.method, &request.address,
                    &request.headers, &request.body, &request.len)
                == 0) {
            break;
        }
        if (!check_initial_validity(request, to, args)) {
            free_request(&request);
            fflush(from);
            continue;
        }
        // Initial checks passed
        Operation* operations = get_operations(request);
        FIBITMAP* image = fi_load_image_from_buffer(request.body, request.len);
        if (image == NULL) {
            invalid_image(&args, &operations, &request, to);
            fflush(from);
            continue;
        }
        if (!process_operations(&image, operations, to, args)) {
            free_operations(&operations);
            free_request(&request);
            if (image != NULL) {
                FreeImage_Unload(image);
            }
            fflush(from);
            continue;
        }
        // Success
        process_success(&request, &operations, &args, &image, to);
        fflush(from);
    }
    end_client_thread(&args, from, to);
    return NULL;
}

/**
 * signal_thread()
 * -----------------
 *  The thread for dealing with SIGHUP signals and processing statistics
 *
 *  void* arg: a void pointer to the data for the thread
 *
 *  Returns: Null pointer
 */
void* signal_thread(void* arg)
{
    SignalArgs args = *(SignalArgs*)arg;
    free(arg);
    int sig;
    while (1) {
        sigwait(&(args.set), &sig);
        if (sig == SIGHUP) {
            pthread_mutex_lock(args.statsMutex);
            sighup_statistics(args.stats);
            pthread_mutex_unlock(args.statsMutex);
        }
    }
    return NULL;
}

/**
 * sighup_statistics()
 * ---------------------
 *  Prints all the statistics to stderr
 *
 *  Statistics* stats: a pointer to the Statistics structure
 */
void sighup_statistics(Statistics* stats)
{
    fprintf(stderr, "Connected clients: %u\n", stats->connected);
    fprintf(stderr, "Serviced clients: %u\n", stats->serviced);
    fprintf(stderr, "Successfully processed HTTP requests: %u\n",
            stats->success);
    fprintf(stderr, "Unsuccessful HTTP requests: %u\n", stats->unSuccess);
    fprintf(stderr, "Operations on images completed: %u\n", stats->operations);
}

/**
 * check_initial_validity()
 * --------------------------
 *  Checks the initial cases for validity of the request. These include
 *  1. Valid Method
 *  2. Valid Get Request
 *  3. Valid Operation
 *  4. Valid Image Size
 *
 *  If any of these fail, creates and sends the appropriate HTTP response, and
 *  returns false.
 *
 *  HttpRequest request: the request to check
 *  FILE* to: the fd for sending to the client
 *  ThreadArgs args: the thread arguments
 *
 *  Returns: true if no errors, false if an error occured
 *
 */
bool check_initial_validity(HttpRequest request, FILE* to, ThreadArgs args)
{
    if (!valid_method(request)) {
        invalid_method_response(to);
        pthread_mutex_lock(args.statsMutex);
        args.stats->unSuccess++;
        pthread_mutex_unlock(args.statsMutex);
        return false;
    }
    if (strcmp(request.method, GET) == 0) {
        if (!valid_get(request)) {
            invalid_get_response(to);
            pthread_mutex_lock(args.statsMutex);
            args.stats->unSuccess++;
            pthread_mutex_unlock(args.statsMutex);
            return false;
        }
        home_page_response(to);
        pthread_mutex_lock(args.statsMutex);
        args.stats->success++;
        pthread_mutex_unlock(args.statsMutex);
        return false;
    }
    if (!valid_operation(request)) {
        invalid_operation_response(to);
        pthread_mutex_lock(args.statsMutex);
        args.stats->unSuccess++;
        pthread_mutex_unlock(args.statsMutex);
        return false;
    }
    if (!valid_image_size(request)) {
        invalid_size_response(to, request);
        pthread_mutex_lock(args.statsMutex);
        args.stats->unSuccess++;
        pthread_mutex_unlock(args.statsMutex);
        return false;
    }
    return true;
}

/**
 * valid_method()
 * -------------------
 *  Check if the specified method is valid (GET or POST)
 *
 *  HttpRequest: the request data
 *
 *  Returns: true if POST or GET, else false
 */
bool valid_method(HttpRequest request)
{
    if (strcmp(request.method, GET) == 0) {
        return true;
    }
    if (strcmp(request.method, POST) == 0) {
        return true;
    }
    return false;
}

/**
 * invalid_method_response()
 * --------------------------
 *  If the specified method is not POST or GET, constructs the response to
 *  send back to the client appropriately
 *
 *  FILE* to: the file descriptor for sending to the client
 */
void invalid_method_response(FILE* to)
{
    HttpResponse response;
    // Status
    response.status = METHOD_NOT_ALLOWED;
    response.statusExplanation = "Method Not Allowed";
    // Body
    response.body = (const unsigned char*)"Invalid method on request list\n";
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/plain";
    response.headers[1]->name = "Content-Length";
    int length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * valid_get()
 * --------------
 *  Checks if the GET request is valid
 *
 *  HttpRequest: the request
 *
 *  Returns: true if valid, false else
 */
bool valid_get(HttpRequest request)
{
    if (strcmp(request.address, "/") == 0) {
        return true;
    }
    return false;
}

/**
 * invalid_get_response()
 * ------------------------
 *  If the format of the GET response was invalid, create and send the
 *  appropriate response
 *
 *  FILE* to: the file descriptor for sending to the client
 */
void invalid_get_response(FILE* to)
{
    HttpResponse response;
    // Status
    response.status = NOT_FOUND;
    response.statusExplanation = "Not Found";
    // Body
    response.body = (const unsigned char*)"Invalid address in GET request\n";
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/plain";
    response.headers[1]->name = "Content-Length";
    int length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * home_page_response()
 * -----------------------
 *  Upon receiving a valid GET request, will construct the appropriate response
 *  and send it back to the client
 *
 *  FILE* to: a file descriptor to send information to the client
 */
void home_page_response(FILE* to)
{
    HttpResponse response;
    // Status
    response.status = OK;
    response.statusExplanation = "OK";
    // Body
    FILE* htmlFile;
    htmlFile = fopen(HTML_PATH, "rb");
    fseek(htmlFile, 0, SEEK_END);
    long fileSize = ftell(htmlFile);
    fseek(htmlFile, 0, SEEK_SET);
    unsigned char* buffer = malloc(sizeof(unsigned char) * (fileSize + 1));
    fread(buffer, 1, fileSize, htmlFile);
    buffer[fileSize] = '\0';
    fclose(htmlFile);
    response.body = buffer;
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/html";
    response.headers[1]->name = "Content-Length";
    int length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(buffer);
    free(response.headers[1]->value);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * valid_operation()
 * --------------------
 *  Check if the supplied operation list is valid
 *
 *  HttpRequest request: the request
 *
 *  Returns: true if valid, false if not
 */
bool valid_operation(HttpRequest request)
{
    char* addresses = strdup(request.address);
    char** operations = split_by_char(addresses, '/', 0);
    int i = 1;
    while (operations[i] != NULL) {
        char** op = split_by_char(operations[i], ',', 0);
        // Check it is a supplied operation
        if (!check_operation(op[0])) {
            free_check_operations(&op, &addresses, &operations);
            return false;
        }
        if (strcmp(op[0], ROTATE) == 0) {
            if (((op[1] == NULL)
                        || (!valid_operation_value(
                                op[1], ROTATE_MIN, ROTATE_MAX)))
                    || (op[2] != NULL)) {
                free_check_operations(&op, &addresses, &operations);
                return false;
            }
        } else if (strcmp(op[0], FLIP) == 0) {
            if ((op[1] == NULL)
                    || ((strcmp(op[1], VERTICAL) != 0)
                            && (strcmp(op[1], HORIZONTAL) != 0))
                    || (op[2] != NULL)) {
                free_check_operations(&op, &addresses, &operations);
                return false;
            }
        } else if (strcmp(op[0], SCALE) == 0) {
            if ((op[1] == NULL)
                    || !valid_operation_value(op[1], SCALE_MIN, SCALE_MAX)) {
                free_check_operations(&op, &addresses, &operations);
                return false;
            }
            if ((op[2] == NULL)
                    || !valid_operation_value(op[2], SCALE_MIN, SCALE_MAX)) {
                free_check_operations(&op, &addresses, &operations);
                return false;
            }
            if (op[LAST_SCALE_ARGUMENT] != NULL) {
                free_check_operations(&op, &addresses, &operations);
                return false;
            }
        }
        free(op);
        i++;
    }
    free(addresses);
    free(operations);
    return true;
}

/**
 * free_check_operations()
 * -------------------------
 *  Frees the memory associated with the variables needed in checking
 *  operations
 *
 *  char*** op: a pointer to the array of values of the operation
 *  char** addresses: a pointer to the string representing the address
 *  char*** operations: a pointer to the array of operations
 */
void free_check_operations(char*** op, char** addresses, char*** operations)
{
    free(*op);
    free(*addresses);
    free(*operations);
}

/**
 * check_operation()
 * -------------------
 *  A helper function to check if an operation given is one of the
 *  available ones
 *
 *  char* name: operation name
 *
 *  Returns: true if valid, false else
 */
bool check_operation(char* name)
{
    if ((strcmp(name, ROTATE) == 0) || (strcmp(name, FLIP) == 0)
            || (strcmp(name, SCALE) == 0)) {
        return true;
    }
    return false;
}

/**
 * valid_operation_value()
 * --------------------------
 *  Checks if the specified value is valid, i.e. an integer and within
 *  the bounds
 *
 *  char* intString: the integer as a string
 *  int min: the minimum bound
 *  int max: the maxmimum bound
 *
 *  Returns: true if its valid, false else
 */
bool valid_operation_value(char* intString, int min, int max)
{
    if ((intString == NULL)) {
        return false;
    }
    char* endPtr;
    long int converted = strtol(intString, &endPtr, BASE10);
    if ((*endPtr != '\0') || (converted < min) || (converted > max)) {
        return false;
    }
    return true;
}

/**
 * invalid_operation_response()
 * -------------------------------
 *  If the supplied operation list is invalid, create the appropriate HTML
 *  response and send it
 *
 *  FILE* to: the fd for sending to the client
 */
void invalid_operation_response(FILE* to)
{
    HttpResponse response;
    // Status
    response.status = BAD_REQUEST;
    response.statusExplanation = "Bad Request";
    // Body
    response.body = (const unsigned char*)"Invalid image operation\n";
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/plain";
    response.headers[1]->name = "Content-Length";
    int length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * valid_image_size()
 * ---------------------
 *  Check if an input image is less than 8Mib, the maxmimum
 *
 *  HttpRequest request: the request to check
 *
 *  Returns: true if smaller than 8Mib, false otherwise
 */
bool valid_image_size(HttpRequest request)
{
    if (request.len > EIGHT_MIB) {
        return false;
    }
    return true;
}

/**
 * invalid_size_response()
 * -------------------------
 *  If the image is too large, construct the appropriate response to send to
 *  the client
 *
 *  FILE* to: the fd for sending the response to
 *  HttpRequest request: the request to get the image size
 */
void invalid_size_response(FILE* to, HttpRequest request)
{
    HttpResponse response;
    // Status
    response.status = PAYLOAD_TOO_LARGE;
    response.statusExplanation = "Payload Too Large";
    // Body
    int length
            = snprintf(NULL, 0, "Image is too large: %ld bytes\n", request.len);
    // Allocate memory for the unsigned char* destination
    response.body = malloc(sizeof(unsigned char) * (length + 1));
    snprintf((char*)response.body, length + 1,
            "Image is too large: %ld bytes\n", request.len);
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/plain";
    response.headers[1]->name = "Content-Length";
    length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    free((char*)response.body);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * invalid_image_response()
 * ---------------------------
 *  If an error occurs reading in the image, send this response to the client
 *
 *  FILE* to: the fd for sending the response to the client
 */
void invalid_image_response(FILE* to)
{
    HttpResponse response;
    // Status
    response.status = UNPROCESSABLE_CONTENT;
    response.statusExplanation = "Unprocessable Content";
    // Body
    response.body = (const unsigned char*)"Invalid image received\n";
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/plain";
    response.headers[1]->name = "Content-Length";
    int length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * get_operations()
 * ------------------
 *  Only runs once this has been confirmed to be a valid set of operations.
 *  Gets all operations from the HTTP request and stores them in an array
 *  of Operation structs.
 *
 *  HttpRequest request: the HTTP request to process the operations from
 */
Operation* get_operations(HttpRequest request)
{
    Operation* operations = malloc(sizeof(Operation));
    char** addresses = split_by_char(request.address, '/', 0);
    int i = 0;
    while (addresses[i + 1] != NULL) {
        char** op = split_by_char(addresses[i + 1], ',', 0);
        operations = realloc(operations, sizeof(Operation) * (i + 1));
        if (strcmp(op[0], ROTATE) == 0) {
            operations[i].operation = strdup(ROTATE);
            operations[i].value1
                    = convert_to_int(op[1], ROTATE_MIN, ROTATE_MAX);
            operations[i].value2 = 0;
            operations[i].direction = NULL;
        } else if (strcmp(op[0], FLIP) == 0) {
            operations[i].operation = strdup(FLIP);
            operations[i].value1 = 0;
            operations[i].value2 = 0;
            operations[i].direction = strdup(op[1]);
        } else if (strcmp(op[0], SCALE) == 0) {
            operations[i].operation = strdup(SCALE);
            operations[i].value1 = convert_to_int(op[1], SCALE_MIN, SCALE_MAX);
            operations[i].value2 = convert_to_int(op[2], SCALE_MIN, SCALE_MAX);
            operations[i].direction = NULL;
        }
        i++;
        free(op);
    }
    operations = realloc(operations, sizeof(Operation) * (i + 1));
    operations[i].operation = NULL;
    operations[i].value1 = 0;
    operations[i].value2 = 0;
    operations[i].direction = NULL;
    free(addresses);
    return operations;
}

/**
 * invalid_image()
 * ------------------
 *  Processes an invalid image and updates stats accordingly, freeing memory
 *  accordingly
 *
 *  ThreadArgs* args: a pointer to the thread arguments
 *  Operation** operations: a pointer to the list of operations
 *  HttpRequest* request: a pointer to the HTTP request
 *  FILE* to: the file descriptor for sending the response through to
 */
void invalid_image(ThreadArgs* args, Operation** operations,
        HttpRequest* request, FILE* to)
{
    free_operations(operations);
    free_request(request);
    invalid_image_response(to);
    pthread_mutex_lock(args->statsMutex);
    args->stats->unSuccess++;
    pthread_mutex_unlock(args->statsMutex);
}

/**
 * process_operations()
 * -------------------------
 *  Processes the operations specified on the image. If an error occurs,
 *  creates the appropriate response and sends it to the client and returns
 *  false.
 *
 *  FIBITMAP** image: a pointer to the image data to manipulate
 *  Operation* operations: the array of operations to perform
 *  FILE* to: the FILE* for sending data to the client if needed
 *  ThreadArgs args: the thread arguments
 *
 *  Returns: true if all were successful, false if any failed
 */
bool process_operations(
        FIBITMAP** image, Operation* operations, FILE* to, ThreadArgs args)
{
    // Process Operations
    for (int i = 0; operations[i].operation != NULL; i++) {
        if (strcmp(operations[i].operation, ROTATE) == 0) {
            *image = FreeImage_Rotate(
                    *image, (double)operations[i].value1, NULL);
        } else if (strcmp(operations[i].operation, FLIP) == 0) {
            if (strcmp(operations[i].direction, VERTICAL) == 0) {
                if (FreeImage_FlipVertical(*image) == 0) {
                    failed_operation_response(to, operations[i]);
                    return false;
                }
            } else if (FreeImage_FlipHorizontal(*image) == 0) {
                failed_operation_response(to, operations[i]);
                return false;
            }
        } else if (strcmp(operations[i].operation, SCALE) == 0) {
            *image = FreeImage_Rescale(*image, operations[i].value1,
                    operations[i].value2, FILTER_BILINEAR);
        }
        // Check if operation failed
        if (*image == NULL) {
            failed_operation_response(to, operations[i]);
            return false;
        }
        pthread_mutex_lock(args.statsMutex);
        args.stats->operations++;
        pthread_mutex_unlock(args.statsMutex);
    }
    return true;
}

/**
 * failed_operation_response()
 * ------------------------------
 *  If an operation failed, constructs and sends the appropriate response to
 *  the client.
 *
 *  FILE* to: the fd for sending data to the client
 *  Operation op: the operation that failed
 */
void failed_operation_response(FILE* to, Operation op)
{
    HttpResponse response;
    // Status
    response.status = FAILED_OPERATION;
    response.statusExplanation = "Not Implemented";
    // Body
    int length = snprintf(
            NULL, 0, "Operation did not complete: %s\n", op.operation);
    // Allocate memory for the unsigned char* destination
    response.body = malloc(sizeof(unsigned char) * (length + 1));
    snprintf((char*)response.body, length + 1,
            "Operation did not complete: %s\n", op.operation);
    response.bodySize = strlen((const char*)response.body);
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "text/plain";
    response.headers[1]->name = "Content-Length";
    length = snprintf(NULL, 0, "%ld", response.bodySize);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", response.bodySize);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, response.body,
            response.bodySize, &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    free((char*)response.body);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * process_success()
 * ----------------------
 *  Upon success, process the results accordingly and free memory when needed
 *
 *  HttpRequest* request: a pointer to the http request
 *  Operation** operations: a pointer to the array of operations
 *  ThreadArgs* args: a pointer to the thread arguments
 *  FIBITMAT** image: a pointer to the image
 *  FILE* to: the file descriptor for sending the success response
 */
void process_success(HttpRequest* request, Operation** operations,
        ThreadArgs* args, FIBITMAP** image, FILE* to)
{
    free_request(request);
    free_operations(operations);
    unsigned long numBytes;
    unsigned char* data = fi_save_png_image_to_buffer(*image, &numBytes);
    success_response(to, data, numBytes);
    free(data);
    FreeImage_Unload(*image);
    pthread_mutex_lock(args->statsMutex);
    args->stats->success++;
    pthread_mutex_unlock(args->statsMutex);
}

/**
 * success_response()
 * ----------------------
 *  If the server was successful in processing all operations on the image,
 *  sends a response back to the client with the image data and the
 *  appropriate request
 *
 *  FILE* to: the fd for sending data to the client
 *  unsigned char* data: the raw binary image data
 *  usigned long numBytes: the number of bytes in the image data
 */
void success_response(FILE* to, unsigned char* data, unsigned long numBytes)
{
    HttpResponse response;
    // Status
    response.status = OK;
    response.statusExplanation = "OK";
    // Construct Headers
    int numHeaders = 2;
    response.headers = malloc((numHeaders + 1) * sizeof(HttpHeader*));
    for (int i = 0; i < numHeaders; i++) {
        response.headers[i] = malloc(sizeof(HttpHeader));
    }
    response.headers[0]->name = "Content-Type";
    response.headers[0]->value = "image/png";
    response.headers[1]->name = "Content-Length";
    int length = snprintf(NULL, 0, "%ld", numBytes);
    response.headers[1]->value = malloc(sizeof(char) * (length + 1));
    snprintf(response.headers[1]->value, length + 1, "%ld", numBytes);
    response.headers[2] = NULL;
    unsigned char* message = construct_HTTP_response(response.status,
            response.statusExplanation, response.headers, data, numBytes,
            &response.len);
    fwrite(message, sizeof(unsigned char), response.len, to);
    fflush(to);
    free(response.headers[1]->value);
    for (int i = 0; i < numHeaders; i++) {
        free(response.headers[i]);
    }
    free(response.headers);
    free(message);
}

/**
 * end_client_thread()
 * ---------------------
 *  Upon exiting the client thread, close the descriptors and update the
 *  statistics accordingly
 *
 *  ThreadArgs* args: a pointer to the thread arguments
 *  FILE* from: the fd for reading the requests
 *  FILE* to: the fd for sending the requests
 */
void end_client_thread(ThreadArgs* args, FILE* from, FILE* to)
{
    fclose(from);
    fclose(to);
    pthread_mutex_lock(args->statsMutex);
    args->stats->connected--;
    args->stats->serviced++;
    pthread_mutex_unlock(args->statsMutex);
    if (args->maxGiven) {
        sem_post(args->semaphore);
    }
}

/**
 * free_operations()
 * -------------------
 *  Frees the memory associated with the Operation array
 *
 *  Operation** operations: a pointer to the array of operations to free
 */
void free_operations(Operation** operations)
{
    for (int i = 0; (*operations)[i].operation != NULL; i++) {
        free((*operations)[i].operation);
        free((*operations)[i].direction);
    }
    free(*operations);
}

/**
 * free_request()
 * -----------------
 *  Frees the memory associated with the HttpRequest
 *
 *  HttpRequest* request: A pointer to the HTTP request to free the memory of
 */
void free_request(HttpRequest* request)
{
    free(request->method);
    free(request->address);
    free(request->body);
    free_array_of_headers(request->headers);
}
