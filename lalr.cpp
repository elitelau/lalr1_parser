#include "lalr.h"

#include <deque>
#include <algorithm>
#include <iomanip>
using namespace std;

/**************************\
 * LR0Item
\**************************/
map<NonTerminal*, LR0Items*>	LR0Item::s_mapNonterm2Items = map<NonTerminal*, LR0Items*>();

void LR0Item::Initialize()
{
	GrammarRepresentation& grammar = Grammar::Instance()->GetGrammar();
	typedef GrammarRepresentation::iterator Iter;
	typedef Grammar::Productions Productions;
	typedef Productions::iterator ProdIter;
	NonTerminal *prod_head = NULL;
	ProdIter it_prod;

	LR0Item* item = NULL;
	pair< map<NonTerminal*, LR0Items*>::iterator, bool > pr;

	for( Iter it = grammar.begin(); it != grammar.end(); ++it )
	{
		prod_head = (*it).first;

		// add a corresponding item which treated as an vertex has not been visited before
		it_prod = (*it).second.begin();
		item = new LR0Item( make_pair(prod_head, (*it_prod)->GetRule()), (*it_prod)->GetRule()->begin() );
		item->m_pData = new VertexAttr(); 

		// add into list
		pr = s_mapNonterm2Items.insert( ValType(prod_head, new LR0Items()) );
		assert( pr.second == true );
		(*pr.first).second->push_back(item);

		for( ++it_prod; it_prod != (*it).second.end(); ++it_prod )  // iterate all the productions
		{
			item = new LR0Item( make_pair(prod_head, (*it_prod)->GetRule()), (*it_prod)->GetRule()->begin() );
			item->m_pData = new VertexAttr();  // item has not been visited
			(*pr.first).second->push_back(item);
		}
	}
}

void LR0Item::Finalize()
{
	typedef map<NonTerminal*, LR0Items*>::iterator Iter;
	Iter it = s_mapNonterm2Items.begin();
	typedef LR0Items::iterator LR0ItemIter;

	for( ; it != s_mapNonterm2Items.end(); ++it )
	{
		LR0Items* items = (*it).second;
		for( LR0ItemIter it_item = items->begin(); it_item != items->end(); ++it_item )
		{
			// free extra data storage
			VertexAttr* pAttr = static_cast<VertexAttr*>((*it_item)->m_pData);
			delete pAttr;
			(*it_item)->m_pData = NULL; // !!!must need

			// free items storage itself
			delete *it_item;			
		}

		delete items;
	}
}

void LR0Item::output( string& s )
{
	out_put_production( m_Rule, s );

	s += GetLineEnd();
	s += " | period: ";

	if( IsCompleteItem() )
	{
		s+= "NULL";
	}
	else
	{
		if( (*m_Period)->IsTerminal() )
		{
			s += TokenTypeStr[static_cast<Terminal*>(*m_Period)->GetTerminalKind()];
		}
		else
			s += static_cast<NonTerminal*>(*m_Period)->GetDesp();
	}

//    s += GetLineEnd();
}

/*
void LR0Item::ResetForDfs()
{
	typedef map<NonTerminal*, LR0Items*>::iterator Iter;
	Iter it = s_mapNonterm2Items.begin();
	typedef LR0Items::iterator LR0ItemIter;

	for( ; it != s_mapNonterm2Items.end(); ++it )
	{
		LR0Items* items = (*it).second;
		for( LR0ItemIter it_item = items->begin(); it_item != items->end(); ++it_item )
		{
			// free extra data storage
			VertexAttr* pAttr = static_cast<VertexAttr*>((*it_item)->m_pData);
			pAttr->Reset();
		}
	}
}*/

void LR0Item::GetDirectClosure( LR0Items& items, bool bRetCopy )
{
	typedef LR0Items::iterator ItemIter;

	if( !IsPoint2Nonterm() ) // item must start with a nonterminal
	{
		return;
	}

	NonTerminal* n = static_cast<NonTerminal*>( *m_Period );
	LR0Items* direct_closures = NULL;
	direct_closures = s_mapNonterm2Items[n];

	ItemIter it = direct_closures->begin();
	
	for( ; it != direct_closures->end(); ++it )
	{
		if( bRetCopy )
		{
			items.push_back( (*it)->Clone() );
		}
		else
		{
			items.push_back( *it );
		}
	}
}

// !!算法实现说明：
// 求项的闭包，需要用到dfs深度优先遍历算法，而深度优先遍历算法要求加入额外的变量
// 但是，最终项的数据结构，我们并不需要保留这些额外的变量
// 因此，采用数据结构的时候，我们对项的拷贝项进行额外变量的运算，最终保留下来的项并没有这些数据结构
// 这样的方式，便于统一内存管理
void LR0Item::Closure( LR0Items & _closure, bool bRetCopy )
{
	typedef LR0Item Vertex;
	typedef LR0Items::iterator ItemIter;
	typedef deque<Vertex*> VertexStack;

	ItemIter it;

	// select an start vertex, which has not been visited before
	Vertex* v = this->Clone();
	v->m_pData = new VertexAttr();

	Vertex* start = v;
	Vertex* next = NULL;

	VertexStack _stack;

	while(1)
	{
Add:
		static_cast<VertexAttr*>(v->m_pData)->_bVisited = true;
		_stack.push_back( v );

Check:
		if( !v->IsPoint2Nonterm() ||                                     // if an item start with a terminal, it's a dead end
			static_cast<VertexAttr*>(v->m_pData)->_nSearchIndicator >=   // or else, all the adjacent vertices has been visited
			s_mapNonterm2Items[static_cast<NonTerminal*>(*v->m_Period)]->size() )
		{
			if( v == start ) // the start vertex is a dead end
			{
				break;
			}

			_closure.push_back( _stack.back() ); // !!! insert reference of a vertex, not a copy
			_stack.pop_back();

			v = _stack.back();
			goto Check;
		}
		else
		{
			next = (*s_mapNonterm2Items[static_cast<NonTerminal*>(*v->m_Period)])
			[static_cast<VertexAttr*>(v->m_pData)->_nSearchIndicator];
			++static_cast<VertexAttr*>(v->m_pData)->_nSearchIndicator;

			if( static_cast<VertexAttr*>(next->m_pData)->_bVisited == true )
			{
				goto Check;
			}
			else
			{
				v = next;
				goto Add;
			}
		}

	}

	delete static_cast<VertexAttr*>( start->m_pData );
	delete start;

	// reset dfs state info
	for( it = _closure.begin(); it != _closure.end(); ++it )
	{
		// reset attributes
		static_cast<VertexAttr*>((*it)->m_pData)->Reset();

		if( bRetCopy ) // control flag, return copy data
		{
			*it = (*it)->Clone();
		}
	}
}

LR0Item* LR0Item::Clone()
{
	return new LR0Item(*this);
}

int LR0Item::operator ==( const LR0Item& other )
{
	int nCmp = 0;

	// for the first, compare two nonterminals which productions start by its names' alphabeta
	nCmp = strcasecmp( this->m_Rule.first->GetDesp(), other.m_Rule.first->GetDesp() );
	if( nCmp != 0 )
		return nCmp > 0 ? 1 : -1;

	// secondly, compare the number of symbols a production rule contains
	nCmp = this->m_Rule.second->size() - other.m_Rule.second->size();
	if( nCmp != 0 )
		return nCmp > 0 ? 1 : -1;

	// iterate all the symbols, and one-by-one compare corresponding symbols
	typedef Rule::iterator Iter;
	Iter it_this = this->m_Rule.second->begin();
	Iter it_other = other.m_Rule.second->begin();

	for( ; it_this != this->m_Rule.second->end(); ++it_this, ++it_other )
	{
		if( (*it_this)->IsTerminal() != (*it_other)->IsTerminal() ) // two symbols are of different kind 
		{
			return (*it_this)->IsTerminal() ? -1 : 1; // terminal has inferior priority than nonterminal
		}
		else
		{
			if( (*it_this)->IsTerminal() )
			{
				nCmp = static_cast<Terminal*>(*it_this)->GetTerminalKind() - 
					static_cast<Terminal*>(*it_other)->GetTerminalKind();
				if( nCmp != 0 )
					return nCmp > 0 ? 1 : -1; // compare two terminals by its type value
			}
			else
			{
				nCmp = strcasecmp( static_cast<NonTerminal*>(*it_this)->GetDesp(),
					static_cast<NonTerminal*>(*it_other)->GetDesp() );
				if( nCmp != 0 )
					return nCmp > 0 ? 1 : -1;
			}
		}
	}

	// if two items are with the same grammar rule, then compare the positions where periods stand
	assert( this->m_Rule.first == other.m_Rule.first &&
		this->m_Rule.second == other.m_Rule.second );

	if( this->m_Period < other.m_Period )
		return -1;
	return this->m_Period > other.m_Period ? 1 : 0;
}

