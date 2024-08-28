#pragma once

#include "Parser.h"

namespace Battler {

    class UnexpectedTokenException {
    public:
        Token t;
        std::string reason;
        UnexpectedTokenException(Token t, std::string reason) : t{ t }, reason{ reason } {}
    };

    class NoNameException {
    public:
        std::string name;
        NoNameException(std::string name) : name{ name } {}
    };

    class NameRedeclaredException {
    public:
        std::string name;
        NameRedeclaredException(std::string name) : name{ name } {}
    };

    void ensureNoEOF(const std::vector<Token>::iterator t, const std::vector<Token>::iterator end);
    void ensureTokenType(TokenType type, const Token& t, std::string message);
    void ensureTokenTypeAndText(TokenType type, std::string text, const Token& t, std::string message);

}