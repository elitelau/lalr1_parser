#include "scanner.h"
#include "err_report.h"
#include <string.h>
#include <stdio.h>

unsigned int g_char2kindMap[256] = { /* 000 nul */	0,
				   /* 001 soh */	0,
				   /* 002 stx */	0,
				   /* 003 etx */	0,
				   /* 004 eot */	0,
				   /* 005 enq */	0,
				   /* 006 ack */	0,
				   /* 007 bel */	0,
				   /* 010 bs  */	0,
				   /* 011 ht  */	BLANK,
				   /* 012 nl  */	NEWLINE,
				   /* 013 vt  */	BLANK,
				   /* 014 ff  */	BLANK,
				   /* 015 cr  */	0,
				   /* 016 so  */	0,
				   /* 017 si  */	0,
				   /* 020 dle */	0,
				   /* 021 dc1 */	0,
				   /* 022 dc2 */	0,
				   /* 023 dc3 */	0,
				   /* 024 dc4 */	0,
				   /* 025 nak */	0,
				   /* 026 syn */	0,
				   /* 027 etb */	0,
				   /* 030 can */	0,
				   /* 031 em  */	0,
				   /* 032 sub */	0,
				   /* 033 esc */	0,
				   /* 034 fs  */	0,
				   /* 035 gs  */	0,
				   /* 036 rs  */	0,
				   /* 037 us  */	0,
				   /* 040 sp  */	BLANK,
				   /* 041 !   */	OTHER,
				   /* 042 "   */	INDICATOR,
				   /* 043 #   */	0,        // undefined symbol in MinusC
				   /* 044 $   */	0,
				   /* 045 %   */	OTHER,
				   /* 046 &   */	INDICATOR,		  // undefined symbol in MinusC, except for "&&"
				   /* 047 '   */	INDICATOR,    // e.g 'a'
				   /* 050 (   */	OTHER,
				   /* 051 )   */	OTHER,
				   /* 052 *   */	OTHER,
				   /* 053 +   */	OTHER,
				   /* 054 ,   */	OTHER,
				   /* 055 -   */	OTHER,
				   /* 056 .   */	INDICATOR,		// undefined symbol in MinusC, except for introduction a float-part
				   /* 057 /   */	OTHER,
				   /* 060 0   */	DIGIT,
				   /* 061 1   */	DIGIT,
				   /* 062 2   */	DIGIT,
				   /* 063 3   */	DIGIT,
				   /* 064 4   */	DIGIT,
				   /* 065 5   */	DIGIT,
				   /* 066 6   */	DIGIT,
				   /* 067 7   */	DIGIT,
				   /* 070 8   */	DIGIT,
				   /* 071 9   */	DIGIT,
				   /* 072 :   */	OTHER,
				   /* 073 ;   */	OTHER,
				   /* 074 <   */	OTHER,
				   /* 075 =   */	OTHER,
				   /* 076 >   */	OTHER,
				   /* 077 ?   */	0,		// undefined symbol in MinusC
				   /* 100 @   */	0,
				   /* 101 A   */	LETTER,
				   /* 102 B   */	LETTER,
				   /* 103 C   */	LETTER,
				   /* 104 D   */	LETTER,
				   /* 105 E   */	LETTER,
				   /* 106 F   */	LETTER,
				   /* 107 G   */	LETTER,
				   /* 110 H   */	LETTER,
				   /* 111 I   */	LETTER,
				   /* 112 J   */	LETTER,
				   /* 113 K   */	LETTER,
				   /* 114 L   */	LETTER,
				   /* 115 M   */	LETTER,
				   /* 116 N   */	LETTER,
				   /* 117 O   */	LETTER,
				   /* 120 P   */	LETTER,
				   /* 121 Q   */	LETTER,
				   /* 122 R   */	LETTER,
				   /* 123 S   */	LETTER,
				   /* 124 T   */	LETTER,
				   /* 125 U   */	LETTER,
				   /* 126 V   */	LETTER,
				   /* 127 W   */	LETTER,
				   /* 130 X   */	LETTER,
				   /* 131 Y   */	LETTER,
				   /* 132 Z   */	LETTER,
				   /* 133 [   */	OTHER,
				   /* 134 \   */	0,       // undefined symbol in MinusC, except for introduction an escape sequence
				   /* 135 ]   */	OTHER,
				   /* 136 ^   */	0,       // undefined symbol in MinusC
				   /* 137 _   */	LETTER,
				   /* 140 `   */	0,
				   /* 141 a   */	LETTER,
				   /* 142 b   */	LETTER,
				   /* 143 c   */	LETTER,
				   /* 144 d   */	LETTER,
				   /* 145 e   */	LETTER,
				   /* 146 f   */	LETTER,
				   /* 147 g   */	LETTER,
				   /* 150 h   */	LETTER,
				   /* 151 i   */	LETTER,
				   /* 152 j   */	LETTER,
				   /* 153 k   */	LETTER,
				   /* 154 l   */	LETTER,
				   /* 155 m   */	LETTER,
				   /* 156 n   */	LETTER,
				   /* 157 o   */	LETTER,
				   /* 160 p   */	LETTER,
				   /* 161 q   */	LETTER,
				   /* 162 r   */	LETTER,
				   /* 163 s   */	LETTER,
				   /* 164 t   */	LETTER,
				   /* 165 u   */	LETTER,
				   /* 166 v   */	LETTER,
				   /* 167 w   */	LETTER,
				   /* 170 x   */	LETTER,
				   /* 171 y   */	LETTER,
				   /* 172 z   */	LETTER,
				   /* 173 {   */	OTHER,
				   /* 174 |   */	INDICATOR,      // undefined symbol in MinusC, except for  "||"
				   /* 175 }   */	OTHER,
				   /* 176 ~   */	0  };   // undefined symbol in MinusC