/**************************\
 * LR1Item
\**************************/

LR1Item* LR1Item::s_pStartItem = NULL;

void LR1Item::Closure( LR1Items& _closure )
{
	LR0Items items;
    this->Closure( items, false );

	typedef LR0Items::iterator LR0ItemIter;
	for( LR0ItemIter it = items.begin(); it != items.end(); ++it )
	{
		LR1Item* item = new LR1Item( (*it)->GetRule() );
		_closure.push_back(item);
	}
}

void LR1Item::GetDirectClosure( LR1Items& _closure )
{
	LR0Items items;
	this->GetDirectClosure( items, false );

	typedef LR0Items::iterator LR0ItemIter;
	for( LR0ItemIter it = items.begin(); it != items.end(); ++it )
	{
		LR1Item* item = new LR1Item( (*it)->GetRule() );
		_closure.push_back(item);
	}
}

/******************************************************\
   class LALR1_DFA::State
\******************************************************/
LALR1_DFA* LALR1_DFA::State::s_pFather = NULL;
int LALR1_DFA::State::s_nAllocatedId = -1;


bool LALR1_DFA::State::AddItem( LR1Item* item, bool bIsCopy )
{
	bool bRet = false;
	LR1Items::iterator it = m_Items.begin();

	// !!!note: insert an item with sorted order

	int nCmp = 0;

	for( ; it != m_Items.end(); ++it )
	{
		nCmp = *static_cast<LR0Item*>(item) == 
			*static_cast<LR0Item*>(*it);

		if( nCmp <= 0 )
		{
			break;
		}
	}

	if( m_Items.size() == 0 || nCmp != 0 ) // insert an identical item will cause a vain action
	{
		if( bIsCopy )
		{
			item = item->Clone();
		}

		m_Items.insert( it, item );
		bRet = true;
	}

	return bRet;
}

bool LALR1_DFA::State::AddItem( const LR1Items &items, LR1Items& duplicate_items )
{
	bool bRet = false;
	LR1Items::const_iterator it = items.begin();

	if( items.size() == 0 )
	{
		bRet = true;
		goto Exit_Func;
	}

	for( ; it != items.end(); ++it )
	{
		if( false == AddItem(*it) )  // the current item has already exsited, so it is a duplicate item
			duplicate_items.push_back( *it );
	}

	if( duplicate_items.size() == 0 )
		bRet = true;

Exit_Func:
	return bRet;
}

bool LALR1_DFA::State::AddItem( const LR1Items& items )
{
    LR1Items  duplicate_items;
    return AddItem(items, duplicate_items);
}

int LALR1_DFA::State::CheckTransition( const Symbol* t, const vector<State*>& newStates, State*& duplicate_t )
{
	typedef vector<State*>::const_iterator StatIter;
	typedef AdjacentStates::iterator Iter;

	int nRet = -1;

	// check whether an identical transition has already exsited
	Iter it = m_AdjacentStates.begin();
	for( ; it != m_AdjacentStates.end(); ++it )
	{
		if( (*it)._pTransition == t )  // has an identical transition
			break;
	}

	if( it != m_AdjacentStates.end() )
	{
		// check whether the identcical transition occured in newly created states(i.e. newStates)
		StatIter it_stat = newStates.begin();
		for( ; it_stat != newStates.end(); ++it_stat )
		{
			if( *it_stat == (*it)._pAdjacentState ) // !!! compare pointers, not invoke State::operator = function
				break;
		}

		duplicate_t = (*it)._pAdjacentState;
		if( it_stat != newStates.end() )
		{
			nRet = 0;
			goto Exit_Func;
		}
		else
		{
			// duplicate transition occured in a state in DFA
			goto Exit_Func;
		}
	}

	nRet = 1;

Exit_Func:
	return nRet;
}

bool LALR1_DFA::State::operator ==( const State& another )
{
	bool bRet = false;
	LR1Items::iterator it_this = m_Items.begin();
	LR1Items::const_iterator it_other = another.m_Items.begin();

	// for the first two identical states must have the same numbers of items
	if( m_Items.size() != another.m_Items.size() )
		goto Exit_Func;

	// iterate all the items, and compare two corresponding items one-by-one
	for( ; it_this != m_Items.end(); ++it_this, ++it_other )
	{
		if( (*static_cast<LR0Item*>(*it_this) == *static_cast<LR0Item*>(*it_other)) != 0 )
			goto Exit_Func;
	}

	bRet = true;

Exit_Func:
	return bRet;
}

#define FREE_ITEMS( lr1_items ) \
{\
	for( LR1Items::iterator it_item = lr1_items.begin(); it_item != lr1_items.end(); ++it_item ) \
   {\
      delete *it_item; \
   }\
}


 /****************************************************************************\
  *  定理1：计算转换先求解所有新状态的过程中，不可能存在两个相同的新状态，
  *        且这两个新状态来自不同的转换
  *  证： cur为当前状态，s1, s2为两转换状态.
          假设 cur-->s1(α) cur-->s2(β), α≠β
          s1由核心项X->.αγ转换为X->α.γ求解闭包得到
          s2由核心项Y->.βδ转换为X->β.δ求解闭包得到
          由于s1=s2, 故s1也包含项X->β.δ  由于α≠β,X->α.γ闭包不可能包含该item
          故，假设矛盾          
  *
  
  *  定理2：计算转换求解完所有新状态的后，不可能存在两个相同的转换，
  *        一个转换到新状态，另外一个转换到DFA中已经存在的状态
  *  证： cur为当前状态，s, o为两转换状态,分别表示为新、旧状态
          cur-->s(α), cur-->o(β), 因为当前状态计算出的转化应当都是新状态
          但s' = O, 所以delete s',保留o. 
          假设α=β, 则cur-->s'(α),而cur-->s(α)，这在求解完所有的新状态过程中，
          这种情况已经做了处理(s'与s合并)，所以不可能出现在现阶段
          
          假设矛盾，得证
           
           
 \****************************************************************************/  
