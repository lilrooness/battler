#pragma once

#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>
#include <iostream>
#include <sstream>
#include <memory>

#include "Battler.h"
#include "Parser.h"
#include "interpreter_errors.h"

enum class ExpressionType {
    FACTOR,
    ADDITION,
    MULTIPLICATION,
    DIVISION,
    SUBTRACTION,
    EQUALITY_TEST, // '=='
    EXPRESSION,
    ASSIGNMENT_TARGET,
    ATTR_TYPE_SPECIFIER,
    ATTR_NAME_DECLARATION,
    ATTR_DECLARATION,
    ATTR_ASSIGNMENT,
    CARD_NAME_DECLARATION,
    GAME_NAME_DECLARATION,
    CARD_DECLARATION,
    GAME_DECLARATION,
    FOREACH,
    IDENTIFIER,
    NAME,
    LITERAL,
    GROUPING,
    BINARY,
    OPERATAOR,
    UNKNOWN,
};

using std::cout;
using std::endl;

class Expression {
    public:
        ExpressionType type;
        std::vector<Token> tokens;
        std::vector<Expression> children;
        Expression() {}
        Expression(ExpressionType type, std::vector<Token> tokens): type{type}, tokens{tokens} {}
};

Expression GetExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end, const ExpressionType type);

ExpressionType GetExpressionTypeFromOperatorTokenType(TokenType type) {
    switch (type) {
        case TokenType::plus: return ExpressionType::ADDITION;
        case TokenType::minus: return ExpressionType::SUBTRACTION;
        case TokenType::times: return ExpressionType::MULTIPLICATION;
        case TokenType::divide: return ExpressionType::DIVISION;
        case TokenType::equality: return ExpressionType::EQUALITY_TEST;
        default: return ExpressionType::UNKNOWN;
    }
}

Expression GetGameDelcarationExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected game name declaration here");
    Expression gameNameDeclaration = Expression(ExpressionType::GAME_NAME_DECLARATION, {*current});
    
    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected a 'start' here");
    ensureNoEOF(++current, end);
    auto gameExpr = GetExpression(current, end, ExpressionType::GAME_DECLARATION);
    gameExpr.children.push_back(std::move(gameNameDeclaration));

    return gameExpr;
}

Expression GetCardDeclarationExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected name of Card declared here");
    Expression cardNameDeclaration = Expression(ExpressionType::CARD_NAME_DECLARATION, {*current});
    ensureNoEOF(++current, end);

    if (current->text != "start") {
        ensureTokenType(TokenType::name, *current, "Expected start or card parent name here");
        cardNameDeclaration.tokens.push_back(*current);
        ensureNoEOF(++current, end);
    }

    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected a 'start' here");
    ensureNoEOF(++current, end);

    auto cardExpr = GetExpression(current, end, ExpressionType::CARD_DECLARATION);
    cardExpr.children.push_back(std::move(cardNameDeclaration));

    return cardExpr;
}

Expression GetAttrDeclarationExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    ensureNoEOF(current+1, end);
    auto attrDeclaration = GetExpression(current, end, ExpressionType::ATTR_DECLARATION);
    return attrDeclaration;
}


Expression GetFactorExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {

    Expression expression;

    if (current->type == TokenType::number || current->type == TokenType::name) {
        
        ensureNoEOF(current+1, end);
        if (IsOperatorType((current+1)->type)) {
            
            auto leftFactor = Expression(ExpressionType::FACTOR, {*current});
            
            current++;
            auto expressionType = GetExpressionTypeFromOperatorTokenType(current->type);
            if (expressionType == ExpressionType::UNKNOWN) {
                throw UnexpectedTokenException(*current, "Unsupported Operator");
            }

            expression.type = expressionType;
            expression.tokens.push_back(*current);

            ensureNoEOF(current+1, end);
            current++;

            auto rightFactor = GetFactorExpression(current, end);

            expression.children.push_back(std::move(leftFactor));
            expression.children.push_back(std::move(rightFactor));

        } else {
            expression.type = ExpressionType::FACTOR;
            expression.tokens.push_back(*current);
        }
    }

    return expression;
}

