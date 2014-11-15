// webmate.cpp : Defines the entry point for the console application.
//
#include "HTTPRequest.h"
/*
void nextReplaceNumberShortcut(char *start, int length)
{
  
}
void replaceNumberShortcut(const String &string)
{
  char *next = nextReplaceNumberShortcut(string.chars, string.length);
  while (next)
  {
    char *next = nextReplaceNumberShortcut(next, (int)(string.chars + string.length - next));
  }
}*/

// this is a quick and dirty implementation of a units system that needs to be more thoroughly dealt with
void parseUnits(String &buffer)
{
  const int numBasicUnits = 17;
  char *basicUnits[numBasicUnits] = {
    "metres", "kilometres", "centimetres", "millimetres", "miles",
    "milliseconds", "seconds", "minutes", "hours", "days", "weeks", "years",
    "radians", "degrees", 
    "grams", "kilograms", "tonnes"};
  const int numAbbreviatedUnits = 11;
  char *abbreviatesUnits[numAbbreviatedUnits][2] = {
    {"mph", "miles per hour"}, {"kph", "kilometers per hour"}, {"kg", "kilograms"}, {"hertz", " per second"}, {"cm", "centimetres"},
    {"newtons", "kilogram metres per second^2"}, {"litres", "(0.1*metres)^3"}, {"millilitres", "centimetres^3"},
    // singular of above
    {"newton", "kilogram metres per second^2"}, {"litres", "(0.1*metre)^3"}, {"millilitre", "centimetre^3"}
  };

  for (int i = 0; i<numAbbreviatedUnits; i++)
  {
    String abbreviatedUnit(abbreviatesUnits[i][0]);
    replaceStringType(buffer, abbreviatedUnit, String(abbreviatesUnits[i][1]), ST_Units);
  }
  for (int i = 0; i<numBasicUnits; i++)
  {
    String units(basicUnits[i]);
    replaceStringType(buffer, units, newString("*(1/Math." + units + ")"), ST_Units); // the 1/ will become nativeUnits/ when we can work this out
    if (units.chars[units.length-1] == 's') // also replace the singular, as this is used sometimes
    {
      String unit = newString(units.substring(0, units.chars + units.length-1));
      replaceStringType(buffer, unit, newString("(1/Math." + units + ")"), ST_Units); // by ignoring the multiply it makes kg metres / second work.
    }
  }
  buffer.scopedReplacement(" per ", "", "/(", ")");  // lets try this instead

}

// convert from made up 'neutral' script into functioning javascript
void HTTPRequest::parseFunction(const String &function, String &buffer)
{
  buffer.set(function);
  if (buffer.length == 0)
    return; // no point in parsing this
  const int numVecs = 26;
  char *dimensionReplacements[numVecs] = 
  {
    "Position", "Velocity", "Acceleration", "Jerk", "Force", "Impulse", "Momentum", "AngularVelocity", "AngularAcceleration", "AngularMomentum", 
    "Torque", "TorqueImpulse", "Length", "Angle", "Multiplier", "TimePeriod", "Mass", "InverseMass", "Inertia", "InverseInertia", "Area", "Value", "Direction", "Frequency", 
    "Scale", "Orientation"
  };
  const int numRems = 7;
  char *cTypeRemoval[numRems] = // helps make simple c code run
  {
    "float", "unsigned int", "int", "double", "bool", "char *", "char"
  };

  buffer.removeComments();
  replaceStringType(buffer, "\n", "\\n\\\n", ST_JustInString);
  for (int i = 0; i<numVecs; i++)
  {
    replaceStringType(buffer, newString(dimensionReplacements[i]), "", ST_IdentifierBeforeCompound);
    replaceStringType(buffer, newString(dimensionReplacements[i]), "var", ST_Identifiers);
  }
  { // C compatibility
    replaceStringType(buffer, ".f", "", ST_AfterNumber); 
    replaceStringType(buffer, "f", "", ST_AfterNumber); 
    for (int i = 0; i<numRems; i++)
      replaceStringType(buffer, newString(cTypeRemoval[i]), "", ST_Identifiers);
  }
  replaceStringType(buffer, "in", "input", ST_NotSpaced); 

  // boolean operators
  replaceString(buffer, " and ", " && "); 
  replaceString(buffer, " or ", " || "); 
  replaceStringType(buffer, "not ", "!", ST_WordStart); 
  
 // replaceNumberShortcut(buffer);
//  replaceStringType(buffer, "<number>", "*", ST_BeforeIdentifier); 

  // ownership
  replaceStringType(buffer, "'s ", ".", ST_AfterIdentifier);
  
  parseUnits(buffer);

  // power
  buffer.scopedReplacement("^2", "Math.square(", ")", "", true); // we'll need to _add_ these functions to math's object
  buffer.scopedReplacement("^3", "Math.cube(", ")", "", true);
  buffer.scopedReplacement("^-1", "Math.invert(", ")", "", true); // as functions they could interpret several types, e.g. quats, matrices here
  buffer.scopedReplacement("^0.5", "Math.sqrt(", ")", "", true); // as functions they could interpret several types, e.g. quats, matrices here

  // unary object operator
  buffer.scopedReplacement(".-[identifier]", "", "([identifier]", ").op_negative()");

  // binary object operators
  buffer.scopedReplacement(" .[identifier] ", "(", ").[identifier](", ")"); 
  buffer.scopedReplacement(".*", "(", ").op_times(", ")"); 
  buffer.scopedReplacement("./", "(", ").op_divide(", ")");
  buffer.scopedReplacement(".+", "(", ").op_plus(", ")"); 
  buffer.scopedReplacement(".-", "(", ").op_minus(", ")"); 
  
  // in-place object operators
  buffer.scopedReplacement(".*=", "(", ").op_timesEquals(", ")");
  buffer.scopedReplacement("./=", "(", ").op_divideEquals(", ")");
  buffer.scopedReplacement(".+=", "(", ").op_plusEquals(", ")"); 
  buffer.scopedReplacement(".-=", "(", ").op_minusEquals(", ")");

  // Basic security guards, to prevent use of lower level functions
//  replaceStringType(buffer, "document", "dontuseplease", ST_Identifiers); 
  replaceStringType(buffer, "window", "dontuseplease", ST_Identifiers); 
  replaceStringType(buffer, "self", "dontuseplease", ST_Identifiers); 
  replaceStringType(buffer, "eval", "dontuseplease", ST_Functions); 
}