/**************************\
 * SS_Comment_DFA
\**************************/
void Scanner::Comment_DFA::getToken( Token*& tok )
{
	bool bExitingComment = false;
	bool bReadInNewLn = false;


	if(*m_refScanner.m_pTokenIter == '/')
	{
		// !!! the scan process will encounter '\0' no matter a line ends with a NEWLINE or an EOF
		while( '\0' != *++m_refScanner.m_pTokenIter )
			;

		tok->_tok = PSEUDO;
	}
	else if( *m_refScanner.m_pTokenIter == '*' )
	{
		for(;;)
		{
			// added by liujian, 2007.7.30
			if( !bReadInNewLn )
			{
				++m_refScanner.m_pTokenIter;
			}
			else
				bReadInNewLn = false;  //!!! when read in a new line, scan from the first letter

			if( '*' == *m_refScanner.m_pTokenIter )
				bExitingComment = true;
			else if( '\0' == *m_refScanner.m_pTokenIter )  // !!! the scan process will encounter '\0' no matter a line ends with a NEWLINE or an EOF
			{
				memset( m_refScanner.m_pLnBuffer, 0, s_nLnBufferSize );
				if( fgets( m_refScanner.m_pLnBuffer, s_nLnBufferSize, m_refScanner.m_fSource ) )
				{
					m_refScanner.m_pTokenIter = m_refScanner.m_pLnBuffer;
					++m_refScanner.m_nLineNo;
				}
				else
				{
					// error type: unclosed comment
					tok->_tok = ERR;
					tok->_val._nIntVal |= (INVALID_TOK << 24);
					tok->_val._nIntVal |= (COMMENT_ERR << 16);
					tok->_val._nIntVal |= (COMMENT_ON_EOF);
					ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
					break;
				}

				bReadInNewLn = true; // added by liujian 2007.7.30
				bExitingComment = false;
			}
			else if( '/' == *m_refScanner.m_pTokenIter )
			{
				if( bExitingComment )
				{
					tok->_tok = PSEUDO;

					++m_refScanner.m_pTokenIter; // !!!!must need!!!   consume '/' symbol

					break; // closed comment
				}
				else
					bExitingComment = false;
			}
			else
			{
				bExitingComment = false;
			}
		}
	}
	
//	m_refScanner.m_pTokenHead = m_refScanner.m_pTokenIter;// do this in the outer invoker
}


/**************************\
 * STRING_DFA
\**************************/

int Scanner::STRING_DFA::s_nMaxStrLength = 32;

