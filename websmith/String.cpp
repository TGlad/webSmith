#include "stdafx.h"
#include "String.h"
__declspec(thread) StringStack stringStack; // required for string allocation, one per thread

//   Example use of string functions
void stringUnitTest()
{
  StringStack stringStack; // stringStack must be accessible by any of the allocating string functions e.g. newString()

  // strings maintained in separate memory
  String hello("hello"); 
  String hello1(QUOTE("hello")); // applying QUOTE() to string literals is faster as it doesn't require iterating the string to get its length 
  String hello2(String(QUOTE("hello"))); // same as above
  char cStyleHello[6] = "hello";
  String hello3 = newString(cStyleHello); // don't apply QUOTE() to char* c-style strings
  String hello4 = newString(String(cStyleHello)); // same as above
  String hello5 = newString(hello4);          // can construct from another String 
  hello4 = newString(cStyleHello + QUOTE(" there")); // concatenate 2 strings, also fine to override the previous hello4
  char *toYou = " to you";
  hello5 = newString("I say " + hello4 + toYou);  // can concatenate different string types 
  int n = 10;
  hello5 = newString("I can count to " + toString(n)); // convert to a string
  append(hello5, "!!"); // fast concatenation
  append(hello5, " If I append a lot of text to the end it we reallocate the whole string"); 

  // strings sharing memory
  String hello6(cStyleHello); // uses cStyleHello's buffer
  String hello7(cStyleHello);
  hello6.chars[0] = 'y'; // hello7 will also be "yello" now
  String hello8, hello9;
  hello8.use(hello6); // same a hello7
  hello9.use(cStyleHello); // same a hello7
  replaceString(hello6, QUOTE("yel"), QUOTE("car")); // hello6 will be "carlo" as will hello7,8 and 9

  // accessors
  bool correct = hello1.startsWith("hel"); // returns true
  char *charPointer = hello5.find("I append"); // note a char * shouldn't be considered a string, but a pointer to memory
  char *startPoint = NULL; 
  String text = hello5.getStringBetween("lot of ", QUOTE(" to the end"), &startPoint); // text is "text", hello5 is unaffected, but text shares hello5's buffer
  printf("%s", text.cString()); // cString is needed to null terminate, that means hello5 will be changed
  // to avoid this, you have to use separate memory:
  text.chars[text.length] = ' '; // restore hello5
  startPoint = NULL;
  String newText = newString(hello5.getStringBetween("lot of ", QUOTE(" to the end"), &startPoint)); 
  printf("%s", newText.cString()); // newText is null terminated here, but that is fine, memory has been allocated for the null terminator

  String el = hello.substring(hello.find("e"), hello.find("lo")); 
  String hel = hello.substring(0, hello.find("lo")); // crops the end off
  String ello = hello.substring(hello.find("el"), 0); // crops the start off the string
}

/////////////////////////////// string functions ////////////////////////////////////////
String g_nullString;
String::String()
{
  length = 0;
  bufferLength = 0;
  chars = 0;
}
String::String(const char *string)
{
  length = strlen(string);
  bufferLength = length+1; // no extra here, other than 1 for null terminator
  chars = (char *)string; // must remember not to try and modify a string literal
}
String::String(const char *string, int bufferSize)
{
  length = bufferSize-1;
  bufferLength = bufferSize; // no extra here, other than 1 for null terminator
  chars = (char *)string; // must remember not to try and modify a string literal
}
String::String(int number, char *buffer, int bufferLen)
{
  chars = buffer;
  int res = _itoa_s(number, chars, bufferLen-1, 10);
  ASSERT(res==0); // something gone wrong
  length = strlen(chars);
  ASSERT(length < bufferLen);
  bufferLength = bufferLen;
}
String::String(const String &data, char *buffer)
{
  *this = data;
  chars = buffer;
  memcpy(chars, data.chars, data.length+1); // faster than strcat because doesn't have to find the end
}
String::String(const TempString &data, char *buffer)
{
  length = 0;
  chars = buffer;
  for (int i = 0; i<data.numStrings; i++)
  {
    memcpy(chars + length, data.strings[i].chars, data.strings[i].length+1); // faster than strcat because doesn't have to find the end
    length += data.strings[i].length;
  }
  ASSERT(length == data.length);
  bufferLength = genBufferLen(length);
}
TempString String::operator +(const TempString &string) const 
{
  TempString sum(*this);
  return sum + string;
}
TempString String::operator +(const String &string) const
{
  TempString sum(*this);
  return sum + TempString(string);
}

