# UQImage

NOTE: PROGRAM DOES NOT COMPILE, AS libcsse2310a4.so IS UNAVAILABLE. THIS IS PURELY FOR RECORD PURPOSES

A server and client program. uqimageclient provides a command line interace that allows you to interact with the server (uqimageproc) as a client - connecting, sending an image to be operated on, receiving the modified image back from the server and saving it to a file. Constructs a HTTP request based on command line arguments, connect to the server, send the request, await a response, and then save the response to a file.

./uqimageclient portno [--input _infile_ ] [--rotate _angle_ | 71
--scale _width_ _height_ | --flip _direction_ ] [--output _outputfilename_ ]

uqimageproc is a networked, multithreaded image processing server allowing clients to connect, send images for manipulation, and then return manipualted images to the client. All communication between clients and the server is via HTTP over TCP.

./uqimageproc [--port _port_ ] [--max _connections_ ]