// modified by liujian 2007-10-9
void LALR1_DFA::State::Transit( vector<State*>& states )
{
	//typedef LALR1_DFA::AdjacentState AdjacentState;
	//typedef LALR1_DFA::AdjacentStates AdjacentStates;

	AdjacentStates		trans_stats;
	AdjacentStatesIter	it_trans;
	vector<State*>::iterator    it_stat;
	
	LR1Items::size_type	i		=	0;
	LR1Item*			item	=	NULL;

	LR1Item*			newItem =	NULL;
	LR1Items			_closures;
	State*				newState= NULL;
	State*				duplicate_t = NULL; // duplicate transition
	State*				duplicate_s = NULL; // duplicate state

	set<State*>			duplicates;         // duplicate states
	set<State*>::iterator it_duplicate;

	bool				bResult = false;


	for( ; i < m_Items.size(); ++i )
	{
		item = m_Items[i];
		if( !item->IsCompleteItem() )
		{
			// a new transitional item
			newItem = item->Clone();
			newItem->MovePeriod(); // move forward by a step

			// a new transitional state
			newState = new State();
			newState->AddItem( newItem );
			if( !newItem->IsCompleteItem() && newItem->IsPoint2Nonterm() )
			{   // closures of the kernel item
				newItem->Closure(_closures);
				newState->AddItem(_closures);
				_closures.clear();
			}

			// make sure the new transitional state is sole in 'new_trans_state_list' &
			// transitions itself are deterministic 
			
			// make sure the new transitinal state is sole in 'new_trans_state_list'
			bResult = StateUtils::Instance()->FindTransition( trans_stats, *item->GetPeriod(), duplicate_t );
			if( bResult ) // there is an identical transition to a newly created state
			{
				bResult = duplicate_t->Combine( newState ) >= 0;
				delete newState;
				if( bResult ) // 'newState' is not identical with 'duplicate_t'
				{
					bResult = StateUtils::Instance()->CheckIdenticalState( duplicate_t, states, duplicate_s, true );
					if( bResult ) // 'duplicate_t' is identical with a new trastional state after combination
					{
						StateUtils::Instance()->RemoveAdjacentState( trans_stats, duplicate_t );
						states.erase( find(states.begin(), states.end(), duplicate_t) ); // !!!remove from 'new_state_list'
						delete duplicate_t;
						StateUtils::Instance()->AddAdjacentState( trans_stats, duplicate_s, *item->GetPeriod() );
					}
				}
			}
			else
			{
				bResult = StateUtils::Instance()->CheckIdenticalState( newState, states, duplicate_s );
				if( bResult ) // 'newState' is identical with a new transitional state
				{
					assert(0); // impossible happened, refer to theorem1
					delete newState;
					StateUtils::Instance()->AddAdjacentState( trans_stats, duplicate_s, *item->GetPeriod() );
				}
				else
				{
					StateUtils::Instance()->AddAdjacentState( trans_stats, newState, *item->GetPeriod() );
					states.push_back( newState );
				}
			}
		} // if( !item->IsCompleteItem() )
	} // for( ; i < m_Items.size();

	for( it_trans = trans_stats.begin(); it_trans != trans_stats.end(); ++it_trans )
	{

		// check whether there is also an identical transition to a state in DFA
		// ...uncessary to do it, refer to theorem2

		// check whether the newly created states are identical with the ones in DFA
		bResult = StateUtils::Instance()->CheckIdenticalState( (*it_trans)._pAdjacentState, s_pFather->_fsa, duplicate_s );
		if( bResult ) // exsit an identical state in DFA
		{
			it_stat = find(states.begin(), states.end(), (*it_trans)._pAdjacentState);
			if( it_stat != states.end() ){
				duplicates.insert(*it_stat);  // duplicate new state
				states.erase( it_stat );
			}

			// combine into a transition
			StateUtils::Instance()->AddAdjacentState( m_AdjacentStates, duplicate_s, (*it_trans)._pTransition );
		}
		else
		{
			m_AdjacentStates.push_back( *it_trans ); // add into adjacent_state_list
		}
	} // for( it_trans = trans_stats.begin(); 

	// remove from 'adjacent_state_list' and destroy the state
	for( it_duplicate =duplicates.begin(); it_duplicate != duplicates.end(); ++it_duplicate  )
	{
		// !!!note: one state is corresponding with one 
		StateUtils::Instance()->RemoveAdjacentState( trans_stats, *it_duplicate ); 
		delete *it_duplicate;
	}
}

/*
void LALR1_DFA::State::Transit( vector<State*>& states )
{
	vector<LR1Item*>::size_type i = 0;
	LR1Item* item = NULL;
	State* newState = NULL;
	LR1Item* newItem = NULL;
	LR1Items _closure;

	LR1Items _duplicates; 

	State* duplicate_s = NULL; // duplicate state

	// used for test only:
	if( this->m_nId == 6 )
	{
		duplicate_s = NULL;
	}

	int nResult = -1;
	State* duplicate_t = NULL; // duplicate transition

	for( ; i < m_Items.size(); ++i )
	{
		item = m_Items[i];
		if( !item->IsCompleteItem() )
		{
			newState = new State();

			newItem = item->Clone();
			newItem->MovePeriod();  // move forward by a step
			newState->AddItem(newItem);

			if( !newItem->IsCompleteItem() && newItem->IsPoint2Nonterm() ) // period point to a nonterminal
			{
				newItem->Closure( _closure );
				newState->AddItem(_closure, _duplicates);
				assert( _duplicates.size() == 0 );
				FREE_ITEMS( _duplicates );

				_closure.clear();
				_duplicates.clear();
			}

			nResult = CheckTransition( *item->GetPeriod(), states, duplicate_t );

			if( nResult > 0 ) // there is no identical transitions
			{
				nResult = s_pFather->CheckState( newState, states, duplicate_s );
				if( nResult > 0 )  // newState is a completely new state which is not identical with any state
				{
					// used for test:
					newState->print();

					this->AddAdjacentState( newState, *item->GetPeriod() );
					states.push_back(newState);
				}
				else
				{
					// identical with a newly created state or a state in DFA
					delete newState;
					this->AddAdjacentState( duplicate_s, *item->GetPeriod() );
				}
			}
			else // exsit an identical transition
			{
				if( 0 == nResult ) // there is an identical transition to a newly created state
				{
					if( duplicate_t->Combine( newState ) >= 0 ) // newState is not identical with duplicate_t
					{
						delete newState;

						// duplicate_t is a new state which combines items in newState
						// but duplicate_t has been added into adjacent-state-list
						// so, if duplicate_t is identical with any state in new state list or in DFA,
						// it must be removed from adjacent-state-list, and be freed for memory
						// !!!note: duplicated_t is must a destination from different transition(edge)
						// !!!note: duplicate_t is a pointer which has already exsited in container 'states'
						//          so it should skipped when iterate(states) on this pointer 
						nResult = s_pFather->CheckState( duplicate_t, states, duplicate_s, true );

						if( nResult <= 0 ) // identical with a newly created state or a state in DFA 
						{
							this->RemoveAdjacentState( duplicate_t );
							states.erase( find(states.begin(), states.end(), duplicate_t) ); // !!!remove from new state list
							delete duplicate_t;
							this->AddAdjacentState( duplicate_s, *item->GetPeriod() );
						}
						else
						{
							;
							// used for test:
							duplicate_t->print();
						}
					}
					else // newState is identical with duplicate_t
					{
						delete newState;
					}
				}
				else // has an identical transition to a state in DFA
				{   // !!!说明，这是个"伪问题"，因为转换状态都是在计算transit的时候建立
					// report error
					assert( 0 ); 
				}
			}
		}
	}
} */

void LALR1_DFA::State::BuildAssociationsOnKernel2Closures()
{
	typedef Kernel2Closures::value_type ValType;

	LR1Items* closures = NULL; // closure
	// find out all kernel items
	LR1Items kernels;
	LR1Items::size_type i = 0;

	/*
	if( this == s_pFather->GetStartState() ) // START state treated in a special way
	{
		closures = new LR1Items();
		for( ; i < m_Items.size(); ++i )
		{
			if( m_Items[i] != LR1Item::GetStartItem() )
			{
				closures->push_back( m_Items[i] );
			}
		}

		m_mapKernel2Closures.insert( ValType( LR1Item::GetStartItem(), closures ) );
		return;
	}*/


	for( i = 0; i < m_Items.size(); ++i )
	{
	//	if( !m_Items[i]->IsInitialItem() && !m_Items[i]->IsCompleteItem() ) // modified by liujian 2007.9.26
		if( !m_Items[i]->IsCompleteItem() )
			kernels.push_back( m_Items[i] );
	}

	// build up association between a kernel item to its closure items
	for( i = 0; i < kernels.size(); ++i )
	{
		closures = new LR1Items();
		GetClosures( kernels[i], *closures );

		if( 0 == closures->size() )
		{
			delete closures;
		}
		else
			m_mapKernel2Closures.insert( ValType(kernels[i], closures) );
	}
}

void LALR1_DFA::State::PrintAssociationOnKernel2Closures()
{
	if( m_mapKernel2Closures.size() > 0 )
		cout << endl << "state: s" << m_nId; 
	
	for( Kernel2ClosuresIter it = m_mapKernel2Closures.begin();
		it != m_mapKernel2Closures.end(); ++it )
	{
		cout << endl << " kernel item: ";
		static_cast<LR0Item*>( (*it).first )->print();
		cout << endl << " has closure items: " ;

		for( vector<LR1Item*>::size_type i = 0; 
			i < (*it).second->size(); ++i )
		{
			static_cast<LR0Item*>( (*(*it).second)[i] )->print();
		}
	}

	if( m_mapKernel2Closures.size() > 0 )
		cout << endl << "----------------------";
}

void LALR1_DFA::State::PrintAssociationOnItem2TransItem()
{
	if( m_lstItem2Transition.size() > 0 )
		cout << endl << "state: s" << m_nId; 

	for( Item2TransItemIter it = m_lstItem2Transition.begin();
		it != m_lstItem2Transition.end(); ++it )
	{
		static_cast<LR0Item*>( (*it).first )->print();
		cout << endl << "   has transitional item: ";

		static_cast<LR0Item*>( (*it).second->_item )->print();
		cout << "    in state s" << (*it).second->_owner->m_nId;
	}

	if( m_lstItem2Transition.size() > 0 )
		cout << endl << "----------------------";
}

