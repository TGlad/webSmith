// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#include <winsock2.h>
#include <stdio.h>
#include <malloc.h>

#define BREAK  __debugbreak()  
#define ASSERT(exp) {if(!(exp)){ printf("%s(%i) ASSERT : %s \n", __FILE__, __LINE__, #exp); BREAK; } }


// TODO: reference additional headers your program requires here
