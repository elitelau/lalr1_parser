#include "grammar.h"
#include <assert.h>

#include <iostream>
using namespace std;

#include "err_report.h"
#include "scanner.h"

void Grammar::Production::PrintProduction()
{
	for( RuleIter itRule = m_pRule->begin(); itRule != m_pRule->end(); ++itRule ) 
	{
		if(	(*itRule)->IsTerminal() )
		{
			cout << TokenTypeStr[static_cast<Terminal*>(*itRule)->GetTerminalKind()];
		}
		else
		{
			cout << static_cast<NonTerminal*>(*itRule)->GetDesp();
		}
		cout << " ";
	}
}



void Grammar::NonTerminal::PrintFirstSet(void)
{
	for( TerminalSetIter it = m_setFirst.begin(); it != m_setFirst.end(); ++it )
	{
		cout << TokenTypeStr[*it] << "	";
	}
}

void Grammar::NonTerminal::PrintFollowSet(void)
{
	for( TerminalSetIter it = m_setFollow.begin(); it != m_setFollow.end(); ++it )
	{
		cout << TokenTypeStr[*it] << "	";
	}
}


void Grammar::Clear()
{
	// release all productions
	for( Iter it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		for( Productions::size_type i = 0; i < (*it).second.size(); ++i ) 
		{
			delete (*it).second[i]; // delete a producion
		}
	}

	// release all the terminal objects
	for( TerminalsIter it = m_Terminals.begin(); it != m_Terminals.end(); ++it )
	{
		delete *it;
	}

	// release all the nonterminals
	for( NonTermIter it = m_NonTerminals.begin(); it != m_NonTerminals.end(); ++it )
	{
		delete *it;
	}
}

void Grammar::CalculateFirstSets(void)
{
	bool bChanged = true;
	Production *p = NULL;

	NonTerminal *prod_head = NULL;
	NonTerminal *n = NULL;
	Terminal* t = NULL;

	Iter it = m_Productions.begin();
	Productions::size_type i = 0;

	list< pair<NonTerminal*, Production*> > prods_begin_with_nonterm;
	list< pair<NonTerminal*, Production*> >::iterator it_prod_begin_with_nonterm;

	RuleIter itRule;

	// check the grammar rules which will not be examined in the second pass when calculating first sets
	for( ; it != m_Productions.end(); ++it )
	{
		prod_head = (*it).first; // !!! a nonterminal corresponds with one or more productions

		for( i = 0; i < (*it).second.size(); ++i )
		{
			p = (*it).second[i];
			if( p->IsStartWithTerminal() ) // when a production begin with a terminal, then the first terminal occured is the first set element of the production
			{
				t = static_cast<Terminal*>( *p->GetRule()->begin() );
				prod_head->InsertInto1stSet( t->GetTerminalKind() );
			}
			else
			{
				prods_begin_with_nonterm.push_back( make_pair(prod_head, p) ); 
			}
		}
	}

	while(1)
	{
		bChanged = false;

		for( it_prod_begin_with_nonterm = prods_begin_with_nonterm.begin(); 
			 it_prod_begin_with_nonterm != prods_begin_with_nonterm.end(); ++it_prod_begin_with_nonterm )
		{
			prod_head = (*it_prod_begin_with_nonterm).first;
			p = (*it_prod_begin_with_nonterm).second;

			for( itRule = p->GetRule()->begin(); itRule != p->GetRule()->end(); ++itRule )
			{
				if( (*itRule)->IsTerminal() )
					break;

				n = static_cast<NonTerminal*>( *itRule );
				prod_head->AddFirstElems( n );

				if( !n->HasEmpty() )
					break;
			}

			// X->x1x2...xn,  ¦Å¡Êxi( 1¡Üi¡Ün )
			if( p->GetRule()->end() == itRule )
				prod_head->InsertInto1stSet( EMPTY );
		}

		// check changed
		for( it_prod_begin_with_nonterm = prods_begin_with_nonterm.begin(); 
			it_prod_begin_with_nonterm != prods_begin_with_nonterm.end(); ++it_prod_begin_with_nonterm )
		{
			prod_head = (*it_prod_begin_with_nonterm).first;
			if( prod_head->IsChanged() )
			{
				bChanged = true;
				break;
			}
		}

		// reset for no change
		for( it_prod_begin_with_nonterm = prods_begin_with_nonterm.begin(); 
			it_prod_begin_with_nonterm != prods_begin_with_nonterm.end(); ++it_prod_begin_with_nonterm )
		{
			prod_head = (*it_prod_begin_with_nonterm).first;
			prod_head->ResetForChange();
		}

		if( !bChanged )
			break;
		
	}

	ResetForNoChange();
}