void LALR1_DFA::State::BuildAssociationOnItem2TransItem()
{
	State* owner = NULL;
	LR1Items::size_type i = 0;
	TransItemInfo* trans_inf = NULL;

	for( ; i < m_Items.size(); ++i )
	{
		if ( !m_Items[i]->IsCompleteItem() )
		{
			LR1Item* trans_item = GetTransitionalItem( m_Items[i], owner );
			assert( trans_item );

			trans_inf = new TransItemInfo( trans_item, owner );
			m_lstItem2Transition.push_back( make_pair(m_Items[i], trans_inf) );

		}
	}
}

int LALR1_DFA::State::Combine( const State* other )
{
    int nRet = -1;
	LR1Items _duplicates;

	LR1Items::const_iterator it = other->m_Items.begin();
	for( ; it != other->m_Items.end(); ++it )
	{
		if( false == AddItem(*it, true) )  // the current item has already exsited, so it is a duplicate item
		{
			_duplicates.push_back( *it );
		}
	}

	if( _duplicates.size() != other->m_Items.size() ) // combine no items
	{
		nRet = 1;
	}

	return nRet;
}

void LALR1_DFA::State::UpdateLookaheadsOnClosures()
{
	LR1Items* _closure = NULL;
	LR1Item*  _kernel = NULL;

	TerminalSet first_set;
	Period		period;

	bool bHasEmpty = false;

	for( Kernel2ClosuresIter it = m_mapKernel2Closures.begin(); 
		 it != m_mapKernel2Closures.end(); ++it )
	{
		_closure = (*it).second;
		_kernel = (*it).first;

		bHasEmpty = false; // !!! must need
		first_set.clear();

		period = _kernel->GetPeriod(); // !!! it has guarantee that _kernl is a kernel item
		++period;

		if( period == _kernel->GetRule().second->end() ) // treated specially as empty
		{
			bHasEmpty = true;
		}
		else
		{
			CalculateFirstSet( period,
							   _kernel->GetRule().second->end(), 
                               first_set, bHasEmpty );
		}

		if( !first_set.empty() )
		{
			// propagate lookaheads to its closure items
			for( LR1Items::iterator it_closure = _closure->begin(); 
				it_closure != _closure->end(); ++it_closure )
			{
				if( (*it_closure)->AddLookahead(first_set) )
					m_bChanged = true;
			}
		}

		if( bHasEmpty )
		{
			// propagate lookaheads to its closure items
			for( LR1Items::iterator it_closure = _closure->begin(); 
				it_closure != _closure->end(); ++it_closure )
			{
				if( (*it_closure)->AddLookahead( _kernel->GetLookahead() ) )
					m_bChanged = true;
			}
		}
	}
}


void LALR1_DFA::State::GetDeterministicTransition( int trans_sym, vector< pair<LR1Item*, LR1Item*> >& from_to )
{
	Item2TransItemIter it = m_lstItem2Transition.begin();
	Terminal* t = NULL;
	for( ; it != m_lstItem2Transition.end(); ++it )
	{
		if( (*it).first->IsPoint2Nonterm() )
			continue;

		t = static_cast<Terminal*>(*((*it).first->GetPeriod()));
		if( t->GetTerminalKind() == trans_sym )
		{
			from_to.push_back( make_pair( (*it).first, static_cast<TransItemInfo*>( (*it).second)->_item ) );
		}
	}
}

void LALR1_DFA::State::UpdateLookaheadsOnTransitions()
{
	Item2TransItemIter it = m_lstItem2Transition.begin();
	LR1Item* item		= NULL;
	LR1Item* trans_item = NULL;
	State*   trans_stat = NULL;

	for( ; it != m_lstItem2Transition.end(); ++it )
	{
		item	   = (*it).first;
		trans_item = (*it).second->_item;
		trans_stat = (*it).second->_owner;

		// in the case of tranisitions to state itself, do no special actions
		// for an item in a state may be transit to a different item
		// added by liujian, 2007.10.04

		if( trans_item->AddLookahead( item->GetLookahead() ) )
		{
			trans_stat->m_bChanged = true;
		}
	}
}

int LALR1_DFA::State::CheckConflict(list< pair<LR1Item*, LR1Item*> >& reduce_reduce_items, 
									list< pair<LR1Item*, LR1Item*> >& shf_reduce_items,
									list< vector<int>*>& reduce_reduce_conflicts, 
									list<int>& shf_reduce_conflicts )
{
	LR1Items::size_type i = 0, j = 0;
	LR1Items complete_items;
	LR1Items items_on_term;  // an item points to a terminal
	LR1ItemsIter it = m_Items.begin();

	int nRet  = NO_CONFLICT;

	for( ; it != m_Items.end(); ++it )
	{
		if( (*it)->IsCompleteItem() )
		{
			complete_items.push_back(*it);
		}
		else
		{
			if( !(*it)->IsPoint2Nonterm() )
			    items_on_term.push_back(*it);
		}
	}

	// check reduce_reduce conflict
	// if two complete items in a state have common in lookaheads, they are of reduce_reduce conflict
	for( i = 0; i < complete_items.size(); ++i )
	{
		const TerminalSet& one = complete_items[i]->GetLookahead();
		for( j = i+1; j < complete_items.size(); ++j )
		{
			const TerminalSet& other = complete_items[j]->GetLookahead();
			vector<int> difference; 
			set_intersection( one.begin(), one.end(), other.begin(), other.end(), back_inserter(difference) );
			
			if( !difference.empty() ) // have intersection
			{
				reduce_reduce_items.push_back( make_pair(complete_items[i], complete_items[j]) );
				reduce_reduce_conflicts.push_back(
					new vector<int>( difference.begin(), difference.end() )
					);
			}
		}
	}

	// check shift-reduce conflict
	for( i = 0; i < complete_items.size(); ++i )
	{
		const TerminalSet& one = complete_items[i]->GetLookahead();
		for( j = 0; j < items_on_term.size(); ++j )
		{
			Terminal* t = static_cast<Terminal*>( *items_on_term[j]->GetPeriod() );
			if( one.find(t->GetTerminalKind()) != one.end() )
			{
				shf_reduce_items.push_back( make_pair(complete_items[i], items_on_term[j]) );
				shf_reduce_conflicts.push_back( t->GetTerminalKind() );
			}
		}
	}

	if( reduce_reduce_items.size() > 0 )
	{
		nRet |= REDUCE_REDUCE_CONLICT;
	}

	if( shf_reduce_items.size() > 0 )
	{
		nRet |= SHIFT_REDUCE_CONFLICT;
	}

	return nRet;
}

class LR1ItemComper
{
public:
	LR1ItemComper( LR1Item* item ) : m_Item(item)
	{}
	bool operator() ( const LR1Item* match )
	{
		return (*m_Item == *match) == 0;
	}

private:
	LR1Item*	m_Item;
};


void LALR1_DFA::State::GetClosures( LR1Item* kernel, LR1Items& closure )
{
	typedef LR1Items::size_type SizeType;
	typedef LR1Items::iterator Iter;
	
	LR1Items _closure;
//	kernel->Closure( _closure );
	kernel->GetDirectClosure( _closure ); // modified by liujian 2007.10.10

	SizeType i = 0;
	for( ; i < m_Items.size(); ++i )
	{
		Iter it = find_if( _closure.begin(), _closure.end(), LR1ItemComper(m_Items[i]) );
		if( it != _closure.end() )
			closure.push_back( m_Items[i] );
	}

	// must need!!!
	for( i = 0; i < _closure.size(); ++i )
		delete _closure[i];
}

void LALR1_DFA::State::GetNonInitialItems( LR1Items& non_initials )
{
	LR1Items::size_type i = 0;
	for( ; i < m_Items.size(); ++i )
	{
		if( !m_Items[i]->IsInitialItem() ) // modified by liujian, 2007.10.3
			non_initials.push_back( m_Items[i] );
	}
}

// !!!note: the corresponding transitional item could be a kernel item in the transitional state
//    or else, it must be a complete item
LR1Item* LALR1_DFA::State::GetTransitionalItem( const LR1Item* from, State*& to_stat )
{
	Symbol* trans_sym = *from->GetPeriod();
	LR1Item* trans_item = NULL;

	// find transitional edge in adjacent list
	AdjacentStatesIter it = m_AdjacentStates.begin();
	for( ; it != m_AdjacentStates.end(); ++it )
	{
		if( (*it)._pTransition == trans_sym )
		{
			break;
		}
	}

	assert( it != m_AdjacentStates.end() );
	to_stat = (*it)._pAdjacentState;

	LR1Items non_initials;
	to_stat->GetNonInitialItems( non_initials );

	LR1Items::size_type i = 0;
	for( ; i < non_initials.size(); ++i )
	{
		if( from->CheckTransition(non_initials[i]) )
			trans_item = non_initials[i];
	}

	return trans_item;
}