void Scanner::STRING_DFA::getToken( Token*& tok )
{
	int nCharCount = 0;

	++m_refScanner.m_pTokenIter;

	// !!! the scan process will encounter '\0' no matter a line ends with a NEWLINE or an EOF
	while( '"' !=  *m_refScanner.m_pTokenIter && '\0' != *m_refScanner.m_pTokenIter ) 
		++m_refScanner.m_pTokenIter;

	if( *m_refScanner.m_pTokenIter != '"' )
	{
		//!!! when scan token in the last line of source file, it maybe interrupted when encounter the EOF
		//!!! so the line of string ended with '\0' and no newline symbol encountered   

		// error type: newline in a constant
		tok->_tok = ERR;
		tok->_val._nIntVal |= (INVALID_TOK << 24);
		tok->_val._nIntVal |= (STR_ERR << 16);
		tok->_val._nIntVal |= (NEWLN_IN_CONSTANT);

		ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
	}
	else
	{
		nCharCount = ( m_refScanner.m_pTokenIter - m_refScanner.m_pTokenHead -1 ) / sizeof(char);
		assert( nCharCount >= 0 );

		if( nCharCount > s_nMaxStrLength )
		{
			// error type: too many characters int a char constant
			tok->_tok = ERR;
			tok->_val._nIntVal |= (INVALID_TOK << 24);
			tok->_val._nIntVal |= (STR_ERR << 16);
			tok->_val._nIntVal |= (STR_EXCEED_LEN);

			ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
		}
		else
		{
			tok->_tok = STRING_LITERAL;
			tok->_val._strVal = new char[nCharCount + 1];
			memset( tok->_val._strVal, 0, nCharCount+1 );
			strncpy( tok->_val._strVal, m_refScanner.m_pTokenHead+1, nCharCount ); 
		}

		++m_refScanner.m_pTokenIter; // consume '"' symbol
	}	
}

/**************************\
 * CHAR_DFA
\**************************/
int Scanner::CHAR_DFA::s_nSeqMaxLength = 4;
char Scanner::CHAR_DFA::s_ReservedEscSeq[10] = { '\0' };

void Scanner::CHAR_DFA::getToken( Token*& tok )
{
	int nCharCount = 0;

	++m_refScanner.m_pTokenIter;

	// !!! the scan process will encounter '\0' no matter a line ends with a NEWLINE or an EOF
	while( '\'' !=  *m_refScanner.m_pTokenIter && '\0' != *m_refScanner.m_pTokenIter )  
		++m_refScanner.m_pTokenIter;

	if( *m_refScanner.m_pTokenIter != '\'' )
	{
		//!!! when scan token in the last line of source file, it maybe interrupted when encounter the EOF
		//!!! so the line of string ended with '\0' and no newline symbol encountered   

		// error type: newline in a constant
		tok->_tok = ERR;
		tok->_val._nIntVal |= (INVALID_TOK << 24);
		tok->_val._nIntVal |= (CHAR_ERR << 16);
		tok->_val._nIntVal |= (NEWLN_IN_CONSTANT);

		ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
	}
	else
	{
		nCharCount = ( m_refScanner.m_pTokenIter - m_refScanner.m_pTokenHead -1 ) / sizeof(char);
		assert( nCharCount >= 0 );

		if( 0 == nCharCount )
		{
			// error type: no characters int a char constant
			tok->_tok = ERR;
			tok->_val._nIntVal |= (INVALID_TOK << 24);
			tok->_val._nIntVal |= (CHAR_ERR << 16);
			tok->_val._nIntVal |= (CHAR_EMPTY);

			ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );

			goto Exit_Func;
		}

		if(  nCharCount > s_nSeqMaxLength )
		{
			// error type: too many characters int a char constant
			tok->_tok = ERR;
			tok->_val._nIntVal |= (INVALID_TOK << 24);
			tok->_val._nIntVal |= (CHAR_ERR << 16);
			tok->_val._nIntVal |= (CHAR_EXCEED_LEN);

			ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );

			goto Exit_Func;
		}

		if( m_refScanner.m_pTokenHead[1] == '\\' )  // deal with escape sequence
		{
			if( nCharCount > 2 )
			{
				// error type: invalid escape sequence
				tok->_tok = ERR;
				tok->_val._nIntVal |= (INVALID_TOK << 24);
				tok->_val._nIntVal |= (CHAR_ERR << 16);
				tok->_val._nIntVal |= (CHAR_EXCEED_LEN);

				ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );

				goto Exit_Func;
			}
			else if( 2 == nCharCount )
			{
				switch( m_refScanner.m_pTokenHead[2] )
				{
				case 'n':
					tok->_tok = CHAR;
					tok->_val._chVal = 0xa; // linefeed
					break;
				case 't':
					tok->_tok = CHAR;
					tok->_val._chVal = 0x9; // table
					break;
				case 'r':
					tok->_tok = CHAR;
					tok->_val._chVal = 0xd; // carriage return
					break;
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					tok->_tok = CHAR;
					tok->_val._chVal = 0x0; // '\0'
					break;
				default:
					tok->_tok = ERR;
					tok->_val._nIntVal |= (INVALID_TOK << 24);
					tok->_val._nIntVal |= (CHAR_ERR << 16);
					tok->_val._nIntVal |= (CHAR_INVALID_ESC_SEQ);

					ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );

					break;
				}

				goto Exit_Func;
			}
		} // if( m_pTokenHead[1] == '\' )

		tok->_tok = CHAR;
		tok->_val._chVal = m_refScanner.m_pTokenHead[1];
	}

