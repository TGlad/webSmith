// webmate.cpp : Defines the entry point for the console application.
//
#include "HTTPRequest.h"

// used to efficiently prepend the reply header behind the web page contents
const String g_replyHeader = QUOTE(
              "HTTP/1.1 200 OK\r\n"
              "Date: Mon, 23 May 2005 22:38:34 GMT\r\n"
              "Server: Apache/1.3.3.7 (Unix)  (Red-Hat/Linux)\r\n"
              "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\r\n"
              "Etag: \"3f80f-1b6-3e1cb03c\"\r\n"
              "Accept-Ranges: bytes\r\n"
              "Content-Length: [len]\r\n"
              "Connection: close\r\n"
              "Content-Type: text/html; charset=UTF-8\r\n\r\n");
const String g_objectRepository = QUOTE("../../clientScript/objects/");
const String g_rawObjectRepository = QUOTE("../../userGeneratedObjects/");

// shared memory, hope this isn't locked
static char g_input[MAX_HTTP_REQUESTS];  
static int g_numHTTPRequests = 0;

const int numFunctionTokens = 9;
const char *functionTokens[numFunctionTokens] = {"[description]", "[wikilink]", "[init]", "[receive]", "[reply]", "[predict]", "[react]", "[generate]", "[analyse]"};

void HTTPRequest::init()
{
  // initialise data so there aren't leftovers from the previous time this object was used.
}

// webPage must have been alocated with allocateReply()
void HTTPRequest::returnPage(String &webPage) 
{
  String pageSize = toString(webPage.length);
  ASSERT(pageSize.length > 0 && pageSize.length <= 6);
  String header = newString(g_replyHeader);
  replaceString(header, QUOTE("[len]"), pageSize);
  ASSERT(header.length < 400);
  int sent = send(s, header.chars, header.length, 0);
  if (sent != header.length)
  {
    if (sent == SOCKET_ERROR)
    {
      int error = GetLastError();
      BREAK;
    }
  }
  ASSERT(webPage.length < 400000 && webPage.length>0);
  sent = send(s, webPage.chars, webPage.length, 0); 
  if (sent != webPage.length)
  {
    if (sent == SOCKET_ERROR)
    {
      int error = GetLastError();
      BREAK;
    }
  }
}
FILE *HTTPRequest::openFile(const String &fileName, int *fileSize, bool textFile = true)
{
  FILE *file = NULL;
  fopen_s(&file, fileName.cString(), textFile ? "rt" : "rb");
  if (!file)
    return NULL;
  fseek(file, 0, SEEK_END);
  *fileSize = (int)ftell(file);
  fseek(file, 0, SEEK_SET);
  if (*fileSize <= 0)
  {
    fclose(file);
    return NULL;
  }
  return file;
}
bool HTTPRequest::saveToFile(const String &fileName, const String &contents)
{
  FILE *saveFile = NULL;
  bool isNewFile = false;

  WIN32_FIND_DATAA data;
  HANDLE hFind;
  hFind = FindFirstFileA(fileName.cString(), &data); // find if file already exists
  if (hFind == INVALID_HANDLE_VALUE)
    isNewFile = true;
  fopen_s(&saveFile, fileName.cString(), "wt");
  ASSERT(saveFile); // no reason why it shouldn't be available for writing
  fseek(saveFile, 0, SEEK_SET);
  fwrite(contents.cString(), 1, contents.length, saveFile);
  fclose(saveFile);
  return isNewFile;
}
void HTTPRequest::saveObjectFromHTMLToObjFile(String &action, String &newObjectName)
{
  char *next = NULL;
  char *found = action.find("=");
  String objectName = action;
  while (found)
  {
    objectName = objectName.substring(found+1, NULL);
    found = objectName.find("=");
  }

  // lastly we write the form data out to a file.
  objectName = objectName.substring(0, objectName.find("."));
  if (objectName.length==0)
    return; // no save-to filename
  replaceString(objectName, "+", ""); // get rid of spaces
  objectName.chars[0] = toupper(objectName.chars[0]); // capitalise first letter since this is the type of an object
  bool isNewFile = saveToFile(newString(g_rawObjectRepository + objectName + QUOTE(".obj")), action);
  String fileName = newString(objectName + QUOTE(".obj"));
  newObjectName.set(objectName); // will assert if objectName > newFilename's buffer size

  // update svn repository
  // WARNING: system() is not a great way to do this, apparently exec() is preferred
  // if we have added a new file then we need to add it to svn repository
  if (isNewFile)
    system(newString(QUOTE("svn add ") + g_rawObjectRepository + fileName).cString());
  // now we have saved to file, we need to check in the file
  system(newString(QUOTE("svn commit ") + g_rawObjectRepository + fileName + QUOTE(" --file ") + g_rawObjectRepository + QUOTE("lastCommit.log")).cString());
}

