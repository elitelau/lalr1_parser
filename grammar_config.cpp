#include "grammar_config.h"
#include "err_report.h"

class grammar_exception : public exception
{
public:
   grammar_exception(const char* w) throw()
      : m_what(w)
   {}
   virtual ~grammar_exception() throw()
   {}
   virtual const char* what() const throw()
   {
       return m_what.c_str();
   }
private:
   string  m_what;
};

/*********************************************************/
/* implementation of class GrammarScanner                */
/*********************************************************/
Token* GrammarConfig::GrammarScanner::GetToken(void)
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
    while( (BLANK|NEWLINE) & g_char2kindMap[static_cast<int>(*m_pTokenIter)] ) ++m_pTokenIter;
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
			goto Exit_Func;
		}
	}

	if( '-' == *m_pTokenIter )
	{
		++m_pTokenIter;
		if( '>' == *m_pTokenIter )
		{
			tok->_tok = O_DIRIVATION; // !!! it's should be a derivation symbol
			++m_pTokenIter; // consume '>' symbol
		}
		else
		{
			tok->_tok = ERR;
			tok->_val._nIntVal |= (INVALID_TOK << 24);
			tok->_val._nIntVal |= (UNDEFINED_OP_ERR << 16);
			ErrorReporter::Instance()->Report( m_nLineNo, tok->_val._nIntVal );
		}

		goto Exit_Func;
	}

	if( '|' == *m_pTokenIter )
	{
		tok->_tok = O_SELECTION;
		++m_pTokenIter; // consume '|' symbol
		goto Exit_Func;
	}

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

/*********************************************************/
/* implementation of class GrammarParser                 */
/*********************************************************/
bool GrammarConfig::GrammarParser::Parse( char (&szPrompt)[100] )
{
	getTok();

	try
	{
		production_list();
		return true;
	}
	catch( exception& e )
	{
        cout << e.what() << endl;
		Grammar::Instance()->Clear();

		for( list<Token*>::iterator it = _tok_lst.begin(); it != _tok_lst.end(); ++it )
		{
			delete *it;
		}

		strcpy( szPrompt, e.what() );

		return false;
	}
}

// production_list->production_list production | production (BNF)
// production_list->production {production} (EBNF)
void GrammarConfig::GrammarParser::production_list()
{
	Grammar::Productions ps;
	Grammar::NonTerminal* n = NULL;
	
	try{
		production( n, ps ); // !!!
		Grammar::Instance()->Add( n, ps );

		// added by liujian 2007.8.10
		Grammar::Instance()->SetStartSymbol( n ); 

		while( _tok->_tok == FLOAT_NUM )
		{
			// reset
			n = NULL;
			ps.clear();

			production( n, ps );
			Grammar::Instance()->Add( n, ps );
		}
	}
	catch( exception& e )
	{
		throw;
	}
}

// production->seq_no rule_id "->" rule_list
void GrammarConfig::GrammarParser::production( Grammar::NonTerminal*& n, Grammar::Productions& ps  )
{
	try
	{
		match( FLOAT_NUM );
		rule_id(n);
		match(O_DIRIVATION);
		rule_list(ps);
	}
	catch( exception& e )
	{
		delete n;

		for( Grammar::Productions::size_type i = 0; i < ps.size(); ++i )
		{
			Grammar::RuleIter it = ps[i]->GetRule()->begin();
			for( ; it != ps[i]->GetRule()->end(); ++it )
			{
				delete *it;
			}

			delete ps[i];
		}

		throw;
	}
}


// rule_list->rule_list "|" rule| rule(BNF)
// rule_list->rule {"|" rule} (EBNF)
void GrammarConfig::GrammarParser::rule_list( Grammar::Productions& ps )
{
	
	Grammar::Production *p = new Grammar::Production();
	try
	{
		rule(p);
		ps.push_back( p );

		while( O_SELECTION == _tok->_tok )
		{
			match( O_SELECTION );
			
			p = new Grammar::Production();
			rule(p);

			ps.push_back( p );
		}
	}
	catch( exception& e )
	{
		delete p;
		throw;
	}
}

