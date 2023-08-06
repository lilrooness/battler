#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>

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

    def_stack_name,
    
    def_card_name,
    def_card_parent,
    def_card_start,
    def_card,

    def_attr_name,
    def_attr,
    
    assign_attr,
    
    function_call,

};

struct AttributeDeclaration {
    CardAttributeType type;
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

bool AtStackLevel(StackLevel level, std::vector<StackLevelData> stack) {
    return !stack.empty() && stack.back().level == level;
}

void ensureTokenType(TokenType type, Token t, std::string message) {
    if (t.type != type) {
        throw UnexpectedTokenException(t, message);
    }
}

void ensureTokenTypeAndText(TokenType type, std::string text, Token t, std::string message) {
    if (t.type != type || t.text != text) {
        throw UnexpectedTokenException(t, message);
    }
}

BattlerGame LoadGameFromTokens(std::vector<Token> &tokens) {

    std::vector<StackLevelData> stack;
    BattlerGame game;
    bool gameDefined = false;

    // throw UnexpectedTokenException(tokens.at(24));
    for (Token t : tokens) {

        if (AtStackLevel(StackLevel::def_card_start, stack)) {
            
            ensureTokenTypeAndText(TokenType::name, "start", t, "Expected the a start here");
            stack.pop_back();
            StackLevelData data;
            data.level = StackLevel::def_card;
            stack.push_back(data);

        } else if (AtStackLevel(StackLevel::def_card_parent, stack)) {
            
            ensureTokenType(TokenType::name, t, "Expected the name of the Parent card or start here");
            
            if (t.text == "start") {
                // there is no parent, go straight to the card definition
                stack.pop_back();
                StackLevelData data;
                data.level = StackLevel::def_card;
                stack.push_back(data);
            } else {
                // TODO: cache parent card name for the previous stack level
                stack.pop_back();
                StackLevelData data;
                data.level = StackLevel::def_card_start;
                stack.push_back(data);
            }

        } else if (AtStackLevel(StackLevel::def_card_name, stack)) {

            ensureTokenType(TokenType::name, t, "Expected the name of the card here");
            // TODO: cache the card name for the previous stack level
            stack.pop_back();

            StackLevelData data;
            data.level = StackLevel::def_card_parent;
            stack.push_back(data);

        } else if (AtStackLevel(StackLevel::def_stack_name, stack)) {

            ensureTokenType(TokenType::name, t, "Expected the name of the stack here");
            // TODO: Cache the declaration for the previous stack level
            stack.pop_back();
        
        } else if (AtStackLevel(StackLevel::def_game, stack)) {

            ensureTokenType(TokenType::name, t, "Expected a card or stack declaration here");

            if (t.text == "stack") {

                StackLevelData data;
                data.level = StackLevel::def_stack_name;
                stack.push_back(data);

            } else if (t.text == "card") {

                StackLevelData data;
                data.level = StackLevel::def_card_name;
                stack.push_back(data);

            }

        } else if (AtStackLevel(StackLevel::def_game_name, stack)) {

            ensureTokenType(TokenType::name, t, "Expected valid game name");

            // TODO: Cache the game name for the previous stack level
            stack.pop_back();

            StackLevelData data;
            data.level = StackLevel::def_game_start;
            stack.push_back(data);

        } else if (AtStackLevel(StackLevel::def_game_start, stack)) {

            ensureTokenTypeAndText(TokenType::name, "start", t, "start expected here");
            stack.pop_back();
            StackLevelData data;
            data.level = StackLevel::def_game;
            stack.push_back(data);

        } else if (t.type == TokenType::name) {

            if (t.text == "game") {

                if (!stack.empty()) {

                    throw UnexpectedTokenException(t, "You can't declare a new game here");
                }
                StackLevelData data;
                data.level = StackLevel::def_game_name;
                stack.push_back(data);
            } else {

                throw UnexpectedTokenException(t, "NOT YET IMPLEMENTED");
            }
        } else {

            throw UnexpectedTokenException(t, "NOT YET IMPLEMENTED");
        }
    }

    return game;
};

