#ifndef _LALR_H
#define _LALR_H
#include "grammar.h"
#include "err_report.h"

#include <vector>
#include <map>
#include <set>
#include <list>
#include <iostream>
#include <stdio.h>
using namespace std;

//#define TEST_CASE

//////////////////////////////////////////////////
/////////     declarations     ///////////////////
typedef Grammar::NonTerminal NonTerminal;
typedef Grammar::Terminal Terminal;
typedef Grammar::Rule Rule;
typedef Grammar::GrammarRepresentation GrammarRepresentation;

class LR0Item;
typedef vector<LR0Item*> LR0Items;
typedef pair<NonTerminal*, Rule*> GrammarRule;
typedef RuleIter Period;
typedef vector<Rule*>  Rules;

//////////////////////////////////////////////////


/****************************************\
  class LR0Item
\****************************************/
class LR0Item;
typedef vector<LR0Item*> LR0Items;
class LR0Item
{
public:
	// mapping relation of nonteriminal to corresponding rules
	static void Initialize();
	// !!!must be invoked when have used Initialize function
	static void Finalize();
protected:
	LR0Item(LR0Item& other) : m_Rule(other.m_Rule), m_Period(other.m_Period), m_pData(NULL)
	{}

	LR0Item( const GrammarRule& r, const Period& p ) : m_Rule(r), m_Period(p)
	{}

	virtual ~LR0Item()
	{}

public:
	// is a complete item
	bool IsCompleteItem() const
	{
		bool bRet = false;

		if( m_Rule.second->end() == m_Period ) // period points to the end of a production is a complete item
			bRet = true;
		else
		{
			if( m_Rule.second->size() == 1 && (*m_Rule.second)[0]->IsTerminal() )
			{
				// item's grammar rule containing only ¦Å is also a complete item
				if( static_cast<Terminal*>((*m_Rule.second)[0])->GetTerminalKind() == EMPTY )
					bRet = true;
			}
		}
		return bRet;
	}

	// is an initial item
	bool IsInitialItem()
	{
		bool bRet = false;

		if( m_Rule.second->begin() == m_Period ) // period points to the end of a production is a complete item
			bRet = true;

		return bRet;
	}

	// move period
	// !!!note: when invoke this function, be sure the item is not a complete item
	void MovePeriod(void)
	{
	   ++m_Period;
	}

	// is point to a nonterminal of the current period
	// !!!note: be sure that the item is not a complete item
	bool IsPoint2Nonterm(void)
	{
		return !(*m_Period)->IsTerminal();
	}

	// get period
	Period GetPeriod(void)
	{
		return m_Period;
	}

	Period GetPeriod(void) const
	{
		return m_Period;
	}

	const GrammarRule& GetRule(void) const
	{
		return m_Rule;
	}

	// calculate closure of an item
	// !!!note: when invoking this function, be sure that the item is not a complete item and period points to a nonterminal
	void Closure(  LR0Items& items, bool bRetCopy = true );

	// clone
	virtual LR0Item* Clone(void);

	// forbid usage
	void operator =( const LR0Item&  );

	// compare whether two items are identical
	// return 0, if identical; -1 if with smaller-than relation; 1 if with greater-than relation
	int operator ==( const LR0Item& );

	// check whether a state is transitional from the current item
	// !!!note: be sure that the current item is not a complete item
	bool CheckTransition( LR0Item* other ) const
	{
		bool bRet = false;

		if( m_Rule.first == other->m_Rule.first &&
			m_Rule.second == other->m_Rule.second )
		{
			Period temp = m_Period;
			if( !IsCompleteItem() )
			{
				++temp;
				if( temp == other->m_Period )
					bRet = true;
			}
		}

		return bRet;
	}

	virtual void output( string& s );