/* 2007.9.20
void LALR1_DFA::State::Transit( vector<State*>& states )
{
	vector<LR1Item*>::size_type i = 0;
	LR1Item* item = NULL;
	State* newState = NULL;
	LR1Item* newItem = NULL;
	LR1Items _closure;

	LR1Items _duplicates; 

	State* old = NULL;

	for( ; i < m_Items.size(); ++i )
	{
		item = m_Items[i];
		if( !item->IsCompleteItem() )
		{
			newState = new State();

			newItem = item->Clone();
			newItem->MovePeriod();  // move forward by a step
			newState->AddItem(newItem);

			if( !newItem->IsCompleteItem() && newItem->IsPoint2Nonterm() ) // period point to a nonterminal
			{
				newItem->Closure( _closure );
				newState->AddItem(_closure, _duplicates);
				FREE_ITEMS( _duplicates );

				_closure.clear();
				_duplicates.clear();
			}

			old = s_pFather->CheckState(newState);
			if( old ) // the state has already exsited in DFA
			{
				delete newState;
				if(	!CheckEdge( old, *item->GetPeriod() )	)  // be sure add into a new edge
					this->AddAdjacentState( old, *item->GetPeriod() );
			}
			else
			{
				// used for test:
			    newState->print();

				this->AddAdjacentState( newState, *item->GetPeriod() );
				states.push_back(newState);
			}
		}
	}
}*/

void LALR1_DFA::State::print_transitions()
{
	cout << endl << "state s" << m_nId << " has "<<  m_AdjacentStates.size() <<  " transitions: " << endl;
	for( AdjacentStates::iterator it = m_AdjacentStates.begin();
		it != m_AdjacentStates.end(); ++it )
	{
		cout << "-->" << "s" << (*it)._pAdjacentState->m_nId << " [";
		if( (*it)._pTransition->IsTerminal() )
		{
			cout << TokenTypeStr[static_cast<Terminal*>((*it)._pTransition)->GetTerminalKind()];
		}
		else
		{
			cout << static_cast<NonTerminal*>((*it)._pTransition)->GetDesp();
		}
		cout << "];  ";
	}
}


/******************************************************\
   class LALR1_DFA
\******************************************************/
void LALR1_DFA::Construct_LR1_DFA()
{
	// !!! must need
	State::SetOwner(this);

	// !!! must need
	LR0Item::Initialize();
	Construct_LR0_DFA();
	Construct_LookAheads();

	// give a number to each of its state
	NumberOnStates();

	// !!! must need
	LR0Item::Finalize();
}

void LALR1_DFA::CheckConflict(void)
{
	list< pair<LR1Item*, LR1Item*> > reduce_reduce_items;
	list< pair<LR1Item*, LR1Item*> > shf_reduce_items;
	list< vector<int>*> reduce_reduce_conflicts; 
	list<int> shf_reduce_conflicts;
	list< vector<int>*>::iterator it;

	int nConflictType = State::NO_CONFLICT;

	Graphy::size_type i = 0;
	for( ; i < _fsa.size(); ++i )
	{
		nConflictType = 
		_fsa[i]->CheckConflict( reduce_reduce_items, shf_reduce_items, reduce_reduce_conflicts, shf_reduce_conflicts );
		if( nConflictType & (State::SHIFT_REDUCE_CONFLICT | State::REDUCE_REDUCE_CONLICT) )
		{
			cout << "state" << _fsa[i]->GetId() << ": has "; 
			if( nConflictType & State::SHIFT_REDUCE_CONFLICT )
			{
				cout << shf_reduce_items.size() << " shift-reduce conflicts ";
				shf_reduce_items.clear();
				shf_reduce_conflicts.clear();
			}

			if( nConflictType & State::REDUCE_REDUCE_CONLICT )
			{
				cout << reduce_reduce_items.size() << " reduce-reduce conflicts";
				reduce_reduce_items.clear();
				for( it = reduce_reduce_conflicts.begin(); it != reduce_reduce_conflicts.end(); ++it )
				{
					delete *it;
				}
				reduce_reduce_conflicts.clear();
			}
		}
	}
}

void LALR1_DFA::out_put()
{
	FILE* f = fopen( "output.txt", "wb" );
	if( !f )
		return;
	fseek( f, 0, SEEK_SET );
	
	typedef Graphy::iterator Iter;
	string sContent;
	sContent = "there are ";
	sContent += Int2Str( _fsa.size() );
	sContent += " states in DFA:";
	sContent += GetLineEnd();
	fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );

	Iter it = _fsa.begin();
	for( ; it != _fsa.end(); ++it )
	{
		sContent = GetLineEnd();
		(*it)->out_put(sContent);

		fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
	}

	sContent = GetLineEnd();
	sContent += GetLineEnd();
	sContent += "output transitions :";
	sContent += GetLineEnd();
	for( it = _fsa.begin(); it != _fsa.end(); ++it )
	{
		(*it)->out_put_transitions( sContent );
		fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
		sContent = "";
	}

	fclose(f);
	}

// construct lookaheads for each item of each state by applying 'propagating lookaheads' algorithm
void LALR1_DFA::Construct_LookAheads()
{
	Graphy::size_type i = 0;
	State* s = NULL;

	// build up association between a kernel item to its closures in a state

	for( i = 0; i < _fsa.size(); ++i )
	{
		_fsa[i]->BuildAssociationsOnKernel2Closures();
#ifdef TESTCASE    
        _fsa[i]->PrintAssociationOnKernel2Closures(); // used for test
#endif
	}

	// build up association between an item to its transitional item
	for( i = 0; i < _fsa.size(); ++i )
	{
		_fsa[i]->BuildAssociationOnItem2TransItem();
#ifdef TESTCASE        
		_fsa[i]->PrintAssociationOnItem2TransItem();
#endif
	}

	// add '$' to the START state
	assert( _fsa.size() > 0 );
	s = _fsa[0];
	// find the item which is started by symbol S'
	LR1Item::GetStartItem()->AddLookahead( DOLLAR );
	s->SetChanged(true);

	// propagating lookaheads
	while(1)
	{
		for( i = 0; i < _fsa.size(); ++i )
		{
			if( _fsa[i]->GetChanged() )
				break;
		}

		if( i == _fsa.size() )
			break;  // no state has changed on lookaheads

		// reset for no change
		for( i = 0; i < _fsa.size(); ++i )
		{
			_fsa[i]->SetChanged(false);
		}

		for( i = 0; i < _fsa.size(); ++i )
		{
			_fsa[i]->UpdateLookaheadsOnClosures();
			_fsa[i]->UpdateLookaheadsOnTransitions();
		}
	}

	// clear allocated memory
	for( i = 0; i < _fsa.size(); ++i )
	{
		// modified by liujian, the relation 'item to its transitional item' will be used in report error
//      _fsa[i]->Clear();
		_fsa[i]->ClearKernel2Closures(); 
	}
}

void LALR1_DFA::NumberOnStates()
{
	for( Graphy::size_type i = 0; i < _fsa.size(); ++i )
	{
		_fsa[i]->SetId(i);
	}
}


int LALR1_DFA::CheckState( const State* other, const vector<State*>& newStates, State*& duplicate, bool bInList )
{
	int nRet = -1;
	typedef vector<State*>::const_iterator Iter;

	// check whether stat is in newStates?
	Iter it = newStates.begin();
	for( ; it != newStates.end(); ++it )
	{
		// added by liujian, 2007.9.22
		if( bInList && ( *it == other ) )
			continue;

		if( *(*it) == *other )
			break;
	}

	if ( it != newStates.end() )
	{
		duplicate = *it;
		nRet = 0;
		goto Exit_Func;
	}

	// check whether stat is in DFA
	for( it = _fsa.begin(); it != _fsa.end(); ++it )
	{
		if( *(*it) == *other )
			break;
	}

	if( it != _fsa.end() )
	{
		duplicate = *it;
		// nRet = -1;
		goto Exit_Func;
	}

	nRet = 1;

Exit_Func:
	return nRet;
}

