#pragma once
#include "stdafx.h"
#include "String.h"

// MAX_HTTP_REQUESTS means the number of http requests that can connect while the socket is being used. 
// That means that these clients will have to wait until all clients before him have been dealt with. 
// If you specify a backlog of 5 and seven people try to connect, then the last 2 will receive an error message and should try to connect again later. 
// Usually a backlog between 2 and 10 is good, depending on how many users are expected on a server.
#define MAX_HTTP_REQUESTS 30           
class HTTPRequest
{
public:
  void init();
  void run();
  void returnPage(String &webPage);
  FILE *openFile(const String &fileName, int *fileSize, bool textFile);
  bool saveToFile(const String &fileName, const String &contents);
  // page specific requests
  void saveObjectFromHTMLToObjFile(String &action, String &newFileName);
  void parseAllFromNeutralToJavascript(); // a helper function
  void parseObjectFromNeutralToJavascript(String &action, String &objectName);
  void loadObjectFromObjFileToHTML(String &action, bool editable);
  void loadPageFromFile(String &action);
  void parseFunction(const String &function, String &buffer);

  SOCKET s; 
};

