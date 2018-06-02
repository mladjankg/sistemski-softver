#ifndef _SS_STRING_TOKENIZER_H_
#define _SS_STRING_TOKENIZER_H_

#include <string>
#include <list>
#include "ss_exceptions.h"
using namespace std;

namespace ss {
    class StringTokenizer {
    public:
        StringTokenizer(const string& delimiter) : delimiter(delimiter), tokens(nullptr) {
            
        } 

        void tokenize(const string& str);

        bool hasNext();

        string& nextToken() throw();

        int tokenNumber();
        ~StringTokenizer();
    private:
        const string delimiter;

        list<string> *tokens;

        list<string>::iterator current;

    };
}

#endif

