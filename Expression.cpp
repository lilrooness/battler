#include "expression.h"

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

std::vector<Token> GetIdentifierTokens(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {

    std::vector<Token> tokens;

    while (current != end) {

        ensureTokenType(TokenType::name, *current, "identifiers must be names");
        tokens.push_back(*current);

        if ((current + 1) != end && (current + 1)->type == TokenType::dot) {
            current += 2;
        }
        else {
            return tokens;
        }
    }
    throw UnexpectedTokenException(*(current - 1), "Unexpected end to token stream");
}

std::vector<Expression> GetBlock(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
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

Expression GetGameDelcarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Token t = *current;
    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected game name declaration here");
    Expression gameNameDeclaration = Expression(ExpressionType::GAME_NAME_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected a 'start' here");
    ensureNoEOF(++current, end);

    Expression gameExpr;
    gameExpr.tokens.push_back(t);
    gameExpr.type = ExpressionType::GAME_DECLARATION;
    gameExpr.children = GetBlock(current, end);
    gameExpr.children.push_back(std::move(gameNameDeclaration));

    return gameExpr;
}

Expression GetAttrDeclarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    ensureNoEOF(current + 1, end);

    Expression attrDeclaration;
    attrDeclaration.type = ExpressionType::ATTR_DECLARATION;
    attrDeclaration.tokens.push_back(*current);

    ensureTokenType(TokenType::name, *current, "Expected an attribute type specifier here EG 'int', 'bool, 'string'");

    auto typeSpecifierExpression = Expression(ExpressionType::ATTR_TYPE_SPECIFIER, { *current });

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected an attribute name here EG 'x', 'HP', 'myAttributeName'. This is not a valid name");
    auto nameDeclarationExpression = Expression(ExpressionType::ATTR_NAME_DECLARATION, {});
    nameDeclarationExpression.tokens = GetIdentifierTokens(current, end);

    attrDeclaration.children.push_back(std::move(typeSpecifierExpression));
    attrDeclaration.children.push_back(std::move(nameDeclarationExpression));

    return attrDeclaration;
}

Expression GetFactorExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {

    Expression expression;

    if (current->type == TokenType::number || current->type == TokenType::name) {

        std::vector<Token> identifierTokens;
        if (current->type == TokenType::name) {
            identifierTokens = GetIdentifierTokens(current, end);
        }
        else {
            identifierTokens = { *current };
        }

        ensureNoEOF(current + 1, end);
        if (IsOperatorType((current + 1)->type)) {
            current++;
            auto leftFactor = Expression(ExpressionType::FACTOR, identifierTokens);

            auto expressionType = GetExpressionTypeFromOperatorTokenType(current->type);
            if (expressionType == ExpressionType::UNKNOWN) {
                throw UnexpectedTokenException(*current, "Unsupported Operator");
            }

            expression.type = expressionType;
            expression.tokens.push_back(*current);

            ensureNoEOF(++current, end);

            auto rightFactor = GetFactorExpression(current, end);

            expression.children.push_back(std::move(leftFactor));
            expression.children.push_back(std::move(rightFactor));

        }
        else {
            expression.type = ExpressionType::FACTOR;
            expression.tokens = identifierTokens;
        }
    }
    else {
        throw UnexpectedTokenException(*current, "expected a value or a var name here");
    }

    return expression;
}

Expression GetAssignmentExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end, std::vector<Token>::iterator& leftAccumulationStart) {

    Expression expression;
    expression.type = ExpressionType::ATTR_ASSIGNMENT;
    expression.tokens.push_back(*current);

    Expression assignmentTarget;
    assignmentTarget.type = ExpressionType::ASSIGNMENT_TARGET;
    ensureTokenType(TokenType::name, *leftAccumulationStart, "Expected a name here as the target of an assignment");
    assignmentTarget.tokens = GetIdentifierTokens(leftAccumulationStart, end);
    leftAccumulationStart++;

    if (leftAccumulationStart != current) {
        // we can't have more than one expression on the left hand side of an assignment
        throw UnexpectedTokenException(*leftAccumulationStart, "illegal assignment expression");
    }
    ensureNoEOF(current + 1, end);
    current++;

    auto rightHandSide = GetFactorExpression(current, end);

    expression.children.push_back(std::move(assignmentTarget));
    expression.children.push_back(std::move(rightHandSide));
    return expression;
}