	// used for test
	void print()
	{
		cout << endl << m_Rule.first->GetDesp() << "->";
		typedef Rule::iterator Iter;
		for( Iter it = m_Rule.second->begin(); it != m_Rule.second->end(); ++it )
		{
			if( (*it)->IsTerminal() )
			{
				cout << TokenTypeStr[static_cast<Terminal*>(*it)->GetTerminalKind()];
			}
			else
				cout << static_cast<NonTerminal*>(*it)->GetDesp();

			cout << " ";
		}
		cout << " | period: ";
		
		if( IsCompleteItem() )
		{
			cout << "NULL";
		}
		else
		{
			if( (*m_Period)->IsTerminal() )
			{
				cout << TokenTypeStr[static_cast<Terminal*>(*m_Period)->GetTerminalKind()];
			}
			else
				cout << static_cast<NonTerminal*>(*m_Period)->GetDesp();
		}
	}
protected:
	// get direct closure from an item
	void GetDirectClosure( LR0Items& items, bool bRetCopy = false );
protected:
	struct VertexAttr
	{
		VertexAttr() : _bVisited(false), _nSearchIndicator(0)
		{}

		void Reset(void)
		{
			_bVisited = false;
			_nSearchIndicator = 0;
		}

		bool	_bVisited;
		unsigned int		_nSearchIndicator;
	};

	/*
	*  auxiliary data structure for calculating closure of an item
	*/
	static map<NonTerminal*, LR0Items*>	s_mapNonterm2Items;
	typedef map<NonTerminal*, LR0Items*>::value_type ValType;

//	static void ResetForDfs(void);

protected:
	GrammarRule	m_Rule;
	// period of an item
	Period		m_Period;
protected:
	void*		m_pData; // reserved for extra usage
};

/****************************************\
  class LR1Item
\****************************************/
class LR1Item;
typedef vector<LR1Item*> LR1Items;
typedef LR1Items::iterator LR1ItemsIter;
class LR1Item : public LR0Item
{
public:
	LR1Item(const GrammarRule& r) : LR0Item(r, r.second->begin())
	{}

	LR1Item(LR1Item& other) : LR0Item(other.m_Rule, other.m_Period)
	{}

	using LR0Item::Closure;
	using LR0Item::GetDirectClosure;

    // !!!note: when invoking this function, 
	// be sure that the item is not a complete item and period points to a nonterminal
	void Closure( LR1Items& _closure );

	// get direct closure
	void GetDirectClosure( LR1Items& _closure );

	virtual LR1Item* Clone(void)
	{
		return new LR1Item(*this);
	}

	static void SetStartItem( LR1Item* item )
	{
		s_pStartItem = item;
	}

	static LR1Item* GetStartItem(void)
	{
		return s_pStartItem;
	}

	bool AddLookahead( int lookahead )
	{
		pair< TerminalSet::iterator, bool > pr;
		pr = m_setLookAheads.insert( lookahead );

		if( pr.second == true )
			return true;
		return false;
	}

	const TerminalSet& GetLookahead(void) const
	{
		return m_setLookAheads;
	}

	bool AddLookahead( const TerminalSet& lookaheads )
	{
		bool bChanged = false;

		pair< TerminalSet::iterator, bool > pr;
		for( TerminalSet::const_iterator it = lookaheads.begin(); 
			it != lookaheads.end(); ++it )
		{
			pr = m_setLookAheads.insert( *it );
			if( pr.second == true )
				bChanged = true;
		}

		return bChanged;
	}
public:
	// used for test only:
	virtual void output( string& s )
	{
		LR0Item::output( s );
		s += "  | lookaheads: ";
		for( TerminalSet::iterator it = m_setLookAheads.begin();
			it != m_setLookAheads.end(); ++it )
		{
			s += TokenTypeStr[*it];
			s += " ";
		}
        s += GetLineEnd();
	}

private:
	TerminalSet	m_setLookAheads;

	// the start item
	static LR1Item* s_pStartItem; 
};

typedef vector<LR1Item*> LR1Items;

/****************************************\
  class LALR1_DFA
\****************************************/
class LALR1_Parser;
class LALR1_DFA
{
public:
	static LALR1_DFA* Instance()
	{
		static LALR1_DFA g_dfa;
		return &g_dfa;
	}

	~LALR1_DFA()
	{
		for( Graphy::size_type i = 0; i < _fsa.size(); ++i )
		{
			delete _fsa[i];
		}
	}

	friend class LALR1_ParserGenerator;
protected:
	class State;
public:
	// construct LR1 DFA
	// firstly, construct a LR0 DFA
	// then, construct lookaheads for each item of a state
	void Construct_LR1_DFA();

	void CheckConflict(void);

	// get the start start
	State* GetStartState(void)
	{
		assert( _fsa.size() > 0 );
		return _fsa[0];
	}

