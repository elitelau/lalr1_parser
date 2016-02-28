#ifndef _ERR_REPORT_H
#define _ERR_REPORT_H

#include <string>
#include <list>

#include <iostream>
using namespace std;

// 第一级错误 				   
enum FirstlyErrConstants  
{ 
	INVALID_TOK = 1 ,     // token that cannot recognized 
	BLOCK_END_OCCUR = 7   // reserved for '\0' delimiter when read a block of strings rather just a line of string( undefined here)
};

// 第二级错误
enum SecondlyErrConstants  
{ 
	UNKNOWN_ERR = 0x0,
	STR_ERR = 0x1, 
	CHAR_ERR = 0x2, 
	NUM_ERR = 0x3, 
	COMMENT_ERR = 0x4, 
	ID_ERR = 0x5, 

	UNDEFINED_OP_ERR = 0x6, /* expect a complex operation, but just a "&" or "|" appeared */

	// !此时，最后一个字节用来存储ASCII值
	UNDEFINED_SYM = 0x11, /*undefined symbols like :'@'*/

//	0x6, // undefined
//	0x7, 
//	0x8 
};

// 第三级错误(具体错误类型)
enum DetailErrConstants  
{ 
	ID_EXCEED_LEN = 0x1,  /* an identifier exceeds the predefined length */
	CHAR_EXCEED_LEN = 0x1, /* too many characters in a char constant */
	NUM_BAD_SUFFIX = 0x1, /* bad suffix on number*/
	STR_EXCEED_LEN = 0x1, /* string exceeds predefined length */

	COMMENT_ON_EOF = 0x1, /* unexpected end of file find in comment*/

	NUM_EXP_UNEXPECTED = 0x2, /* predict exponent part, but not the fact*/
	CHAR_EMPTY	= 0x2, /* no characters in a char constant */

	CHAR_INVALID_ESC_SEQ = 0x3, /*invalid escape sequence*/
	NUM_EXCEED_LEN = 0x3, /*exceeds the buffer can represent*/

	NUM_INT_OVERFLOW = 0x4, /* an integer number exceeds the word length*/
	NUM_FLOAT_OVERFLOW = 0x5, /* a float number exceeds the word length */

	NEWLN_IN_CONSTANT = 0x7, /* occured when an unclosed char constant or string literal ends with newline*/

};

extern const char* TokenTypeStr[];

class ErrorReporter
{
public:
	static ErrorReporter* Instance()
	{
		static ErrorReporter reporter;
		return &reporter;
	}

	ErrorReporter() : m_nErrCount(0)
	{}

	// lexical error reporting format
	void Report( int nLineNo, const char* pErrBodyHead, const char* pErrBodyTail, int nErrInfo );
	void Report( int nLineNo, int nErrInfo );

	// get error count 
	int GetErrCount(void)
	{
		return m_lstErrInfo.size();
	}

	// used for test only
	void PrintErrInfo()
	{
		list<string>::iterator it;
		string * pStr = NULL;


		for( it = m_lstErrInfo.begin(); it != m_lstErrInfo.end(); ++it )
		{
			pStr = &(*it);
			cout << *pStr << endl;
		}
	}

	/*
	const char* operator[](int index)
	{
		if( index < 0 || index < m_lstErrInfo.size() )
			return NULL;
		list<string>::iterator it = m_lstErrInfo.begin();
		int i = 0;

		while( it != m_lstErrInfo.end() && i <= index )
		{
		}
	}*/

	void Clear()
	{
		m_lstErrInfo.clear();
	}

private:
	list<string>	m_lstErrInfo;	// all the error info
	int				m_nErrCount;	// count the number of error

	static	int		s_nMaxError;	// the maximum number of errors can report
};

#endif

