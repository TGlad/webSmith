// webmate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <tchar.h>
#include "HTTPRequest.h"
SOCKET createBindAndListenSocket()
{
  // used to store information about WinSock version
  WSADATA w;
  int error = WSAStartup (0x0202, &w);   // version 2.2
  if (error)
  { // there was an error
    return 0;
  }
  if (w.wVersion != 0x0202)
  { // wrong WinSock version!
    WSACleanup (); // unload ws2_32.dll
    return 0;
  }

  // create socket s
  SOCKET s = socket(AF_INET, SOCK_STREAM, 0); // family, type (SOCK_STREAM is TCP/IP) and protocol

  // bind s to a port
  // Note that you should only bind server sockets, not httpRequest sockets
  sockaddr_in addr; // the address structure for a TCP socket

  addr.sin_family = AF_INET;      // Address family Internet
  addr.sin_port = htons (81);   // Assign port 8080 to this socket

  addr.sin_addr.s_addr = htonl(INADDR_ANY);   // No destination.. or inet_addr ("129.42.12.241")
  if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
  { // error
    WSACleanup ();  // unload WinSock
    return 0;         // quit
  }

  // now listen
  if (listen(s, MAX_HTTP_REQUESTS)==SOCKET_ERROR)
  { // error!  unable to listen
    WSACleanup ();  // unload WinSock
    return false;
  }
  return s;
}
inline void endian_swap(unsigned int& x)
{
  x = (x>>24) | 
    ((x<<8) & 0x00FF0000) |
    ((x>>8) & 0x0000FF00) |
    (x<<24);
}

DWORD WINAPI runThread(LPVOID args)
{
  HTTPRequest httpRequest;
  httpRequest.s = (SOCKET)args; // we are hoping that this gets called before a new request can be read
  httpRequest.init();
  httpRequest.run();
  shutdown (httpRequest.s, SD_SEND);	// s cannot send anymore
  char temp[4000];
  int size = recv(httpRequest.s, temp, sizeof(temp), 0); // receive any final data
  if (size > 0)
  {
    BREAK;
  }
  if (size == SOCKET_ERROR)
  {
    int error = GetLastError();
    error;
    BREAK;
  }
  // you should check to see if any last data has arrived here
  closesocket (httpRequest.s);	// close
  return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
  stringUnitTest();
  SOCKET s = createBindAndListenSocket();

  // listening…  
  for(;;) // this loop will only exit if something has gone so wrong that the whole server needs to shut down
  {
    SOCKET requestSocket;
    sockaddr address; // for some reason we don't seem to need this info in the thread
    int addr_size = sizeof(address);
    do 
    {
      requestSocket = accept(s, &address, &addr_size); 
    } while(requestSocket == INVALID_SOCKET); // hopefully it doesn't just accept the same thing again if the socket is invalid... but I doubt that
    // httpRequest connected successfully, so find a 
    
    // start a thread that will communicate with httpRequest
    DWORD threadId;
    CreateThread(NULL, 0, runThread, (void *)requestSocket, 0, &threadId); // pass the socket by value, for safety
  }

 
  shutdown(s, SD_BOTH);  // s cannot send anymore
  // you should check to see if any last data has arrived here
  closesocket(s);   // close

  WSACleanup();
  return 0;
}

