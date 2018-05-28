#include <string>
#include <list>
using namespace std;

namespace ss {
    class StringTokenizer {
    public:
        StringTokenizer(const string& delimiter) : delimiter(delimiter), tokens(nullptr) {
            
        } 
    
        void tokenize(const string& str);

        bool hasNext();

        string nextToken();

        ~StringTokenizer();
    private:
        const string delimiter;

        list<string> *tokens;

        list<string>::iterator current;

    };



}