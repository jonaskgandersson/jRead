// main.c
//
// Test harness command-line for jRead 
//
//
#define _CRT_SECURE_NO_WARNINGS			// stop complaining about unsafe functions

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>


#include "jRead.h"


//-------------------------------------------------
// Do a query and print the results
//
void testQuery( char * pJson, char *query )
{
	struct jReadElement jElement;
	jRead( pJson, query, &jElement );
	printf( "Query: \"%s\"\n", query );
	printf( "return: %d= %s\n", jElement.error, jReadErrorToString(jElement.error) );
	printf( " dataType = %s\n", jReadTypeToString(jElement.dataType) );
	printf( " elements = %d\n", jElement.elements );
	printf( " bytelen  = %d\n", jElement.bytelen );
	printf( " value    = %*.*s\n\n", jElement.bytelen,jElement.bytelen, jElement.pValue );
}

//=================================================================
// Examples
// - whole bunch of jRead examples
//
void runExamples()
{
	long l;
	int i;
	double d;
	char str[128];
	struct jReadElement arrayElement;

	char * exampleJson=
		"{" 
		"  \"astring\": \"This is a string\",\n"
		"  \"number1\": 42,\n"
		"  \"number2\":  -123.45,\n"
		"  \"anObject\":{\"one\":1,\"two\":{\"obj2.1\":21,\"obj2.2\":22},\"three\":333},\n"
		"  \"anArray\":[0, \"one\", {\"two.0\":20,\"two.1\":21}, 3, [4,44,444]],\n"
		"  \"isnull\":null,\n"
		"  \"emptyArray\":[],\n"
		"  \"emptyObject\":{  },\n"
		"  \"yes\": true,\n"
		"  \"no\":  false\n"
		"}\n";

	testQuery( exampleJson, "" );
	testQuery( exampleJson, "[1" );
	testQuery( exampleJson, "{'astring'" );
	testQuery( exampleJson, "{'number1'" );
	testQuery( exampleJson, "{'number2'" );
	testQuery( exampleJson, "{'anObject'" );
	testQuery( exampleJson, "{'anArray'" );
	testQuery( exampleJson, "{'isnull'" );
	testQuery( exampleJson, "{'yes'" );
	testQuery( exampleJson, "{'no'" );
	testQuery( exampleJson, "{'missing'" );
	testQuery( exampleJson, "{'anObject'{'two'" );
	testQuery( exampleJson, "{'anObject' {'two' {'obj2.2'" );
	testQuery( exampleJson, "{'anObject'{'three'" );
	testQuery( exampleJson, "{'anArray' [1" );
	testQuery( exampleJson, "{'anArray' [2 {'two.1'" );
	testQuery( exampleJson, "{'anArray' [4 [2" );
	testQuery( exampleJson, "{'anArray' [999" );

	printf("Empty array or object...\n");
	testQuery( exampleJson, "{'emptyArray'" );
	testQuery( exampleJson, "{'emptyObject'" );

	printf("Return the key at a given index in an object...\n");
	testQuery( exampleJson, "{3" );
	testQuery( exampleJson, "{'anObject' {1" );
	testQuery( exampleJson, "{999" );

	// examples of helper functions
	l= jRead_long( exampleJson, "{'number1'", NULL );			// 42
	i= jRead_int( exampleJson, "{'yes'", NULL );				// 1	(BOOL example)
	d= jRead_double( exampleJson, "{'number2'", NULL );			// -123.45
	jRead_string( exampleJson, "{'astring'", str, 16, NULL );	// "This is a strin\0" (buffer too short example)

	printf("Helper Functions...\n" );
	printf("  \"number1\"= %ld\n", l );
	printf("  \"yes\"    = %d\n", i );
	printf("  \"number2\"= %lf\n", d );
	printf("  \"astring\"= \"%s\"\n", str );

	// Example of cascading queries
	printf("\nQueries on sub-elements and use of query parameters...\n");

	// locate "anArray"...
	jRead( exampleJson, "{'anArray'", &arrayElement );	
	printf("  \"anArray\": = %*.*s\n\n", arrayElement.bytelen,arrayElement.bytelen, arrayElement.pValue );

	// do queries within "anArray"...
	for( i=0; i < arrayElement.elements; i++ )
	{
		// index the array using queryParam
		jRead_string( (char *)arrayElement.pValue, "[*", str, 128, &i ); 
		printf("  anArray[%d] = %s\n", i, str );
	}

	// example using a parameter array
	{
		int params[2]={ 2, 1 };
		jRead_string( (char *)arrayElement.pValue, "[*{*", str, 128, params ); 
		printf("\n  anArray[%d] objectKey[%d] = \"%s\"\n", params[0], params[1], str );
	}

}

//=================================================================
//
// Example used in article
//
#define NAMELEN 32
struct NamesAndNumbers{
    char Name[NAMELEN];
    long Number;
};