TempString operator +(const char *cstr, const TempString &string) 
{
  TempString sum(cstr);
  return sum + string;
}
void String::insert(const TempString &tString, char *startPoint)
{
  char *start = startPoint ? startPoint : chars;
  ASSERT(tString.length + (int)(start - chars) < bufferLength);
  for (int i = 0; i<tString.numStrings; i++)
  {
    memcpy(start, tString.strings[i].chars, tString.strings[i].length);
    start += tString.strings[i].length;
  }
}
int String::genBufferLen(int length) 
{
  int preferredSize = length + (length>>1) + 1; // 50% larger to allow some expansion when replacing strings, +1 for optional null terminator
  return (((preferredSize + 3)>>4)<<4) + 12;
}
void String::use(const String &string)
{
  *this = string;
}
void String::use(const char *string)
{
  length = strlen(string);
  bufferLength = length+1; // no extra here, extept 1 for null terminator
  chars = (char *)string;
}
// allows you to increase past the string length
void String::use(const char *string, int maxBufferLen)
{
  length = strlen(string);
  ASSERT(maxBufferLen >= length+1);
  bufferLength = maxBufferLen; 
  chars = (char *)string;
}
bool String::startsWith(const String &string) const
{
  return !memcmp(chars, string.chars, string.length);
}
// for precedence you just decide which order to apply the replacements in
// the scope will include multiplies and divides, but not plus/minus... but will include negation
void String::scopedReplacement(const String &token, const String &left, const String &between, const String &right, bool highPrecedence)
{
  char *startPoint = chars;
  while (startPoint)
  {
    startPoint = nextScopedReplace(token, left, between, right, startPoint, highPrecedence);
  }
}
bool isNumeric(char c)
{
  return c>='0' && c<='9';
}
bool isAlphaNumeric(char c)
{
  return isNumeric(c) || (c>='a' && c<='z') || (c>='A' && c<='Z') || c=='_' || c=='$';
}
char *findEnd(char *start, int dir, int maxSize, bool highPrecedence)
{
  int bracketStack = 0;
  bool inString = false;
  for (int i = 0; i<maxSize; i++)
  {
    start += dir;
    char newChar = *start;

    if (inString && (newChar == '\"' || newChar == '\'' && *(start-1)!='\\'))
    {
      inString = false; // out of string again
      continue;
    }
    if (newChar == '(' || newChar == '[' || newChar == '{')
    {
      bracketStack+=dir;
      if (bracketStack == -1)
        return start;
      continue;
    }
    if (newChar == ')' || newChar == ']' || newChar == '}')
    {
      bracketStack-=dir;
      if (bracketStack == -1)
        return start;
      continue;
    }
    if (newChar == '\"' || newChar == '\'' && *(start-1)!='\\')
    {    
      inString = true;
      continue;
    }
    if (bracketStack > 0)
      continue; // we haven't found the end of the scope if this is the case
    if (newChar == '\n' || newChar == '\r' || newChar == ';' || newChar == '=' || newChar == '?' || newChar == ':' || newChar == ',')
    {
      return start; // found the end of scope
    }
    if (newChar == '&' || newChar == '|') // operators and functions should probaly have precedence over && and ||
    {
      return start; 
    }
    if (isAlphaNumeric(newChar) || newChar == '.')
    {
      continue;
    }
    if (newChar == '*' || newChar == '/')
    {
      if (highPrecedence)
        return start; 
      if (dir == 1)
      {
        start+=dir;
        while (*start == ' ' || *start == '.' || *start == '-')
          start += dir;
        start -= dir;
      }
      continue;
    }
    if (newChar == '+')
    {
      if (dir == -1)
        return start;
      if (*(start-1) == '.')
        return start - 1;
      return start;
    }
    if (newChar == '-')
    {
      if (highPrecedence)
      {
        if (dir==1)
          continue; // x^-2 is (x^-2)
        else 
          return start; // -3^2 should be -9, not 9
      }
      if (dir == 1) // definitely not unary negation
        return start;
      if (*(start-1) == '.')
        start--;
      start--;
      while (*start == ' ') // remove space
        start += dir;
      if (*start == '*' || *start == '/')
        continue;
      return start;
    }
    if (dir == 1 && newChar == '/' && start[1] == '/') // a comment
      return start;
    int h = 3;
    h; // find out what I'm missing
  }
  if (dir == 1)
    return start;
  return start-1; // reached end of file
}
int getIdentifierLength(char *id)
{
  if (!id)
    return 0;
  if (id[0] >= '0' && id[0] <= '9')
    return 0;
  int i = 0;
  while (isAlphaNumeric(id[i]))
    i++;
  return i;
}
char *String::nextScopedReplace(const String &token, const String &left, const String &between, const String &right, char *startPoint, bool highPrecedence)
{
  int tokenLength = token.length;

  // special case here, for function calls
  String identifier = QUOTE("[identifier]");
  char *id = token.find(identifier);
  String actualIdentifier;
  char *found = NULL;
  if (id)
  {
    String pre = newString(token.substring(0, id));
    String post = newString(token.substring(id + identifier.length, 0));
    char *idStart = startPoint;
    // special find loop... TODO this is unbounded
    do 
    {
      found = find(pre, idStart);
      if (!found)
        return NULL;
      idStart = found + pre.length;
      int len = getIdentifierLength(idStart);
      if (!len)
        continue;
      String temp = newString(substring(idStart + len, 0));
      if (!temp.startsWith(post))
        continue;
      actualIdentifier = newString(substring(idStart, idStart + len)); 
      tokenLength = len + pre.length + post.length;
      break;
    } while(found);
  }
  else
  {
    char *fStart = startPoint;
    do 
    {
      found = find(token, fStart);
      if (!found)
        return NULL;
      fStart = found + token.length;
    } while (*fStart == '=');
  }
  char *foundLeft = findEnd(found, -1, (int)(found - chars), highPrecedence);
  char *foundRight = findEnd(found + tokenLength-1, 1, (int)(1 + chars + length - found - tokenLength), highPrecedence);
  
  if (foundLeft && foundRight)
  {
    nextReplace(foundLeft+1, 0, left);
    found += left.length;
    foundRight += left.length;
    char *id2 = between.find(identifier);
    if (id && id2)
    {
      String pre = between.substring(0, id2);
      String post = between.substring(id2 + identifier.length, 0);
      String together = newString(pre + actualIdentifier + post);
      found = nextReplace(found, tokenLength, together);
      foundRight += together.length - tokenLength;
    }
    else
    {
      found = nextReplace(found, tokenLength, between);
      foundRight += between.length - tokenLength;
    }
    nextReplace(foundRight, 0, right);
    return foundRight + right.length;
  }
  return NULL;
}