void Grammar::PrintFirstSets(void) {
	NonTerminal *n = NULL;
	Iter it = m_Productions.begin();

	for( it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		n = (*it).first;
		cout << n->GetDesp() << ": ";
		n->PrintFirstSet();
        cout << endl;
	}
}

void Grammar::CalculateFollowSets()
{
	typedef  Rule::reverse_iterator RuleRIter; 

	bool bChanged = false;

	Iter it = m_Productions.begin();
	NonTerminal* prod_head = NULL;
	Production* p = NULL;

	NonTerminal* n = NULL;
	Terminal* t = NULL;

	bool bContinuous = true;

	// add '$' as a follow set element of the start symbol
	m_pStartSym->InsertIntoFollowSet( DOLLAR );

	while(1)
	{
		bChanged = false;

		for( ; it != m_Productions.end(); ++it )
		{
			prod_head = (*it).first;

			for( Productions::size_type i = 0; i < (*it).second.size(); ++i )
			{
				p = (*it).second[i];
				bContinuous = true;

				RuleRIter itRule = p->GetRule()->rbegin();
				RuleRIter prev = itRule;

				if( !(*itRule)->IsTerminal() ) // the last symbol of a production rule is a nonterminal
				{
					n = static_cast<NonTerminal*>(*itRule);
					n->AddFollowElems( prod_head->GetFollowSet() );
				}
				else
					bContinuous = false;

				++itRule;
				for( ; itRule != p->GetRule()->rend(); ++itRule, ++prev )
				{
					if( bContinuous && !(*itRule)->IsTerminal() )
					{
						n = static_cast<NonTerminal*>(*itRule);

						if( static_cast<NonTerminal*>(*prev)->HasEmpty() ) // modified by liujian 2007.8.10
						{
							n->AddFollowElems( prod_head->GetFollowSet() );
						}
						else
							bContinuous = false;
					}
					else
						bContinuous = false;

					if( !(*itRule)->IsTerminal() )
					{
						n = static_cast<NonTerminal*>(*itRule);
						if( (*prev)->IsTerminal() )
						{
							t = static_cast<Terminal*>(*prev);
							n->InsertIntoFollowSet( t->GetTerminalKind() );
						}
						else
							n->AddFollowElems( static_cast<NonTerminal*>(*prev)->GetFirstSet() );
					}

				} // for( itRule..
				
			} // for( int i = 0
		} // for( it = 

		for( it = m_Productions.begin(); it != m_Productions.end(); ++it )
		{
			prod_head = (*it).first;
			if( prod_head->IsChanged() )
			{
				bChanged = true;
				break;
			}
		}

		ResetForNoChange();

		if( !bChanged )
			break;
	} // while(1)

}

void Grammar::PrintFollowSets(void) {
	NonTerminal *n = NULL;
	Iter it = m_Productions.begin();

	for( it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		n = (*it).first;
		cout << n->GetDesp() << ": ";
		n->PrintFollowSet();
        cout << endl;
	}
}