void articleExample()
{
struct NamesAndNumbers people[42];
struct jReadElement element;
int i;
char *pJson=
"{"
  "\"Company\": \"The Most Excellent Example Company\","
  "\"Address\": \"Planet Earth\","
  "\"Numbers\":["
              "{ \"Name\":\"Fred\",   \"Ident\":12345 },"
              "{ \"Name\":\"Jim\",    \"Ident\":\"87654\" },"
              "{ \"Name\":\"Zaphod\", \"Ident\":\"0777621\" }"
            "]"
"}";
  jRead( pJson, "{'Numbers'", &element );    // we expect "Numbers" to be an array
  if( element.dataType == JREAD_ARRAY ) 
  {
      for( i=0; i<element.elements; i++ )    // loop for no. of elements in JSON
      {
          jRead_string( pJson, "{'Numbers'[*{'Name'", people[i].Name, NAMELEN, &i );
          people[i].Number= jRead_long( pJson, "{'Numbers'[*{'Ident'", &i ); 
      }
  }
  i=0;
}
//=================================================================
//
// Helper functions to read a JSON file into a malloc'd buffer with '\0' terminator
//
struct FileBuffer{
	unsigned long length;			// length in bytes
	unsigned char *data;			// malloc'd data, free with freeFileBuffer()
};
#define FILE_BUFFER_MAXLEN 1024*1024

unsigned long readFileBuffer( char *filename, struct FileBuffer *pbuf, unsigned long maxlen );
void freeFileBuffer( struct FileBuffer *buf );

// readFileBuffer
// - reads file into a malloc'd buffer with appended '\0' terminator
// - limits malloc() to maxlen bytes
// - if file size > maxlen then the function fails (returns 0)
//
// returns: length of file data (excluding '\0' terminator)
//          0=empty/failed
//
unsigned long readFileBuffer( char *filename, struct FileBuffer *pbuf, unsigned long maxlen )
{
  FILE *fp;
  int i;

	if( (fp=fopen(filename, "rb")) == NULL )
	{
		printf("Can't open file: %s\n", filename );
		return 0;
	}
	// find file size and allocate buffer for JSON file
	fseek(fp, 0L, SEEK_END);
	pbuf->length = ftell(fp);
	if( pbuf->length >= maxlen )
	{
		fclose(fp);
		return 0;
	}
	// rewind and read file
	fseek(fp, 0L, SEEK_SET);
	pbuf->data= (unsigned char *)malloc( pbuf->length + 1 );
	memset( pbuf->data, 0, pbuf->length+1 );	// +1 guarantees trailing \0

	i= fread( pbuf->data, pbuf->length, 1, fp );	
	fclose( fp );
	if( i != 1 )
	{
		freeFileBuffer( pbuf );
		return 0;
	}
	return pbuf->length;
}

// freeFileBuffer
// - free's buffer space and zeros it
//
void freeFileBuffer( struct FileBuffer *buf )
{
	if( buf->data != NULL )
		free( buf->data );
	buf->data= 0;
	buf->length= 0;
}

// read unsigned int from string
unsigned int fast_atoi( char *p )
{
    unsigned int x = 0;
    while (*p >= '0' && *p <= '9') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    return x;
};


//====================================================================================
//
// Simple speed test
//
void runSpeedTest()
{
	char linebuf[128];
	long runs=100000;
	DWORD tStart, tEnd;
	double elapsed;
	char * exampleJson=
		"{" 
		"  \"astring\": \"This is a string\",\n"
		"  \"number1\": 42,\n"
		"  \"number2\":  -123.45,\n"
		"  \"anObject\":{\"one\":1,\"two\":{\"obj2.1\":21,\"obj2.2\":22},\"three\":333},\n"
		"  \"anArray\":[0, \"one\", {\"two.0\":20,\"two.1\":21}, 3, [4,44,444]],\n"
		"  \"isnull\":null,\n"
		"  \"test\": [ 0,1,2,3,4,\n"
	    "            {\"ZERO\":0, \"ONE\":1, \"TWO\":2, \"THREE\":3,\n"
		"             \"TEST\":[0,\"one\",\"two\",1234]\n"
        "            },\n"
		"            6,7\n"
		"          ],\n"
		"  \"yes\": true,\n"
		"  \"no\":  false\n"
		"}\n";
	char * exampleQuery="{'test' [5 {'TEST' [3";
	int i, value;
	printf("\n\nSimple Speed test...\nUsing this JSON:\n%s\n", exampleJson );
	printf("we run this query: jRead_int( exampleJson, \"%s\", NULL)\n", exampleQuery );

	printf("\nEnter no. of times to run query (default=100000): " );
	gets( linebuf );
	sscanf( linebuf, "%ld", &runs );

	printf("Running %ld times... ", runs );

	tStart= GetTickCount();
	for( i=0; i<runs; i++ )
	{
		value= jRead_int( exampleJson, exampleQuery, NULL );
		if( value != 1234 )
			printf(" Big FAIL!" );
	}
	tEnd= GetTickCount();
	elapsed= (double)(tEnd-tStart);
	printf("\n...Done in %3.2lf secs (av. %3.2lf uS/query)\n\n", elapsed/1000.0, (elapsed*1000.0)/(double)runs );

	printf("\n\nSpeed Comparisons:\n");
	printf(" 3.4GHz i7 8-core=  770,000 queries/sec = 1.3uS per query = ~ 50nS per simple element\n");
	printf(" 3.4GHz Pentium D=  245,000 queries/sec = 4.1uS per query = ~170nS per simple element\n");
}