Expression GetAssignmentExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end, std::vector<Token>::iterator &leftAccumulationStart) {
    
    Expression expression;
    expression.type = ExpressionType::ATTR_ASSIGNMENT;
    
    auto assignmentTarget = GetExpression(leftAccumulationStart, end, ExpressionType::ASSIGNMENT_TARGET);

    ensureNoEOF(leftAccumulationStart, end);
    leftAccumulationStart++;
    if (leftAccumulationStart != current) {
        // we can't have more than one expression on the left hand side of an assignment
        throw UnexpectedTokenException(*leftAccumulationStart, "illegal assignment expression");
    }
    ensureNoEOF(current+1, end);
    current++;

    auto rightHandSide = GetFactorExpression(current, end);

    expression.children.push_back(std::move(assignmentTarget));
    expression.children.push_back(std::move(rightHandSide));
    return expression;
}


Expression GetExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end, const ExpressionType type) {

    Expression expression;
    expression.type = type;

    // sometimes we want to process the left hand side expression later EG `my.draw_stack -> my.hand`
    auto leftAccumulationStart = current;

    while (current != end) {
        if (type == ExpressionType::CARD_DECLARATION) {
            ensureTokenType(TokenType::name, *current, "Expected an attribute declaration or assignment here");

            if (current->text == "end") {
                return expression;
            }

            expression.children.push_back(GetExpression(current, end, ExpressionType::EXPRESSION));
        }
        else if (type == ExpressionType::GAME_DECLARATION) {
            ensureTokenType(TokenType::name, *current, "Expected end, or a legal declaration here such as 'card', 'phase', 'turn', 'setup' etc ...");

            if (current->text == "end") {
                return expression;
            }

            expression.children.push_back(GetExpression(current, end, ExpressionType::EXPRESSION));
        }
        else if (type == ExpressionType::ATTR_DECLARATION) {
            ensureTokenType(TokenType::name, *current, "Expected an attribute type specifier here EG 'int', 'bool, 'string'");

            auto typeSpecifierExpression = Expression(ExpressionType::ATTR_TYPE_SPECIFIER, {*current});

            ensureNoEOF(++current, end);
            ensureTokenType(TokenType::name, *current, "Expected an attribute name here EG 'x', 'HP', 'myAttributeName'. This is not a valid name");
            auto nameDeclarationExpression = Expression(ExpressionType::ATTR_NAME_DECLARATION, {*current});

            expression.children.push_back(std::move(typeSpecifierExpression));
            expression.children.push_back(std::move(nameDeclarationExpression));

            return expression;
        }
        else if (type == ExpressionType::ASSIGNMENT_TARGET) {
            ensureTokenType(TokenType::name, *current, "Expected a name here as the target of an assignment");

            expression.tokens.push_back(*current);

            return expression;
        }
        else if (type == ExpressionType::EXPRESSION) {
            if (current->type == TokenType::name) {
                if (current->text == "game") {
                    return GetGameDelcarationExpression(current, end);
                }
                else if (current->text == "card") {
                    return GetCardDeclarationExpression(current, end);
                }
                else if (current->text == "int") {
                    return GetAttrDeclarationExpression(current, end);
                }
                else if (current->text == "setup") {
                    throw UnexpectedTokenException(*current, "setup declarations are not implemented yet ... :(");
                }
                else if(current->text == "phase") {
                    throw UnexpectedTokenException(*current, "phase declarations are not implemented yet ... :(");
                }
                else if(current->text == "turn") {
                    throw UnexpectedTokenException(*current, "turn declarations are not implemented yet ... :(");
                }
                else if (current->text == "bool" || current->text == "string") {
                    throw UnexpectedTokenException(*current, "Attribute Declaration isn't implemented yet ... :( ");
                }
            }

            else if (current->type == TokenType::assignment) {
                return GetAssignmentExpression(current, end, leftAccumulationStart);
            }
        }

        ensureNoEOF(++current, end);
    }

    return expression;
}