	// used for test:
	void print_transitions(void)
	{
		typedef Graphy::iterator Iter;
		for( Iter it = _fsa.begin(); it != _fsa.end(); ++it )
		{
			(*it)->print_transitions();
			cout << endl;
		}
	}

	// used for test
	void print_state_info(bool has_lookahead)
	{
		typedef Graphy::iterator Iter;
		for( Iter it = _fsa.begin(); it != _fsa.end(); ++it )
		{
			(*it)->print(has_lookahead);
			cout << endl;
		}
	}

	// used for test
	void out_put(void);
	
protected:
//	struct AdjacentState;

	class State
	{
	public:
		struct AdjacentState;
		typedef list<AdjacentState> AdjacentStates;
		typedef AdjacentStates::iterator AdjacentStatesIter;

		enum ConflictConstants
		{
			NO_CONFLICT				=	0,  // no conflict
			SHIFT_REDUCE_CONFLICT	=	1,  // shift-reduce conflict
			REDUCE_REDUCE_CONLICT	=	2   // shift-shift conflict
		};

		State() : m_bChanged(false)
		{
//			m_nId = ++s_nAllocatedId;
		}


		~State()
		{
			Clear();

			for( LR1Items::size_type i = 0; i < m_Items.size(); ++i ) 
			{
				delete m_Items[i];
			}
		}

		void ClearKernel2Closures()
		{
			Kernel2Closures::iterator it_closure = m_mapKernel2Closures.begin();
			for( ; it_closure != m_mapKernel2Closures.end(); ++it_closure )
			{
				LR1Items* closures = (*it_closure).second;
				delete closures;
			}
			m_mapKernel2Closures.clear();
		}

		void ClearItem2TransItem()
		{
			Item2TransItem::iterator it = m_lstItem2Transition.begin();
			for( ; it != m_lstItem2Transition.end(); ++it )
			{
				TransItemInfo* trans_inf = (*it).second;
				delete trans_inf;
			}

			m_lstItem2Transition.clear();
		}

		void Clear()
		{
			ClearKernel2Closures();
			ClearItem2TransItem();
		}

	public:
		static void SetOwner( LALR1_DFA* father )
		{
			s_pFather = father;
		}

		// transit to a set of new states
		void Transit( vector<State*>& states );

		// compare whether two states are identical
		bool operator == ( const State& another );

		// add an item to a state
		// !!! return false, if an identical item has already been exsited
		//     or else, return true, if successfully be inserted into 
		// !!! restriction condition: if bIsCopy = ture, insert an copy item
		//                            or else, insert the item
		bool AddItem( LR1Item* item, bool bIsCopy = false );

		// add a set of items to a state
		// !!! return false, if identical items has already exsited and return the duplicated ones
		//     return true, if there is no identical item exsit in list
		bool AddItem( const LR1Items& items, LR1Items& duplicate_items );

        bool AddItem( const LR1Items& items );

		// add an adjacent state in a sorted way
		void AddAdjacentState( State* to, Symbol* edge )
		{
			AdjacentState adj_stat(to, edge);
			AdjacentStatesIter it = m_AdjacentStates.begin();
			for( ; it != m_AdjacentStates.end(); ++it )
			{
				if( adj_stat < (*it) )
					break;
			}

			m_AdjacentStates.insert( it, adj_stat );
//			m_AdjacentStates.push_back( AdjacentState(to, edge) );
		}

		// !!!note: t is must exsited in adjacent list
		void RemoveAdjacentState( State* t )
		{
			typedef AdjacentStates::iterator Iter;
			Iter it = m_AdjacentStates.begin();
			for( ; it != m_AdjacentStates.end(); ++it )
			{
				if( (*it)._pAdjacentState == t ) // !!!note: compare pointers. not check whether two states are identical
					break;
			}

			assert( it != m_AdjacentStates.end() );
			m_AdjacentStates.erase( it );
		}

		// build association between a kernel item with its closure items
		// !!!note: when it is the START state, it should not apply this algorithm 
		//          and treated in a special way
		void BuildAssociationsOnKernel2Closures();

		// used for test only:
		void PrintAssociationOnKernel2Closures();

		// build association between a non-complete item to its transitional item
		void BuildAssociationOnItem2TransItem();

