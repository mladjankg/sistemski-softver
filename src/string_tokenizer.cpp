#include <string>
#include "string_tokenizer.h"
#include "utils.h"
#include "ss_exceptions.h"

using namespace ss;

void StringTokenizer::tokenize(const std::string& str) {
    size_t pos;
    size_t lastPos = 0;

    //Creating list for storing tokens.
    tokens = new list<std::string>();

    //Coping string that needs to be formatted in new string.
    std::string formatted(str);

    //Removing empty characters on the beggining and the end of the string.
    formatted = Utils::trim(formatted);

    //Removing repeating sequence of delimiter char in string.
    formatted = Utils::removeRepeatingChars(formatted, delimiter);

    //Finding position of the first occurence of the delimiter.
    pos = formatted.find(delimiter, lastPos);
    
    if (pos != std::string::npos) {
        while ((pos != string::npos) && ((pos + 1) != formatted.length())) {
            std::string token = formatted.substr(lastPos, pos - lastPos);
            lastPos = pos + 1;
            tokens->push_back(token);

            pos = formatted.find(delimiter, lastPos);
        }
    }
    else {
        tokens->push_back(formatted);
        lastPos = pos;
    }
    
    if (lastPos < formatted.length() && lastPos != std::string::npos) {
        std::string token = formatted.substr(lastPos);
        tokens->push_back(token);
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

string& StringTokenizer::nextToken() throw() {
    if (!hasNext()) {
        throw StringTokenizerException();
    }

    string& retValue = *current;
    ++current;
    return retValue;
}

int StringTokenizer::tokenNumber() {
    if (this->tokens == nullptr) {
        //Tokenizer isn't initialized.
        return -1;
    }

    else {
        return tokens->size();
    }
}

StringTokenizer::~StringTokenizer() {
    delete tokens;
    tokens = nullptr;
}