void Grammar::AddSymbol(Grammar::Symbol *sym)
{
	if( sym->IsTerminal() )
	{
	    m_Terminals.push_back(static_cast<Terminal*>(sym));
	}
	else
	{
		m_NonTerminals.push_back(static_cast<NonTerminal*>(sym));
	}
}

bool NonTerminalComper( const NonTerminal* one, const NonTerminal* other )
{
	return strcasecmp( other->GetDesp(), one->GetDesp() ) > 0;
}

void Grammar::NumberOnNonterminals(void)
{
	// sort according alphabeta order of names
	sort( m_NonTerminals.begin(), m_NonTerminals.end(), NonTerminalComper ); 

	vector<NonTerminal*>::size_type i = 0;
	for( ; i < m_NonTerminals.size(); ++i )
	{
		m_NonTerminals[i]->SetId(i);
	}
}

void Grammar::PrintGrammar()
{
	cout << "there are " << m_Productions.size() << " productions in the grammar rule" << endl;
	for( Iter it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		NonTerminal * n = (*it).first;
		Productions& ps = (*it).second;

		cout << n->GetDesp() << "->";
		for( Productions::size_type i = 0; i < ps.size(); ++i )
		{
			Production* p = ps[i];
			for( RuleIter itRule = p->GetRule()->begin(); itRule != p->GetRule()->end(); ++itRule )
			{
				if( (*itRule)->IsTerminal() )
				{
					Terminal* t = static_cast<Terminal*>(*itRule);
					cout << TokenTypeStr[t->GetTerminalKind()] << " ";
				}
				else
				{
					NonTerminal* nonterm = static_cast<NonTerminal*>(*itRule);
					cout << nonterm->GetDesp() << " ";
				}
			}

			cout << "|";
		}
		cout << endl;
	}
}


bool StrComper( const char* s1, const char* s2 )
{
	if( strcasecmp(s1, s2) <= 0 )
		return true;
	return false;
}


// check whether there are nonterminal spelled with mistakes
void Grammar::PrintNonterminals(int)
{
	vector<const char*> nonterms;
	vector<NonTerminal*> vNonTerms;
	for( Iter it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		NonTerminal* n = (*it).first;
		if( find(vNonTerms.begin(), vNonTerms.end(), n) == vNonTerms.end() ) // !! do not find
		{
			vNonTerms.push_back( n );
		}

		for( Productions::size_type i = 0; i < (*it).second.size(); ++i )
		{
			Production* p = (*it).second[i];

			for( RuleIter itRule = p->GetRule()->begin(); itRule != p->GetRule()->end(); ++itRule )
			{
				if(	!(*itRule)->IsTerminal() )
				{
					NonTerminal* nonterm = static_cast<NonTerminal*>(*itRule);
					if( find(vNonTerms.begin(), vNonTerms.end(), nonterm) == vNonTerms.end() ) // !! do not find
					{
						vNonTerms.push_back( nonterm );
					}
				}
			}
		}

	}

	for( vector<NonTerminal*>::size_type i =0; i < vNonTerms.size(); ++i )
	{
		nonterms.push_back( vNonTerms[i]->GetDesp() );
	}

	sort( nonterms.begin(), nonterms.end(), StrComper );
	cout << "there are " << nonterms.size() << " nonterminals in grammar file!" << endl;

	for( vector<const char*>::iterator it = nonterms.begin(); it != nonterms.end(); ++it ) // !!! can just apply to a random-accessable container
	{
		cout << *it << "	";
	}


}

void Grammar::PrintNonterminals()
{
	vector<const char*> nonterms;
	for( Iter it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		nonterms.push_back( (*it).first->GetDesp() );
	}

	sort( nonterms.begin(), nonterms.end(), StrComper );
	cout << "there are " << nonterms.size() << " nonterminals in grammar file!" << endl;

	for( vector<const char*>::iterator it = nonterms.begin(); it != nonterms.end(); ++it ) // !!! can just apply to a random-accessable container
	{
		cout << *it << "	";
	}
}

