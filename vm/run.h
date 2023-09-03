/**
 * run.h
 * Joseph Frangoudes 2023
 * 
 * A tree-walk interpreter for battler
 * This is really really slow.
 * Eventually this should be replaced by a VM running 
 * some bytecode.
 * 
*/

#include "../Parser.h"
#include "game.h"
#include "../expression.h"



using std::string;
using std::cout;
using std::endl;
using std::vector;

class RuntimeError {
    public:
        Token t;
        string reason;
        RuntimeError(string reason, Token t) : reason{reason}, t{t} {}
};

class UnsupportedTypeException {};
class UnsupportedOperationException {};
class WrongTypeException {};
class UnImplementedFeatureException {};

string GetNameDeclarationFromTokens(vector<Token> tokens) {
    std::stringstream ss;

    for (auto t : tokens) {
        ss << t.text;
    }

    return ss.str();
}

void ensureExpressionType(ExpressionType actual, ExpressionType expected, string reason, Token t) {
    if (actual != expected) {
        throw RuntimeError(reason, t);
    }
}

void ensureNotExpressionType(ExpressionType actual, ExpressionType expectedNot, string reason, Token t) {
    if (actual == expectedNot) {
        throw RuntimeError(reason, t);
    }
}

void StoreValue(string name, string value, AttributeType type, AttributeContainer container) {
    if (type == AttributeType::INT) {
        container.StoreInt(name, std::stoi(value));
    }
    else if (type == AttributeType::BOOL) {
        if (value == "true") {
            container.StoreBool(name, true);
        } else if (value == "false") {
            container.StoreBool(name, false);
        } else {
            throw WrongTypeException();
        }
    }
    else if (type == AttributeType::STRING) {
        container.StoreString(name, value);
    }
    else if (type == AttributeType::FLOAT) {
        container.StoreFloat(name, std::stof(value));
    }
}

void SetValueAtCorrectLocale(string name, string value, Game& game, vector<AttributeContainer>& stack) {
    for (int i=stack.size()-1; i>=0; i--) {
        if (stack[i].typeInfo.find(name) != stack[i].typeInfo.end()) {
            AttributeType type = stack[i].typeInfo.find(name)->second;
            StoreValue(name, value, type, stack[i]);
            return;
        }
    }

    if (game.attributes.typeInfo.find(name) != game.attributes.typeInfo.end()) {
        AttributeType type = game.attributes.typeInfo.find(name)->second;
        StoreValue(name, value, type, game.attributes);
        return;
    }
    return;
}

std::pair<string, string> ResolveTokensToTypeAndValue(vector<Token> tokens) {
    string type;
    string value;
    if (tokens.size() > 1) {
        throw RuntimeError("TODO: Joe really needs to implement attribute dot operator", tokens[0]);
    } else {
        auto t = tokens[0];
        if (t.type == TokenType::name) {
            throw RuntimeError("TODO: Joe really needs to implement attribute resolution", tokens[0]);
        } else {
            value = t.text;
            type = "int";
        }
    }

    return make_pair(type, value);
}

