#ifndef _GRAMMAR_CONFIG_H
#define _GRAMMAR_CONFIG_H

#include "scanner.h"
#include "grammar.h"
#include <stdio.h>

const int SYM_DIRVATION = 1000;

#define O_DIRIVATION	O_ASSIGN  // !! in BNF, there is no denotion of assignment operation , but instand of dirivation
#define O_SELECTION		O_OR       // '|', not "||"


//typedef Grammar::Productions Productions;
class GrammarConfig
{
public:
	GrammarConfig( const char* pFileName ) : m_sFileName(pFileName)
	{}

	// read in all the productions from grammar file
	bool ReadInProductions( char (&szPrompt)[100] );

private:
	class GrammarScanner : public Scanner
	{
	public:
		GrammarScanner( FILE* f ) : Scanner(f)
		{}
		virtual Token* GetToken(void);
		const char* GetKeyword( const char* str )
		{
			return m_dfaIdentifier.getKeyword( str );
		}
	};


	class GrammarParser
	{
	public:
		GrammarParser( FILE* f ) : _nMaxSymbolCount(10), _nMaxRuleCount(5), _nMaxProductionCount(1000), _nUnrecognizedTerminal(-1), _scanner(f), _tok( NULL )
		{}
		~GrammarParser()
		{
			Clear();
		}
		bool Parse( char (&szPrompt)[100] );

		void Clear() // free all the tokens
		{
			for( TokIter it = _tok_lst.begin(); it != _tok_lst.end(); ++it )
				delete *it;
			_tok_lst.clear();
		}
	private:
		void production_list(void);
		void production( Grammar::NonTerminal*& n, Grammar::Productions& ps  );
		void rule_id( Grammar::NonTerminal*& n );
		void rule_list( Grammar::Productions& ps );
		void rule( Grammar::Production*& p );
		void symbol( Grammar::Symbol*& sym );

		void match( int tok_type );
		Token* getTok(void)
		{
			_tok = _scanner.GetToken();
			while( _tok->_tok == PSEUDO ) // get a token that is not psuedo
			{
				_tok_lst.push_back(_tok);
				_tok = _scanner.GetToken();
			}

			_tok_lst.push_back(_tok);

			return _tok;
		}

		int getTokenKind(const char* str);
	private:
		int _nMaxSymbolCount; // the maximum symbols a rule can contain
		int	_nMaxRuleCount; // the maximum rules a production can contain
		int _nMaxProductionCount; // the maximum productions

		int	_nUnrecognizedTerminal;

	private:
		GrammarScanner	_scanner;  
		Token* _tok;
		typedef list<Token*> Tokens;
		typedef Tokens::iterator TokIter;
		Tokens _tok_lst;  // !!! used for collect all the memories occupied by tokens, and thus freed memory at a time finally
	};

	string m_sFileName;  // the name of grammar file 
};

#endif

