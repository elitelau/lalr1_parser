#ifndef _GRAMMAR_H
#define _GRAMMAR_H

#include <set>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#include <deque>
using namespace std;

#include <assert.h>
#include <string.h>
#include "scanner.h"


#define EMPTY			PSEUDO
#define DOLLAR			E_O_F // modified by liujian 2007.10.19, origin: DOLLAR(ERR)

typedef set<int,less<int> > TerminalSet;
typedef set<int,less<int> >::const_iterator TerminalSetIter;


class Grammar
{
public:
	class Terminal;
	class NonTerminal;

	static Grammar* Instance() 
	{
		static Grammar g_Grammar;
		return &g_Grammar;
	}

	int GetCount(void)
	{
		return m_Productions.size();
	}

	const list<Terminal*>* GetTerminals(void)
	{
		return &m_Terminals;
	}

	const vector<NonTerminal*>* GetNonTerminals(void)
	{
		return &m_NonTerminals;
	}

	// give a number to each of nonterminal
	void NumberOnNonterminals(void);

	// used for test only
	void PrintGrammar(void);

    // used for test only, added by liujian. 2010.12.09
    void PrintFirstSets(void);

    // used for test only, added by liujian. 2010.12.09
    void PrintFollowSets(void);

	// used for test only
	void PrintNonterminals(void);

	// used for test only
	void PrintTerminals(void);

	// used for test only
	void PrintTerminals(int);

	// used for test only
	void PrintNonterminals(int);

	/*
	 * algorithms for calculating first sets and follow sets will cause infinite loops, 
	 * when two nonterminasl have associative relation with each other
	 * there are two solutions to this problem: one is rewriting the algorithm, other is limitation the time running within a thread,
	 *
	 */
	// calculate first sets
	void CalculateFirstSets(void);
	// calculate follow sets
	void CalculateFollowSets(void);

	~Grammar()
	{
		Clear();
	}

	void Clear();

private:
	Grammar() : m_pStartSym(NULL)
	{}

public:

	class Symbol
	{
	public:
		virtual bool IsTerminal(void) = 0;
		virtual ~Symbol()
		{}
	};

	// terminal including ¦Å
	class Terminal : public Symbol
	{
	public:
		Terminal( int kind ) : m_nTerminalKind(kind)
		{}

		virtual bool IsTerminal(void)
		{
			return true;
		}

		bool operator == ( const Terminal& other )
		{
			return this->m_nTerminalKind == other.m_nTerminalKind;
		}

		int GetFirstSet(void)
		{
			return m_nTerminalKind;
		}

		int GetTerminalKind(void) const
		{
			return m_nTerminalKind;
		}

	private:
		int m_nTerminalKind; // enumrated all the token types including ¦Å
	};

	// Nonterminals from grammar file
	class NonTerminal : public Symbol
	{
	public:
		NonTerminal( const Grammar& g, const char* name ) 
			: m_sDesp(name), m_pData(NULL), m_bChanged(false), m_bHasEmpty(false)
		{}

		virtual bool IsTerminal(void)
		{
			return false;
		}

		virtual const TerminalSet* GetFirstSet(void)
		{
			return &m_setFirst;
		}

		virtual const TerminalSet* GetFollowSet(void)
		{
			return &m_setFollow;
		}

		// added by liujian 2007.10.18
		void SetId( int id ) 
		{
			m_nId = id;
		}

		// added by liujian 2007.10.18
		int GetId(void)
		{
			return m_nId;
		}

		// used for test only:
		void PrintAssociation( void );
		// used for test only:
		void PrintFirstSet(void);
		// used for test only:
		void PrintFollowSet(void);
		// used for test only:
		void PrintAffection(void);

		const char* GetDesp(void)
		{
			return m_sDesp.c_str();
		}

		const char* GetDesp(void) const
		{
			return m_sDesp.c_str();
		}

		/*
		 * auxiliary functions for calculating first sets and follow sets
		 */

		void InsertInto1stSet( int terminal )
		{
			pair< set<int,less<int> >::iterator, bool> pr;
			pr = m_setFirst.insert( terminal );

			if( true == pr.second )
				m_bChanged = true;

			if( EMPTY == terminal )
				m_bHasEmpty = true;
		}

		void InsertIntoFollowSet( int terminal )
		{
			pair< set<int,less<int> >::iterator, bool> pr;
			pr = m_setFollow.insert( terminal );

			if( true == pr.second )
				m_bChanged = true;
		}

		// insert all the first set elements from an associated nonterminal into the first set of the current nonterminal
		void AddFirstElems( NonTerminal* another )
		{
			for( TerminalSetIter it = another->m_setFirst.begin(); it != another->m_setFirst.end(); ++it )
			{
				if( EMPTY != *it )
				  InsertInto1stSet( *it );
			}
		}

		// insert all the follow set elements from an associated nonterminal into the follow set of the current nonterminal
		void AddFollowElems( const TerminalSet* terminals )
		{
			for( TerminalSetIter it = terminals->begin(); it != terminals->end(); ++it )
			{
				if( EMPTY != *it )
				  InsertIntoFollowSet( *it );
			}
		}