Exit_Func:
	if( *m_refScanner.m_pTokenIter == '\'' )
		++m_refScanner.m_pTokenIter; // consume '\'' symbol
}

/**************************\
 * NUM_DFA
\**************************/
int Scanner::NUM_DFA::s_nWordLength = 16;
unsigned int Scanner::NUM_DFA::s_nBufferMaxSize = 32;

void Scanner::NUM_DFA::getToken( Token*& tok )
{
	unsigned int nCharCount = 0;

	while( DIGIT & g_char2kindMap[static_cast<int>(*++m_refScanner.m_pTokenIter)] )
	;

	if('.' == *m_refScanner.m_pTokenIter || 'E' == *m_refScanner.m_pTokenIter) 
	{
		// enter float part, or exponent part
		;
	}
	else
	{ 
		if( LETTER == g_char2kindMap[static_cast<int>(*m_refScanner.m_pTokenIter)] )
		{
			// error type: bad suffix on number
			tok->_tok = ERR; 
			tok->_val._nIntVal |= (INVALID_TOK << 24);
			tok->_val._nIntVal |= (NUM_ERR << 16);
			tok->_val._nIntVal |= NUM_BAD_SUFFIX;

			ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
		}
		else
		{
			// an integer token
			tok->_tok = INT_NUM;
		}

		goto EXIT_FUNC;
	} 

	if( '.' == *m_refScanner.m_pTokenIter )
	{ 
		while( DIGIT & g_char2kindMap[static_cast<int>(*++m_refScanner.m_pTokenIter)] )
			;

		if( 'E' == *m_refScanner.m_pTokenIter )
		{
EXP_PART:
			if( '+' == *m_refScanner.m_pTokenIter || '-' == *m_refScanner.m_pTokenIter )
				++m_refScanner.m_pTokenIter;

			if( DIGIT & g_char2kindMap[static_cast<int>(*++m_refScanner.m_pTokenIter)] )
			{
				while( DIGIT & 
                       g_char2kindMap[static_cast<int>(*++m_refScanner.m_pTokenIter)] )
				;

				if( LETTER & g_char2kindMap[static_cast<int>(*m_refScanner.m_pTokenIter)] )
				{
					// error type: bad suffix on number
					tok->_tok = ERR; 
					tok->_val._nIntVal |= (INVALID_TOK << 24);
					tok->_val._nIntVal |= (NUM_ERR << 16);
					tok->_val._nIntVal |= (NUM_BAD_SUFFIX);

					ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
				}
				else
				{
					tok->_tok = FLOAT_NUM; // float number with E-part(exp part)
				}
			}
			else
			{
				// error type: predict exponent part, but not the fact
				tok->_tok = ERR; 
				tok->_val._nIntVal |= (INVALID_TOK << 24);
				tok->_val._nIntVal |= (NUM_ERR << 16);
				tok->_val._nIntVal |= (NUM_EXP_UNEXPECTED); 

				ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
			}
			
		}  // E-part
		else 
		{
			if( LETTER & g_char2kindMap[static_cast<int>(*m_refScanner.m_pTokenIter)] )
			{
				// error type: bad suffix on number
				tok->_tok = ERR; 
				tok->_val._nIntVal |= (INVALID_TOK << 24);
				tok->_val._nIntVal |= (NUM_ERR << 16);
				tok->_val._nIntVal |= (NUM_BAD_SUFFIX); 	

				ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
			}
			else
			{
				tok->_tok = FLOAT_NUM; // float number without E-part(exp part)
			}
		} // else( 'E' == *m_pTokenIter )
	}
	else if( 'E' == *m_refScanner.m_pTokenIter )
	{
		goto EXP_PART;
	}


EXIT_FUNC:
	if( INT_NUM == tok->_tok || FLOAT_NUM == tok->_tok )
	{
		nCharCount = m_refScanner.m_pTokenIter-m_refScanner.m_pTokenHead;

		if( nCharCount > s_nBufferMaxSize )
		{
			tok->_tok = ERR; 
			tok->_val._nIntVal |= (INVALID_TOK << 24);
			tok->_val._nIntVal |= (NUM_ERR << 16);
			tok->_val._nIntVal |= (NUM_EXCEED_LEN); 	

			ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
		}
		else
		{
			tok->_val._strVal = new char[nCharCount+1];
			memset( tok->_val._strVal, 0, nCharCount + 1 );
			strncpy( tok->_val._strVal, m_refScanner.m_pTokenHead, nCharCount );
		}
	}

//	if( *m_refScanner.m_pTokenIter == 0xa)
//	{
//		++m_refScanner.m_pTokenIter;
//	}
}

