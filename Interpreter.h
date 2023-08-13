#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>
#include <iostream>
#include <sstream>

#include "Battler.h"
#include "Parser.h"
#include "interpreter_errors.h"

enum class StackLevel {

    def_game_name,
    def_game_start,
    def_game,
    
    def_card,

    function_call,

};

struct AttributeDeclaration {
    std::string type;
    std::string name;
    std::string value;
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

            // check for attribute assignment here too
            ensureNoEOF(t+1, end);
            if ((t+1)->type == TokenType::assignment) {
                t++;
                ensureNoEOF(t+1, end);
                ensureTokenType(TokenType::number, *(t+1), "Expected an integer value here");
                t++;

                decl.value = t->text;
            }

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
            stack.back().cardDecl.attributeAssignments.push_back(decl);

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

    assert( stack.size() == 1, "done consuming tokens, but the stack size is greather than 1");

    GameDeclaration decl = stack.back().gameDecl;

    std::vector<CardDeclaration> subclasses;


    // at first only check for base card class declarations
    for (auto cardDecl : decl.cardDeclarations) {
        
        if (!cardDecl.parent.empty()) {
            subclasses.push_back(std::move(cardDecl));
            continue;
        }

        std::vector<Attribute> attributes;
        for (const auto& attributeDecl : cardDecl.attributeDeclarations) {
            
            // handle attribute type resolution and value if present
            Attribute attribute(attributeDecl.name, CardAttributeType::CAT_INT);
            if (!attributeDecl.value.empty()) {
                attribute._int = stoi(attributeDecl.value);
            } else {
                attribute._int = 0;
            }

            attributes.push_back(attribute);
        }

        CardClass cardClass(cardDecl.name, attributes);
        game.cardClasses.push_back(cardClass);
    }

    // now process parent hierarchies and attribute assignments

    for (auto cardDecl : subclasses) {
        
        auto isClass = [cardDecl](CardClass c) {return c.name == cardDecl.parent; };
        auto parentClassPtr = std::find_if(game.cardClasses.begin(), game.cardClasses.end(), isClass);
        
        if (parentClassPtr == game.cardClasses.end()) {
        
            throw NoNameException(cardDecl.parent);
        }

        

        std::vector<Attribute> attributes{parentClassPtr->attributes};
        
        for (auto attributeDecl : cardDecl.attributeDeclarations) {
            
            Attribute attribute(attributeDecl.name, CardAttributeType::CAT_INT);
            attribute._int = stoi(attributeDecl.value);
            auto attrExists = [attribute](Attribute a) {return a.name == attribute.name; };

            if (attributes.end() != std::find_if(attributes.begin(), attributes.end(), attrExists)) {

                throw NameRedeclaredException(attribute.name);
            }

            attributes.push_back(attribute);
        }

        for (auto attributeAssignment : cardDecl.attributeAssignments) {
                
            auto attrExists = [attributeAssignment](Attribute a) {return a.name == attributeAssignment.name; };
            auto thisAttrPtr = std::find_if(attributes.begin(), attributes.end(), attrExists);
            if (thisAttrPtr == attributes.end()) {

                throw NoNameException(attributeAssignment.name);
            }

            thisAttrPtr->_int = stoi(attributeAssignment.value);
        }

        CardClass cardClass(cardDecl.name, attributes);
        cardClass.parentIndex = std::distance(game.cardClasses.begin(), parentClassPtr);
        game.cardClasses.push_back(cardClass);
    }

    return game;
};

