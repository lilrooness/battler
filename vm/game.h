#ifndef GAME_H
#define GAME_H

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../expression.h"

class Card;
class AttributeContainer;

enum class AttributeType {INT, FLOAT, STRING, BOOL, CARD_REF, STACK_REF, PLAYER, PLAYER_REF, PHASE_REF, STACK_POSITION_REF, UNDEFINED};
enum class Scope {LOCAL, GAME};

using std::cout;
using std::endl;

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
        std::string stackRef;
        std::tuple<std::string, int> stackPositionRef;
        std::string cardRef;
        std::string phaseRef;
        
        union {
            int i;
            bool b;
            float f;
            int playerRef;
        };

        std::string ToString() {
            std::stringstream ss;
            
            if (type == AttributeType::BOOL) {
                ss << "BOOL " << b;
            }
            else if (type == AttributeType::INT) {
                ss << "INT " << i;
            }
            else if (type == AttributeType::FLOAT) {
                ss << "FLOAT " << f;
            }
            else if (type == AttributeType::STRING) {
                ss << "STRING " << s;
            }
            else if (type == AttributeType::CARD_REF) {
                ss << "CARD_REF " << cardRef;
            }
            else if (type == AttributeType::PLAYER_REF) {
                ss << "PLAYER_REF " << playerRef;
            }
            else if (type == AttributeType::STACK_REF) {
                ss << "STACK_REF " << stackRef;
            }
            else if (type == AttributeType::STACK_POSITION_REF) {
                ss << "STACK_POSITION_REF" << std::get<0>(stackPositionRef) << " " << std::get<1>(stackPositionRef);
            }

            return ss.str();
        }
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

        std::string ToString(std::string prefix = "") {
            std::stringstream ss;
            for (auto pair : attrs) {
                ss << endl << prefix << pair.first << ": " << pair.second.ToString();
            }

            return ss.str();
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
        Game() : winner(-1) {}
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
        int currentPlayerIndex;
        int winner;

        void Print() {
            cout << "Cards:" << endl;

            for (auto pair : cards) {
                cout << pair.first << ":"<< pair.second.attributes.ToString("    ") << endl;
            }

            cout << "Stacks:" << endl;

            for (auto pair : stacks) {
                cout << pair.first << ":"<< pair.second.attributes.ToString("    ") << endl;
            }

            cout << "Players:" << endl;

            for (int i=0; i<players.size(); i++) {
                cout << i << ":"<< players[i].attributes.ToString("    ") << endl;
            }

            cout << "Game Attributes:" << endl;

            cout << attributeCont.ToString("    ") << endl;
        }
};

#endif // !GAME_H