/**************************\
 * ID_DFA
\**************************/

int	Scanner::ID_DFA::s_nMaxIdLength = 32;
const char* Scanner::ID_DFA::s_Keywords[] =
{
	"if",
	"else",
	"for",
	"while",
	"return",
	"continue",
	"break",
	"char",
	"goto",
	"int",
	"float",
	"void",
	"read",
	"write",
	NULL
};

void Scanner::ID_DFA::getToken(Token*& tok)
{
	const char* keyword = NULL;
	char temp = '\0';

	while( (DIGIT|LETTER) & 
           g_char2kindMap[static_cast<int>(*++m_refScanner.m_pTokenIter)] )
		;

	if( (m_refScanner.m_pTokenIter - m_refScanner.m_pTokenHead) / sizeof(char) > s_nTokenMaxLen )
	{
		tok->_tok = ERR; //Id exceeds predefined max length
		tok->_val._nIntVal |= (INVALID_TOK << 24);
		tok->_val._nIntVal |= (ID_ERR << 16);
		tok->_val._nIntVal |= ID_EXCEED_LEN;

		ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, 
                                           m_refScanner.m_pTokenHead, 
                                           m_refScanner.m_pTokenIter, 
										   tok->_val._nIntVal );
	}
	else
	{
		assert( m_refScanner.m_pTokenIter > m_refScanner.m_pTokenHead );

		temp = *m_refScanner.m_pTokenIter;
		*m_refScanner.m_pTokenIter = '\0'; // !!!!set m_pTokenIter '\0' for convenience and reduce the time cost of copy strings between to pointers

		//whether token is a keyword
		// !!!modified by liujian 2007.7.31
		keyword = getKeyword(m_refScanner.m_pTokenHead); // m_hashKeywords.match( m_refScanner.m_pTokenHead );

		*m_refScanner.m_pTokenIter = temp; // !!! resume its original value		 

		if( NULL != keyword )
		{
			switch(*keyword)
			{
			case 'i':
				if( *++keyword == 'n' )
					tok->_tok = K_INT;
				else
					tok->_tok = K_IF;
				break;
			case 'e':
				tok->_tok = K_ELSE;
				break;
			case 'f':
				if(*++keyword == 'o')
					tok->_tok = K_FOR;
				else
					tok->_tok = K_FLOAT;
				break;
			case 'w':
				if(*++keyword == 'h')
					tok->_tok = K_WHILE;
				else
					tok->_tok = F_WRITE;
				break;
			case 'b':
				tok->_tok = K_BREAK;
				break;
			case 'c':
				if( *++keyword == 'h' )
					tok->_tok = K_CHAR;
				else
					tok->_tok = K_CONTINUE;
				break;
			case 'g':
				tok->_tok = K_GOTO;
				break;
			case 'r':
				if( keyword[2] == 'a' )
					tok->_tok = F_READ;
				else
					tok->_tok = K_RET;
				break;
			case 'v':
				tok->_tok = K_VOID;
				break;
			default: // should not happen
				assert( 0 ); 
				tok->_tok = ERR;
				ErrorReporter::Instance()->Report( m_refScanner.m_nLineNo, tok->_val._nIntVal );
				break;
			}
		}
		else // ID, do not consume any delimiter
		{
			tok->_tok = ID;
			tok->_val._strVal = new char[m_refScanner.m_pTokenIter-m_refScanner.m_pTokenHead+1];
			memset( tok->_val._strVal, 0, m_refScanner.m_pTokenIter-m_refScanner.m_pTokenHead+1 );
			strncpy( tok->_val._strVal, m_refScanner.m_pTokenHead, m_refScanner.m_pTokenIter-m_refScanner.m_pTokenHead );
		}
	}