void HTTPRequest::parseAllFromNeutralToJavascript()
{
  // read the contents of objectRepository and parse each file
  WIN32_FIND_DATAA data;
  HANDLE hFind;
  String pattern = newString(g_rawObjectRepository + QUOTE("*.obj"));
  
  hFind = FindFirstFileA(pattern.cString(), &data);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    FindClose(hFind);
    return;
  }
  BOOL found = true;
  while (found)
  {
    String fileName(data.cFileName);
    String pathAndFileName = newString(g_rawObjectRepository + fileName);
    FILE *neutralFile = NULL;
    fopen_s(&neutralFile, pathAndFileName.cString(), "rt");
    long neutralFileSize = 0;
    String neutral;
    if (!neutralFile)
      continue;
    fseek(neutralFile, 0, SEEK_END);
    neutralFileSize = ftell(neutralFile); 
    if(neutralFile <= 0)
    {
      fclose(neutralFile);
      continue;
    }
    fseek(neutralFile, 0, SEEK_SET);
    neutral = readStringFromFile(neutralFile, neutralFileSize);
    fclose(neutralFile);

    fileName = fileName.substring(0, fileName.find(".obj"));
    parseObjectFromNeutralToJavascript(neutral, fileName);
    found = FindNextFileA(hFind, &data);
  }
  FindClose(hFind);

}

void HTTPRequest::parseObjectFromNeutralToJavascript(String &action, String &objectName)
{
  char *next = NULL;
  String strings[8]; // 7 function codes, then the save file name
  char parsedFunction[10000];
  String temp(parsedFunction, sizeof(parsedFunction)); // with fixed buffer size
  action = action.substring(action.find("init="), NULL); // ignore the description at the start
  for (int i = 0; i<8; i++)
  {
    strings[i] = action.getStringBetween("=", String("&"), &next);
    strings[i].convertFromHTML();
    if (i<7)
      parseFunction(strings[i], temp);
    strings[i] = newString(temp); // allocate new memory
  }
  int maxFileSize;
  FILE *objectTemplate = openFile("objectTemplate.js", &maxFileSize);
  if (!objectTemplate)
    return;
  for (int i = 0; i<8; i++)
    maxFileSize += strings[i].length; // so we allocate enough space for the new code
  String templateString = readStringFromFile(objectTemplate, maxFileSize);
  fclose(objectTemplate);
  for (int i = 2; i<numFunctionTokens; i++)
    replaceString(templateString, functionTokens[i], strings[i-2]);

  replaceString(templateString, "[objectName]", objectName);
  String fileName = newString(objectName + QUOTE(".js"));
  bool isNewFile = saveToFile(newString(g_objectRepository + fileName), templateString);
}

