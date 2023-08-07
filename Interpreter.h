#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>
#include <iostream>
#include <sstream>

#include "Battler.h"
#include "Parser.h"

class UnexpectedTokenException {
    public:
        Token t;
        std::string reason;
        UnexpectedTokenException(Token t, std::string reason): t{t}, reason{reason} {} 
};

enum class StackLevel {

    def_game_name,
    def_game_start,
    def_game,
    
    def_card,

    assign_attr,
    
    function_call,

};

struct AttributeDeclaration {
    std::string type;
    std::string name;
};

struct AttributeAssignment {
    std::string name;
    std::string value;
};

struct CardDeclaration {
    std::string name;
    std::string parent;
    std::vector<AttributeDeclaration> attributeDeclarations;
    std::vector<AttributeAssignment> attributeAssignments;
};

struct StackDeclaration {
    std::string name;
};

struct GameDeclaration {
    std::string name;
    std::vector<StackDeclaration> stackDeclarations;
    std::vector<CardDeclaration> cardDeclarations;
};

struct StackLevelData {

    StackLevel level;

    GameDeclaration gameDecl;
    StackDeclaration stackDecl;
    CardDeclaration cardDecl;
    AttributeDeclaration attrDecl;
    AttributeAssignment attrAssign;

};

bool AtStackLevel(StackLevel level, const std::vector<StackLevelData> &stack) {

    return !stack.empty() && stack.back().level == level;
}

void ensureTokenType(TokenType type, const Token &t, std::string message) {

    if (t.type != type) {

        throw UnexpectedTokenException(t, message);
    }
}

void ensureTokenTypeAndText(TokenType type, std::string text, const Token &t, std::string message) {

    if (t.type != type || t.text != text) {
        
        throw UnexpectedTokenException(t, message);
    }
}

void pushStackLevel(StackLevel level, std::vector<StackLevelData> &stack) {

    StackLevelData defCardLevel;
    defCardLevel.level = level;
    stack.push_back(defCardLevel);
}

void ensureNoEOF(const std::vector<Token>::iterator &t, const std::vector<Token>::iterator end) {
    if (t == end) {
        throw UnexpectedTokenException(*t, "This token cannot end a file");
    }
}

void ConsumeTokens(std::vector<StackLevelData> &stack, std::vector<Token>::iterator &t, const std::vector<Token>::iterator end) {
    assert(t != end, "next does not equal end");

    if (AtStackLevel(StackLevel::def_card, stack)) {
        
        ensureTokenType(TokenType::name, *t, "Expected an attribute declaration, an 'end' or an assignment here");

        if (t->text == "end") {

            CardDeclaration decl = stack.back().cardDecl;
            stack.pop_back();
            stack.back().gameDecl.cardDeclarations.push_back(decl);
        } else if (t->text == "int"){

            AttributeDeclaration decl;
            decl.type = t->text;

            ensureNoEOF(t+1, end);
            ensureTokenType(TokenType::name, *(t+1), "Expenced an attribute name here");
            t++;

            decl.name = t->text;
            stack.back().cardDecl.attributeDeclarations.push_back(decl);

        } else {
            
            AttributeAssignment decl;
            decl.name = t->text;

            ensureNoEOF(t+1, end);
            ensureTokenType(TokenType::assignment, *(t+1), "Expenced an '=' sign here");
            t++;

            ensureNoEOF(t+1, end);
            ensureTokenType(TokenType::number, *(t+1), "Expected an integer value here");
            t++;
            
            decl.value = t->text;
            
        }

    } else if (AtStackLevel(StackLevel::def_game, stack)) {

        ensureTokenType(TokenType::name, *t, "Expected a card or stack declaration here");

        if (t->text == "stack") {

            ensureNoEOF(t+1, end);
            ensureTokenType(TokenType::name, *(t+1), "Expected the name of the stack here");
            t++;

            StackDeclaration decl{decl.name};
            stack.back().gameDecl.stackDeclarations.push_back(decl);

        } else if (t->text == "card") {

            pushStackLevel(StackLevel::def_card, stack);
            
            ensureNoEOF(t+1, end);
            ensureTokenType(TokenType::name, *(t+1), "Expected the name of the card here");
            t++;
            stack.back().cardDecl.name = t->text;
            
            ensureNoEOF(t+1, end);
            ensureTokenType(TokenType::name, *(t+1), "Expected either the name of the parent card or a 'start' here here");
            t++;

            if (t->text != "start") {
                stack.back().cardDecl.parent = t->text;
                ensureNoEOF(t+1, end);
                ensureTokenTypeAndText(TokenType::name, "start", *(t+1), "Expected a start here");
                t++;
            }
        }

    } else if (AtStackLevel(StackLevel::def_game_name, stack)) {

        ensureTokenType(TokenType::name, *t, "Expected valid game name");

        stack.pop_back();

        stack.back().gameDecl.name = t->text;

        pushStackLevel(StackLevel::def_game_start, stack);

    } else if (AtStackLevel(StackLevel::def_game_start, stack)) {

        ensureTokenTypeAndText(TokenType::name, "start", *t, "start expected here");
        stack.pop_back();

    } else if (t->type == TokenType::name) {

        if (t->text == "game") {

            if (!stack.empty()) {

                throw UnexpectedTokenException(*t, "You can't declare a new game here");
            }
            pushStackLevel(StackLevel::def_game, stack);
            pushStackLevel(StackLevel::def_game_name, stack);
        } else {

            throw UnexpectedTokenException(*t, "NOT YET IMPLEMENTED");
        }
    } else {

        throw UnexpectedTokenException(*t, "NOT YET IMPLEMENTED");
    }
    ++t;
}

BattlerGame LoadGameFromTokens(std::vector<Token> &tokens) {

    std::vector<StackLevelData> stack;
    BattlerGame game;
    bool gameDefined = false;

    auto begin = tokens.begin();
    auto end = tokens.end();

    while (begin != end) {
        ConsumeTokens(stack, begin, end);
    }

    return game;
};

