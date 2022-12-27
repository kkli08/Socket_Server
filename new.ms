.TH CMPUT379 ASSIGNMENT3 
.SH NAME
Server /- Set up Server & Handle Trans
.SH COMMAND
server <port number>
.SH DESCRIPTION
Accepts one command-line argument: server port. 
where port is the port number that the server listens to for communications from clients. The port must be in the range 5,000 to 64,000.
.SH NAME
Client /- Set up Client & Send Trans to Server
.SH COMMAND
client <port number> <ip address>
.SH DESCRIPTION
Accepts two command line arguments: client port ip-address.
where ip-address is the IP (Internet Protocol) address of the machine that is hosting the server, and port is the port number that the server will read from (in the range 5000 to 64,000, the same number that the server uses).