// this is a fast replacement method, as no extra memory needs to be allocated and memcpys can be smaller
void String::reduce(const String &token, const String &field, StringType type)
{
  ASSERT(token.length >= field.length); // this is a reduction
  char *startPoint = chars;
  char *bufferHead = chars;
  int foundLength = 0;
  char *found = find(token, startPoint, type, &foundLength);
  // this incrementally memcpys each of the gaps between the finds
  while (found) 
  {
    bufferHead += (int)(found - startPoint);
    startPoint = found + foundLength;
    memcpy(bufferHead, field.chars, field.length);
    bufferHead += field.length;
    ASSERT(bufferHead <= startPoint);

    found = find(token, startPoint, type, &foundLength);
    if (!found)
    {
      int change = (int)(chars + length - startPoint);
      memcpy(bufferHead, startPoint, change);
      length = (int)(bufferHead + change - chars);
      return;
    }
    memcpy(bufferHead, startPoint, (int)(found - startPoint));
  } 
}


char *String::find(const String &string, char *startPoint, StringType type, int *foundLength) const
{
  if (foundLength)
    *foundLength = string.length;
  if (length == 0)
    return NULL;
  char endChar = chars[length];
  char stringEndChar = string.chars[string.length];
  // have to null terminate in order to use strstr.
  char *end1 = chars + length;
  if (*end1) // these ifs should assure they never try and modify a constr string
   *end1 = 0; // this suggests the function is non const, but we revert the change back at the end of the function
  char *end2 = string.chars + string.length;
  if (*end2)
    *end2 = 0;
  int modulo = 0;
  if (startPoint == NULL || startPoint == chars)
    modulo = 1;
  char *found = NULL;
  char *str = startPoint ? startPoint : chars;
  if (type == ST_Any)
    found = strstr(str, string.chars);
  else
  {
    found = str-1;
    for(;;)
    {
      char *nextFind = strstr(found+1, string.chars);
      char *quote = found;
      int numQuotes = 0;
      do 
      {
        quote = strchr(quote+1, '\"');
        if (!quote)
          break;
        if (quote < nextFind && *(quote-1) != '\\')
          numQuotes++;
      } while(quote < nextFind);
      if (type == ST_JustInString && numQuotes%2 == modulo)
      {
        found = nextFind;
        break;
      }
      if (numQuotes%2 == 1 || type == ST_JustInString) // then we're inside a string declaration
      {
        modulo = 0;
        found = quote; // fast forward to next end quote
        if (!found)
          break;
        continue;
      }
      found = nextFind;
      if (!found)
        break;
      if (type == ST_NotInString)
        break; // we have found the first occurence that isn't in a string
      if (type == ST_NotSpaced)
      {
        if (isAlphaNumeric(*(found-1)) || isAlphaNumeric(found[string.length])) // is mid-way through an identifier eg function name
          continue;
        if (found[string.length] == ' ' && *(found-1) == ' ')
          continue;
        break;
      }
      // for ST_Units we need to see before it:
      //   a space, a number
      // after it:
      //   space, ;, ',', ')', '^', '*', '/'
      if (type == ST_Units)
      {
        if (found == str)
          continue; // occurs at start of string
        bool isDotType = (found > str+1) && *(found-1) == '.' && *(found-2) == ' ';
        if (!isNumeric(*(found-1)) && *(found-1)!=' ' && !isDotType)
          continue;
      }
      else if (type == ST_AfterIdentifier)
      {
        if (found == str)
          continue;
        if (!isAlphaNumeric(*(found - 1))) // must come after an identifier, in this case we just check the previous character is alphanumeric
          continue;
      }
      else if (type == ST_ToLineEnd) // deliberately do nothing here
      {
        char *end = strstr(found + string.length, "\n");
        if (!end)
          end = chars + length;
        if (foundLength)
          *foundLength = (int)(end - found);
        break;
      }
      else if (type == ST_AfterNumber)
      {
        if (isAlphaNumeric(found[string.length]))
          continue; // 300.4f4 isn't a match, nor is 300.4fg
        // 2e-10f   blahe-10f
        // check value backwards is alphanumeric and contains only numbers or e or 'e-'
        char *ptr = found;
        bool isNum = false;
        bool eFound = false;
        if (!isAlphaNumeric(*(ptr-1))) // e.g. e-f won't be thought of as a number
          continue;
        while (ptr > chars) // complicated by exponential notation!
        {
          char c = *--ptr;
          if (c=='-' && *(ptr-1) == 'e')
            continue;
          if (c=='e' && !eFound)
          {
            eFound = true; // can only be 1 e in a number
            continue;
          }
          if (!isAlphaNumeric(c))
          {
            isNum = ptr < found-1; 
            break;
          }
          if (!isNumeric(c))
            break; // not a number
        }
        if (isNum && ptr[1]!='e')
          break;
        else
          continue;
      }
      else if (found > str && isAlphaNumeric(*(found - 1)))
        continue; // it isn't an identifier or a function
      if (type == ST_Functions && found[string.length] != '(')
        continue;
      if (type == ST_IdentifierBeforeCompound)
      {
        if (found[string.length] != ' ')
          continue;
        int length = getIdentifierLength(found + string.length+1);
        if (length <= 0)
          continue;
        if (found[string.length + 1 + length] != '.')
          continue;
        break;
      }
      if (type == ST_Identifiers || type == ST_Units)
      {
        if ((found+string.length>=chars+length) || isAlphaNumeric(found[string.length]))
          continue;
        if (found[string.length] == '(') // functions don't count as identifiers
          continue;
      } 
      break;
    }
  }
  // return to normal
  if (endChar)
    *end1 = endChar;
  if (stringEndChar)
    *end2 = stringEndChar;
  return found;
}

