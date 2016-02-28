#include <iostream>
#include "grammar.h"
#include "grammar_config.h"
#include "lalr.h"

void LR_Parse( LALR1_ParserGenerator& generator, const char* src_file )
{
	string sFileName;
	sFileName = src_file;
    {
		FILE* f = fopen( sFileName.c_str(), "r+t" );
		if( !f ) return;

		LALR1_Parser parser( f , generator );
		parser.Parse();
		fclose(f);
	}
}
bool calculate_lr1_item_dfa(const char* grammar_file)
{
    char szPrompt[100];
    bool bSuccess = false;

    memset(szPrompt, 0, 100);
	GrammarConfig config( grammar_file );
	bSuccess = config.ReadInProductions( szPrompt );
	if( !bSuccess )
	{
		cout << "read in grammar file failed!" << endl;
		cout << "reason:" << szPrompt << endl;
        return bSuccess;
	}
	Grammar::Instance()->CalculateFirstSets();
	Grammar::Instance()->CalculateFollowSets();
    
	//////////////////////////////////////////////////////////
	// construct LR(1) DFA

	LALR1_DFA*  lalr_parser = LALR1_DFA::Instance();
	lalr_parser->Construct_LR1_DFA();
//    lalr_parser->CheckConflict();

    return true;
}

int main( int argc, char** argv )
{
   if( argc < 2 ) {
      cout << "please specify a grammar file.. " << endl;
      return -1;
   }

   if( argc < 3) {
      cout << "please specify a source file.. " << endl;
      return -1;
   }

   initialize_on_environment_variables();

   bool ret = false;
   ret = calculate_lr1_item_dfa(argv[1]);
   if( ret ) {
      LALR1_ParserGenerator generator;
      if( !generator.Initialize() ) {
         cout << "fail to construct action & goto table .." << endl;
         return -1;
      }
      else {
         generator.out_put_tables();
      }
      LR_Parse(generator, argv[2]);
   }

   return 0;
}
