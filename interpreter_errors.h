#include "Battler.h"
#include "Parser.h"

class UnexpectedTokenException {
    public:
        Token t;
        std::string reason;
        UnexpectedTokenException(Token t, std::string reason): t{t}, reason{reason} {} 
};

class NoNameException {
    public:
        std::string name;
        NoNameException(std::string name): name{name} {}
};

class NameRedeclaredException {
    public:
        std::string name;
        NameRedeclaredException(std::string name): name{name} {}
};