		// used for test only:
		void PrintAssociationOnItem2TransItem();

		// if has no identical transition to other state, return 1
		// if has an identical transition to a state in newStates, return 0, and get the state
		// if has an identical transition to a state in DFA, return -1, and get the state
		int CheckTransition( const Symbol* t, const vector<State*>& newStates, State*& dulicate_t );

		// combine into a state
		// !!!note: other is a newly created state
		int Combine( const State* other );

		// update lookaheads on closure items
		void UpdateLookaheadsOnClosures();
		// update lookaheads on all the transitional items
		void UpdateLookaheadsOnTransitions();

		// check are there any conflicts exsit in a state, e.g reduce-reduce conflict, shift-reduce conflict, 
		// or the former two conflict concurr, or no conflict 
		// param one: record concreate items with reduce-reduce conflict
		// param two: record concreate items with shift_reduce conflict
		// param 3rd: record concreate input conflict of reduce-reduce
		// param 4th: record concreate input conflict of shift-reduce 
		int CheckConflict( list< pair<LR1Item*, LR1Item*> >&, list< pair<LR1Item*, LR1Item*> >&,
			list< vector<int>*>&, list<int>& );

		int GetId(void)
		{
			return m_nId;
		}

		void SetId( int id )
		{
			m_nId = id;
		}

		const LR1Items* GetItems(void)
		{
			return &m_Items;
		}

		const AdjacentStates* GetAdjacentStates(void) const
		{
			return &m_AdjacentStates;
		}

		// get the item 'from' in the state which transit to item 'to' by symbol 'trans'
		// if there is exsit only one or more transitional item, return true
		void GetDeterministicTransition( int trans_sym, vector< pair<LR1Item*, LR1Item*> >& from_to );

		// used for test:
		void print(bool has_lookahead) const
		{
			print_body(has_lookahead);
		}

		// used for test:
		void out_put_transitions( string& str )
		{
			str += "state s";;
			str += Int2Str( m_nId );
			str += " has "; 
			str += Int2Str( m_AdjacentStates.size() );
			str += " transitions";
			str += GetLineEnd();

			for( AdjacentStates::iterator it = m_AdjacentStates.begin();
				it != m_AdjacentStates.end(); ++it )
			{
				str += "   -->s";
				str += Int2Str((*it)._pAdjacentState->m_nId);
				str += " [";

				if( (*it)._pTransition->IsTerminal() )
				{
					str += TokenTypeStr[static_cast<Terminal*>((*it)._pTransition)->GetTerminalKind()];
				}
				else
				{
					str += static_cast<NonTerminal*>((*it)._pTransition)->GetDesp();
				}
				str += "];  ";
				str += GetLineEnd();
			}
		}

		// used for test:
		void out_put( string& str )
		{
			string strTemp;
			str += "          state s";
			str += Int2Str( m_nId );
			str += ":";
			str += GetLineEnd();

			LR1Items::size_type i = 0;
			for( ; i < m_Items.size(); ++i )
			{
				m_Items[i]->output(strTemp);
				str += strTemp;
				str += GetLineEnd();
				strTemp.clear();
			}


		}

		// used for test:
		void print()
		{
			cout << endl << "          state " << m_nId << ":";
			LR1Items::size_type i = 0;
			for( ; i < m_Items.size(); ++i )
			{
				m_Items[i]->print();
			}

			cout << endl;
			cout << "=========================";
		}

		// used for test:
		void print_body(bool has_lookahead) const
		{
			cout << endl << "          state " << m_nId << ":" << endl;
			LR1Items::size_type i = 0;
			for( ; i < m_Items.size(); ++i )
			{
                if( !has_lookahead ) {
				   m_Items[i]->print();
                }
                else {
                   std::string str;
                   m_Items[i]->output(str);
                   cout << str;
                }
			}
		}

		// used for test:
		// test adjacent states
		void print_transitions();

	public:
		// definition of an adjacent state
		struct AdjacentState
		{
			AdjacentState( State* to, Symbol* edge ) : _pAdjacentState(to),_pTransition(edge)
			{}