void LALR1_DFA::Construct_LR0_DFA()
{
	State* _state = NULL;

	// construct an initial state
	// !!! must add an augumental rule S'->S to the grammar for the first
	_state = new State();
	GrammarRepresentation& gramr = Grammar::Instance()->GetGrammar();
	typedef GrammarRepresentation::iterator Iter;
	Iter it = gramr.find( Grammar::Instance()->GetStartSymbol() );
	assert( (*it).second.size() == 1 );

	Rule* r = it->second[0]->GetRule();
	LR1Item* item = new LR1Item( 
		make_pair( it->first, r ) 
		);

	LR1Item::SetStartItem(item); // added by liujian 2007.10.4

	LR1Items _closure, _duplicates;
	item->Closure( _closure ); // closure of an initial item

	_state->AddItem( item );  // add initial item
	_state->AddItem( _closure, _duplicates ); 
	assert( _duplicates.size() == 0 );
	FREE_ITEMS(_duplicates);

#ifdef TESTCASE
	// used for test:
	cout << endl;
	_state->print();
#endif


	// add initial state to a queue
	typedef deque<State*> StateQueue;
	StateQueue  stat_que;
	stat_que.push_back( _state );
	_fsa.push_back( _state );

	vector<State*> newStates;

	// bfs agorithm for constructing LR1_DFA
	while( !stat_que.empty() )
	{
		_state = stat_que.front();
		stat_que.pop_front();

		// push all new states into queue
		newStates.clear();
		_state->Transit( newStates );
		copy( newStates.begin(), newStates.end(), back_inserter(stat_que) );
		copy( newStates.begin(), newStates.end(), back_inserter(_fsa) );
	}
}


/*******************************************************\
 * implementation of class LALR1_ParserGenerator
\*******************************************************/
bool LALR1_ParserGenerator::Initialize()
{
	bool bRet = false;

	m_nStateCount = LALR1_DFA::Instance()->_fsa.size();
	m_nInputSymCount = Grammar::Instance()->GetTerminals()->size() + 1;  // !!!  '$'should be counted
	m_nNontermCount  = Grammar::Instance()->GetNonTerminals()->size();

	const list<Terminal*>* terminals = Grammar::Instance()->GetTerminals();
	if( find_if(terminals->begin(), terminals->end(), TerminalFinder(EMPTY)) != terminals->end() ) // has 'ε'
	{
		m_nInputSymCount = m_nInputSymCount - 1;  // exclude 'ε'
	}
  
	if( !CreateActionTab() )
	{
		goto Exit_Func;
	}
	
	if( !CreateGotoTab() )
	{
		goto Exit_Func;
	}

	ConstructDrivenTab();
	bRet = true;
Exit_Func:
	return bRet;
}


void LALR1_ParserGenerator::out_put_tables()
{
    FILE* f = fopen( "tables.txt", "wb" );
	if( !f )
	{
		return;
	}
	fseek( f, 0, SEEK_SET );
	
	string sContent;

	// productions:
	for( vector<GrammarRule>::size_type k = 0; k != m_vAllGrammarRules.size(); ++k )
	{
		sContent = "rule";
		sContent += Int2Str(k);
		sContent += ": ";
		out_put_production( m_vAllGrammarRules[k], sContent ); 
		sContent += GetLineEnd();
		fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
		sContent = "";
	}

	// table action:
	sContent = "action table: ";
	sContent += GetLineEnd();
	sContent += "==================";
	sContent += GetLineEnd();
	fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
	
	sContent = "    ";
	vector<Terminal>::iterator it_term = m_vAllTerminals.begin();
	for( it_term = m_vAllTerminals.begin(); it_term != m_vAllTerminals.end(); ++it_term )
	{
		if( (*it_term).GetTerminalKind() == EMPTY )
			continue;
		sContent += TokenTypeStr[(*it_term).GetTerminalKind()];
		sContent += " ";
	}
	fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );

	int i = 0, j = 0;
	TableEntry entry;
	for( i = 0; i < m_nStateCount; ++i )
	{
		sContent = GetLineEnd();
		sContent += Int2Str(i);
		sContent += "	";
		for( j = 0; j < m_nInputSymCount; ++j )
		{
			entry = m_ppAction[i][j];
			if( ((entry & 0xff00) >> 8) == ACT_UNDEF )
			{
				sContent += "_";
			}
			else if ( ((entry & 0xff00) >> 8) == ACT_SHIFT )
			{
				sContent += "s";
				sContent += Int2Str(entry & 0x00ff);
				sContent += "[";
				sContent += TokenTypeStr[m_vAllTerminals[j].GetTerminalKind()];
				sContent += "]";
			}
			else if( ((entry & 0xff00) >> 8) == ACT_REDUCE )
			{
				sContent += "r";
				sContent += Int2Str(entry & 0x00ff);
				sContent += "[";
				sContent += TokenTypeStr[m_vAllTerminals[j].GetTerminalKind()];
				sContent += "]";
			}
			else
				sContent += "accept";
			sContent += " ";
		}
		fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
	}

	// table GOTO
    sContent = GetLineEnd();
	sContent += GetLineEnd();
	sContent += "GOTO table: ";
	sContent += GetLineEnd();
	sContent += "==================";
	sContent += GetLineEnd();
	fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
	
	sContent = "	";
	const vector<NonTerminal*>* nonterminals = Grammar::Instance()->GetNonTerminals();
	vector<NonTerminal*>::const_iterator it = nonterminals->begin();
	for( ; it != nonterminals->end(); ++it )
	{
		sContent += (*it)->GetDesp();
		sContent += " ";
	}	
	fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );

	for( i = 0; i < m_nStateCount; ++i )
	{
		sContent = GetLineEnd();
		sContent += Int2Str(i);
		sContent += " ";
		for( j = 0; j < m_nNontermCount; ++j )
		{
			entry = m_ppGOTO[i][j]; 
			if( (entry & 0xc000) == 0xc000 )
				sContent += "_";
			else
			{
				sContent += Int2Str( entry );
				sContent += "[";
				sContent += GetNonTerminal(j)->GetDesp();
				sContent += "]";
			}
			sContent += " ";
		}
		fwrite( sContent.c_str(), sizeof(char), sContent.length(), f );
	}

	fclose(f);

}

bool LALR1_ParserGenerator::CreateActionTab()
{
	TableEntry* p = NULL;
	bool bRet = false;
	int i = 0, j = 0;

	try
	{
		p = new TableEntry[m_nStateCount * m_nInputSymCount];
	}
	catch( bad_alloc& )
	{
		goto Exit_Func;
	}

	try
	{
		m_ppAction = new TableEntry*[m_nStateCount];
	}
	catch( bad_alloc& )
	{
		delete []p;
		goto Exit_Func;
	}

	for( i = 0; i < m_nStateCount; ++i )
	{
		m_ppAction[i] = p + m_nInputSymCount * i;
	}

	for( i = 0; i < m_nStateCount; ++i )
		for( j = 0; j < m_nInputSymCount; ++j )
		{
			m_ppAction[i][j] = (ACT_UNDEF >> 8);
		}

    bRet = true;

Exit_Func:
	return bRet;
}

bool LALR1_ParserGenerator::CreateGotoTab()
{
	TableEntry* p = NULL;
	bool bRet = false;	
	int i = 0, j = 0;

	try
	{
		p = new TableEntry[m_nStateCount * m_nNontermCount];
	}
	catch( bad_alloc& )
	{
		goto Exit_Func;
	}

	try
	{
		m_ppGOTO = new TableEntry*[m_nStateCount];
	}
	catch( bad_alloc& )
	{
		delete []p;
		goto Exit_Func;
	}
		
	for( i = 0; i < m_nStateCount; ++i )
	{
		m_ppGOTO[i] = p + m_nNontermCount * i;
	}

	for( i = 0; i < m_nStateCount; ++i )
		for( j = 0; j < m_nNontermCount; ++j )
		{
			m_ppGOTO[i][j] = 0xc000; // 11xxxxxx xxxxxxxx
		}

    bRet = true;

Exit_Func:
	return bRet;
}

