.TL
Server & Client CMPUT 379
.AU
Ke Li
.AI
University of Alberta
.NH
Server
.PP
[Command]
.PP
.BI
server <port number>
.NH
Client
.PP
[Command]
.PP
.BI
client <port number> <ip address>
.NH
README
.PP
.BI
CAUTIONS:
.PP
use signal & alarm to exit the server after waiting 30 seconds of no incoming messages, than the server should exit.
.PP
and in the signal handler function, server fprint the summary message to the server.log before exit from the server.c.
.PP
please wait for 30 seconds so that the server.log can print the summary messages perfectly.
.PP
please and lot of thanks!
