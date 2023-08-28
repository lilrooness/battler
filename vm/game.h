#pragma once

#include <memory>
#include <string>
#include <unordered_map>

enum class AttributeType {INT, FLOAT, STRING, BOOL, CARD, STACK, UNDEFINED};

class OperationError {
    public:
        std::string reason;
        OperationError(std::string reason): reason{reason} {}
};

class Card {
    public:
        std::string name;
        AttributeType attributes;
};

enum class StackType {VISIBLE, PRIVATE, HIDDEN};

class Stack {
    public:
        StackType t;
        std::vector<Card>  cards;
};

class AttributeContainer {
    public:
        
        std::unordered_map<std::string, AttributeType> typeInfo;
        
        std::unordered_map<std::string, bool> bools;
        std::unordered_map<std::string, int> ints;
        std::unordered_map<std::string, float> floats;
        std::unordered_map<std::string, std::string> strings;
        std::unordered_map<std::string, Card> cards;
        std::unordered_map<std::string, Stack> stacks;

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

        void StoreCard(std::string name, Card&& value)  {
            Store<Card>(name, value, AttributeType::CARD, cards);
        }

        void StoreStack(std::string name, Stack value)  {
            Store<Stack>(name, value, AttributeType::STACK, stacks);
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

class Phase {
    public:
        AttributeContainer attributes;
};

class Game {
    public:
        AttributeContainer attributes;
};
