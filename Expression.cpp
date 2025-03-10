#include "expression.h"

namespace Battler {

    ExpressionType GetExpressionTypeFromOperatorTokenType(TokenType type) {
        switch (type) {
        case TokenType::plus: return ExpressionType::ADDITION;
        case TokenType::minus: return ExpressionType::SUBTRACTION;
        case TokenType::times: return ExpressionType::MULTIPLICATION;
        case TokenType::divide: return ExpressionType::DIVISION;
        case TokenType::equality: return ExpressionType::EQUALITY_TEST;
        case TokenType::lessthan: return ExpressionType::LESSTHAN_TEST;
        case TokenType::greaterthan: return ExpressionType::GREATHERTHAN_TEST;
        default: return ExpressionType::UNKNOWN;
        }
    }

    std::vector<Token> GetIdentifierTokensFromCommaSeperatedList(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end)
    {
        std::vector<Token> tokens;

        while (current != end) {

            ensureTokenType(TokenType::name, *current, "identifiers must be names");
            tokens.push_back(*current);

            if ((current + 1) != end && (current + 1)->type == TokenType::dot)
            {
                current += 2;
            }
            else if ((current + 1) != end && (current + 1)->type == TokenType::comma)
            {
                current += 1;
                tokens.push_back(*current);
                current += 1;
            }
            else
            {
                return tokens;
            }
        }
        throw UnexpectedTokenException(*(current - 1), "Unexpected end to token stream");
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
            if (current->text == "end" || current->text == "elseif" || current->text == "else") {
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

        Expression leftFactor;

        if (current->type == openbr) {
            auto i = current;
            while (i != end) {
                if (i->type == closebr) {
                    break;
                }
                i++;
            }

            if (i == end) {
                throw UnexpectedTokenException(*end, "expected a closing bracket to the bracketed expression");
            }

            auto brackedtedExpressionCurrent = current + 1;
            auto bracketedExpressionEnd = i;
            Expression brackedtedExpression = GetFactorExpression(brackedtedExpressionCurrent, bracketedExpressionEnd);
            current = bracketedExpressionEnd;

            if (current + 1 != end && (current + 1)->type == dot) {
                current += 2;
                std::vector<Token> identifierTokens = GetIdentifierTokens(current, end);
                leftFactor.type = ExpressionType::RESOLVED_IDENTIFIER_ATTRIBUTE_ACCESS;
                leftFactor.children.push_back(brackedtedExpression);
                leftFactor.tokens = identifierTokens;
            }
            else {
                leftFactor = brackedtedExpression;
            }

        } else if (current->type == TokenType::number || current->type == TokenType::name) {

            std::vector<Token> identifierTokens;
            if (current->type == TokenType::name) {
                identifierTokens = GetIdentifierTokens(current, end);
            }
            else {
                identifierTokens = { *current };
            }
            leftFactor = Expression(ExpressionType::FACTOR, identifierTokens);

        } else if (current->type == TokenType::open_sq_br) {
            // expect card sequence here

            leftFactor = Expression(ExpressionType::CARD_SEQUENCE, {*current});

            ensureNoEOF(++current, end);

            if (current->type != TokenType::close_sq_br && current->type != TokenType::name) {
                throw UnexpectedTokenException(*(current +1), "expecting a valid card squence here, but didn't get one");
            }

            while (current != end && current->type != TokenType::close_sq_br) {
                ensureTokenType(TokenType::name, *current, "expected a card here");
                auto cardIdentifierTokens = GetIdentifierTokens(current, end);
                leftFactor.children.push_back(Expression(ExpressionType::FACTOR, cardIdentifierTokens));
                current ++;
            }

            ensureTokenType(TokenType::close_sq_br, *current, "Expected an end to to the sequence ']' here");
            current++;

        }
        else {
            throw UnexpectedTokenException(*current, "expected a value, var name or a bracketed expression here");
        }

        if (current + 1 != end && IsOperatorType((current + 1)->type)) {
            current++;

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
            expression = leftFactor;
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

        expr.type = ExpressionType::STACK_MOVE_SOURCE;

        if (current->text == "random")
        {
            ensureNoEOF(++current, end);
            expr.type = ExpressionType::STACK_MOVE_RANDOM_SOURCE;
        }
        
        else if  (current->text == "choose")
        {
            ensureNoEOF(++current, end);
            expr.type = ExpressionType::STACK_MOVE_CHOOSE_SOURCE;
        }

        else if (current->text == "place")
        {
            ensureNoEOF(++current, end);
            expr.type = ExpressionType::STACK_MOVE_SPECIFIC_CARD_SOURCE;
        }

        //expr.tokens = GetIdentifierTokens(current, end);
        
        std::vector<Token> sourceIdentifers = GetIdentifierTokensFromCommaSeperatedList(current, end);

        bool hasMultipleIdentifiers = false;

        for (Token t : sourceIdentifers)
        {
            if (t.type == TokenType::comma)
            {
                hasMultipleIdentifiers = true;
                break;
            }
        }

        if (hasMultipleIdentifiers)
        {
            if (expr.type == ExpressionType::STACK_MOVE_SOURCE)
            {
                expr.type = ExpressionType::STACK_MOVE_SOURCE_MULTI;
            }
            else if (expr.type == ExpressionType::STACK_MOVE_CHOOSE_SOURCE)
            {
                expr.type = ExpressionType::STACK_MOVE_CHOOSE_SOURCE_MULTI;
            }
            else if (expr.type == ExpressionType::STACK_MOVE_RANDOM_SOURCE)
            {
                throw UnexpectedTokenException(sourceIdentifers[0], "random moves do not support multiple source identifiers");
            }
        }

        expr.tokens = sourceIdentifers;

        return expr;
    }

    Expression GetStackMoveTargetExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end, bool requirePosition/*=true*/, bool requireAmount/*=true*/) {
        ensureTokenType(TokenType::name, *current, "this is not a valid left side move expression");

        Expression expr;

//        auto stackIdentifierTokens = GetIdentifierTokens(current, end);
        std::vector<Token> targetIdentifierTokens = GetIdentifierTokensFromCommaSeperatedList(current, end);

        bool hasMultipleIdentifiers = false;

        for (Token t : targetIdentifierTokens)
        {
            if (t.type == TokenType::comma)
            {
                hasMultipleIdentifiers = true;
                break;
            }
        }
        Expression stackIdentifierExpr(ExpressionType::IDENTIFIER, std::move(targetIdentifierTokens));

        if (requirePosition)
        {
            ensureNoEOF(++current, end);
            ensureTokenType(TokenType::name, *current, "Expected one of 'top' or 'bottom' here");

            if (current->text == "top") {
                expr.type = ExpressionType::STACK_SOURCE_TOP;
            }
            else if (current->text == "bottom") {
                expr.type = ExpressionType::STACK_SOURCE_BOTTOM;
            }
            else {
                throw UnexpectedTokenException(*current, "Expected one of 'top' or 'bottom' here");
            }
            expr.tokens.push_back(*current);
        }
        else {
            expr.type = ExpressionType::STACK_SOURCE_TOP;
        }

        if (hasMultipleIdentifiers)
        {
            if (expr.type == ExpressionType::STACK_SOURCE_TOP)
            {
                expr.type = ExpressionType::STACK_SOURCE_TOP_DESTINATION_MULTI;
            }
            else if (expr.type == ExpressionType::STACK_SOURCE_BOTTOM)
            {
                expr.type = ExpressionType::STACK_SOURCE_BOTTOM_DESTINATION_MULTI;
            }
        }

        expr.children.push_back(std::move(stackIdentifierExpr));


        if (requireAmount)
        {
            ensureNoEOF(++current, end);
            auto factorExpression = GetFactorExpression(current, end);
            expr.children.push_back(std::move(factorExpression));
        }

        return expr;
    }

    Expression GetTransferExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end, std::vector<Token>::iterator& leftAccumulationStart) {
        if (current->type != TokenType::move
            && current->type != TokenType::move_under
            && current->type != TokenType::cut
            && current->type != TokenType::cut_under) {

            throw UnexpectedTokenException(*current, "move expressions must contain a move operator '->' or '->_'");
        }

        Expression expr;
        expr.type = ExpressionType::STACK_TRANSFER;
        Expression sourceStackExpr, operationExpr, targetStackExpr;

        sourceStackExpr = GetStackMoveSourceExpression(leftAccumulationStart, end);
        ensureNoEOF(++leftAccumulationStart, end);
        if (leftAccumulationStart != current) {
            throw UnexpectedTokenException(*leftAccumulationStart, "You can't have more than one expression on the left hand side of a move");
        }

        if (current->type == TokenType::move)
        {
            operationExpr.type = ExpressionType::STACK_MOVE;
        }
        else if(current->type == TokenType::move_under)
        {
            operationExpr.type = ExpressionType::STACK_MOVE_UNDER;
        }
        else if (current->type == TokenType::cut)
        {
            operationExpr.type = ExpressionType::STACK_CUT;
        }
        else if (current->type == TokenType::cut_under)
        {
            operationExpr.type = ExpressionType::STACK_CUT_UNDER;
        }
        ensureNoEOF(++current, end);

        bool requireAmount = true;
        bool requirePosition = true;

        if (sourceStackExpr.type == ExpressionType::STACK_MOVE_CHOOSE_SOURCE
        && (operationExpr.type == ExpressionType::STACK_CUT_UNDER
            || operationExpr.type == ExpressionType::STACK_CUT))
        {
            requireAmount = false;
        }

        if (sourceStackExpr.type == ExpressionType::STACK_MOVE_RANDOM_SOURCE
            || sourceStackExpr.type == ExpressionType::STACK_MOVE_SPECIFIC_CARD_SOURCE
            || sourceStackExpr.type == ExpressionType::STACK_MOVE_CHOOSE_SOURCE)
        {
            requirePosition = false;
        }

        targetStackExpr = GetStackMoveTargetExpression(current, end, requirePosition, requireAmount);

        expr.children.push_back(sourceStackExpr);
        expr.children.push_back(operationExpr);
        expr.children.push_back(targetStackExpr);

        return expr;
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
        expr.children.push_back(booleanExpression);
        auto blockExprs = GetBlock(current, end);
        expr.children.insert(expr.children.end(), blockExprs.begin(), blockExprs.end());

        while (current->text == "elseif") {
            Expression elseIfExpr(ExpressionType::ELSEIF_DECLARATION, {*current});
            ensureNoEOF(++current, end);
            auto elseIfGuard = GetFactorExpression(current, end);
            ensureNoEOF(++current, end);
            ensureTokenTypeAndText(TokenType::name, "then", *current, "Expected 'then' here");
            ensureNoEOF(++current, end);
            auto elseIfBlockExprs = GetBlock(current, end);
            elseIfExpr.children.push_back(elseIfGuard);
            elseIfExpr.children.insert(elseIfExpr.children.end(), elseIfBlockExprs.begin(), elseIfBlockExprs.end());
            expr.children.push_back(elseIfExpr);
        }

        if (current->text == "else") {
            Expression elseExpr(ExpressionType::ELSE_DECLARATION, {*current});
            ensureNoEOF(++current, end);
            auto elseBlockExprs = GetBlock(current, end);
            elseExpr.children = elseBlockExprs;
            expr.children.push_back(elseExpr);
        }

        return expr;
    }