			bool operator < ( const AdjacentState& other )
			{
				bool bRet = false;
				if( _pTransition->IsTerminal() == other._pTransition->IsTerminal() )
				{
					if( _pTransition->IsTerminal() )
					{
						if( static_cast<Terminal*>(_pTransition)->GetTerminalKind()
							<= static_cast<Terminal*>(other._pTransition)->GetTerminalKind()
							)
						{
							bRet = true;
						}
					} 
					else
					{
						if( strcasecmp( static_cast<NonTerminal*>(_pTransition)->GetDesp(),
							static_cast<NonTerminal*>(other._pTransition)->GetDesp()
							) <= 0 )
						{
							bRet = true;
						}
					}
				}
				else
				{
					if( _pTransition->IsTerminal() )
						bRet = true;
				}

				return bRet;
			}

			State*	_pAdjacentState;  // vertex to
			Symbol*	_pTransition;     // edge to an adjacent vertex
		};


	private:
		// get closure items in a state of a kernel item
		// !!!note: calculate direct closure from a kernel item
		void GetClosures( LR1Item* kernel, LR1Items& closure );

		// get all non-initial items
		void GetNonInitialItems( LR1Items& non_initials );

		// get all the corresponding transitional items from the items in the state
		// !!! if there is no corresponding transitional item are found, return NULL
		//     else return it and the state which the item belongs to
		// !!!note: be sure that 'from' is not a complete item
		LR1Item* GetTransitionalItem( const LR1Item* from, State*& to_stat );

	private:
		// all the items a state contains
		LR1Items		m_Items;
		// all the adjacent states 
		AdjacentStates	m_AdjacentStates;
		// the id of current state
		int				m_nId;

		static LALR1_DFA*		s_pFather;
		static int				s_nAllocatedId;

	public:
		/* auxiliary functions */
		void SetChanged( bool flag )
		{
			m_bChanged = flag;
		}

		bool GetChanged(void)
		{
			return m_bChanged;
		}

	private:
		/*
		 * auxiliary variable
		 */

        typedef map<LR1Item*, vector<LR1Item*>* > Kernel2Closures;
		typedef Kernel2Closures::iterator Kernel2ClosuresIter;

		// kernel to its closures
		Kernel2Closures	m_mapKernel2Closures;

		// structure of correponding transitional item
		struct TransItemInfo
		{
			explicit TransItemInfo( LR1Item* item = NULL, State* owner = NULL ) :
			_item(item), _owner(owner)
			{}

			LR1Item* _item;
			State*     _owner;
		};

		typedef list< pair<LR1Item*,TransItemInfo*> > Item2TransItem;
		typedef Item2TransItem::iterator Item2TransItemIter;
		Item2TransItem	m_lstItem2Transition;

		// used to judge whether lookahead set has changed in any of its items
		bool	m_bChanged;
	};

	typedef State::AdjacentState	AdjacentState;
	typedef State::AdjacentStates   AdjacentStates;

	class StateUtils
	{
	public:
		bool FindTransition( const AdjacentStates& trans_lst, const Symbol* t, State*& target )
		{
			typedef AdjacentStates::const_iterator Iter;
			for( Iter it = trans_lst.begin(); it != trans_lst.end(); ++it )
			{
				if( (*it)._pTransition == t )
				{
					target = (*it)._pAdjacentState;
					return true;
				}
			}

			return false;
		}

		bool CheckIdenticalState( const State* src, const vector<State*>& states, State*& duplicate_s, bool bInList = false )
		{
			typedef vector<State*>::const_iterator Iter;
			Iter it = states.begin();

			for( ; it != states.end(); ++it )
			{
				if( bInList && (*it == src) )
					continue;

				if( *(*it) == *src )
				{
					duplicate_s = *it;
					return true;
				}
			}

			return false;
		}

		// remove the state from adjacent_state_list
		// !!!a state correspond with one transition, there is impossible over 2 transitions
		//    connnect with a state,refer to theorem1
		void RemoveAdjacentState( AdjacentStates& trans_lst, const State* t )
		{
			typedef AdjacentStates::iterator Iter;
			Iter it = trans_lst.begin();
			for( ; it != trans_lst.end(); ++it )
			{
				if( (*it)._pAdjacentState == t ) // !!!note: compare pointers. not check whether two states are identical
					break;
			}

			assert( it != trans_lst.end() );
			trans_lst.erase( it );
		}