//=================================================================
//
// 10,000 entry reading test
// - compares jRead indexed query with jReadArrayStep()
//
void runLongJsonTest()
{
	struct FileBuffer json;
	struct jReadElement arrayElement, objectElement;
	char *filename= "TESTJSON.json";
	char *query= "[*{'Users'";
	int i;
	char *pJsonArray;
	DWORD tStart, tEnd;
	double elapsed;
	int *UserCounts;

	if( readFileBuffer( filename, &json, FILE_BUFFER_MAXLEN ) == 0 )
	{
		printf("Can't open file: %s\n", filename );
		return;
	}
	printf("\nLong JSON file test: read %s ok\n\n", filename );

	// identify the whole JSON element

	printf("Identifying the whole of the JSON 1000 times...");
	tStart= GetTickCount();
	for( i=0; i<1000; i++ )
		jRead( (char *)json.data, "", &arrayElement );
	tEnd= GetTickCount();
	elapsed= (double)(tEnd-tStart);
	printf("\n...Done in %3.2lf secs (av. %3.2lf mS per run)\n", elapsed/1000.0, elapsed/1000.0 );
	printf("JSON is array of %d elements\n\n", arrayElement.elements );

	// application wants "Users" values in this array
	UserCounts= (int *)malloc( sizeof(int)*arrayElement.elements );


	printf("Starting %d separate \"%s\"queries... ", arrayElement.elements, query );
	tStart= GetTickCount();
	// perform query on JSON file
	// - access each array by indexing
	//
	for( i=0; i<arrayElement.elements; i++ )
	{
		UserCounts[i]= jRead_int( (char *)json.data, query, &i  );
	}
	tEnd= GetTickCount();
	elapsed= (double)(tEnd-tStart);
	printf("\n...Done in %3.2lf secs (av. %3.2lf mS/query)\n\n", elapsed/1000.0, elapsed/(double)arrayElement.elements );

	//
	// Now using jReadArrayStep()...
	//

	printf("Now using jReadArrayStep()...\n");
	printf("Perform the %d-steps thru array 1000 times...", arrayElement.elements);
	tStart= GetTickCount();
	for( i=0; i<1000; i++ )
	{
		// identify the whole JSON element
		if( arrayElement.dataType == JREAD_ARRAY )
		{
			int step;
			pJsonArray= (char *)arrayElement.pValue;
			for( step=0; step<arrayElement.elements; step++ )
			{
				pJsonArray= jReadArrayStep( pJsonArray, &objectElement );
				if( objectElement.dataType == JREAD_OBJECT )
				{
					UserCounts[step]= jRead_int( (char *)objectElement.pValue, "{'Users'", NULL  );
				}else
				{
					printf("Array element wasn't an object!\n");
				}
			}
		}
	}
	tEnd= GetTickCount();
	elapsed= (double)(tEnd-tStart);
	printf("\n...Done in %3.2lf secs (av. %3.2lf mS per %d elements read, %3.2lf uS/element)\n\n", 
			elapsed/1000.0, elapsed/1000.0, arrayElement.elements,
			elapsed/(double)arrayElement.elements);
	printf("Done\n\n");

	freeFileBuffer( &json );
	return;
}

//====================================================================================
// Functions for command-ine interface
//

void printHelp()
{
	printf("jRead - an in-place JSON element reader\n");
	printf("usage:\n");
	printf("  jRead t        runs built-in test examples\n");
	printf("  jRead s        simple speed test\n");
	printf("  jRead l        run Long file test (using TESTJSON.json)\n");
	printf("  jRead <jsonFileName> \"query String\"\n" );
	printf("e.g.\n");
	printf("  jRead jReadExample.json \"{'astring'\"\n");
};

//-------------------------------------------------
// Command-line interface
// usage:
//  jRead ?			prints help text
//  jRead t			runs test examples
//
//	jRead jsonfile "query string"
//  - reads jsonfile and executes "query string"
//
int main(int argc, char * argv[])
{
	struct FileBuffer json;

	articleExample();	// look at this in debug

	if( argc == 2 )
	{
		switch( *argv[1] )
		{
		case '?':	printHelp(); break;
		case 't':	runExamples(); break;
		case 's':	runSpeedTest();	break;
		case 'l':	runLongJsonTest(); break;
		}
		return 0;
	}

	if( argc != 3 )
	{
		printHelp();
		return 1;
	};

	if( readFileBuffer( argv[1], &json, FILE_BUFFER_MAXLEN ) == 0 )
	{
		printf("Can't open file: %s\n", argv[1] );
		return 1;
	}

	// perform query on JSON file
	testQuery( (char *)json.data, argv[2] );

	freeFileBuffer( &json );
	return 0;
}