    Expression GetWinnerIsExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {

        Expression expr(ExpressionType::WINNER_DECLARATION, { *current });
        ensureNoEOF(++current, end);

        expr.tokens = GetIdentifierTokens(current, end);

        return expr;
    }

    Expression GetLooserIsExpression(std::vector<Token>::iterator& current, const std::vector<Token>::iterator end) {
        Expression expr(ExpressionType::LOOSER_DECLARATION, { *current });
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
        Expression expr(ExpressionType::PLAYERS_DECLARATION, {*current});

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
                else if (current->text == "looseris") {
                    return GetLooserIsExpression(current, end);
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
                else if (current->text == "choose") {}
                else if (current->text == "place") {}
                else if ((current + 1) != end && (current + 1)->type == TokenType::name) {
                    return GetAttrDeclarationExpression(current, end);
                }
            }

            else if (current->type == TokenType::assignment) {
                return GetAssignmentExpression(current, end, leftAccumulationStart);
            }
            else if (current->type == TokenType::move) {
                return GetTransferExpression(current, end, leftAccumulationStart);
            }
            else if (current->type == TokenType::move_under) {
                return GetTransferExpression(current, end, leftAccumulationStart);
            }
            else if (current->type == TokenType::cut) {
                return GetTransferExpression(current, end, leftAccumulationStart);
            }
            else if (current->type == TokenType::cut_under) {
                return GetTransferExpression(current, end, leftAccumulationStart);
            }

            ensureNoEOF(++current, end);
        }

        return expression;
    }

}
