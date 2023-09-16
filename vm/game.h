#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../expression.h"

class Card;
class AttributeContainer;

enum class AttributeType {INT, FLOAT, STRING, BOOL, CARD_REF, STACK_REF, PLAYER, PLAYER_REF, UNDEFINED};
enum class Scope {LOCAL, GAME};

class OperationError {
    public:
        std::string reason;
        OperationError(std::string reason): reason{reason} {}
};

enum class StackType {VISIBLE, PRIVATE, HIDDEN};

class Attr {
    public:
        AttributeType type;
        std::string s;
        std::string stackRef;
        std::string cardRef;
        
        union {
            int i;
            bool b;
            float f;
            int playerRef;
        };
};

class AttrCont {
    public:

        bool Contains(std::string name) {
            if (attrs.find(name) == attrs.end()) {
                return false;
            }
            return true;
        }

        void Store(std::string name, Attr a) {
            attrs[name] = a;
        }

        Attr& Get(std::string name) {
            return attrs[name];
        }

    private:
        std::unordered_map<std::string, Attr> attrs;
};

class Stack {
    public:
        StackType t;
        int ownerID;
        std::vector<Card>  cards;
        AttrCont attributes;
};

class Card {
    public:
        int ID;
        std::string name;
        std::string parentName;
        AttrCont attributes;
};

class Player {
    public:
        int ID;
        std::string name;
        AttrCont attributes;
};

class Phase {
    public:
        int ID;
        std::string name;
        AttrCont attributes;
        Expression expression;
};

class Game {
    public:
        int ID;
        std::string name;
        AttrCont attributeCont;
        std::unordered_map<std::string, Phase> phases;
        std::unordered_map<std::string, Card> cards;
        std::unordered_map<std::string, Stack> stacks;
        std::unordered_map<std::string, int> playerBindings;
        std::vector<Player> players;
        Expression setup;
        Expression turn;
};