//	if( *m_refScanner.m_pTokenIter == 0xd )
//	{
//		++m_refScanner.m_pTokenIter;
//	}
}

/*************************************************************************************/
/* implementation of class Scanner                                                   */
/*************************************************************************************/
int Scanner::s_nLnBufferSize = 2049; // refer to the line's max length in vc++6.0 compiler(2049)
unsigned int Scanner::s_nTokenMaxLen = 32;


Scanner::Scanner(FILE* f) : m_dfaIdentifier(*this), m_dfaNumerics(*this), m_dfaChar(*this), m_dfaString(*this),
							m_dfaComment(*this),
							m_pLnBufferTail(NULL)
{
	assert( f != NULL );
	m_fSource = f;

	m_pLnBuffer = new char[s_nLnBufferSize];
	memset( m_pLnBuffer, 0, s_nLnBufferSize );

	fgets( m_pLnBuffer, s_nLnBufferSize, m_fSource );
	m_pTokenHead = m_pTokenIter = m_pLnBuffer;

	m_nLineNo = 1;
}

Scanner::~Scanner()
{
	delete m_pLnBuffer;
}


#define COMPLEX_OPERATOR_DFA( ch1stSymbol, ch2ndSymbol, singleOperator, complexOperator )	\
{\
	if( ch1stSymbol == *m_pTokenIter ){	\
		++m_pTokenIter;	\
		if( ch2ndSymbol == *m_pTokenIter ){	\
			tok->_tok = complexOperator;	\
			++m_pTokenIter;}\
		else {\
			if( INDICATOR == g_char2kindMap[static_cast<int>(*m_pTokenHead)] ){	\
				tok->_tok = ERR;	\
				tok->_val._nIntVal |= (INVALID_TOK << 24);	\
				tok->_val._nIntVal |= (UNDEFINED_OP_ERR << 16);	\
				ErrorReporter::Instance()->Report( m_nLineNo, tok->_val._nIntVal );}	\
			else{ tok->_tok = singleOperator; } \
        }	\
		goto Exit_Func; \
	}\
}
   
/*
#define COMPLEX_OPERATOR_DFA( ch1stSymbol, ch2ndSymbol, singleOperator, complexOperator )	\
{\
	if( ch1stSymbol == *m_pTokenIter ){	\
		++m_pTokenIter;	\
		if( ch2ndSymbol == *m_pTokenIter )	\
		{	\
			tok->_tok = complexOperator;	\
			++m_pTokenIter;	\
		}	\
		else	\
		{	\
			if( INDICATOR == g_char2kindMap[*m_pTokenHead] )	\
			{	\
				tok->_tok = ERR;	\
				tok->_val._nIntVal |= (INVALID_TOK << 24);	\
				tok->_val._nIntVal |= (UNDEFINED_OP_ERR << 16);	\
				ErrorReporter::Instance()->Report( m_nLineNo, tok->_val._nIntVal ); \
			}	\
			else \
			{ \
				tok->_tok = singleOperator;	\
			} \
		}	\
		goto Exit_Func;	\
	}	\
}	\
*/


