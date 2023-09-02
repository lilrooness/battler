#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class Card;
class AttributeContainer;

enum class AttributeType {INT, FLOAT, STRING, BOOL, CARD, STACK, UNDEFINED};

class OperationError {
    public:
        std::string reason;
        OperationError(std::string reason): reason{reason} {}
};

enum class StackType {VISIBLE, PRIVATE, HIDDEN};

class AttributeContainer {
    public:
        
        std::unordered_map<std::string, AttributeType> typeInfo;
        
        std::unordered_map<std::string, bool> bools;
        std::unordered_map<std::string, int> ints;
        std::unordered_map<std::string, float> floats;
        std::unordered_map<std::string, std::string> strings;

        void StoreInt(std::string name, int value)  {
            Store<int>(name, value, AttributeType::INT, ints);
        }

        void StoreBool(std::string name, bool value)  {
            Store<bool>(name, value, AttributeType::BOOL, bools);
        }

        void StoreFloat(std::string name, float value)  {
            Store<float>(name, value, AttributeType::FLOAT, floats);
        }

        void StoreString(std::string name, std::string value)  {
            Store<std::string>(name, value, AttributeType::STRING, strings);
        }

        template<typename T>
        T& GetRef(std::string name, AttributeType t, std::unordered_map<std::string, T>& map) {
            auto itr = typeInfo.find(name);
            if (itr == typeInfo.end()) {
                throw NoNameException(name);
            }

            return map[name];
        }

    private:
        template<typename T>
        void Store(std::string name, T value, AttributeType attrType, std::unordered_map<std::string, T>& map) {
            auto itr = typeInfo.find(name);
            if (itr == typeInfo.end() || itr->second == attrType) {
                typeInfo[name] = attrType;
                map[name] = std::move(value);
            } else {
                throw OperationError("trying to store the wrong type in this attribute");
            }
        }
};

class Card {
    public:
        int ID;
        std::string name;
        std::string parentName;
        AttributeContainer attributes;
};

class Stack {
    public:
        StackType t;
        int ownerID;
        std::vector<Card>  cards;
};

class Player {
    public:
        int ID;
        std::string name;
        AttributeContainer attributes;
};

class Phase {
    public:
        int ID;
        std::string name;
        AttributeContainer attributes;
        Expression expression;
};

class Game {
    public:
        int ID;
        std::string name;
        AttributeContainer attributes;
        std::unordered_map<std::string, Phase> phases;
        std::unordered_map<std::string, Card> cards;
        std::unordered_map<std::string, Stack> stacks;
        std::vector<Player> players;
        Expression setup;
        Expression turn;
};