// returns the string between start and end 
// if it doesn't find the start it will use this's start, if it doesn't find end it will use this's end, and return false
String String::getStringBetween(const String &start, String &end, char **searchStart) const
{
  String string;
  string.chars = find(start, *searchStart);
  bool found = true;
  if (!string.chars)
  {
    string.chars = chars;
    found = false;
  }
  else
    string.chars += start.length;
  char *endChars = find(end, string.chars);
  if (!endChars)
  {
    endChars = chars + length;
    found = false;
  }
  else
    *searchStart = endChars+1;
  string.length = (int)(endChars - string.chars); 
  string.bufferLength = bufferLength - (int)(string.chars - chars); // keep buffer length large, if we want to append
  if (!found)
    *searchStart = NULL; // surrounding strings not found
  return string;
}
// generic routine for cropping the start or the end or both (use NULL to not crop that end)
String String::substring(char *start, char *end) const
{
  String string = *this;
  if (start)
  {
    ASSERT((int)(start - chars) <= length && (int)(start - chars)>=0); // bad substring
    string.length -= (int)(start - chars);
    string.bufferLength -= (int)(start - chars);
    string.chars = start;
  }
  if (end)
  {
    ASSERT((int)(end - chars) <= length && (int)(end - chars)>=0); // bad substring
    ASSERT(end > start); // will also be true if start is NULL
    string.length = (int)(end - string.chars);
  }
  return string;
}
// replaces contents with string's up to its max buffer length
void String::set(const String &string)
{
  ASSERT(bufferLength > 1 + string.length);
  int bufLen = this->bufferLength;
  *this = String(string, chars);
  bufferLength = bufLen;
}
// replaces contents with string's up to its max buffer length
void String::set(const TempString &string)
{
  ASSERT(bufferLength > 1 + string.length);
  int bufLen = this->bufferLength;
  *this = String(string, chars);
  bufferLength = bufLen;
}