void HTTPRequest::loadObjectFromObjFileToHTML(String &action, bool editable)
{
  // first open the file (name.obj)
  String folderPlusName = newString(g_rawObjectRepository + action);
  if (!folderPlusName.find(".obj"))
    append(folderPlusName, ".obj");
  FILE *javascriptFile = NULL;
  fopen_s(&javascriptFile, folderPlusName.cString(), "rt");
  long javascriptFileSize = 0;
  String javascript;
  if (javascriptFile)
  {
    fseek(javascriptFile, 0, SEEK_END);
    javascriptFileSize = ftell(javascriptFile); 
    if(javascriptFile <= 0)
    {
      fclose(javascriptFile);
    }
    else
    {
      fseek(javascriptFile, 0, SEEK_SET);
      javascript = readStringFromFile(javascriptFile, javascriptFileSize);
      fclose(javascriptFile);
    }
  }
  action = action.substring(0, action.find(".")); // get rid of the .js in 'action'

  // next open the template file
  int templateFileSize;
  FILE *templateFile = openFile("template.html", &templateFileSize);
  if (!templateFile)
    return;

  String webPage = readStringFromFile(templateFile, templateFileSize+javascriptFileSize);
  fclose(templateFile);

  char *next = NULL;
  String strings[numFunctionTokens];  // + 1 for the save file name
  for (int i = 0; i<numFunctionTokens; i++)
  {
    strings[i] = javascript.getStringBetween("=", String("&"), &next);
    strings[i].convertFromHTML();
  }

  if (editable)
  {
    replaceString(webPage, "readonly", "");
    replaceString(webPage, "<!--save button-->", QUOTE("<input type=\"text\" name=\"save as\" value=\"[objectFile]\"/><input type=\"submit\" value=\"save changes\">"));
    replaceString(webPage, "<!--description-->", QUOTE("<textarea style=\"WIDTH: 100%\" name=\"description\" cols=80 rows=5>"));
    replaceString(webPage, "<!--/description-->", QUOTE("</textarea>"));
    replaceString(webPage, "<a href=\"[wikilink]\">[wikilink]</a>", "<textarea name=\"wikilink\" cols=20 rows=1>[wikilink]</textarea>");
  }
  else // otherwise need to add an edit button
  {
    replaceString(webPage, "<!--edit-->", "<input type=\"submit\" value=\"edit\"/>");
  }

  // now replace the tokens in the template with the function strings
  for (int i = 0; i<numFunctionTokens; i++)
    replaceString(webPage, functionTokens[i], strings[i]);
  replaceString(webPage, "[objectFile]", action); 

  returnPage(webPage);
}
void HTTPRequest::loadPageFromFile(String &action)
{
  String pathPlusFile;
  if (action.startsWith("editarea/"))
  {
    action = action.substring(&action.chars[sizeof("editarea/")-1], 0);
    pathPlusFile = newString(QUOTE("../../editarea_0_8_2/") + action);
  }
  else if (action.startsWith("codemirror/"))
  {
    action = action.substring(&action.chars[sizeof("codemirror/")-1], 0);
    pathPlusFile = newString(QUOTE("../../codemirror/") + action);
  }
  else
    pathPlusFile = newString(QUOTE("../../clientScript/") + action);
  // so to start with we will just open up the test html page
  bool isText = action.find(".js") || action.find(".html");
  int fileSize = 0;
  FILE *file = openFile(pathPlusFile, &fileSize, isText);
  if (!file)
    return;

  // if this is the main file (test.html) then we are going to be adding in some dependencies, so we need to allocate some more space
  bool isRoot = action.startsWith("test.html"); // test will become the scene name I guess
  String fileBuffer = readStringFromFile(file, fileSize);
  fclose(file);

  // if client has requested the root page, then we need to convert the list of objects in the objects directory into some javascript to 
  // declare the array of them
  if (isRoot) 
  {
    String pattern = newString(g_objectRepository + QUOTE("*.js"));

    WIN32_FIND_DATAA data;
    HANDLE hFind;
    hFind = FindFirstFileA(pattern.cString(), &data);
    if (hFind == INVALID_HANDLE_VALUE)
    {
      FindClose(hFind);
      return;
    }
    String fileName(data.cFileName);
    String string = newString(QUOTE("<script src=\"objects/") + fileName + QUOTE("\"> </script>\n"));
    String objectName = fileName.substring(0, fileName.find(".js"));
    String arrayString = newString(QUOTE("g_objects = [") + objectName);
    BOOL found = true;
    do
    {
      found = FindNextFileA(hFind, &data);
      fileName.use(data.cFileName);
      if (found)
      {
        append(string, QUOTE("<script src=\"objects/") + fileName + QUOTE("\"> </script>\n"));
        append(arrayString, QUOTE(", ") + fileName.substring(0, fileName.find(".js")));
      }
    } while (found);
    FindClose(hFind);
    append(arrayString, QUOTE("];"));

    String objectIncludeToken = QUOTE("<!--add objects here-->");
    replaceString(fileBuffer, objectIncludeToken, string);
//    String objectArrayToken = QUOTE("g_objects = [];");
//    replaceString(fileBuffer, objectArrayToken, arrayString);
  }

  // definitely need to generalise this into reply(char *contents);
  returnPage(fileBuffer); 
}