Expression GetCardDeclarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Expression cardExpr;
    cardExpr.tokens.push_back(*current);

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected name of Card declared here");
    Expression cardNameDeclaration = Expression(ExpressionType::CARD_NAME_DECLARATION, { *current });
    ensureNoEOF(++current, end);

    if (current->text != "start") {
        ensureTokenType(TokenType::name, *current, "Expected start or card parent name here");
        cardNameDeclaration.tokens.push_back(*current);
        ensureNoEOF(++current, end);
    }

    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected a 'start' here");
    ensureNoEOF(++current, end);


    cardExpr.type = ExpressionType::CARD_DECLARATION;
    cardExpr.children = GetBlock(current, end);
    cardExpr.children.push_back(std::move(cardNameDeclaration));

    return cardExpr;
}

Expression GetSetupExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Expression expr;
    expr.tokens.push_back(*current);

    ensureTokenTypeAndText(TokenType::name, "setup", *current, "you can't start a setup expression with this");
    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "expected 'start' here");
    ensureNoEOF(++current, end);


    expr.type = ExpressionType::SETUP_DECLARATION;
    expr.children = GetBlock(current, end);

    return expr;
}

Expression GetForEachPlayerExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    ensureTokenTypeAndText(TokenType::name, "foreachplayer", *current, "foreachplayer expected for this kind of loop");

    Expression expr(ExpressionType::FOREACHPLAYER_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "expected each player identifier here");

    Expression identExpr(ExpressionType::FOREACHPLAYER_IDENTIFIER_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "expected 'start' here");
    ensureNoEOF(++current, end);


    expr.children = GetBlock(current, end);
    expr.children.push_back(identExpr);

    return expr;
}

Expression GetStackMoveSourceExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    ensureTokenType(TokenType::name, *current, "this is not a valid left side move expression");

    Expression expr;

    if (current->text == "random") {
        ensureNoEOF(++current, end);
        expr.tokens = GetIdentifierTokens(current, end);;
        expr.type = ExpressionType::STACK_MOVE_RANDOM_SOURCE;
        return expr;
    }

    expr.type = ExpressionType::STACK_MOVE_SOURCE;
    expr.tokens = GetIdentifierTokens(current, end);
    return expr;
}

Expression GetStackMoveTargetExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    ensureTokenType(TokenType::name, *current, "this is not a valid left side move expression");

    Expression expr;

    auto stackIdentifierTokens = GetIdentifierTokens(current, end);
    Expression stackIdentifierExpr(ExpressionType::IDENTIFIER, std::move(stackIdentifierTokens));

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected one of 'choose', 'top' or 'bottom' here");

    if (current->text == "choose") {
        expr.type = ExpressionType::STACK_SOURCE_CHOOSE;
    }
    else if (current->text == "top") {
        expr.type = ExpressionType::STACK_SOURCE_TOP;
    }
    else if (current->text == "bottom") {
        expr.type = ExpressionType::STACK_SOURCE_BOTTOM;
    }
    else {
        throw UnexpectedTokenException(*current, "Expected one of 'choose', 'top' or 'bottom' here");
    }
    expr.tokens.push_back(*current);

    ensureNoEOF(++current, end);

    auto factorExpression = GetFactorExpression(current, end);

    expr.children.push_back(std::move(stackIdentifierExpr));
    expr.children.push_back(std::move(factorExpression));

    return expr;
}

Expression GetMoveExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end, std::vector<Token>::iterator& leftAccumulationStart, ExpressionType moveType) {
    if (current->type != TokenType::move && current->type != TokenType::move_under) {
        throw UnexpectedTokenException(*current, "move expressions must contain a move operator '->' or '->_'");
    }

    Expression moveExpr(moveType, { *current });
    auto leftExpr = GetStackMoveSourceExpression(leftAccumulationStart, end);
    ensureNoEOF(++leftAccumulationStart, end);
    if (leftAccumulationStart != current) {
        throw UnexpectedTokenException(*leftAccumulationStart, "You can't have more than one expression on the left hand side of a move");
    }
    ensureNoEOF(++current, end);
    auto rightExpr = GetStackMoveTargetExpression(current, end);


    moveExpr.children.push_back(std::move(leftExpr));
    moveExpr.children.push_back(std::move(rightExpr));

    return moveExpr;
}

