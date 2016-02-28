#ifndef _SCANNER_H
#define _SCANNER_H

#include <assert.h>
#include "utils.h"

extern unsigned int g_char2kindMap[256];


/*
 * indicator: indicate what the token will be, or indicate what should be specailly treated; and they cannot recoginzed as a token solely.
 * In minusC, there are: '&', '|', '\'', '"', '.'
 */
enum SymbolConstants{ LETTER = 01, BLANK=02, DIGIT = 04, OTHER = 010, NEWLINE = 020, INDICATOR = 040 };

/*************************************************\
 * definitions all kinds of tokens
\*************************************************/
enum TokenTypeConstants {
	/* key words: int, if, else, char, return, while, for, break, continue, goto, float, read, write */
	K_INT, K_IF, K_ELSE, K_CHAR, K_RET, K_WHILE, K_FOR, K_BREAK, K_CONTINUE, K_GOTO, K_FLOAT, K_VOID, F_READ, F_WRITE,

	/* operators*/
    O_ASSIGN, O_PLUS, O_MINUS, O_TIMES, O_DIV, O_MOD, // arithmetic operators: =, +, -, *, /, %
	O_EQUAL, O_NOTEQUAL, O_LESSTHAN, O_GREATHAN, O_NOTLESS, O_NOTGREAT,  // relational operations: ==, !=, <, >, >=, <=
	O_NOT, O_AND, O_OR, // logic operations: !, &&, || 

    /* punctuators: , ; : ( ) [ ] { } */
    P_COMA, P_SEMICOLON, P_COLON, P_LPAREN, P_RPAREN, P_LSQUARE, P_RSQUARE, P_LCURLY, P_RCURLY,

	/* identifier*/
	ID,
	
	/* number*/
	INT_NUM, FLOAT_NUM, NUM,
	
	/* char*/
	CHAR,
	
	/* string literal*/
    STRING_LITERAL,

	/* end of file*/
	E_O_F,

	/* undefined or error token*/
	ERR,

	/* pseudo-token are not really token*/
	PSEUDO
};

/*************************************************\
 * data structure of a token
\*************************************************/
struct Token{

	// constructors
	Token( TokenTypeConstants kind = ERR )
	{
		_tok = kind;
		_val._nIntVal = 0; 
	}

	Token( TokenTypeConstants kind, char ch )
	{
		_tok = kind;
		_val._chVal = ch;
	}

	Token( TokenTypeConstants kind, int n )
	{
		_tok = kind;
		_val._nIntVal = n;
	}

	Token( TokenTypeConstants kind, char* str )
	{
		_tok = kind;
		_val._strVal = str;
	}

	Token( TokenTypeConstants kind, float f )
	{
		_tok = kind;
		_val._fVal = f;
	}

	// destructors
	~Token()
	{
		if( STRING_LITERAL == _tok || INT_NUM == _tok || 
			FLOAT_NUM == _tok || NUM == _tok || ID == _tok  )
		{
			delete _val._strVal;
		}
	}

	// interface for accessing internal data

	int iVal()
	{
		assert( INT_NUM == _tok );
		return _val._nIntVal;
	}

	float fVal()
	{
		assert( FLOAT_NUM == _tok );
		return _val._fVal;
	}

	char cVal()
	{
		assert( CHAR == _tok );
		return _val._chVal;
	}

	char* sVal()
	{
		assert( STRING_LITERAL == _tok );
		return _val._strVal;
	}

	TokenTypeConstants	_tok; // kind of token
	union
	{
		char	_chVal;
        // !note , when token is an error, its integer value respresents for 
        // the error type has been occured
		int		_nIntVal; 
		char*	_strVal;
		float	_fVal;
	} _val;
};

/*************************************************\
 * Scanner
\*************************************************/
class Scanner
{
public:
	explicit Scanner( FILE* f );
	virtual ~Scanner();
	Scanner( const Scanner& ); // !!!forbid copy constructor
	const Scanner& operator = ( const Scanner& ); // !!!fordib assignment operation
public:
	virtual Token* GetToken(void);

	int GetLineNo(void)
	{
		return m_nLineNo;
	}

	/*
	void Reset( FILE* f )
	{
		m_fSource = f;
		m_nLineNo = 0;
	}*/

protected:

	class Comment_DFA
	{
	public:
		explicit Comment_DFA( Scanner& _scaner ) : m_refScanner(_scaner)
		{}

	public:
		void getToken( Token*& );

	private:
		Scanner&	m_refScanner;

	};

    class STRING_DFA
	{
	public:
		explicit STRING_DFA( Scanner& _scaner ) : m_refScanner(_scaner)
		{}
	public:
		void getToken( Token*& );
	private:
		Scanner&	m_refScanner;

		static int	s_nMaxStrLength; // the maximum length of a string
	};

	class CHAR_DFA
	{
	public:
		explicit CHAR_DFA( Scanner& _scaner ) : m_refScanner(_scaner)
		{}

	public:
		void getToken( Token*& );
	private:
		static int		s_nSeqMaxLength;      // maximum chars can contain in a char sequence enclosed by quotes
		static char		s_ReservedEscSeq[10]; // reserved for user-defined escape sequence

		Scanner&		m_refScanner;
	};
	
	class NUM_DFA
	{
	public:
		explicit NUM_DFA( Scanner& _scaner ) : m_refScanner(_scaner)
		{}

	public:
		void getToken( Token*& );
	private:
		Scanner& m_refScanner;

		static int s_nWordLength; // word length
		static unsigned int s_nBufferMaxSize;
	};

	class ID_DFA
	{
	public:
		ID_DFA( Scanner& _scaner ) : m_refScanner(_scaner)
		{
			m_hashKeywords.build( s_Keywords );
		}

		void getToken( Token*& ); 
		const char* getKeyword( const char* str )
		{
			return m_hashKeywords.match( str );
		}
	private:
		Scanner&	m_refScanner;
		HashingTab	m_hashKeywords;

		// max length of an identifier
		static	int	s_nMaxIdLength;
		static const char*  s_Keywords[];
	};
protected: /*associated objects used to deal with a concreate DFA*/
    ID_DFA		m_dfaIdentifier;
	NUM_DFA		m_dfaNumerics;
	CHAR_DFA	m_dfaChar;
	STRING_DFA	m_dfaString;
	Comment_DFA	m_dfaComment;

protected:
	FILE*	m_fSource;
	char*	m_pLnBuffer;      // buffer for source codes
	char*	m_pLnBufferTail;  // point to the block end

	char*   m_pTokenHead;

	char*   m_pTokenIter; // characters between m_pTokenHead and m_pTokenIter are the token being recognized

	static int s_nLnBufferSize; // max size of the length of a line buffer
	static unsigned int s_nTokenMaxLen;     // max length of a token

	int		m_nLineNo;
};

#endif