Expression FactorExpression(Expression& expr, const Game& game, const vector<AttributeContainer>& stack) {

    if (expr.type == ExpressionType::FACTOR) {
        return expr;
    }

    auto leftSide = expr.children[0];
    auto rightSide = expr.children[1];

    string leftSideValue; 
    string leftSideType;
    
    auto res = ResolveTokensToTypeAndValue(leftSide.tokens);
    leftSideType = res.first;
    leftSideValue = res.second;

    
    auto rightFactor = FactorExpression(rightSide, game, stack);
    
    string rightSideType;
    string rightSideValue;

    auto rightPair = ResolveTokensToTypeAndValue(rightFactor.tokens);
    rightSideType = rightPair.first;
    rightSideValue = rightPair.second;
    
    switch (expr.type) {
        case ExpressionType::ADDITION:
            if (leftSideType == "int") {
                int left = std::stoi(leftSideValue);
                int right = std::stoi(rightSideValue);

                int result = left + right;

                Expression e;
                Token t;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot currently add these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::SUBTRACTION:
            if (leftSideType == "int") {
                int left = std::stoi(leftSideValue);
                int right = std::stoi(rightSideValue);

                int result = left - right;

                Expression e;
                Token t;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot currently subtract these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::MULTIPLICATION:
            if (leftSideType == "int") {
                int left = std::stoi(leftSideValue);
                int right = std::stoi(rightSideValue);

                int result = left * right;

                Expression e;
                Token t;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot currently multiply these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::DIVISION:
            if (leftSideType == "int") {
                int left = std::stoi(leftSideValue);
                int right = std::stoi(rightSideValue);

                int result = left / right;

                Expression e;
                Token t;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot currently divide these types", expr.tokens[0]);
            }
            break;
            break;
        case ExpressionType::EQUALITY_TEST: throw RuntimeError("TODO: Joe really needs to implement equality testing", expr.tokens[0]);
    }
    throw RuntimeError("Unsupported Operation", expr.tokens[0]);
}

void RunExpression(Expression& expr, Game& game, ExpressionType parent, vector<AttributeContainer>& localeStack) {
    if (expr.type == ExpressionType::GAME_DECLARATION) {
        ensureExpressionType(parent, ExpressionType::UNKNOWN, "Can't declare a game here", expr.tokens[0]);
        ensureExpressionType(expr.children.back().type, ExpressionType::GAME_NAME_DECLARATION, "Expected a game name declaration", expr.children.back().tokens[0]);
        
        auto nameDecl = expr.children.back();
        game.name = nameDecl.tokens[0].text;
        
        expr.children.pop_back();

        for (auto e : expr.children) {
            RunExpression(e, game, expr.type, localeStack);
        }
    }
    else if (expr.type == ExpressionType::CARD_DECLARATION) {
        ensureExpressionType(parent, ExpressionType::GAME_DECLARATION, "Can't declare a card here", expr.tokens[0]);
        Card c{};

        ensureExpressionType(expr.children.back().type, ExpressionType::CARD_NAME_DECLARATION, "Expected to find a card name declaration here", expr.children.back().tokens[0]);
        auto nameTokens = expr.children.back().tokens; 
        c.name = nameTokens[0].text;

        if (nameTokens.size() == 2) {
            c.parentName = nameTokens[1].text;
        }
        expr.children.pop_back();

        localeStack.push_back(AttributeContainer{});
        for (auto e : expr.children) {
            RunExpression(e, game, expr.type, localeStack);
        }
        c.attributes = localeStack.back();
        localeStack.pop_back();

        game.cards[c.name] = std::move(c);
    }
    else if (expr.type == ExpressionType::ATTR_DECLARATION) {
        ensureExpressionType(expr.children.back().type, ExpressionType::ATTR_NAME_DECLARATION, "Expected to find an attr name decl here", expr.children.back().tokens[0]);
        if (expr.children.back().tokens.size() > 1) {
            throw RuntimeError("can't delcare attribute like this", expr.tokens[0]);
        }

        string name = expr.children.back().tokens[0].text;
        string type = (expr.children.end()-2)->tokens[0].text;

        if (type == "int") {
            if (localeStack.empty()) {
                game.attributes.StoreInt(name, 0);
            } else {
                localeStack.back().StoreInt(name, 0);
            }
        }
        else if (type == "bool") {
            if (localeStack.empty()) {
                game.attributes.StoreBool(name, false);
            } else {
                localeStack.back().StoreBool(name, false);
            }
        }
        else if (type == "string") {
            if (localeStack.empty()) {
                game.attributes.StoreString(name, "");
            } else {
                localeStack.back().StoreString(name, "");
            }
        }
        else if (type == "float") {
            if (localeStack.empty()) {
                game.attributes.StoreFloat(name, 0.0f);
            } else {
                localeStack.back().StoreFloat(name, 0.0f);
            }
        }
        else if (type == "visiblestack") {
            Stack s;
            s.t = StackType::VISIBLE;
            game.stacks[name] = s;
        }
        else if (type == "hiddenstack") {
            Stack s;
            s.t = StackType::HIDDEN;
            game.stacks[name] = s;
        }
        else if (type == "privatestack") {
            Stack s;
            s.t = StackType::PRIVATE;
            game.stacks[name] = s;
        }
        else {
            std::stringstream ss;
            ss << "unsupported Type " << type;
            throw RuntimeError(ss.str(), expr.tokens[0]);
        }
    }
    else if (expr.type == ExpressionType::ATTR_ASSIGNMENT) {
        auto rightHandSide = expr.children.back();
        auto assignmentTarget = *(expr.children.end()-2);

        string name = assignmentTarget.tokens[0].text;
        
        Expression factor = FactorExpression(rightHandSide, game, localeStack);
        string value = factor.tokens[0].text;
        SetValueAtCorrectLocale(name, value, game, localeStack);
    }
    else if (expr.type == ExpressionType::SETUP_DECLARATION) {
        game.setup = expr;
    }
    else if (expr.type == ExpressionType::TURN_DECLARATION) {
        game.turn = expr;
    }
    else if (expr.type == ExpressionType::PHASE_DECLARATION) {
        Phase phase;
        phase.name = expr.tokens[0].text;
        phase.expression = expr;
    }
    else if (expr.type == ExpressionType::STACK_MOVE || expr.type == ExpressionType::STACK_MOVE_UNDER) {
        auto sourceStackExpr = expr.children[0];
        auto targetStackExpr = expr.children[1];

        if (sourceStackExpr.tokens.size() > 1) {
            throw RuntimeError("Joe needs to implement referencing stacks by '.'", sourceStackExpr.tokens[2]);
        }

        if (targetStackExpr.tokens.size() > 1) {
            throw RuntimeError("Joe needs to implement referencing stacks by '.'", targetStackExpr.tokens[2]);
        }

        string targetName = targetStackExpr.children[0].tokens[0].text;
        if (game.stacks.find(targetName) == game.stacks.end()) {
                throw RuntimeError(targetName + " is not defined", targetStackExpr.children[0].tokens[0]);
        }

        Expression targetNumberExpression = FactorExpression(targetStackExpr.children[1], game, localeStack);
        int moveNumber = std::stoi(targetNumberExpression.tokens[0].text);

        if (moveNumber < 1) {
            cout << "moving zero, ignoring move expression" << endl;
            return;
        }

        if (sourceStackExpr.type == ExpressionType::STACK_MOVE_SOURCE) {
            string sourceName = sourceStackExpr.tokens[0].text;
            
            if (game.stacks.find(sourceName) == game.stacks.end()) {
                throw RuntimeError(sourceName + " is not defined", sourceStackExpr.tokens[0]);
            }

            Stack& source = game.stacks[sourceName];
            Stack& target = game.stacks[targetName];

            if (source.cards.size() == 0) {
                cout << "source for move contained no cards, ignoring expression" << endl;
                return;
            }

            vector<Card> cardsTaken;

            // the target Expression type determines we want to take cards from the source stack
            if (targetStackExpr.type == ExpressionType::STACK_SOURCE_TOP) {
                // take cards from the top of the source deck
                int limit = source.cards.size() - std::min(
                    static_cast<int>(source.cards.size()),
                    moveNumber
                );

                auto start = source.cards.end() - limit;
                auto end = source.cards.end() - 1;

                cardsTaken = vector<Card>(start, end);
                source.cards.erase(start, end);
            }
            else if (targetStackExpr.type == ExpressionType::STACK_SOURCE_BOTTOM) {
                // take cards from the bottom of the source deck
                int limit = std::min(
                    static_cast<int>(source.cards.size()),
                    moveNumber
                );
                
                auto start = source.cards.begin();
                auto end = source.cards.begin() + (limit-1);

                cardsTaken = vector<Card>(start, end);
                source.cards.erase(start, end);

                /**
                 * the code that takes the cards and puts them in the new stack
                 * shouldn't care about the order in which they were taken from the
                 * source stack. It assumes that the 'backmost' card is the one you
                 * would take first, whether from the bottom, or the top
                */
                std::reverse(cardsTaken.begin(), cardsTaken.end());
            }
            else if (targetStackExpr.type == ExpressionType::STACK_SOURCE_CHOOSE) {
                throw RuntimeError("'choose' move isn't implemented yet", targetStackExpr.tokens[0]);
            }

            if (expr.type == ExpressionType::STACK_MOVE) {
                std::reverse(cardsTaken.begin(), cardsTaken.end());
                target.cards.insert(target.cards.end(), cardsTaken.begin(), cardsTaken.end());
            }
            else if (expr.type == ExpressionType::STACK_MOVE_UNDER) {
                target.cards.insert(target.cards.begin(), cardsTaken.begin(), cardsTaken.end());
            }
        }
        else if (sourceStackExpr.type == ExpressionType::STACK_MOVE_RANDOM_SOURCE) {
            throw RuntimeError("random move isn't implemented yet", expr.tokens[0]);
        }
    }
    else {
        std::stringstream ss;
        ss << "Unsupported Expression ";
        for (auto t : expr.tokens) {
            ss << t.text;
        }
        throw RuntimeError(ss.str(), expr.tokens[0]);
    }
}