// rule->rule symbol|symbol(BNF)
// rule->symbol{symbol}
void GrammarConfig::GrammarParser::rule( Grammar::Production*& p )
{
	try
	{
		char szPrompt[100];
		int nRecursiveCount = 0;

		Grammar::Symbol* sym = NULL;
		symbol( sym );

		p->Add( sym );

		while( ID == _tok->_tok || STRING_LITERAL == _tok->_tok )
		{
			sym = NULL;
			symbol(sym);
			p->Add(sym);

			if( ++nRecursiveCount == _nMaxSymbolCount ) // recursive reach the maximum depth
			{
				sprintf( szPrompt, "recursive reach max depth in a grammar rule, line(%d)", _scanner.GetLineNo() );
				throw grammar_exception( szPrompt );
			}
		}
	}
	catch( exception& e )
	{
		// !!! note: all the terminals and nonterminals are freed by the global Object Grammar::Instance()
		/*
		for( Grammar::RuleIter it = p->GetRule()->begin(); it != p->GetRule()->end(); ++it )
		{
			delete *it;
		}*/

		throw; // rethrow
	}
}

#define ELSE_SPEC_SYM( type, str ) \
	else if( strcasecmp(_tok->_val._strVal, str) == 0 ) \
	{\
	    t = Grammar::Instance()->FindTerminal( type ); \
		if( NULL == t) \
        {\
			s = new Grammar::Terminal( type );\
    		Grammar::Instance()->AddSymbol( s ); \
        }\
		else \
        {\
		    s = t; \
        }\
	}\


void GrammarConfig::GrammarParser::symbol( Grammar::Symbol*& sym )
{
	Grammar::Symbol* s = NULL;
	Grammar::Terminal* t = NULL;
	char szPrompt[100];
	
	try
	{
		if( ID == _tok->_tok || STRING_LITERAL == _tok->_tok )
		{

			if(ID == _tok->_tok) // nonterminal or ¦Å
			{
				if( strcasecmp(_tok->_val._strVal, "empty") == 0 ) // ¦Åis a special terminal
				{
					t = Grammar::Instance()->FindTerminal( EMPTY );
					if( NULL == t )
					{
						s = new Grammar::Terminal( EMPTY );
						Grammar::Instance()->AddSymbol( s ); // added by liujian 2007.8.1
					}
					else
					{
						s = t;
					}
				}
				ELSE_SPEC_SYM(NUM, "NUM")
			    ELSE_SPEC_SYM(CHAR, "CHAR")
				ELSE_SPEC_SYM(ID, "ID")
				ELSE_SPEC_SYM(STRING_LITERAL, "STRING_LITERAL")
				else // nonterminal
				{
					Grammar::NonTerminal* n = Grammar::Instance()->FindNonterminal(_tok->_val._strVal);
					if( NULL == n )
					{
						s = new Grammar::NonTerminal( *Grammar::Instance(), _tok->_val._strVal );
						Grammar::Instance()->AddSymbol( s ); // added by liujian 2007.8.1
					}
					else // nonterminal has appeared before
					{
						s = n;
					}
				}

				match(ID);
			}
			else // terminal
			{		
				int nKind = getTokenKind( _tok->_val._strVal );
				if( _nUnrecognizedTerminal == nKind )
				{
					sprintf( szPrompt, "unrecognized terminal appeared in line(%d)", _scanner.GetLineNo() );
					throw grammar_exception( szPrompt );
				}
				else
				{
					t = Grammar::Instance()->FindTerminal( nKind );
					if( NULL == t )
					{
						s = new Grammar::Terminal( nKind ); 
						Grammar::Instance()->AddSymbol( s ); // added by liujian 2007.8.1
					}
					else
					{
						s = t;
					}
				}

				match(STRING_LITERAL);
			}

			sym = s;
		}
		else
		{
			sprintf( szPrompt, "grammar file has error: check line(%d)", _scanner.GetLineNo() );
			throw grammar_exception( szPrompt );
		}
	}
	catch( exception& e )
	{
		// !!! delete the allocated memory when exception occured from 'match' action
		// !!! note: all the terminals and nonterminals are freed by the global Object Grammar::Instance()
		// delete s; 
		throw;
	}
}

// rule_id->ID
void GrammarConfig::GrammarParser::rule_id(Grammar::NonTerminal*& n)
{
	try
	{
		Grammar::NonTerminal* nonterm = NULL;
		if( _tok->_tok == ID )
		{
			// modified by liujian 2007.8.1
			nonterm = Grammar::Instance()->FindNonterminal(_tok->_val._strVal);
			if( NULL == nonterm )
			{
				nonterm = new Grammar::NonTerminal( *Grammar::Instance(), _tok->_val._strVal );
				Grammar::Instance()->AddSymbol( nonterm ); // added by liujian 2007.8.1
			}

		    n = nonterm;
		}

		match( ID );
		
	}
	catch( exception& e )
	{
		throw;
	}
}