void Grammar::PrintTerminals()
{
	vector<const char*> terms;
	cout << "terminal types are :" << endl;
	for( TerminalsIter it = m_Terminals.begin(); it != m_Terminals.end(); ++it )
	{
		terms.push_back( TokenTypeStr[(*it)->GetTerminalKind()] );
		cout << (*it)->GetTerminalKind() << "	";
	}
	cout << endl;

	sort( terms.begin(), terms.end(), StrComper );
	cout << "there are " << terms.size() << " terminals in grammar file!" << endl;

	for( vector<const char*>::iterator it = terms.begin(); it != terms.end(); ++it ) // !!! can just apply to a random-accessable container
	{
		cout << *it << "	";
	}
}

void Grammar::PrintTerminals(int)
{
	vector<int> terms;
	for( Iter it = m_Productions.begin(); it != m_Productions.end(); ++it )
	{
		for( Productions::size_type i = 0; i < (*it).second.size(); ++i )
		{
			Production* p = (*it).second[i];

			for( RuleIter itRule = p->GetRule()->begin(); itRule != p->GetRule()->end(); ++itRule )
			{
				if(	(*itRule)->IsTerminal() )
				{
					Terminal* t = static_cast<Terminal*>(*itRule);
					if( find(terms.begin(), terms.end(), t->GetTerminalKind()) == terms.end() ) // !! do not find
					{
						terms.push_back( t->GetTerminalKind() );
					}
				}
			}
		}

	}

	vector<const char*> vTerms;
	for( vector<int>::size_type i =0; i <terms.size(); ++i )
	{
		vTerms.push_back( TokenTypeStr[terms[i]] );
	}

	sort( vTerms.begin(), vTerms.end(), StrComper );
	cout << "there are " << vTerms.size() << " terminals in grammar file!" << endl;

	for( vector<const char*>::iterator it = vTerms.begin(); it != vTerms.end(); ++it ) // !!! can just apply to a random-accessable container
	{
		cout << *it << "	";
	}
}

void CalculateFirstSet( RuleIter& it_start, const RuleIter& it_end, TerminalSet& first, bool& bContainEmpty )
{
	if( (*it_start)->IsTerminal() )  // in case that X1..Xn is just a ¦Å symbol
	{
		if( static_cast<Terminal*>(*it_start)->GetFirstSet() == EMPTY )
		{
			bContainEmpty = true;
			return;
		}
	}

	while( it_start != it_end )
	{
		if( (*it_start)->IsTerminal() )
		{
			first.insert( static_cast<Terminal*>(*it_start)->GetFirstSet() );
			break;
		}
		else
		{
			const TerminalSet* temp = static_cast<NonTerminal*>(*it_start)->GetFirstSet();
			for( TerminalSet::const_iterator it = temp->begin(); it != temp->end(); ++it )
			{
				if( *it != EMPTY )
				{
					first.insert( *it );
				}
			}

/*
            // used for test only:
            if( 0 == strcmp(static_cast<NonTerminal*>(*it_start)->GetDesp(),
                            "declarator_list") )
            {
               cout << "special watch on declarator_list' first set of" 
                    << temp->size() << " elements." <<  endl;
			   for( TerminalSet::const_iterator it = temp->begin(); it != temp->end(); ++it )
			   {
			      cout << TokenTypeStr[*it] << "   ";
			   }
               cout << endl;
            }
*/
			if( !static_cast<NonTerminal*>(*it_start)->HasEmpty() )
				break;
		}

		++it_start;
	}

	if( it_start == it_end )
		bContainEmpty = true;
	else
		bContainEmpty = false;
}

bool IsContainEmpty( const TerminalSet& s )
{
	typedef TerminalSet::const_iterator Iter;
	Iter it = s.begin();
	for( ; it != s.end(); ++it )
	{
		if( *it == EMPTY )
			break;
	}

	if( it != s.end() )
		return true;
	return false;
}
