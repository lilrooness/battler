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
    OPERATAOR
};


class Expression {
    public:
        ExpressionType type;
        std::vector<Token> tokens;
        std::vector<Expression> children;
        Expression() {}
        Expression(ExpressionType type, std::vector<Token> tokens): type{type}, tokens{tokens} {}
};

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
        }
        else if (type == ExpressionType::GAME_DECLARATION) {
            ensureTokenType(TokenType::name, *current, "Expected end, or a legal declaration here such as 'card', 'phase', 'turn', 'setup' etc ...");

            if (current->text == "end") {
                return expression;
            }

            expression.children.push_back(GetExpression(current, end, ExpressionType::EXPRESSION));
        }
        else if (type == ExpressionType::EXPRESSION) {
            if (current->type == TokenType::name) {
                if (current->text == "game") {

                    ensureNoEOF(++current, end);
                    ensureTokenType(TokenType::name, *current, "Expected game name declaration here");
                    Expression gameNameDeclaration = Expression(ExpressionType::GAME_NAME_DECLARATION, {*current});
                    
                    ensureNoEOF(++current, end);
                    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected a 'start' here");
                    ensureNoEOF(++current, end);
                    auto gameExpr = GetExpression(current, end, ExpressionType::GAME_DECLARATION);
                    gameExpr.children.push_back(std::move(gameNameDeclaration));
                    expression.children.push_back(gameExpr);

                    return expression;
                }
                
                else if (current->text == "card") {
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
                    cardExpr.children.push_back(cardNameDeclaration);
                    expression.children.push_back(cardExpr);

                    return expression;
                }

                // else if (current->text == "int" || current->text == "bool" || current->text == "string") {
                //     return GetExpression(current, end, ExpressionType::ATTR_DECLARATION);
                // }
            }
        }

        ensureNoEOF(++current, end);
    }

    return expression;
}
