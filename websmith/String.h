#pragma once
#include <string.h>
// Warning, Strings may reference shared character buffers. If you use this class it is up to you to
// remember whether the String is using a shared buffer or its own. If in doubt create it new with newString()
// Also: strings are stored on the stack, so do not reallocate the string in a sub function
// Also: the chars array isn't guaranteed to be null terminated. To use it as a C string then call .cString(). Note this will permanently null terminate the string

// See here for how to use String class
void stringUnitTest(); 

// global functions (used when resizing/allocating is an option)
// new string with new memory, lasts for lifetime of the calling function
#define newString(_x_) (stringStack.tempString.set(_x_) ? String(stringStack.tempString, (char *)alloca(String::genBufferLen(stringStack.tempString.length))) : g_nullString)
// int or float to string etc
#define toString(_x_) String(_x_, (char *)alloca(20), 20)
// a string literal. Efficient as it doesn't require a strlen call
#define QUOTE(_x_) String(_x_, sizeof(_x_))
// replace all instances in _string_ of _token_ with _field_
#define replaceString(_string_, _token_, _field_) if(int bufLen = stringStack.stringReplacement.apply(_string_, _token_, _field_)) stringStack.stringReplacement.apply((char *)alloca(String::genBufferLen(bufLen))) 
// same as above but includes replacement option types: 1 = identifier, 2 = function
#define replaceStringType(_string_, _token_, _field_, _ID_) if(int bufLen = stringStack.stringReplacement.apply(_string_, _token_, _field_, _ID_)) stringStack.stringReplacement.apply((char *)alloca(String::genBufferLen(bufLen))) 
// allocates and reads string from file stream
#define readStringFromFile(_file_, _size_) ((stringStack.bufLen = String::genBufferLen(_size_)) ? String::fromFile(_file_, stringStack.bufLen, (char *)alloca(stringStack.bufLen)) : g_nullString)
// adds to the end of existing string, will allocate memory if exceeds the string's buffer length
#define append(_string_, _field_) if(int bufLen = stringStack.tryAppend(_string_, _field_)) stringStack.string->use(String(*stringStack.string + stringStack.tempString, (char *)alloca(String::genBufferLen(bufLen))))

class TempString;
enum StringType
{
  ST_Any,
  ST_JustInString,
  ST_NotInString,
  ST_AfterNumber, 
  ST_Identifiers, 
  ST_IdentifierBeforeCompound, // e.g. <blah> thing.other = 74;  replaces blah only when this . is found
  ST_Functions,
  ST_WordStart, // finds this string as the start of a word
  ST_AfterIdentifier,
  ST_ToLineEnd, // find the string startinng with that specified and going to the end of the line
  ST_Units, // special identification of units
  ST_NotSpaced, // string does not have space before and after it
  ST_Max
};
class String
{
public:
// Constructors
  String();
  String(const char *string);
  String(const char *string, int bufferLen);
  String(int number, char *buffer, int bufferLength);
  String(const String &data, char *buffer);
  String(const TempString &data, char *buffer);

// Accessors (they are const)
  bool startsWith(const String &string) const;
  char *find(const String &string, char *startPoint = NULL, StringType type = ST_Any, int *foundLength = NULL) const;
  // returns the string between start and end. If it doesn't find the start it will use the string's start, if it doesn't find end it will use the string's end, and return false
  String getStringBetween(const String &start, String &end, char **searchStart) const;
  // generic routine for cropping the start or the end or both (use NULL to not crop that end)
  String substring(char *start, char *end) const;
  // warning, this will alter the string buffer which might be shared, best to use this sparingly
  // also it sadly is disobeying the const 
  char *cString() const { if (chars[length])chars[length] = 0; /* terminate it */ return chars; }
  // concatenation
  TempString operator +(const TempString &string) const;
  TempString operator +(const String &string) const;

// Modifiers
  void insert(const TempString &tString, char *startPoint = 0);
  void convertFromHTML();
  // string will reference another's character data
  void use(const String &string);
  void use(const char *string);
  void use(const char *string, int maxBufferLen); // allows you to increase past the string length
  void set(const String &string); // replaces contents with string's up to its max buffer length
  void set(const TempString &string); // replaces contents with string's up to its max buffer length
  void reduce(const String &token, const String &field, StringType type = ST_Any);
  void removeComments(){ reduce(QUOTE("//"), QUOTE("\n"), ST_ToLineEnd); }
  // scopedReplacement("+", "(", ").plus(", ")") would replace  y = sqrt(3*x) + -50*b-2 with y = (sqrt(3*x)).plus(-50*b)-2
  void scopedReplacement(const String &token, const String &left, const String &between, const String &right, bool highPrecedence = false);
  char *nextScopedReplace(const String &token, const String &left, const String &between, const String &right, char *startPoint, bool highPrecedence = false);
  char *nextReplace(char *position, int lengthToReplace, const String &field);
// Statics
  static String fromFile(FILE *file, int size, char *buffer);
  static int genBufferLen(int length);

// Data, be careful if you are manipulating this raw data
  int length;
  int bufferLength;
  char *chars; // do not expect this to be null terminated
private:
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// internal classes
#define MAX_STRING_COLLECTION 15
class TempString // this is a temporary variable, you should not be able to change it
{
public:
  TempString();
  TempString(const String &str1);
  TempString operator +(const TempString &string) const;
  TempString operator +(const String &string) const;
  TempString *set(const TempString &strCol);
  TempString *set(const String &str);
  // these should just be private
  String strings[MAX_STRING_COLLECTION]; // max allowed
  int numStrings;
  int length;
};
TempString operator +(const char *, const TempString &string);

class StringReplacement
{
public:
  int apply(String &string, const String &token, const String &field, StringType type = ST_Any);
  void apply(char *memoryBuffer);
  String *m_string;
  String m_token;
  String m_field;
  StringType m_type; 
};
extern String g_nullString;

// add one of these at the top of each function that will allocate strings (uses the global macros)
struct StringStack 
{
  int tryAppend(String &str, const TempString &tempStr);
  int tryAppend(String &str, const String &other);
  String *string;
  TempString tempString;
  StringReplacement stringReplacement;
  int bufLen; // just temporary storage
};
extern __declspec(thread) StringStack stringStack;

// internal functions
int getStringLength(char *str);
int getStringLength(const String &str);
int getStringLength(const TempString &str);