Token* Scanner::GetToken()
{
	assert( NULL  != m_pTokenIter );

	Token* tok = new Token();

	unsigned int sym_type;

	// read next line when scan at the end of last line
CHECK:
	if( *m_pTokenHead == '\0' )
	{
		if( fgets( m_pLnBuffer, s_nLnBufferSize, m_fSource ) )
		{
			m_pTokenIter = m_pTokenHead = m_pLnBuffer;
			++m_nLineNo;
		}
		else
		{
			tok->_tok = E_O_F; // encounter end of file
			goto Exit_Func;
		}
	}

	// scan over blanks when recognize a new token 
    while( (BLANK|NEWLINE) & g_char2kindMap[static_cast<int>(*m_pTokenIter)] ) 
       ++m_pTokenIter;
	m_pTokenHead = m_pTokenIter; // !!! must need
	
	if( '\0' == *m_pTokenIter ) goto CHECK;

	// recoginize token by different DFAs
	sym_type = g_char2kindMap[static_cast<int>(*m_pTokenIter)];

	if( sym_type & LETTER )
	{
		m_dfaIdentifier.getToken( tok );
		goto Exit_Func;
	}
	
	if( sym_type & DIGIT )
	{
		m_dfaNumerics.getToken( tok );
		goto Exit_Func;
	}

	if( '\'' == *m_pTokenIter )
	{
		m_dfaChar.getToken(tok);
		goto Exit_Func;
	}

	if( '"' == *m_pTokenIter )
	{
		m_dfaString.getToken(tok);
		goto Exit_Func;
	}

	if( '/' == *m_pTokenIter )
	{
		++m_pTokenIter;
		if( *m_pTokenIter == '/' || *m_pTokenIter == '*' )
		{
			m_dfaComment.getToken(tok);
		}
		else
		{
			tok->_tok = O_DIV;	
    	}

		goto Exit_Func;
	}

	////////////////////////////////////////////////////////////////////
	////////////////// complex oprations  //////////////////////////////
	COMPLEX_OPERATOR_DFA( '=', '=', O_ASSIGN, O_EQUAL );		// '=' or '=='
	COMPLEX_OPERATOR_DFA( '!', '=', O_NOT, O_NOTEQUAL );		// '!' or '!='
	COMPLEX_OPERATOR_DFA( '>', '=', O_GREATHAN, O_NOTLESS );	// '>' or '>='
	COMPLEX_OPERATOR_DFA( '<', '=', O_LESSTHAN, O_NOTGREAT );	// '<' or '<='
	COMPLEX_OPERATOR_DFA( '&', '&', ERR, O_AND );				// '&'(undefined operator)  or '&&'
	COMPLEX_OPERATOR_DFA( '|', '|', ERR, O_OR );				// '|'(undefined operator)  or '||' 
	////////////////////////////////////////////////////////////////////

	if( sym_type & (OTHER|INDICATOR)  )
	{
		assert( *m_pTokenIter != '!' && *m_pTokenIter != '&' && *m_pTokenIter != '|'  &&
			    *m_pTokenIter != '>' && *m_pTokenIter != '<' && *m_pTokenIter != '='  &&
			    *m_pTokenIter != '/' && *m_pTokenIter != '\'' && *m_pTokenIter != '"' );

		// !!!note: symbol like '.' and '\' will not be used as a token, since it must be used with a FLOAT_NUM token, 
		//          or an escape seqence--which is just a part of a CHAR or STRING token. 

		switch( *m_pTokenIter )
		{
		case '%':	tok->_tok = O_MOD; break;
		case '(':	tok->_tok = P_LPAREN; break;
		case ')':	tok->_tok = P_RPAREN; break;
		case '*':	tok->_tok = O_TIMES;  break;
		case '+':	tok->_tok = O_PLUS;   break;
		case '-':	tok->_tok = O_MINUS;  break;
		
		case ',':	tok->_tok = P_COMA;   break;
		case ':':	tok->_tok = P_COLON;  break;
		case ';':	tok->_tok = P_SEMICOLON; break;
		case '[':	tok->_tok = P_LSQUARE;	break;
		case ']':	tok->_tok = P_RSQUARE;	break;
		case '{':	tok->_tok = P_LCURLY;	break;
		case '}':	tok->_tok = P_RCURLY;	break;
		default: break; // it could not happened actually
		}
		++m_pTokenIter; // consume input symbol

		goto Exit_Func;
	}

	assert( 0 == g_char2kindMap[static_cast<int>(*m_pTokenIter)] );

	// error type: undefined symbol
	tok->_tok = ERR;
	tok->_val._nIntVal |= (INVALID_TOK << 24);	
	tok->_val._nIntVal |= (UNDEFINED_SYM << 16);	
	tok->_val._nIntVal |= static_cast<unsigned int>( *m_pTokenIter );
	++m_pTokenIter; // !!!! consume undefined symbol

	ErrorReporter::Instance()->Report( m_nLineNo, tok->_val._nIntVal );

Exit_Func:

	if( *m_pTokenIter == 0xa )
	{
		++m_pTokenIter;
	}

	// set new token start position
	m_pTokenHead = m_pTokenIter;

	return tok;
}
