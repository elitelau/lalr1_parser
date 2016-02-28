#include "err_report.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


const char* TokenTypeStr[] = {
	/* key words: int, if, else, char, return, while, for, break, continue, goto, float, read, write */
	"int", "if", "else", "char", "return", "while", "for", "break", "continue", "goto", "float", "void", "read", "write",

	/* operators*/
    "=", "+", "-", "*", "/", "%", // arithmetic operators: =, +, -, *, /, %
	"==", "!=", "<", ">", ">=", "<=",  // relational operations: ==, !=, <, >, >=, <=
	"!", "&&", "||", // logic operations: !, &&, || 

    /* punctuators: , ; : ( ) [ ] { } */
	",", ";", ":", "(", ")", "[", "]", "{", "}",

	/* identifier*/
	"ID",
	
	/* number*/
	"INT", "FLOAT", "NUM",
	
	/* char*/
	"CHAR",
	
	/* string literal*/
    "STRING_LITERAL",

	/* end of file*/
	"$",

	/* undefined or error token*/
	"ERR",

	/* pseudo-token are not really token*/
	"PSEUDO"
};

/****************************************************************************/
/* implementation of class ErrorReporter                                    */
/****************************************************************************/
int ErrorReporter::s_nMaxError = 200;

void ErrorReporter::Report( int nLineNo, const char* pErrBodyHead, const char* pErrBodyTail, int nErrInfo )
{
	assert( ID_ERR == (nErrInfo & 0xf0000) >> 16 );
	assert( pErrBodyTail - pErrBodyHead > 0 );

	char *pErrInfo = new char[ (pErrBodyTail - pErrBodyHead) / sizeof(char) + 200 ];
	memset( pErrInfo, 0, (pErrBodyTail - pErrBodyHead) / sizeof(char) + 200 );
	char *p = pErrInfo;

	sprintf( pErrInfo, "error(line %d): identifier\"", nLineNo );
	p += sizeof(pErrInfo);
	strncat( p, pErrBodyHead, (pErrBodyTail - pErrBodyHead) / sizeof(char) );
	p = pErrInfo + sizeof(pErrInfo);
	strcat( p, "\" exceeds predefined max length" );

	m_lstErrInfo.push_back( pErrInfo );

	delete []pErrInfo;
}

void ErrorReporter::Report( int nLineNo, int nErrInfo )
{
	assert( INVALID_TOK == ((nErrInfo & 0xff00000) >> 24) );
	char szErrInfo[200];

	switch( (nErrInfo & 0xff0000) >> 16  )
	{
	case STR_ERR:
		{
			switch( nErrInfo & 0xff )
			{
			case NEWLN_IN_CONSTANT:
				sprintf( szErrInfo, "error(line %d): newline in constant", nLineNo );
				break;
			case STR_EXCEED_LEN:
				sprintf( szErrInfo, "error(line %d): string literal exceeds the maximum chars it can represent", nLineNo );				
				break;
			default:
				sprintf( szErrInfo, "error(line %d): undefined string error ", nLineNo );  // !!it shouldn't happen
				break;
			}
			break;
		}
	case CHAR_ERR:
		{
			switch( nErrInfo & 0xff )
			{
			case CHAR_EMPTY:
				sprintf( szErrInfo, "error(line %d): empty character constant", nLineNo );
				break;
			case CHAR_INVALID_ESC_SEQ:
				sprintf( szErrInfo, "error(line %d): unknown escape sequence", nLineNo );
				break;
			case NEWLN_IN_CONSTANT:
				sprintf( szErrInfo, "error(line %d): missing terminating \' character", nLineNo );
				break; 
			case CHAR_EXCEED_LEN:
				sprintf( szErrInfo, "error(line %d): too many characters in constant", nLineNo );
				break;
			default:
				sprintf( szErrInfo, "error(line %d): undefined char error ", nLineNo );  // !!it shouldn't happen
				break;
			}
			break;
		}
	case NUM_ERR:
		{
			switch( nErrInfo & 0xff )
			{
			case NUM_BAD_SUFFIX:
				sprintf( szErrInfo, "error(line %d): bad suffix on number", nLineNo );
				break;
			case NUM_EXP_UNEXPECTED:
				sprintf( szErrInfo, "error(line %d): expected exponent value", nLineNo );
				break;
			case NUM_EXCEED_LEN:
				sprintf( szErrInfo, "error(line %d): token overflowed internal buffer", nLineNo );				
				break;
			default:
				sprintf( szErrInfo, "error(line %d): undefined NUM error ", nLineNo );  // !!it shouldn't happen
				break;
			}
			break;
		}
	case COMMENT_ERR:
		{
			if( COMMENT_ON_EOF == (nErrInfo & 0xff) )
			{
				sprintf( szErrInfo, "error(line %d): unexpected end of file find in comment", nLineNo );
				break;
			}
			else
			{
				sprintf( szErrInfo, "error(line %d): unknown comment error", nLineNo );
				break;
			}
			break;
		}
	case ID_ERR:
		{
			if(  ID_EXCEED_LEN == (nErrInfo & 0xff) )
			{
				sprintf( szErrInfo, "error(line %d): identifier exceeds the predefined length", nLineNo );
				break;
			}
			else
			{
				sprintf( szErrInfo, "error(line %d): unknown identifier error", nLineNo );
				break;
			}
			break;

		}
	case UNDEFINED_OP_ERR:
		sprintf( szErrInfo, "error(line %d): expect a complex operation, but just a \"&\" or \"|\" appeared", nLineNo );
		break;
	case UNDEFINED_SYM:
		sprintf( szErrInfo, "error(line %d): unknown character '0x%x'", nLineNo, (nErrInfo & 0xff) );
	    break;
	default:
		sprintf( szErrInfo, "error(line %d): unknown error", nLineNo );
		break;
	}

	m_lstErrInfo.push_back( szErrInfo );
}