		void ResetForChange(void)
		{
			m_bChanged = false;
		}

		bool IsChanged(void)
		{
			return m_bChanged;
		}

		bool HasEmpty(void)
		{
			return m_bHasEmpty;
		}
	private:	
		// used for map a pointer of nonterminal into subscript of parser table GOTO
		int			m_nId; // added by liujian 2007.10.18, 

		string		m_sDesp;
		TerminalSet	m_setFirst;
		TerminalSet	m_setFollow;

	public:
		// reserved for additional usage
		void*	m_pData;
	
    private:
		/*
		 * auxiliary variable used for calculating first sets and follow sets
		 */
		bool		m_bChanged;
		bool		m_bHasEmpty;
	};

	typedef vector<Symbol*> Rule;
	typedef Rule::iterator RuleIter;

	class Production
	{
	public:
		Production()
		{
			m_pRule = new Rule();
		}

		// !!! forbid assign operation
		void operator = ( const Production& );

		// !!! forbid copy constructor
		Production( const Production& );

		~Production()
		{
			delete m_pRule;
		}

		void Add( Symbol* sym )
		{
			m_pRule->push_back(sym);
		}

		bool IsContainNonterm( NonTerminal* n )
		{
			RuleIter itRule = m_pRule->begin();
			for( ; itRule != m_pRule->end(); ++itRule )
			{
				if( static_cast<NonTerminal*>(*itRule) == n )
					break;
			}

			if(  itRule == m_pRule->end() )
				return false;
			return true;
		}

		// used for test only:
		void PrintProduction(void);

		/*
		const Rule* GetRule(void)
		{
			return m_pRule;
		}*/

		Rule* GetRule(void)
		{
			return m_pRule;
		}

		bool IsStartWithTerminal(void)
		{
			assert( m_pRule->size() > 0 );
			return (*m_pRule->begin())->IsTerminal();
		}
	private:
		Rule*	m_pRule;

		/*
		 * auxiliary data struture used to calculate first set and follow set
		 */
	};

public:
	typedef vector<Production*> Productions;
	typedef map<NonTerminal*, Productions> GrammarRepresentation;
//	typedef list< pair<NonTerminal*, Productions> > GrammarRepresentation;

public:
	void Add( NonTerminal* n, Productions& ps )
	{
//		m_Productions.push_back( make_pair<n, ps> );
		m_Productions.insert( ValType(n, ps) );
	}

	void SetStartSymbol( NonTerminal* n )
	{
		m_pStartSym = n;
	}

	NonTerminal* GetStartSymbol(void)
	{
		return m_pStartSym;
	}

	GrammarRepresentation& GetGrammar(void)
	{
		return m_Productions;
	}

	Terminal*  FindTerminal( int kind )
	{
		TerminalsIter it = m_Terminals.begin();
		for( ; it != m_Terminals.end(); ++it )
		{
			if( kind == (*it)->GetTerminalKind() )
				return *it;
		}

		return NULL;
	}

	NonTerminal* FindNonterminal( const char* name )
	{
		for( NonTermIter it = m_NonTerminals.begin(); it != m_NonTerminals.end(); ++it )
		{
			if( 0 == 
				strcasecmp( (*it)->GetDesp(), name )
				)
				return (*it);
		}

		return NULL;

	}

	void AddSymbol( Symbol* sym );

private:
	void ResetForNoChange(void)
	{
		NonTerminal* n = NULL;
		for( Iter it = m_Productions.begin(); it != m_Productions.end(); ++it )
		{
			n = (*it).first;
			n->ResetForChange();
		}
	}

private:
	GrammarRepresentation	m_Productions; // a production may contain not only one rules
	list<Terminal*>				m_Terminals;
	vector<NonTerminal*>			m_NonTerminals;
	NonTerminal*				m_pStartSym;  // start symbol of grammar rules

	typedef GrammarRepresentation::iterator Iter;
	typedef GrammarRepresentation::const_iterator CIter;
	typedef map<NonTerminal*, Productions>::value_type ValType;
	typedef list<Terminal*>::iterator TerminalsIter;
	typedef vector<NonTerminal*>::iterator NonTermIter;
};

typedef Grammar::Symbol			Symbol;
typedef Grammar::Terminal		Terminal;
typedef Grammar::NonTerminal	NonTerminal;


typedef Grammar::RuleIter RuleIter;

// calculate first set of a sequence of symbols
// e.g. first( XiXj...Xn )
// if bContainEmpty = true, XiXj..Xn->¦Å
// !!!note:  ¦Å will not included in terminal set
//           user can add ¦Å to terminal set if necessary if bContainEmpty is true
void CalculateFirstSet( RuleIter& it_start, const RuleIter& it_end, TerminalSet& first, bool& bContainEmpty );

// is contain ¦Å in a set( first set or follow set )
bool IsContainEmpty( const TerminalSet& s );


#endif

