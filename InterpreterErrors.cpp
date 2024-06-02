#include "interpreter_errors.h"

void ensureNoEOF(const std::vector<Token>::iterator t, const std::vector<Token>::iterator end) {
    if (t == end) {
        throw UnexpectedTokenException(*(t - 1), "This token cannot end a file");
    }
}

void ensureTokenType(TokenType type, const Token& t, std::string message) {

    if (t.type != type) {

        throw UnexpectedTokenException(t, message);
    }
}

void ensureTokenTypeAndText(TokenType type, std::string text, const Token& t, std::string message) {

    if (t.type != type || t.text != text) {

        throw UnexpectedTokenException(t, message);
    }
}

