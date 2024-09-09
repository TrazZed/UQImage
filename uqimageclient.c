#include <stdlib.h>
#include <stdio.h>
#include <csse2310a4.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 1024
#define INPUT "--input"
#define OUTPUT "--output"
#define ROTATE "--rotate"
#define MIN_ROTATE (-359)
#define MAX_ROTATE (359)
#define FLIP "--flip"
#define SCALE "--scale"
#define MIN_SCALE 1
#define MAX_SCALE 10000
#define HORIZONTAL_FLIP "h"
#define VERTICAL_FLIP "v"
#define COMMAND_LINE_ERROR 7
#define BASE10 10
#define HTTP_OK 200
#define NO_REDIRECTION (-5)
#define INPUT_FAIL 8
#define OUTPUT_FAIL 2
#define FAILED_PORT 19
#define EMPTY_IMAGE 17
#define BAD_HTTP_RESPONSE 4
#define WRITE_FAIL 5
#define CONNECTION_CLOSED 15

/**
 * A struct storing information regarding command line parameters
 */
typedef struct {
    const char* portNo; // String to represent port number or name
    bool inputFile; // Bool to represent if user specified input file
    char* inputName; // String of input file name
    bool rotate; // Bool to represent if user specified rotate
    int angle; // Angle of rotation
    bool scale; // Bool to represent if user specified scale
    int widthScale; // Image width scaling
    int heightScale; // Image height scaling
    bool flip; // Bool to represent if user specified flip
    char direction; // Horizontal or Vertical
    bool outputFile; // Bool to represent if user specified output file
    char* outputName; // Name of output file

} CommandParameters;

/**
 * A struct storing basic information about the image being read from input or
 * stdin
 */
typedef struct {
    unsigned char* imageData; // The binary data for the image
    unsigned long size; // The size of the binary data
} ImageData;

/*******************************DECLARATIONS***********************************/
CommandParameters command_line_arguments(int argc, char** argv);
void rotate_check(CommandParameters* params, int argc, char** argv, int i,
        bool operationGiven);
void flip_check(CommandParameters* params, int argc, char** argv, int i,
        bool operationGiven);
void scale_check(CommandParameters* params, int argc, char** argv, int i,
        bool operationGiven);
void check_empty_string(char* arg);
void cmd_line_check_port(char** argv);
void check_out_of_bounds(int num, int bound);
void check_boolean(bool boolean);
void set_string(char* argument, char** location, bool* stringBoolean);
int convert_to_int(char* intString, int min, int max);
void check_direction(CommandParameters* params, char* argument);
void command_line_error();

FILE* connect_to_server(CommandParameters params);
void redirection(CommandParameters params);
int redirect_input(CommandParameters params);
int redirect_output(CommandParameters params);
void failed_connection(CommandParameters params);
void no_data_error();
ImageData construct_image_data();
unsigned char* construct_http_request(
        CommandParameters params, ImageData body, int* size);
void construct_http_request_type(
        CommandParameters params, unsigned char** httpRequest, int* textSize);
void construct_http_headers(
        ImageData body, unsigned char** httpRequest, int* textSize);
void construct_http_body(
        ImageData body, unsigned char** httpRequest, int* textSize);

void process_http_response(FILE* from);

void free_command_parameters(CommandParameters* params);

/******************************************************************************/

/**
 * main()
 * ------------
 *  Executes the program
 *
 *  int argc: the number of command line parameters
 *  char** argv: the command line parameters
 *
 *  Returns: 0 on successful exit of program
 */
int main(int argc, char** argv)
{
    CommandParameters params = command_line_arguments(argc, argv);
    FILE* from = connect_to_server(params);
    process_http_response(from);
    free_command_parameters(&params);
    return 0;
}

/**
 * command_line_arguments()
 * ----------------------------
 *  Parses the command line arguments and store the values in an associated
 *  CommandParameters struct
 *
 *  int argc: the number of parameters in the command line arguments
 *  char** argv: an array of strings storing necessary command line argument
 *  information
 *
 *  Returns: a CommandParameters struct storing all required information
 *  specified by the user in the command line
 */