void HTTPRequest::run()
{
  // loop and do stuff permanently here, in particular receives and sends
  // SOCKET s is initialized
  const int MAX_RECEIVE_LENGTH = 50000; // far too large a get call
  char inBuffer[MAX_RECEIVE_LENGTH];
  int receivedSize = recv(s, inBuffer, MAX_RECEIVE_LENGTH, 0); // TODO do we have to actually receive this or can we be content with just calling peek?
  if (receivedSize == SOCKET_ERROR || receivedSize <= 0)
  {
    int error = WSAGetLastError();
//    BREAK;
    return; // an error, so return.
  }
  if (receivedSize > MAX_RECEIVE_LENGTH) // we have reached a readable length
    return; // too large a request, so just ignore it!
  String input(inBuffer, receivedSize);
//  expecting:
// GET /temp HTTP/1.1
// Host: localhost:8080
// Connection: keep-alive
// Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
// User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US) AppleWebKit/534.1 (KHTML, like Gecko) Chrome/6.0.437.3 Safari/534.1
// Accept-Encoding: gzip,deflate,sdch
// Accept-Language: en-GB,en-US;q=0.8,en;q=0.6
// Accept-Char

  if (!input.startsWith("GET ") || !input.find("HTTP/1.1\r\n") || input.chars[4]!='/') 
    return; // i.e. wait for the next packet
  char *next = NULL;
  String action = input.getStringBetween(String("GET "), String(" "), &next);
  action = action.substring(action.chars+1, 0);
  if (!next)
    return;
  // system command to parse all the neutral files into js files
  if (action.startsWith("parseAll"))
  {
    parseAllFromNeutralToJavascript();
    return;
  }
  // serve the page that saves code changes
  if (action.startsWith("saveChanges"))
  {
    String objectName = newString("this is the max size of an object name");
    saveObjectFromHTMLToObjFile(action, objectName);
    loadObjectFromObjFileToHTML(newString(objectName + QUOTE(".obj")), true); // if you hit save it wantd to stay editable
    // we parse after having sent the page back
    parseObjectFromNeutralToJavascript(action, objectName);
    return;
  }
  if (!action.find("/"))
  {
    char *firstDot = action.find(".");
    if (!firstDot || action.substring(firstDot, 0).startsWith(".obj"))
    {
      action.chars[0] = toupper(action.chars[0]); // capitalise the first letter, even if we search for it with lower case
      loadObjectFromObjFileToHTML(action, firstDot!=NULL); // if contains .obj then is editable
      return;
    }
  }
  // if all specialist examples above are not found, then just serve whatever file it requests 
  loadPageFromFile(action);
  return;
}
