#include <string>
#include "string_tokenizer.h"

using namespace std;
using namespace ss;

void StringTokenizer::tokenize(const string& str) {
    size_t pos;
    size_t lastPos = 0;

    tokens = new list<string>();

    pos = str.find(delimiter, lastPos);

    while (pos != string::npos) {
        string token = str.substr(lastPos + 1, pos - lastPos);
        lastPos = pos + 1;

        tokens->push_back(token);

        pos = str.find(delimiter, lastPos);
    }

    current = tokens->begin();
} 

bool StringTokenizer::hasNext() {
    if (tokens->size() == 0) {
        return false;
    }

    if (current == tokens->end()) {
        return false;
    }

    return true;

}

string StringTokenizer::nextToken() {
    if (!hasNext()) {
        return nullptr;
    }

    string retValue = *current;
    ++current;
    return retValue;
}

StringTokenizer::~StringTokenizer() {

}