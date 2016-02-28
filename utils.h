#ifndef _UTILS_H
#define _UTILS_H
#include <vector>
#include <list>
#include <string>
using namespace std;

enum env_var_constants {
   LR_DEBUG_ON_TOKEN = 0,
   LR_DEBUG_ON_PARSE,
   LR_DEBUG_ON_DFA,
   LR_DEBUG_ON_LOOKAHEAD,
   ENV_VAR_SIZE
};

extern const char* g_env_var_names[ENV_VAR_SIZE];
extern bool g_env_var_enable[ENV_VAR_SIZE];

void initialize_on_environment_variables(void);

// a hashing function making use of accumulation of its charatcters
int hash_func_accum( const char* str );

// if file exsit, truncated file content to empty
// return: hand of file 
int create_file( const char* );

void close_file( int handle );

string GetLineEnd(void);

string Int2Str( int data );

// write into file
int write_2_file( int handle, const char* pContent, int size );

class HashingTab
{
private:
	typedef list<string> SeparateChain;
	typedef SeparateChain::iterator ChainIter;
	typedef vector< SeparateChain* > OpenHashing;
	typedef int (*HashFunc)( const char* str );

	OpenHashing			m_hashTab;
	HashFunc			m_hashFunc;     // a concreate hashing function

public:
	static const int	s_nLoadFactor = 17;  

public:
    HashingTab() : m_hashTab( s_nLoadFactor )
	{
		OpenHashing::size_type i = 0;
		for( i = 0; i < m_hashTab.size(); ++i )
		{
			m_hashTab[i] = NULL;
		}
		m_hashFunc = &hash_func_accum;
	}

	~HashingTab()
	{
		OpenHashing::size_type i = 0;
		for( i = 0; i < m_hashTab.size(); ++i )
		{
			delete m_hashTab[i];
		}
	}

	// build hash table
	void build( const char** str_arr );

	// match for a string in hash table; if failed, return NULL
	const char* match( const char* src );

	const char* match( const char* pStrHead, const char* pStrTail );

};

#endif

