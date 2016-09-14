#ifndef KHD_DAEMON_H
#define KHD_DAEMON_H

bool StartDaemon();
void TerminateDaemon();
bool ConnectToDaemon(int *SockFD);

char *ReadFromSocket(int SockFD);
void WriteToSocket(const char *Message, int SockFD);
void CloseSocket(int SockFD);

#endif