char entityTable[95] = // TSL would be good if we could use an existing conversion here
{
  ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
  '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~'
};
// converts in-place as the string will only get shorter
void String::convertFromHTML()
{
  char *start = chars;
  char *head = chars;
  int len = length;
  while (chars < start + len)
  {
    if (*chars == '%')
    {
      chars++;
      int index1 = *chars-'2'; 
      chars++;
      length -= 2;
      int index = index1*16 + (*chars<='9'? *chars-'0' : 10 + *chars-'A'); // convert from hex
      chars++;
      if (index == -22)
        *head++ = 10; // new line \n
//      else if (index == -19)
//        *head++ = 13; // return \r, actually we may want to ignore this
      else if (index1>=0 && index>=0 && index < 95)
        *head++ = entityTable[index];
      else
        length--;
    }
    else if (*chars == '+')
    {
      chars++;
      *head++ = ' ';
    }
    else
    {
      *head++ = *chars++;
    }
  }
  chars = start;
}
char *String::nextReplace(char *position, int lengthToReplace, const String &field)
{
  int endLength = length - (int)(position-chars) - lengthToReplace;
  ASSERT(endLength >= 0); // kind of bad input
  ASSERT(length + field.length - lengthToReplace < bufferLength); // gone out of range
  char *end = (char *)alloca(endLength+1);
  memcpy(end, position+lengthToReplace, endLength);
  memcpy(position, field.chars, field.length);
  memcpy(position+field.length, end, endLength);
  length += field.length - lengthToReplace;
  return position + field.length;
}
String String::fromFile(FILE *file, int size, char *buffer)
{
  String string;
  string.bufferLength = size;
  string.chars = buffer;
  string.length = fread(string.chars, 1, size, file); // read into the buffer
  string.chars[string.length] = 0;
  return string;
}
///////////////////////// Global functions /////////////////////////////////////////////////////////
int getStringLength(char *str)
{
  return strlen(str);
}
int getStringLength(const String &str)
{
  return str.length;
}
int getStringLength(const TempString &str)
{
  return str.length;
}

