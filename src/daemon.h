#ifndef DAEMON_H
#define DAEMON_H

bool StartDaemon();
void TerminateDaemon();
bool ConnectToDaemon(int *SockFD);

char *ReadFromSocket(int SockFD);
void WriteToSocket(const char *Message, int SockFD);
void CloseSocket(int SockFD);

#endif
