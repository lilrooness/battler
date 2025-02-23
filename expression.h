#ifndef EXPRESSION_H
#define EXPRESSION_H

#pragma once

#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>
#include <iostream>
#include <sstream>
#include <memory>

#include "Parser.h"
#include "interpreter_errors.h"

namespace Battler {

    enum class ExpressionType {
        // EXPRESSION,
        // OPERATAOR,
        FACTOR,
        ADDITION,
        MULTIPLICATION,
        DIVISION,
        SUBTRACTION,
        EQUALITY_TEST, // '=='
        // FOREACH,
        IDENTIFIER,
        // NAME,
        // LITERAL,
        // GROUPING,
        // BINARY,
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
        FOREACHPLAYER_IDENTIFIER_DECLARATION,
        STACK_TRANSFER, // General expression for stack move and cut operations
        STACK_MOVE,
        STACK_MOVE_UNDER,
        STACK_MOVE_SOURCE,
        STACK_MOVE_SOURCE_MULTI,
        STACK_MOVE_RANDOM_SOURCE,
        STACK_MOVE_CHOOSE_SOURCE,
        STACK_MOVE_CHOOSE_SOURCE_MULTI,
        STACK_CUT_CHOOSE_SOURCE,
        STACK_CUT_SOURCE,
        STACK_CUT,
        STACK_CUT_UNDER,
        STACK_CUT_SOURCE_BOTTOM,
        STACK_CUT_SOURCE_TOP,
        STACK_SOURCE_BOTTOM,
        STACK_SOURCE_TOP,
        STACK_SOURCE_CHOOSE,
        STACK_POSITION,
        PHASE_DECLARATION,
        ONPLACE_DECLARATION,
        ONPLACE_STACK_DECLARATION,
        ONPLACE_IDENTIFIER_DECLARATION,
        IF_DECLARATION,
        TURN_DECLARATION,
        WINER_DECLARATION,
        DO_DECLARATION,
        PLAYERS_DECLARATION,
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
        Expression(ExpressionType type_in, std::vector<Token> tokens_in) : type( type_in ), tokens(tokens_in) {}
    };

    ExpressionType GetExpressionTypeFromOperatorTokenType(TokenType type);
    std::vector<Token> GetIdentifierTokens(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    std::vector<Expression> GetBlock(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetGameDelcarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetAttrDeclarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetFactorExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetAssignmentExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end, std::vector<Token>::iterator& leftAccumulationStart);
    Expression GetCardDeclarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetSetupExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetForEachPlayerExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetStackMoveSourceExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetStackMoveTargetExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end, bool requirePosition=true, bool requireAmount=true);
    Expression GetPhaseDeclarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetOnPlaceExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetIfExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetWinnerIsExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetTurnExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetDoExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetPlayersExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);
    Expression GetExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end);


}
#endif // !EXPRESSION_H