CommandParameters command_line_arguments(int argc, char** argv)
{
    bool operationGiven = false;
    CommandParameters params = {argv[1], false, NULL, false, 0, false, 0, 0,
            false, ' ', false, NULL};
    if (argc == 1) {
        command_line_error();
    }
    for (int i = 1; i < argc; i++) {
        // First input is port number
        if (i == 1) {
            cmd_line_check_port(argv);
        }
        // Check if any argument is the empty string
        else if (strcmp("", argv[i]) == 0) {
            command_line_error();
        } else if (strcmp(INPUT, argv[i]) == 0) {
            check_boolean(params.inputFile);
            check_out_of_bounds(i + 1, argc);
            set_string(argv[i + 1], &params.inputName, &params.inputFile);
            i++;
        } else if (strcmp(OUTPUT, argv[i]) == 0) {
            check_boolean(params.outputFile);
            check_out_of_bounds(i + 1, argc);
            set_string(argv[i + 1], &params.outputName, &params.outputFile);
            i++;
        } else if (strcmp(ROTATE, argv[i]) == 0) {
            rotate_check(&params, argc, argv, i, operationGiven);
            operationGiven = true;
            i++;
        } else if (strcmp(FLIP, argv[i]) == 0) {
            flip_check(&params, argc, argv, i, operationGiven);
            operationGiven = true;
            i++;
        } else if (strcmp(SCALE, argv[i]) == 0) {
            scale_check(&params, argc, argv, i, operationGiven);
            operationGiven = true;
            i += 2;
        } else {
            command_line_error();
        }
    }
    return params;
}

/**
 * rotate_check()
 * ----------------------
 *  Checks the specified rotate argument in the command line and check if it
 *  is valid, throwing an error if not
 *
 *  CommandParameters* params: a pointer to the parameters structure
 *  int argc: the number of command line arguments
 *  char** argv: the array of command line arguments
 *  int i: the command line argument number being checked
 *  bool operationGiven: true if an operation has already been specified
 */
void rotate_check(CommandParameters* params, int argc, char** argv, int i,
        bool operationGiven)
{
    check_boolean(operationGiven);
    check_out_of_bounds(i + 1, argc);
    check_empty_string(argv[i + 1]);
    params->angle = convert_to_int(argv[i + 1], MIN_ROTATE, MAX_ROTATE);
    params->rotate = true;
}

/**
 * flip_check()
 * ----------------------
 *  Checks the specified flip argument in the command line and check if it
 *  is valid, throwing an error if not
 *
 *  CommandParameters* params: a pointer to the parameters structure
 *  int argc: the number of command line arguments
 *  char** argv: the array of command line arguments
 *  int i: the command line argument number being checked
 *  bool operationGiven: true if an operation has already been specified
 */
void flip_check(CommandParameters* params, int argc, char** argv, int i,
        bool operationGiven)
{
    check_boolean(operationGiven);
    check_out_of_bounds(i + 1, argc);
    check_empty_string(argv[i + 1]);
    check_direction(params, argv[i + 1]);
    params->flip = true;
}

/**
 * scale_check()
 * ----------------------
 *  Checks the specified scale argument in the command line and check if it
 *  is valid, throwing an error if not
 *
 *  CommandParameters* params: a pointer to the parameters structure
 *  int argc: the number of command line arguments
 *  char** argv: the array of command line arguments
 *  int i: the command line argument number being checked
 *  bool operationGiven: true if an operation has already been specified
 */