		// add an state into adjacent_state_list
		void AddAdjacentState( AdjacentStates& trans_lst, State* to, Symbol* edge )
		{
			typedef AdjacentStates::iterator AdjacentStatesIter;
			AdjacentState adj_stat(to, edge);
			AdjacentStatesIter it = trans_lst.begin();
			for( ; it != trans_lst.end(); ++it )
			{
				if( adj_stat < (*it) )
					break;
			}

			trans_lst.insert( it, adj_stat );
			//trans_lst.push_back( 
			//	AdjacentState(to, edge) 
			//	);
		}

		static StateUtils* Instance(void)
		{
			static StateUtils _utils;
			return & _utils;
		}
	protected:
		StateUtils()
		{}
	};
private:
	LALR1_DFA()
	{}

	// construct a LR0 DFA for the first
	void Construct_LR0_DFA();

	// construct lookaheads for each state's each item
	void Construct_LookAheads();

	// give number to each state
	// !!!note: used when has constructed all the states
	void NumberOnStates(void);

	// check whether a state has already been in DFA
	// if true, return the state of DFA; otherwise, return NULL
	// !!!note: bInList is a restriction condition, if true, pointer 'stat' is already in container newStates
	//          so skipped if iterate on the same pointer
	int CheckState( const State* stat, const vector<State*>& newStates, State*& duplicate, bool bInList = false );

private:
	typedef vector<State*> Graphy;
	// finite state automa with the data structure of graphy, utilized by adjacent-list representation
	Graphy	_fsa;  
};


/********************************************\
 * Generaotr is responsible for constructing
 * Action table & GOTO table, and support for
 * parsing
 *
\********************************************/ 
class LALR1_Parser;

enum ActionConstants
{
	ACT_UNDEF	=	0,  // action undefined
	ACT_SHIFT	=	1,  // shift
	ACT_REDUCE	=	2,  // reduce
	ACT_ACCEPT	=	4   // accpt
};

class LALR1_ParserGenerator
{
public:
	friend class LALR1_Parser;

	// !! as Action table entry:
	//    high byte represents for concreate action like SHIFT, ACCEPT, REDUCE
	//    low byte represents for concreate number, like 'num of a state' when execute SHIFT action,
	//    or 'num of production' when execute REDUCE action
	// !! as GOTO table entry:
	//    11xxxxxx xxxxxxxx represents for unvalid entry
	//    00xxxxxx xxxxxxxx represents for 'num of a state'
	typedef unsigned short TableEntry;
	typedef LALR1_DFA::State State;

	LALR1_ParserGenerator(): m_ppAction(NULL), m_ppGOTO(NULL),m_nStateCount(0), m_nInputSymCount(0),
		m_nNontermCount(0), m_nTerminalState(0)
	{}

	~LALR1_ParserGenerator()
	{
		Fininalize();
	}

	// !!!must be initilized, before any usage
	bool Initialize();

	// !!!free the memory of table Action & GOTO
	void Fininalize()
	{
		if( m_ppAction ) FreeActionTab();
		if( m_ppGOTO ) FreeGOTOTab();
	}

	// look up for an entry in Action table
	TableEntry LookUpActionTab( int nStateNo, int tok ) const
	{
		return m_ppAction[nStateNo][tok];
    }

	// look up for an entry in GOTO table
	TableEntry LookUpGotoTab( int nStateNo, int nNonTermNo ) const
	{
		return m_ppGOTO[nStateNo][nNonTermNo];
	}

	State* GetState( int id ) const
	{
		return LALR1_DFA::Instance()->_fsa[id];
	}

	// get the state which will terminate parsing
	int GetTerminalState(void) const
	{
		return m_nTerminalState;
	}

	const GrammarRule* GetRule( int id ) const
	{
		return &m_vAllGrammarRules[id];
	}

	// !!! be sure tok is between[K_INT, DOLLAR]
	int GetTokenType( int tok ) const
	{
		if( tok == INT_NUM || tok == FLOAT_NUM )
			return NUM;
		return tok;
	}

	// used for test:
	void out_put_tables(void);
private:
    // create action table
	bool CreateActionTab();
	// create GOTO table
	bool CreateGotoTab();
	// construct action table & GOTO table
	void ConstructDrivenTab();
	// set accept action
	void SetAcceptAction(void);

