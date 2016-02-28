#include "utils.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char* g_env_var_names[ENV_VAR_SIZE] = 
{
   "lr_debug_on_token",
   "lr_debug_on_parse",
   "lr_debug_on_dfa",
   "lr_debug_on_lookahead"
};

bool g_env_var_enable[ENV_VAR_SIZE] = 
{
   false,
   false,
   false,
   false
};

void initialize_on_environment_variables(void) {
    char* env_str_val =NULL;
    /*
    for( int i = 0; i < ENV_VAR_SIZE; ++i ){
       g_env_var_enable[i] = false;
    }*/
    
    for( int i = 0; i < ENV_VAR_SIZE; ++i ){
       env_str_val = getenv(g_env_var_names[i]);
       if( NULL != env_str_val ) { 
          if( strcmp(env_str_val, "true") == 0 ) {
             g_env_var_enable[i] = true;
          }
       }
    }
}

int hash_func_accum( const char* str )
{
	int nAccum = 0;
	int i = 0;

	for( ; str[i] != '\0'; ++i )
	{
		nAccum += (str[i] - 'a');
	}

	nAccum = 3*str[0] + 1*str[i-1] + 2*str[(i-1) >> 2]; // 3,1,2: continue, return; goto, char;
	
	return (	abs(nAccum)	) % HashingTab::s_nLoadFactor;
}

string GetLineEnd(void)
{
/*
	string sLineEnd = "";

	char chCarriage = 0xd;
	char chLineFeed = 0xa;

	sLineEnd += chCarriage;
	sLineEnd += chLineFeed;

	return sLineEnd;
    */
    string sLineEnd;
    sLineEnd += static_cast<char>(0xa);
    return sLineEnd;
}

string Int2Str( int data )
{
	char szInt[20];
	memset( szInt, 0, 20 );
	sprintf(szInt, "%d", data);
	return szInt;
}


/*************************************************************************************/
/* implementation of class HashingTab                                                */
/*************************************************************************************/

void HashingTab::build( const char** str_arr )
{
	int nAccum = 0;
	int nKey = 0;
	int i = 0;
	for( ;NULL != str_arr[i]; ++i )
	{
		nAccum = m_hashFunc( str_arr[i] );
		nKey = nAccum % s_nLoadFactor;

		if( !m_hashTab[nKey] )
		{
			m_hashTab[nKey] = new SeparateChain;
		}

		// insert a key int a separate chaining
		m_hashTab[nKey]->push_back(str_arr[i]);
	}
}

const char* HashingTab::match( const char* src )
{
	int nPattern = hash_func_accum(src);
	SeparateChain* pChain = m_hashTab[nPattern % s_nLoadFactor];
	ChainIter iter;

	if( !pChain )
	{
		return NULL; // not match
	}
	else
	{
		iter = pChain->begin();
		for( ; iter != pChain->end(); ++iter )
		{
			if( 0 == 
				strcasecmp(src, (*iter).c_str()) 
				)
			{
				break;
			}
		}

		if( iter == pChain->end() )
			return NULL;
	}
	
	return (*iter).c_str();
}


		