void GrammarConfig::GrammarParser::match( int tok_type )
{
	char szPrompt[100];

	if( _tok->_tok == tok_type ) // the current token is the expected type
	{
		_tok = getTok();

		if( ERR == _tok->_tok )
		{
			sprintf( szPrompt, "grammar file has error: check line(%d)", _scanner.GetLineNo() );
			throw grammar_exception(szPrompt);
		}
	}
	else
	{
		sprintf( szPrompt, "grammar file has error: check line(%d)", _scanner.GetLineNo() );
		throw grammar_exception(szPrompt);	
	}
}

#define COMPLEX_OPERATOR( ch1stSymbol, ch2ndSymbol, complexOperator )	\
{\
	if( ch1stSymbol == *str ){	\
		++str;	\
		if( ch2ndSymbol == *str ){	\
			nKind = complexOperator;	\
			}\
		goto Exit_Func; \
	}\
}

int GrammarConfig::GrammarParser::getTokenKind(const char* str)
{
	int nKind = _nUnrecognizedTerminal; // error
	int nLen = strlen( str );
	const char* keyword = NULL;

	if( 0 == nLen )
	{
		goto Exit_Func;
	}

	if( 1 == nLen )
	{
		switch(*str)
		{
		case '[': nKind = P_LSQUARE;	break;
		case ']': nKind = P_RSQUARE;	break;
		case ';': nKind = P_SEMICOLON;	break;
		case ':': nKind = P_COLON;		break;
		case ',': nKind = P_COMA;		break;
		case '(': nKind = P_LPAREN;		break;
		case ')': nKind = P_RPAREN;		break;
		case '{': nKind = P_LCURLY;	;	break;
		case '}': nKind = P_RCURLY;		break;
		case '=': nKind = O_ASSIGN;		break;
		case '+': nKind = O_PLUS;		break;
		case '-': nKind	= O_MINUS;		break;
		case '*': nKind	= O_TIMES;		break;
		case '/': nKind = O_DIV;		break;
		case '%': nKind = O_MOD;		break;
		case '!': nKind = O_NOT;		break;
		case '<': nKind = O_LESSTHAN;	break;
		case '>': nKind = O_GREATHAN;	break;
		default:  break;
		}

		goto Exit_Func;
	}

	if( 2 == nLen )
	{
		COMPLEX_OPERATOR( '>', '=', O_NOTLESS );
		COMPLEX_OPERATOR( '<', '=', O_NOTGREAT );
		COMPLEX_OPERATOR( '=', '=', O_EQUAL );
		COMPLEX_OPERATOR( '!', '=', O_NOTEQUAL );
		COMPLEX_OPERATOR( '&', '&', O_AND );
		COMPLEX_OPERATOR( '|', '|', O_OR );
	}

	keyword = _scanner.GetKeyword( _tok->_val._strVal );
	if( NULL != keyword )
	{
		switch(*keyword)
		{
		case 'i':
			if( *++keyword == 'n' )
				nKind = K_INT;
			else
				nKind = K_IF;
			break;
		case 'e':
			nKind = K_ELSE;
			break;
		case 'f':
			if(*++keyword == 'o')
				nKind = K_FOR;
			else
				nKind = K_FLOAT;
			break;
		case 'w':
			if(*++keyword == 'h')
				nKind = K_WHILE;
			else
				nKind = F_WRITE;
			break;
		case 'b':
			nKind = K_BREAK;
			break;
		case 'c':
			if( *++keyword == 'h' )
				nKind = K_CHAR;
			else
				nKind = K_CONTINUE;
			break;
		case 'g':
			nKind = K_GOTO;
			break;
		case 'r':
			if( keyword[2] == 'a' )
				nKind = F_READ;
			else
				nKind = K_RET;
			break;
		case 'v':
			nKind = K_VOID;
			break;
		default: // should not happen
			break;
		}
	}

Exit_Func:
	return nKind;
}



/*************************************************************************************/
/* implementation of class GrammarConfig                                             */
/*************************************************************************************/
bool GrammarConfig::ReadInProductions( char (&szPrompt)[100] /*error information*/ )
{
	ErrorReporter::Instance()->Clear();

	FILE *f = fopen( m_sFileName.c_str(), "r+t" );
	if( !f ) 
	{
		sprintf( szPrompt, "cannot open file: %s", m_sFileName.c_str() );
		return false;
	}

	GrammarParser _parser( f );
	bool bRet = _parser.Parse( szPrompt );

	_parser.Clear();

	fclose(f);

	return bRet;
}
