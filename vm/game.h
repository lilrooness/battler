#ifndef GAME_H
#define GAME_H

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../expression.h"

class Card;
class AttributeContainer;

enum class AttributeType {INT, FLOAT, STRING, BOOL, CARD_REF, STACK_REF, PLAYER, PLAYER_REF, PHASE_REF, STACK_POSITION_REF, UNDEFINED};
enum class Scope {LOCAL, GAME};

using std::cout;
using std::endl;
using std::vector;
using std::string;

class OperationError {
    public:
        std::string reason;
        OperationError(std::string reason): reason{reason} {}
};

enum class StackType {VISIBLE, PRIVATE, HIDDEN};

class Attr {
    public:
        Attr() {}
        Attr(AttributeType t) : type(t) {}
        AttributeType type;
        std::string s;
        int stackRef;
        std::tuple<int, int> stackPositionRef;
        std::string cardRef;
        std::string phaseRef;
        
        union {
            int i;
            bool b;
            float f;
            int playerRef;
        };

        std::string ToString();
};

class AttrCont {
    public:

        bool Contains(std::string name);

        void Store(std::string name, Attr a);

        Attr& Get(std::string name);

        std::string ToString(std::string prefix = "");

    private:
        std::unordered_map<std::string, Attr> attrs;
};

class Stack {
    public:
        int ID;
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
        Expression expression; // old tree walk mode
};

class Game {
    public:
        Game() : winner(-1), currentPlayerIndex(0) {}
        int ID;
        std::string name;
        AttrCont attributeCont;
        std::unordered_map<std::string, Phase> phases;
        std::unordered_map<std::string, Card> cards;
        std::unordered_map<int, Stack> stacks;
        std::unordered_map<std::string, int> playerBindings;
        std::vector<Player> players;
        Expression setup; // old tree walk mode
        Expression turn; // old tree walk mode
        int currentPlayerIndex;
        int winner;

        void Print();

        vector<Card> get_cards_of_type(string type);
};

#endif // !GAME_H