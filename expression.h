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
    EXPRESSION,
    OPERATAOR,
    FACTOR,
    ADDITION,
    MULTIPLICATION,
    DIVISION,
    SUBTRACTION,
    EQUALITY_TEST, // '=='
    FOREACH,
    IDENTIFIER,
    NAME,
    LITERAL,
    GROUPING,
    BINARY,
    ASSIGNMENT_TARGET,
    ATTR_TYPE_SPECIFIER,
    ATTR_NAME_DECLARATION,
    ATTR_DECLARATION,
    ATTR_ASSIGNMENT,
    CARD_NAME_DECLARATION,
    GAME_NAME_DECLARATION,
    CARD_DECLARATION,
    GAME_DECLARATION,
    SETUP_DECLARATION,
    FOREACHPLAYER_DECLARATION,
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

Expression GetExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end);

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

std::vector<Expression> GetBlock(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    std::vector<Expression> expressions;

    while (current != end) {
        if (current->text == "end") {
            return expressions;
        }

        expressions.push_back(GetExpression(current, end));
        
        current++;
    }

    return expressions;
}

Expression GetGameDelcarationExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected game name declaration here");
    Expression gameNameDeclaration = Expression(ExpressionType::GAME_NAME_DECLARATION, {*current});
    
    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected a 'start' here");
    ensureNoEOF(++current, end);

    Expression gameExpr;
    gameExpr.type = ExpressionType::GAME_DECLARATION;
    gameExpr.children = GetBlock(current, end);
    gameExpr.children.push_back(std::move(gameNameDeclaration));

    return gameExpr;
}

Expression GetAttrDeclarationExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    ensureNoEOF(current+1, end);
    
    Expression attrDeclaration;
    attrDeclaration.type = ExpressionType::ATTR_DECLARATION;

    ensureTokenType(TokenType::name, *current, "Expected an attribute type specifier here EG 'int', 'bool, 'string'");

    auto typeSpecifierExpression = Expression(ExpressionType::ATTR_TYPE_SPECIFIER, {*current});

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected an attribute name here EG 'x', 'HP', 'myAttributeName'. This is not a valid name");
    auto nameDeclarationExpression = Expression(ExpressionType::ATTR_NAME_DECLARATION, {*current});

    attrDeclaration.children.push_back(std::move(typeSpecifierExpression));
    attrDeclaration.children.push_back(std::move(nameDeclarationExpression));

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
    } else {
        throw UnexpectedTokenException(*current, "expected a value or a var name here");
    }

    return expression;
}

Expression GetAssignmentExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end, std::vector<Token>::iterator &leftAccumulationStart) {
    
    Expression expression;
    expression.type = ExpressionType::ATTR_ASSIGNMENT;

    Expression assignmentTarget;
    assignmentTarget.type = ExpressionType::ASSIGNMENT_TARGET;
    ensureTokenType(TokenType::name, *leftAccumulationStart, "Expected a name here as the target of an assignment");
    assignmentTarget.tokens.push_back(*leftAccumulationStart);
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

    Expression cardExpr;
    cardExpr.type = ExpressionType::CARD_DECLARATION;
    cardExpr.children = GetBlock(current, end);
    cardExpr.children.push_back(std::move(cardNameDeclaration));

    return cardExpr;
}

Expression GetSetupExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
    ensureTokenTypeAndText(TokenType::name, "setup", *current, "you can't start a setup expression with this");
    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "expected 'start' here");
    ensureNoEOF(++current, end);

    Expression expr;
    expr.type = ExpressionType::SETUP_DECLARATION;
    expr.children = GetBlock(current, end);

    return expr;
}

// Expression GetForEachPlayerExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {
//     ensureTokenTypeAndText(TokenType::name, "foreachplayer", *current, "foreachplayer expected for this kind of loop");
//     ensureNoEOF(++current, end);
//     ensureTokenType(TokenType::name, *current, "expected each player identifier here");
//     ensureNoEOF(++current, end);
//     ensureTokenTypeAndText(TokenType::name, "start", *current, "expected 'start' here");
//     ensureNoEOF(++current, end);

//     Expression expr;
//     expr.type = ExpressionType::FOREACHPLAYER_DECLARATION;
//     expr.children = GetBlock(current, end);

//     return expr;
// }
 

Expression GetExpression(std::vector<Token>::iterator &current, const std::vector<Token>::iterator end) {

    Expression expression;

    // sometimes we want to process the left hand side expression later EG `my.draw_stack -> my.hand`
    auto leftAccumulationStart = current;

    while (current != end) {
        if (current->type == TokenType::name) {
            if (current->text == "game") {
                return GetGameDelcarationExpression(current, end);
            }
            else if (current->text == "card") {
                return GetCardDeclarationExpression(current, end);
            }
            else if (current->text == "setup") {
                return GetSetupExpression(current, end);
            }
            else if (current->text == "players") {
                throw UnexpectedTokenException(*current, "player declarations are not implemented yet ... :(");
            }
            else if(current->text == "phase") {
                throw UnexpectedTokenException(*current, "phase declarations are not implemented yet ... :(");
            }
            else if(current->text == "turn") {
                throw UnexpectedTokenException(*current, "turn declarations are not implemented yet ... :(");
            }
            else if (current->text == "foreachplayer") {
                throw UnexpectedTokenException(*current, "turn declarations are not implemented yet ... :(");
                // return GetForEachPlayerExpression(current, end);
            }
            else if ((current+1) !=end && (current+1)->type == TokenType::name) {
                return GetAttrDeclarationExpression(current, end);
            }
        }

        else if (current->type == TokenType::assignment) {
            return GetAssignmentExpression(current, end, leftAccumulationStart);
        }

        ensureNoEOF(++current, end);
    }

    return expression;
}