void scale_check(CommandParameters* params, int argc, char** argv, int i,
        bool operationGiven)
{
    check_boolean(operationGiven);
    check_out_of_bounds(i + 2, argc);
    check_empty_string(argv[i + 1]);
    check_empty_string(argv[i + 2]);
    params->widthScale = convert_to_int(argv[i + 1], MIN_SCALE, MAX_SCALE);
    params->heightScale = convert_to_int(argv[i + 2], MIN_SCALE, MAX_SCALE);
    params->scale = true;
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
 * cmd_line_check_port()
 * ---------------------------
 *  A helper function that checks if the first input on the command line
 *  (the port) is valid, i.e. is not an optional argument specified
 *
 *  char** argv: the array of strings of command line arguments
 */
void cmd_line_check_port(char** argv)
{
    int i = 1;
    // Ensure it is not an optional parameter
    if (!strcmp(argv[i], INPUT) || !strcmp(argv[i], OUTPUT)
            || !strcmp(argv[i], ROTATE) || !strcmp(argv[i], FLIP)
            || !strcmp(argv[i], SCALE) || strcmp(argv[i], "") == 0) {
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
 * set_string()
 * ----------------
 *  A helper function that assigns an argument to a string location, and
 *  sets a boolean flag
 *
 *  char* argument: the string to be copied
 *  char** location: a pointer to a string to copy the argument to
 *  bool* stringBoolean: a pointer to a boolean that represents if the string
 *  has been modified
 */
void set_string(char* argument, char** location, bool* stringBoolean)
{
    if (strcmp(argument, "") == 0) {
        command_line_error();
    }
    *location = malloc(sizeof(char) * (strlen(argument) + 1));
    strcpy(*location, argument);
    *stringBoolean = true;
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
 * check_direction()
 * ---------------------
 *  A helper function that checks if the direction that the user specified
 *  is valid (either 'v' or 'h'), and assign the value if so, else throw
 *  an error
 *
 *  CommandParameters* params: a pointer to a structure storing information
 *  regarding command line arguments
 *  char* argument: a string of the argument to check validity of
 */
void check_direction(CommandParameters* params, char* argument)
{
    if (strcmp(argument, HORIZONTAL_FLIP) == 0) {
        params->direction = 'h';
    } else if (strcmp(argument, VERTICAL_FLIP) == 0) {
        params->direction = 'v';
    } else {
        command_line_error();
    }
}

/**
 * command_line_error()
 * ----------------------
 * If the user specified command line arguments of an invalid form, print out
 * the appropriate error message to stderr and exit with the appropriate exit
 * code
 */
void command_line_error()
{
    fprintf(stderr,
            "Usage: uqimageclient portno [--input infile] [--rotate angle | "
            "--scale width height | --flip direction] [--output "
            "outputfilename]\n");
    exit(COMMAND_LINE_ERROR);
}

/**
 * connect_to_server()
 * ------------------------
 *  Attempts to connect to the server provided by the user in the command
 *  line arguments, and deals with errors appropriately. If connection is
 *  successful, constructs a http request and sends it to the server, as well
 *  as returning a FILE* for the response from the server
 *
 *  CommandParameters params: the struct storing information about the
 *  command line parameters
 *
 *  Returns: a FILE* to the server response
 */
FILE* connect_to_server(CommandParameters params)
{
    redirection(params);
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err = getaddrinfo("localhost", params.portNo, &hints, &ai);
    if (err) {
        failed_connection(params);
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, ai->ai_addr, sizeof(struct sockaddr))) {
        failed_connection(params);
    }
    FILE* from = fdopen(fd, "r");
    ImageData body = construct_image_data(params);
    int size = 0;
    unsigned char* httpRequest = construct_http_request(params, body, &size);
    send(fd, httpRequest, size, 0);
    free(httpRequest);
    free(body.imageData);
    freeaddrinfo(ai);
    return from;
}

/**
 * redirection()
 * ----------------
 *  Performs stdin and stdout redirection if necessary
 *
 *  CommandParametrs params: the struct storing information about the arguments
 *  passed in the command line
 */
void redirection(CommandParameters params)
{
    int inputfd = redirect_input(params);
    int outputfd = redirect_output(params);
    if (inputfd != NO_REDIRECTION) {
        dup2(inputfd, STDIN_FILENO);
        close(inputfd);
    }
    if (outputfd != NO_REDIRECTION) {
        dup2(outputfd, STDOUT_FILENO);
        close(outputfd);
    }
}

/**
 * redirect_input()
 * -----------------
 *  If the user specified an input redirection, open the file and return the
 *  fd, if an error, exit appropriately
 *
 *  CommandParameters params: the struct storing information regarding the
 *  command line parameters
 */
int redirect_input(CommandParameters params)
{
    if (params.inputFile) {
        int input = open(params.inputName, O_RDONLY);
        if (input < 0) {
            fprintf(stderr,
                    "uqimageclient: unable to open file \"%s\" for reading\n",
                    params.inputName);
            exit(INPUT_FAIL);
        }
        return input;
    }
    return NO_REDIRECTION;
}

/**
 * redirect_output()
 * --------------------
 *  If the user specified an output redirection, open the file and return the
 *  fd, if an error, exit appropriately
 *
 *  CommandParameters params: the struct storing information regarding the
 *  command line parameters
 */
int redirect_output(CommandParameters params)
{
    if (params.outputFile) {
        int output = open(
                params.outputName, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        if (output < 0) {
            fprintf(stderr,
                    "uqimageclient: unable to open file \"%s\" for writing\n",
                    params.outputName);
            exit(OUTPUT_FAIL);
        }
        return output;
    }
    return NO_REDIRECTION;
}

/**
 * construct_image_data()
 * ---------------------------
 *  Given the stdin file descriptor for redirection if needed, and the struct
 *  storing Command Parameter information, construct the body request for
 *  the HTTP request, storing the binary data and size
 *
 *  CommandParameters params: the Command Line parameters parsed
 *
 *  Returns: A struct ImageData containing information about the image data
 *  and the size
 */
ImageData construct_image_data()
{
    ImageData body = {NULL, 0};
    unsigned char buffer[BUFFER_SIZE];
    int numRead;
    body.imageData = malloc(sizeof(unsigned char));
    memset(body.imageData, 0, sizeof(unsigned char));
    while ((numRead = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        body.imageData = realloc(
                body.imageData, sizeof(unsigned char) * (body.size + numRead));
        memcpy(body.imageData + body.size, buffer, numRead);
        body.size += numRead;
    }
    if (body.size == 0) {
        no_data_error();
    }
    return body;
}

/**
 * construct_http_request()
 * ----------------------------
 *  Given the command parameters information and data from the image, construct
 *  a HTTP request to send to the server client
 *
 *  CommandParameters params: the struct containing information about the
 *  command line parameter arguments specified
 *  ImageData body: the struct storing information about the image
 *  int* size: pointer to integer representing the request size
 *
 *  Returns: a string of the HTTP request to send to the server
 */
unsigned char* construct_http_request(
        CommandParameters params, ImageData body, int* size)
{
    unsigned char* httpRequest = malloc(sizeof(unsigned char));
    memset(httpRequest, 0, sizeof(unsigned char));
    construct_http_request_type(params, &httpRequest, size);
    construct_http_headers(body, &httpRequest, size);
    construct_http_body(body, &httpRequest, size);
    return httpRequest;
}

/**
 * construct_http_request_type()
 * ----------------------------------
 *  Constructs the request type for the http request based on the parameters
 *  passed by the user
 *
 *  CommandParameters* params: the struct storing command line parameters
 *  information
 *  unsigned char** httpRequest: a pointer to a string that is the construction
 * of the http request int* textSize: pointer to integer representing the
 * request size
 */
void construct_http_request_type(
        CommandParameters params, unsigned char** httpRequest, int* textSize)
{
    int size;
    char* requestType;
    if (params.rotate) {
        size = snprintf(NULL, 0, "POST /rotate,%d HTTP/1.1\r\n", params.angle)
                + 1;
        requestType = malloc(sizeof(char) * size);
        sprintf(requestType, "POST /rotate,%d HTTP/1.1\r\n", params.angle);
    } else if (params.scale) {
        size = snprintf(NULL, 0, "POST /scale,%d,%d HTTP/1.1\r\n",
                       params.widthScale, params.heightScale)
                + 1;
        requestType = malloc(sizeof(char) * size);
        sprintf(requestType, "POST /scale,%d,%d HTTP/1.1\r\n",
                params.widthScale, params.heightScale);
    } else if (params.flip) {
        size = snprintf(NULL, 0, "POST /flip,%c HTTP/1.1\r\n", params.direction)
                + 1;
        requestType = malloc(sizeof(char) * size);
        sprintf(requestType, "POST /flip,%c HTTP/1.1\r\n", params.direction);
    } else {
        // Default case, rotate by 0
        size = snprintf(NULL, 0, "POST /rotate,%d HTTP/1.1\r\n", 0) + 1;
        requestType = malloc(sizeof(char) * size);
        sprintf(requestType, "POST /rotate,%d HTTP/1.1\r\n", 0);
    }
    *httpRequest = realloc(*httpRequest,
            sizeof(unsigned char)
                    * (strlen((char*)*httpRequest) + strlen(requestType) + 1));
    strcat((char*)*httpRequest, requestType);
    free(requestType);
    *textSize += size;
}

/**
 * construct_http_headers()
 * -----------------------------
 *  Constructs the header part of the HTTP request with the Content-Length
 *  size of the image to be sent over
 *
 *  ImageData body: a struct containing information about the image to be sent,
 *  including the size
 *  unsigned char** httpRequest: a pointer to the string detailing the HTTP
 * request being constructed int* textSize: pointer to integer representing the
 * request size
 */
void construct_http_headers(
        ImageData body, unsigned char** httpRequest, int* textSize)
{
    int size = snprintf(NULL, 0, "Content-Length: %lu\r\n", body.size) + 1;
    char* requestLength = malloc(sizeof(char) * size);
    sprintf(requestLength, "Content-Length: %lu\r\n", body.size);
    *httpRequest = realloc(*httpRequest,
            sizeof(unsigned char)
                    * (strlen((char*)*httpRequest) + strlen(requestLength)
                            + 1));
    strcat((char*)*httpRequest, requestLength);
    free(requestLength);
    *textSize += size;
}

/**
 * construct_http_body()
 * ------------------------
 *  Constructs the body of the HTTP request
 *
 *  ImageData body: the struct storing information about the image, including
 *  the binary data
 *  unsigned char** htppRequest: a pointer to the string detailing the HTTP
 * request being constructed int* textSize: pointer to integer representing the
 * request size
 */
void construct_http_body(
        ImageData body, unsigned char** httpRequest, int* textSize)
{
    char* endOfHeaders = "\r\n";
    *httpRequest = realloc(*httpRequest,
            sizeof(unsigned char)
                    * (strlen((char*)*httpRequest) + strlen(endOfHeaders) + 1));
    strcat((char*)*httpRequest, endOfHeaders);
    *httpRequest = realloc(*httpRequest,
            sizeof(unsigned char)
                    * (strlen((char*)*httpRequest) + body.size + 1));
    memcpy(*httpRequest + strlen((char*)*httpRequest), body.imageData,
            body.size);
    *textSize += body.size;
}

/**
 * failed_connection()
 * ---------------------
 *  A helper function that will print a message to stderr and exit the program
 *  with the required code if any error occurs while connecting to the server
 */
void failed_connection(CommandParameters params)
{
    fprintf(stderr, "uqimageclient: unable to connect to port \"%s\"\n",
            params.portNo);
    exit(FAILED_PORT);
}

/**
 * no_data_error()
 * -------------------
 *  If when procesing an image, there is no data in the image file, print the
 *  appropriate message to standard error and exit with the appropriate code
 */
void no_data_error()
{
    fprintf(stderr, "uqimageclient: no data in input image\n");
    exit(EMPTY_IMAGE);
}

/**
 * process_http_response()
 * ---------------------------
 *  Processes the received HTTP response appropriately
 *
 *  FILE* from: the fd for reading the HTTP response from
 */
void process_http_response(FILE* from)
{
    int status;
    char* statusExplanation;
    unsigned char* bodyText;
    unsigned long len;
    HttpHeader** headers;
    if (!get_HTTP_response(
                from, &status, &statusExplanation, &headers, &bodyText, &len)) {
        fprintf(stderr, "uqimageclient: server connection terminated\n");
        exit(CONNECTION_CLOSED);
    }
    if (status == HTTP_OK) {
        if (write(STDOUT_FILENO, bodyText, len) < 0) {
            fprintf(stderr, "uqimageclient: unable to write output\n");
            exit(WRITE_FAIL);
        }
    } else {
        if (len) {
            fprintf(stderr, "%.*s", (int)len, bodyText);
            exit(BAD_HTTP_RESPONSE);
        }
    }
    free(statusExplanation);
    free(bodyText);
    free_array_of_headers(headers);
    fclose(from);
}

/**
 * free_command_parameters()
 * -----------------------------
 *  Frees all the memory associated with the CommandParameters struct passed
 *  into the function
 *
 *  CommandParameters params: a pointer to the CommandParameters structure to
 *  free the memory from
 */
void free_command_parameters(CommandParameters* params)
{
    // Free all the memory allocate by command line arguments
    if (params->inputFile) {
        free(params->inputName);
    }
    if (params->outputFile) {
        free(params->outputName);
    }
}