///////////////////////// Other class functions ////////////////////////////////////////////////////
// tries appending tempStr to str, returning a new required buffer length if there isn't enough space
int StringStack::tryAppend(String &str, const TempString &tempStr)
{
  int newLength = tempStr.length + str.length;
  if (newLength > str.bufferLength)
  {
    // only need to copy these temporaries if we are creating a new stack
    string = &str;
    tempString = tempStr; 
    return newLength;
  }
  int offset = str.length;
  for (int i = 0; i<tempStr.numStrings; i++)
  {
    memcpy(str.chars + offset, tempStr.strings[i].chars, tempStr.strings[i].length+1);
    offset += tempStr.strings[i].length;
  }
  str.length = newLength;
  return 0;
}
int StringStack::tryAppend(String &str, const String &other)
{
  int newLength = other.length + str.length;
  if (newLength > str.bufferLength)
  {
    // only need to copy these temporaries if we are creating a new stack
    string = &str;
    tempString.set(other); 
    return newLength;
  }
  memcpy(str.chars + str.length, other.chars, other.length+1);
  str.length = newLength;
  return 0;
}
TempString::TempString()
{
  numStrings = 0;
  length = 0;
}
TempString::TempString(const String &str1)
{
  numStrings = 1;
  strings[0] = str1;
  length = strings[0].length;
}
TempString TempString::operator +(const TempString &string) const
{
  ASSERT(numStrings + string.numStrings < MAX_STRING_COLLECTION);
  TempString sum = *this;
  for (int i = 0; i<string.numStrings; i++)
    sum.strings[numStrings + i] = string.strings[i];
  sum.length += string.length;
  sum.numStrings += string.numStrings;
  return sum;
}
TempString TempString::operator +(const String &string) const
{
  return *this + TempString(string);
}
TempString *TempString::set(const TempString &strCol)
{
  *this = strCol; // direct copy
  return this;
} 
TempString *TempString::set(const String &str)
{
  numStrings = 1;
  strings[0] = str;
  length = strings[0].length;
  return this;
}
void StringReplacement::apply(char *memoryBuffer)
{
  m_string->use(String(*m_string, memoryBuffer));
  int maxLen = apply(*m_string, m_token, m_field, m_type);
  ASSERT(maxLen==0);
}

int StringReplacement::apply(String &string, const String &token, const String &field, StringType type)
{
  int maxLength = 0;
  m_type = type;
  m_string = &string;
  m_token = token;
  m_field = field;
  if (field.length <= token.length)
  {
    m_string->reduce(token, field, type); // this is a faster function
    return 0; // success
  }
  int foundLength = 0;
  char *found = string.find(token, 0, m_type, &foundLength);
  while (found) 
  {
    int newLength = string.length + field.length - foundLength;
    if (newLength > string.bufferLength-1) // hasn't managed to replace all the instances as we run out of space
    {
      // this means we need to discover exactly how much space will be needed
      maxLength = string.length;
      do 
      {
        maxLength += field.length - foundLength;
      } while (found = string.find(token.chars, found + token.length, m_type, &foundLength));
      return maxLength; // we return but have stored the required data
    }
    // we don't want to move everything if we are going to move it all again in the next find... but keep it simple for now
    char *newPos = string.nextReplace(found, foundLength, field);
    found = string.find(token, newPos, m_type, &foundLength);
  };
  return 0;
}