int LALR1_ParserGenerator::Terminal2Ord(int tok) const
{
	int order = -1;
	assert( tok <= DOLLAR && tok >= K_INT );

	if( tok == INT_NUM || tok == FLOAT_NUM ) // INT_NUM->NUM, FLOAT_NUM->NUM
	{
		tok = NUM;
	}
 
	// binary search
	int low = 0;
	int high = m_vAllTerminals.size()-1;
    int pivot = 0;
	while(1)
	{
		pivot = ((high+low) >> 1);
		if( 0 == m_vAllTerminals[pivot].GetTerminalKind() - tok )
		{
			order = pivot;
			break;
		}
		else if( 0 < m_vAllTerminals[pivot].GetTerminalKind() - tok )
		{
			high = pivot -1;
		}
		else
		{
			low = pivot + 1;
		}

		if( high-low <= 1 )
		{
			if( 0 == m_vAllTerminals[high].GetTerminalKind() - tok )
			{
				order = high;
			}
			else if( 0 == m_vAllTerminals[low].GetTerminalKind() - tok )
			{
				order = low;
			}
			break;
		}
	}

	assert( order >= 0 && order < m_vAllTerminals.size() );

	return order;

}

bool TerminalComper( const Terminal& one, const Terminal& other )
{
	return other.GetTerminalKind() > one.GetTerminalKind();
}

void LALR1_ParserGenerator::SetupMappingRelation()
{
	GrammarRepresentation& grammar = Grammar::Instance()->GetGrammar();
	typedef GrammarRepresentation::iterator Iter;
	typedef Grammar::Productions Productions;
	typedef Productions::iterator ProdIter;
	NonTerminal *prod_head = NULL;
	ProdIter it_prod;

	for( Iter it = grammar.begin(); it != grammar.end(); ++it )
	{
		prod_head = (*it).first;
		for( it_prod = (*it).second.begin(); it_prod != (*it).second.end(); ++it_prod )
		{
			m_vAllGrammarRules.push_back( make_pair(prod_head, (*it_prod)->GetRule()) );
		}
	}

	// map a terminal to table Action's subscript
	const list<Terminal*>* terminals = Grammar::Instance()->GetTerminals();
	list<Terminal*>::const_iterator it_term = terminals->begin();
	for( ; it_term != terminals->end(); ++it_term )
	{
		m_vAllTerminals.push_back( *(*it_term) );
	}

	sort( m_vAllTerminals.begin(), m_vAllTerminals.end(), TerminalComper );
	vector<Terminal>::iterator it_empty = 
		find_if(m_vAllTerminals.begin(), m_vAllTerminals.end(), TerminalFinder(EMPTY));
	if( it_empty !=	m_vAllTerminals.end() ) 
	{
	    m_vAllTerminals.erase( it_empty ); // erase 'ε'
	}
	m_vAllTerminals.push_back( Terminal(DOLLAR) ); // count into '$'
}



int LALR1_ParserGenerator::Rule2Ord( const Rule* r )
{
	vector<GrammarRule>::iterator it = m_vAllGrammarRules.begin();
	int i = 0;
	for( ; it != m_vAllGrammarRules.end(); ++it, ++i )
	{
		if( (*it).second == r )
		{
			return i;
		}
	}

	assert( it != m_vAllGrammarRules.end() );
	return -1;
}

// !!!note: when shift-reduce conflict occurs, shift action first;
//          so, reduce action will be overlapped when there is also exsit a shift action
void LALR1_ParserGenerator::ConstructDrivenTab()
{
	typedef LALR1_DFA::Graphy::iterator Iter;
	typedef State::AdjacentStates AdjacentStates;

	LALR1_DFA* dfa = LALR1_DFA::Instance();
	vector<const LR1Item*> complete_items;
	vector<const LR1Item*>::size_type i = 0;
	LR1Items::const_iterator it_item;

	AdjacentStates::const_iterator it_adj_stat;

	int nRuleNo = 0;

	Grammar::Instance()->NumberOnNonterminals(); // !!!!must need
	SetupMappingRelation(); //!!!! must need
	
	for( Iter it = dfa->_fsa.begin(); it != dfa->_fsa.end(); ++it )
	{
		complete_items.clear();
		for( it_item = (*it)->GetItems()->begin(); it_item != (*it)->GetItems()->end(); ++it_item )
		{
			if( (*it_item)->IsCompleteItem() )
			{
				complete_items.push_back( *it_item );
			}
		}

		// set reduce actions
		for( i = 0; i < complete_items.size(); ++i )
		{
			const TerminalSet& lookaheads = complete_items[i]->GetLookahead();
			nRuleNo = Rule2Ord(complete_items[i]->GetRule().second); // map rule into a subscript
			for( TerminalSetIter it_term = lookaheads.begin(); it_term != lookaheads.end(); ++it_term )
			{
				m_ppAction[(*it)->GetId()][Terminal2Ord(*it_term)] = 
					((ACT_REDUCE << 8) | nRuleNo);
			}
		}

		// set goto actions
		const AdjacentStates* adj_stats = (*it)->GetAdjacentStates();
		for( it_adj_stat = adj_stats->begin(); it_adj_stat != adj_stats->end(); ++it_adj_stat )
		{
			if( (*it_adj_stat)._pTransition->IsTerminal() )
			{
				m_ppAction[(*it)->GetId()]
				[Terminal2Ord( static_cast<Terminal*>((*it_adj_stat)._pTransition)->GetTerminalKind() )] = 
					((ACT_SHIFT << 8) | ((*it_adj_stat)._pAdjacentState->GetId()));
			}
			else
			{
				m_ppGOTO[(*it)->GetId()][static_cast<NonTerminal*>((*it_adj_stat)._pTransition)->GetId()] =
					(*it_adj_stat)._pAdjacentState->GetId();
			}
		}

		// set table entry of 'accept'
		SetAcceptAction();
	} // iterate state
}

void LALR1_ParserGenerator::SetAcceptAction(void)
{
	LALR1_DFA::State* stat = LALR1_DFA::Instance()->GetStartState();
	LR1Item* item = LR1Item::GetStartItem(); // start item in the initial state

	Rule* rule = item->GetRule().second;
	RuleIter it = rule->begin();

	// find the state which result in reduction
	// 根据文法串序列，找出start item->...->complete item(from start item)
	for( ; it != rule->end(); ++it )
	{
		bool bFound = LALR1_DFA::StateUtils::Instance()->FindTransition( *stat->GetAdjacentStates(), *it, stat );
		assert( bFound );
	}

	// find the item which will cause reduction
	LR1Items::size_type i = 0;
	for( ; i < stat->GetItems()->size(); ++i )
	{
		if( (*stat->GetItems())[i]->GetRule().second == rule )
		{
			item = (*stat->GetItems())[i];
			break;
		}
	}
	assert( i != stat->GetItems()->size() );
	const TerminalSet& terminals = item->GetLookahead();
	TerminalSetIter it_term =
		terminals.find( DOLLAR );
	assert( it_term != terminals.end() );

	m_ppAction[stat->GetId()][Terminal2Ord(*it_term)] = ((ACT_ACCEPT << 8) | Rule2Ord(rule));

	// !!!set terminal state
	m_nTerminalState = stat->GetId();
}

/*******************************************************\
 * implementation of class lalr parser
\*******************************************************/