Expression GetPhaseDeclarationExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected name of phase declared here");

    Expression expr(ExpressionType::PHASE_DECLARATION, { *current });
    ensureNoEOF(++current, end);

    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected 'start' here");
    ensureNoEOF(++current, end);

    expr.children = GetBlock(current, end);

    return expr;
}

Expression GetOnPlaceExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Expression expr(ExpressionType::ONPLACE_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected name of target stack here");

    auto stackIdentifierTokens = GetIdentifierTokens(current, end);

    auto stackNameDeclaration = Expression(ExpressionType::ONPLACE_STACK_DECLARATION, stackIdentifierTokens);

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected name of card identifier here");

    auto cardIdentifier = Expression(ExpressionType::ONPLACE_IDENTIFIER_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected 'start' here");

    ensureNoEOF(++current, end);
    expr.children = GetBlock(current, end);
    expr.children.push_back(std::move(stackNameDeclaration));
    expr.children.push_back(std::move(cardIdentifier));

    return expr;
}

Expression GetIfExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {

    Expression expr(ExpressionType::IF_DECLARATION, { *current });

    ensureNoEOF(++current, end);

    if (current->text == "start") {
        throw UnexpectedTokenException(*current, "expected boolean expression here");
    }

    auto booleanExpression = GetFactorExpression(current, end);
    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected 'start' here");

    ensureNoEOF(++current, end);
    expr.children = GetBlock(current, end);
    expr.children.push_back(std::move(booleanExpression));

    return expr;
}

Expression GetWinnerIsExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {

    Expression expr(ExpressionType::WINER_DECLARATION, { *current });
    ensureNoEOF(++current, end);

    expr.tokens = GetIdentifierTokens(current, end);

    return expr;
}

Expression GetTurnExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Expression expr(ExpressionType::TURN_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenTypeAndText(TokenType::name, "start", *current, "Expected 'start' here");

    ensureNoEOF(++current, end);
    expr.children = GetBlock(current, end);

    return expr;
}

Expression GetDoExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Expression expr(ExpressionType::DO_DECLARATION, { *current });

    ensureNoEOF(++current, end);
    ensureTokenType(TokenType::name, *current, "Expected phase name here");

    expr.tokens.push_back(*current);

    return expr;
}

Expression GetPlayersExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
    Expression expr(ExpressionType::PLAYERS_DECLARATION, { *current });

    ensureNoEOF(++current, end);

    expr.children.push_back(GetFactorExpression(current, end));

    return expr;
}

Expression GetExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {

    Expression expression;

    expression.tokens.push_back(*current);

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
            else if (current->text == "phase") {
                return GetPhaseDeclarationExpression(current, end);
            }
            else if (current->text == "foreachplayer") {
                return GetForEachPlayerExpression(current, end);
            }
            else if (current->text == "onplace") {
                return GetOnPlaceExpression(current, end);
            }
            else if (current->text == "if") {
                return GetIfExpression(current, end);
            }
            else if (current->text == "winneris") {
                return GetWinnerIsExpression(current, end);
            }
            else if (current->text == "turn") {
                return GetTurnExpression(current, end);
            }
            else if (current->text == "do") {
                return GetDoExpression(current, end);
            }
            else if (current->text == "players") {
                return GetPlayersExpression(current, end);
            }
            // these keywords are part of larger expressions and should be accumulated before evaluation
            // (this allows them not to be immediately evauated as an attribute declaration, even if `random StacName ...` looks like one)
            else if (current->text == "random") {}
            else if ((current + 1) != end && (current + 1)->type == TokenType::name) {
                return GetAttrDeclarationExpression(current, end);
            }
        }

        else if (current->type == TokenType::assignment) {
            return GetAssignmentExpression(current, end, leftAccumulationStart);
        }
        else if (current->type == TokenType::move) {
            return GetMoveExpression(current, end, leftAccumulationStart, ExpressionType::STACK_MOVE);
        }
        else if (current->type == TokenType::move_under) {
            return GetMoveExpression(current, end, leftAccumulationStart, ExpressionType::STACK_MOVE_UNDER);
        }

        ensureNoEOF(++current, end);
    }

    return expression;
}