	void FreeActionTab()
	{
		delete []*m_ppAction;
		delete []m_ppAction;
		m_ppAction = NULL;
	}
	void FreeGOTOTab()
	{
		delete []*m_ppGOTO;
		delete []m_ppGOTO;
		m_ppGOTO = NULL;
	}

private:
	/*
	 * auxiliary functions:
	 */
	// set up the relations of mapping GOTO's subscript to a grammar production
	void SetupMappingRelation();

	// map a terminal to the subscript/sequence no in Action table
	// !!! be sure that tok¡Ê[K_INT, E_O_F]
	// !!! note: 'm_vAllTerminals' mush has been set up!!
	int Terminal2Ord( int tok ) const;

	// map a Rule to the subscript in GOTO table
	// !!!note: 'm_vAllGrammarRules' must has been set up!!!
	int Rule2Ord( const Rule* r );

	// map a subscript into a nonterminal
	const NonTerminal* GetNonTerminal( int id )
	{
		const vector<NonTerminal*>* nonterms =
			Grammar::Instance()->GetNonTerminals();
		assert( id >= 0 && id < nonterms->size() );

		return (*nonterms)[id];
	}

	int GetTerminal( int index ) const
	{
		return m_vAllTerminals[index].GetTerminalKind();
	}

private:
	// function object
	class TerminalFinder
	{
	public:
		TerminalFinder( int tok ) : m_nTok(tok)
		{}
		bool operator ()( const Terminal& t )
		{
			return t.GetTerminalKind() == m_nTok;
		}

		bool operator() ( const Terminal* t )
		{
			return t->GetTerminalKind() == m_nTok;
		}
	private:
		int m_nTok;
	};

private:
	/*
	 * auxiliary data structure:
 	 */
	vector<GrammarRule>	m_vAllGrammarRules; // used for mapping GOTO's subscript to a grammar production
	vector<Terminal>    m_vAllTerminals;    // used for mapping a terminal into Action table's subscript

private:
	TableEntry**	m_ppAction;
	TableEntry**	m_ppGOTO;
	int				m_nStateCount;		// height for table Action & GOTO
	int				m_nInputSymCount;	// length of table Action
	int				m_nNontermCount;	// length of table GOTO

	int				m_nTerminalState;   // the terminal of terminal state which contain an item terminates parsing
};

/*
void print_lr_parse_step( ostream& os,
                          const Token* tok, 
                          const ParserStack& list,
                          int act_type,
                          int transit_state,
                          const GrammarRule* rule );
*/

/*******************************************************\
 * lalr parser
\*******************************************************/
class LALR1_Parser
{
protected:
	typedef LALR1_ParserGenerator::State		State;
	typedef LALR1_ParserGenerator::TableEntry	TableEntry;

	enum StackItemConstants
	{
		TERMINAL = 0, 
		NONTERMINAL = 1,
		STATE
	};
	struct StackItem
	{
		StackItem( StackItemConstants type = STATE, void* item = NULL ) : _type(type), _pItem(item)
		{}
		StackItemConstants	_type; 
		void*				_pItem;
	};
	typedef list<StackItem*> ParserStack;
public:
	LALR1_Parser( FILE* f, const LALR1_ParserGenerator& ref ) : m_generator(ref), m_Scanner(f)
	{}

	~LALR1_Parser()
	{
		Clear();
	}

	void Reset(  )
	{
		Clear();
		// 
	}

	void Parse(void);
    
    void print_lr_parse_step( ostream& os,
                          const Token* tok, 
                          const ParserStack& list,
                          int act_type,
                          int transit_state = 0,
                          const GrammarRule* rule = NULL );

private:
	bool Reduce( const GrammarRule* prod, 
                 int rule_no, 
                 char (&szPrompt)[100] );

	void Clear(void)
	{
		list<Token*>::iterator it = m_lstTokens.begin();
		for( ; it != m_lstTokens.end(); ++it )
		{
			delete (*it);
		}
		m_lstTokens.clear();
	}

private:
	void ReportErrorOnShift( char (&szPrompt)[100], const Token* tok, int nLineNo );
private:
	const LALR1_ParserGenerator&	m_generator;
	ParserStack		m_stkParser;
	Scanner			m_Scanner;
	list<Token*>	m_lstTokens;
};

// used for test:
void out_put_production( const GrammarRule& prod, string& s );
#endif