void LALR1_Parser::Parse(void)
{
	int nErrKind = -1; // 0: no error, >0: parsing error <0: lexical error  
	State* cur_stat = LALR1_DFA::Instance()->GetStartState();
	m_stkParser.push_back( new StackItem(STATE, cur_stat) );

	TableEntry entry;
	int act_type = -1;

	char szPrompt[100];

	Token* tok = NULL;

	while(1)
	{
		tok = m_Scanner.GetToken();

        if( g_env_var_enable[LR_DEBUG_ON_TOKEN] ) {
            cout << "read in token: " << TokenTypeStr[tok->_tok] << endl;
        }

		if( tok->_tok == ERR )
		{
			nErrKind = -1;
			delete tok;
			break;
		}

		if( tok->_tok == PSEUDO ) // read in comment
		{
			delete tok;
			continue;
		}

		m_lstTokens.push_back(tok); // '$' is also in the token list

RE_READ:
		// get current state from the top of stack
		assert( STATE == m_stkParser.back()->_type  );
		cur_stat = static_cast<State*>(m_stkParser.back()->_pItem);

		// look up in action table
		entry = m_generator.LookUpActionTab( cur_stat->GetId(), m_generator.Terminal2Ord(tok->_tok) );
		act_type = ((entry >> 8) & 0xf);

		if( tok->_tok == E_O_F ) // end of file
		{
			if( m_generator.GetTerminalState() == cur_stat->GetId() )
			{
				nErrKind = 0;
				break;
			}
		}

		switch( act_type )
		{
		case ACT_SHIFT:
            if( g_env_var_enable[LR_DEBUG_ON_PARSE] ) {
                print_lr_parse_step(std::cout, tok, m_stkParser, ACT_SHIFT, entry&0xff);
            }
			m_stkParser.push_back( new StackItem(TERMINAL, tok) );
			m_stkParser.push_back( new StackItem(STATE, m_generator.GetState(entry&0xff)) );
			break;
		case ACT_REDUCE: 
            if( g_env_var_enable[LR_DEBUG_ON_PARSE] ) {
                print_lr_parse_step(std::cout, tok, m_stkParser, ACT_REDUCE, 0, 
                                    m_generator.GetRule(entry&0xff));
            }
			if ( !Reduce( m_generator.GetRule(entry&0xff), entry&0xff, szPrompt ) )
			{
				nErrKind = 1;
				goto Exit_Func;
			}
			break;
		default:
			{
                if( g_env_var_enable[LR_DEBUG_ON_PARSE] ) {
                    print_lr_parse_step(std::cout, tok, m_stkParser, act_type);
                }
				nErrKind = 1;
//				strcpy( szPrompt, "unknow action!!" );
				ReportErrorOnShift( szPrompt, tok, m_Scanner.GetLineNo() );
				goto Exit_Func;
			}

		}

		// when applying reduce operation, stop reading in a token!!!
		if( act_type == ACT_REDUCE )
		{
			goto RE_READ;
		}
	}
Exit_Func:
	if( nErrKind == 0 )
		cout << "parsing succeed!!!" << endl;
	else
	{
		if(nErrKind < 0) // lexical error
		{
			ErrorReporter::Instance()->PrintErrInfo();
			cout << endl << "parsing failed!!!" << endl;
		}
		else
		{

			cout << szPrompt;
//			cout << endl << "error(line" << m_Scanner.GetLineNo() << "):" << szPrompt; 
			cout << endl << "parsing failed!!!" << endl;
		}
		
	}
}

// refer to: LR_parser.vsd /reduce/reduce_simplification
bool LALR1_Parser::Reduce( const GrammarRule* prod, 
                           int rule_no, 
                           char (&szPrompt)[100] )
{
	typedef Rule::reverse_iterator RIter;

	Rule* prod_body = prod->second;
	NonTerminal* prod_head = prod->first;
	RIter rit_sym;

	StackItem* item = NULL;
	TableEntry entry = 0;

	bool bRet = false;

	if( 1 == prod_body->size() && (*prod_body)[0]->IsTerminal() ) // A->ε
	{
		if( EMPTY == static_cast<Terminal*>((*prod_body)[0])->GetTerminalKind() )
		{
			goto Look_Up_GOTO;
		}
	}

	for( rit_sym = prod_body->rbegin(); rit_sym != prod_body->rend(); ++rit_sym )
	{
		assert( !m_stkParser.empty() );

		// pop off the state from the top of stack
		item = m_stkParser.back();
		m_stkParser.pop_back(); 
		delete item;

		assert( !m_stkParser.empty() );

		// get a symbol from top of the stack
		item = m_stkParser.back();

		assert( STATE != item->_type );
		if( TERMINAL == item->_type )  // terminal
		{
			if( (*rit_sym)->IsTerminal() ) // two terminals must be the same
			{
				if(	m_generator.GetTokenType( static_cast<Token*>(item->_pItem)->_tok ) !=
					static_cast<Terminal*>( *rit_sym )->GetTerminalKind()
					) 
				{
					sprintf( szPrompt, "reduce error(r%d): nonterminal unmatched!", rule_no );
					assert(0); // error will never occur
					goto Exit_Func;  // reduce error2
				}
			}
			else
			{
				if( (*rit_sym) != static_cast<NonTerminal*>(item->_pItem) )
				{
					sprintf( szPrompt, "reduce error(r%d): terminal unmatche!", rule_no );
					assert(0); // error will never occur
					goto Exit_Func; // reduce error3
				}
			}
		}

		// pop off the symbol from the top of stack
		m_stkParser.pop_back();
		delete item;
	}

Look_Up_GOTO:
	// get the current state
	item = m_stkParser.back(); 
	assert( STATE == item->_type );

	entry = m_generator.LookUpGotoTab( static_cast<State*>(item->_pItem)->GetId(), prod_head->GetId() );
	if( (entry & 0xc000) == 0xc000 )  // error5
	{
		sprintf( szPrompt, "reduce error(r%d): unexpected nonterminal!", rule_no );
		assert( 0 ); // 不会发生不正确的规约，陈火旺<程序设计语言编译原理> p127
	}
	else
	{
		m_stkParser.push_back( new StackItem(NONTERMINAL, prod_head) );
		m_stkParser.push_back( new StackItem(STATE, m_generator.GetState(entry&0xfff)) );
		bRet = true;
	}

Exit_Func:
	return bRet;
}


void LALR1_Parser::ReportErrorOnShift( char (&szPrompt)[100], const Token* tok, int nLineNo)
{
	typedef ParserStack::reverse_iterator ReverIter;

	bool bTreated = false;

	ReverIter it = m_stkParser.rbegin();
	State* curState = NULL;
	State* prevState = NULL;
	Token* trans_t = NULL;
	vector< pair<LR1Item*, LR1Item*> > from_to;
	Terminal* t = NULL;

	if( E_O_F == tok->_tok )
	{
		bTreated = true;
		sprintf( szPrompt, "fatal error(line%d): unexpected end of file", nLineNo );
		goto Exit_Func;
	}
	
	assert( STATE == (*it)->_type );
	curState = static_cast<State*>(	(*it)->_pItem	);
	
	++it;
	if( it == m_stkParser.rend() )
		goto Exit_Func;
	
	if( (*it)->_type == TERMINAL )
	{
		trans_t = static_cast<Token*>( (*it)->_pItem );
	}
	else // there exsits a reduction after the previous state
	{
		goto Exit_Func;
	}

	++it;
	assert( STATE == (*it)->_type );
	prevState = static_cast<State*>( (*it)->_pItem );
	prevState->GetDeterministicTransition( m_generator.GetTerminal(m_generator.Terminal2Ord(trans_t->_tok)), from_to );
	
	if( from_to.size() == 1 )
	{
		if( !from_to[0].second->IsPoint2Nonterm() ) // current item points to a terminal, so expect input to be the TERMINAL
		{
			bTreated = true;
			t = static_cast<Terminal*>( *from_to[0].second->GetPeriod() );
			sprintf( szPrompt, "line %d:expect '%s', but '%s' appeared", nLineNo,
				TokenTypeStr[t->GetTerminalKind()], TokenTypeStr[tok->_tok] );
		}
	}

Exit_Func:
	if( !bTreated )
	{
		sprintf( szPrompt, "line %d: syntax error, '%s'", nLineNo, TokenTypeStr[tok->_tok] );
	}
}


//////////////////////////////////////////////////////////////////

void LALR1_Parser::print_lr_parse_step( ostream& os,
                          const Token* tok, 
                          const ParserStack& list,
                          int act_type,
                          int transit_state,
                          const GrammarRule* rule )
{
   ParserStack::const_iterator it = list.begin();
   std::string prod;
   for( ; it != list.end(); ++it ) {
      if( STATE == (*it)->_type ) {
         os << static_cast<State*>((*it)->_pItem)->GetId();
      }
      else if( TERMINAL == (*it)->_type ){
         os << TokenTypeStr[static_cast<Token*>((*it)->_pItem)->_tok];
      }
      else {
         assert( NONTERMINAL == (*it)->_type );
         os << static_cast<NonTerminal*>((*it)->_pItem)->GetDesp();
      }
      os << " ";
   }

   os << " |" << TokenTypeStr[tok->_tok] << " |";

   switch( act_type ) {
   case ACT_SHIFT:
      os << "shift " << transit_state;
      break;
   case ACT_REDUCE:
      prod.clear();
      out_put_production( *rule, prod );
      os << "reduce " << prod;
      break;
   case ACT_ACCEPT:
      os << "accept"; 
      break; 
   default:
      os << "unkn action" << endl;
      return;
   }

   os << endl;

}                          

void out_put_production( const GrammarRule& prod, string& s )
{
	s += prod.first->GetDesp();
	s += "->";

	typedef Rule::iterator Iter;
	for( Iter it = prod.second->begin(); it != prod.second->end(); ++it )
	{
		if( (*it)->IsTerminal() )
		{
			s += TokenTypeStr[static_cast<Terminal*>(*it)->GetTerminalKind()];
		}
		else
			s += static_cast<NonTerminal*>(*it)->GetDesp();

		s += " ";
	}
}
