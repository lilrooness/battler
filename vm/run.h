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

#include <cstdlib>

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

void StoreValue(string name, string value, AttributeType type, AttrCont& container) {
    if (type == AttributeType::INT) {
        Attr a;
        a.i = std::stoi(value);
        a.type = AttributeType::INT;
        container.Store(name, a);
    }
    else if (type == AttributeType::BOOL) {
        Attr a;
        a.type = type;
        if (value == "true") {
            a.b = true;
            container.Store(name, a);
        } else if (value == "false") {
            a.b = false;
            container.Store(name, a);
        } else {
            throw WrongTypeException();
        }
    }
    else if (type == AttributeType::STRING) {
        Attr a;
        a.s = value;
        a.type = type;
        container.Store(name, a);
    }
    else if (type == AttributeType::FLOAT) {
        Attr a;
        a.f = std::stof(value);
        a.type = type;
        container.Store(name, a);
    }
}

Attr ResolveTokensToAttr(vector<Token>& tokens, Game& game, vector<AttrCont>& localeStack) {
    assert(tokens.size() > 0);

    if (tokens.size() == 1 && tokens[0].type == TokenType::number) {
        Attr res;
        res.i = std::stoi(tokens[0].text);
        res.type = AttributeType::INT;

        return res;
    }

    auto tokenItr = tokens.begin();

    string firstName = tokenItr->text;
    ++tokenItr;

    Attr current;
    bool found = false;


    for (int i = localeStack.size() - 1; !found && i >= 0; i--) {
        if (localeStack[i].Contains(firstName)) {
            current = localeStack[i].Get(firstName);
            found = true;
        }
    }

    if (!found && game.attributeCont.Contains(firstName)) {
        current = game.attributeCont.Get(firstName);
        found = true;
    }

    if (!found && game.cards.find(firstName) != game.cards.end()) {
        current.type = AttributeType::CARD_REF;
        current.cardRef = firstName;
        found = true;
    }

    if (!found) {
        std::stringstream ss;
        ss << "Could not find name: \""<< tokens[0].text << "\"";
        throw RuntimeError(ss.str(), tokens[0]);
    }

    while (tokenItr != tokens.end()) {
        if (tokenItr->type == TokenType::name) {
            if (current.type == AttributeType::CARD_REF) {
                if (game.cards[current.cardRef].attributes.Contains(tokenItr->text)) {
                    current = game.cards[current.cardRef].attributes.Get(tokenItr->text);
                } else {
                    throw RuntimeError("No attr found with this name on this card", *tokenItr);
                }
            }
            else if (current.type == AttributeType::STACK_REF) {
                if (tokenItr->text == "top") {
                    auto stack = game.stacks[current.stackRef];
                    auto stackRef = current.stackRef;
                    current = Attr();
                    current.type = AttributeType::STACK_POSITION_REF;
                    // the top of the stack is the index of the last card in the stack
                    current.stackPositionRef = { stackRef, stack.cards.size()-1 };
                } else if (tokenItr->text == "bottom") {
                    auto stackRef = current.stackRef;
                    current = Attr();
                    current.type = AttributeType::STACK_POSITION_REF;
                    // the bottom of the stack is always 0
                    current.stackPositionRef = { stackRef, 0 };
                } else if (game.stacks[current.stackRef].attributes.Contains(tokenItr->text)) {
                    current = game.stacks[current.stackRef].attributes.Get(tokenItr->text);
                } else {
                    throw RuntimeError("No attr found with this name on this stack", *tokenItr);
                }
            }
            else if (current.type == AttributeType::PLAYER_REF) {
                if (game.players[current.playerRef].attributes.Contains(tokenItr->text)) {
                    current = game.players[current.playerRef].attributes.Get(tokenItr->text);
                } else {
                    throw RuntimeError("No attr found with this name on this player", *tokenItr);
                }
            }
            else {
                throw RuntimeError("Not possible to select an attribute on this type", *tokenItr);
            }
        }

        ++tokenItr;
    }

    return current;
}

bool typeCanHaveAttrs(AttributeType type) {
    return 
       type == AttributeType::CARD_REF 
    || type == AttributeType::STACK_REF
    || type == AttributeType::PLAYER_REF
    || type == AttributeType::PHASE_REF;
}

AttrCont* GetGlobalObjectAttrContPtr(Game& game, AttrCont& cont, Token& nameToken) {
    if (!cont.Contains(nameToken.text)) {
        throw RuntimeError("this attribute does not exist ", nameToken);
    }

    if(cont.Get(nameToken.text).type == AttributeType::CARD_REF) {
        return &game.cards[cont.Get(nameToken.text).cardRef].attributes;
    }

    if(cont.Get(nameToken.text).type == AttributeType::PLAYER_REF) {
        return &game.players[cont.Get(nameToken.text).playerRef].attributes;
    }

    if(cont.Get(nameToken.text).type == AttributeType::PHASE_REF) {
        return &game.phases[cont.Get(nameToken.text).phaseRef].attributes;
    }

    if(cont.Get(nameToken.text).type == AttributeType::STACK_REF) {
        return &game.stacks[cont.Get(nameToken.text).stackRef].attributes;
    }

    throw RuntimeError("this type cannot have attributes ", nameToken);
}

AttrCont* GetObjectAttrContPtrFromIdentifier(vector<Token>::iterator tokenBegin, vector<Token>::iterator tokenEnd, Game& game, vector<AttrCont>& localeStack) {
    assert(tokenBegin != tokenEnd);

    auto tokenItr = tokenBegin;

    string firstName = tokenItr->text;

    AttrCont* current;
    bool found = false;

    for (int i=localeStack.size()-1; !found && i>=0; i--) {
        if (localeStack[i].Contains(firstName)) {
            current = GetGlobalObjectAttrContPtr(game, localeStack[i], *tokenBegin);
            found = true;
        }
    }

    if (!found && game.attributeCont.Contains(firstName)) {
        current = GetGlobalObjectAttrContPtr(game, game.attributeCont, *tokenBegin);
        found = true;
    }

    if (!found) {
        throw RuntimeError("Could not find attribute", *tokenBegin);
    }

    ++tokenItr;

    while (tokenItr != tokenEnd) {
        if (current->Contains(tokenItr->text)) {
            current = GetGlobalObjectAttrContPtr(game, *current, *tokenItr);
        } else {
            throw RuntimeError(tokenItr->text + " not found in " + firstName, *tokenItr);
        }

        ++tokenItr;
    }

    return current;
}

AttrCont* GetContainingAttrContPtrFromIdentifier(vector<Token>::iterator tokenBegin, vector<Token>::iterator tokenEnd, Game& game, vector<AttrCont>& localeStack) {
    assert(tokenBegin != tokenEnd);
    
    int numberOfTokens = std::distance(tokenBegin, tokenEnd);


    if (numberOfTokens == 1) {
        for (int i=localeStack.size()-1; i>=0; i--) {
            if (localeStack[i].Contains(tokenBegin->text)) {
                return &localeStack[i];
            }
        }

        if (game.attributeCont.Contains(tokenBegin->text)) {
            return &game.attributeCont;
        }

        throw RuntimeError(tokenBegin->text + "not found", *tokenBegin);
    }


    return GetObjectAttrContPtrFromIdentifier(tokenBegin, tokenEnd-1, game, localeStack);
}

Expression FactorExpression(Expression& expr, Game& game, vector<AttrCont>& stack) {

    if (expr.type == ExpressionType::FACTOR) {
        return expr;
    }

    auto leftSide = expr.children[0];
    auto rightSide = expr.children[1];

    Attr leftAttr = ResolveTokensToAttr(leftSide.tokens, game, stack);
    AttributeType leftSideType = leftAttr.type;
    
    auto rightFactor = FactorExpression(rightSide, game, stack);
    Attr rightAttr = ResolveTokensToAttr(rightFactor.tokens, game, stack);
    AttributeType rightSideType = rightAttr.type;
    
    switch (expr.type) {
        case ExpressionType::ADDITION:
            if (leftSideType == AttributeType::INT) {
                int left = leftAttr.i; //std::stoi(leftSideValue);
                int right = rightAttr.i; //std::stoi(rightSideValue);

                int result = left + right;

                Expression e;
                Token t;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot add these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::SUBTRACTION:
            if (leftSideType == AttributeType::INT) {
                int left = leftAttr.i;
                int right = rightAttr.i;

                int result = left - right;

                Expression e;
                Token t;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = { t };
                e.type = ExpressionType::FACTOR;
                return e;
            } else if (leftSideType == AttributeType::STACK_POSITION_REF) {
                int right = rightAttr.i;
                auto stackName = std::get<0>(leftAttr.stackPositionRef);
                auto stack = game.stacks[stackName];
                int stackPositionIndex = std::get<1>(leftAttr.stackPositionRef);
                Expression e;
                Token t;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                t.type = TokenType::name;
                t.text = stack.cards[stackPositionIndex - right].name;
                e.tokens = { t };
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot subtract these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::MULTIPLICATION:
            if (leftSideType == AttributeType::INT) {
                int left = leftAttr.i;
                int right = rightAttr.i;

                int result = left * right;

                Expression e;
                Token t;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot multiply these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::DIVISION:
            if (leftSideType == AttributeType::INT) {
                int left =  leftAttr.i;
                int right = rightAttr.i;

                int result = left / right;

                Expression e;
                Token t;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                t.text = std::to_string(result);
                t.type = TokenType::number;
                e.tokens = {t};
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("You cannot divide these types", expr.tokens[0]);
            }
            break;
        case ExpressionType::EQUALITY_TEST:
            if (leftAttr.type == AttributeType::STACK_POSITION_REF)
            {
                auto stack = game.stacks[std::get<0>(leftAttr.stackPositionRef)];
                leftAttr.type = AttributeType::CARD_REF;
                leftSideType = leftAttr.type;
                leftAttr.cardRef = stack.cards[std::get<1>(leftAttr.stackPositionRef)].name;
            }
            if (rightAttr.type == AttributeType::STACK_POSITION_REF)
            {
                auto stack = game.stacks[std::get<0>(rightAttr.stackPositionRef)];
                rightAttr.type = AttributeType::CARD_REF;
                rightSideType = rightAttr.type;
                rightAttr.cardRef = stack.cards[std::get<1>(rightAttr.stackPositionRef)].name;
            }
            if (leftSideType == AttributeType::INT && rightSideType == AttributeType::INT) {
                Expression e;
                Token t;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                t.type = TokenType::name;
                if (leftAttr.i == rightAttr.i) {
                    t.text = "true";
                } else {
                    t.text = "false";
                }


                e.tokens = { t };
                e.type = ExpressionType::FACTOR;
                return e;
            } else if (leftSideType == AttributeType::CARD_REF && rightSideType == AttributeType::CARD_REF) {
                Expression e;
                Token t;
                t.c = expr.tokens[0].c;
                t.l = expr.tokens[0].l;
                t.type = TokenType::name;
                if (leftAttr.cardRef == rightAttr.cardRef) {
                    t.text = "true";
                }
                else {
                    t.text = "false";
                }

                e.tokens = { t };
                e.type = ExpressionType::FACTOR;
                return e;
            } else {
                throw RuntimeError("these types cannot be tested for equality", expr.tokens[0]);
            }
            break;
        
    }
    throw RuntimeError("Unsupported Operation", expr.tokens[0]);
}

void RunExpression(Expression& expr, Game& game, ExpressionType parent, vector<AttrCont>& localeStack) {
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
            // push a copy of the parent attributes onto the stack
            localeStack.push_back(game.cards[c.parentName].attributes);
        } else {
            localeStack.push_back(AttrCont());
        }

        expr.children.pop_back();
        
        for (auto e : expr.children) {
            RunExpression(e, game, expr.type, localeStack);
        }

        // copy the new attributes into the card
        c.attributes = std::move(localeStack.back());
        localeStack.pop_back();

        game.cards[c.name] = std::move(c);
    }
    else if (expr.type == ExpressionType::ATTR_DECLARATION) {
        ensureExpressionType(expr.children.back().type, ExpressionType::ATTR_NAME_DECLARATION, "Expected to find an attr name decl here", expr.children.back().tokens[0]);

        AttrCont* attributeContainer;

        bool storeGlobalTypesInGameAttrs = true;

        if (expr.children.back().tokens.size() > 1) {
            // if the declaration is nestted.inside.a.declared.object with dot notation
            // we need to discern the storage location
            storeGlobalTypesInGameAttrs = false;
            attributeContainer = GetObjectAttrContPtrFromIdentifier(
                expr.children.back().tokens.begin(),
                expr.children.back().tokens.end()-1,
                game,
                localeStack
            );
        }
        else if (localeStack.empty()) {
            // if there is nothing in the locals stack, we can safely store the attribute at the game level
            attributeContainer = &game.attributeCont;
        } else {
            // store the attribute on the last pushed locale stack location
            attributeContainer = &localeStack.back();
        }

        string name = expr.children.back().tokens.back().text;
        string type = (expr.children.end()-2)->tokens[0].text;

        Attr a;
        bool isGlobalType = false;
        if (type == "int") {
            a.i = 0;
            a.type = AttributeType::INT;
        }
        else if (type == "bool") {
            a.b = false;
            a.type = AttributeType::BOOL;
        }
        else if (type == "string") {
            a.s = "";
            a.type = AttributeType::STRING;
        }
        else if (type == "float") {
            a.f = 0.0f;
            a.type = AttributeType::FLOAT;
        }
        else if (type == "visiblestack") {
            isGlobalType=true;
            Stack s;
            s.t = StackType::VISIBLE;
            a.type = AttributeType::STACK_REF;

            a.stackRef = game.stacks.size();
            game.stacks[game.stacks.size()] = std::move(s);
        }
        else if (type == "hiddenstack") {
            isGlobalType=true;
            Stack s;
            s.t = StackType::HIDDEN;
            a.type = AttributeType::STACK_REF;
            a.stackRef = game.stacks.size();
            game.stacks[game.stacks.size()] = std::move(s);
        }
        else if (type == "privatestack") {
            isGlobalType=true;
            Stack s;
            s.t = StackType::PRIVATE;
            a.type = AttributeType::STACK_REF;
            a.stackRef = game.stacks.size();
            game.stacks[game.stacks.size()] = std::move(s);
        }
        else {
            std::stringstream ss;
            ss << "unsupported Type " << type;
            throw RuntimeError(ss.str(), expr.tokens[0]);
        }

        if (isGlobalType && storeGlobalTypesInGameAttrs) {
            game.attributeCont.Store(name, std::move(a));
        } else {
            attributeContainer->Store(name, std::move(a));
        }
    }
    else if (expr.type == ExpressionType::ATTR_ASSIGNMENT) {
        auto rightHandSide = expr.children.back();
        auto assignmentTarget = *(expr.children.end()-2);
        
        Expression factor = FactorExpression(rightHandSide, game, localeStack);
        string value = factor.tokens[0].text;

        AttrCont *attrCont = GetContainingAttrContPtrFromIdentifier(
            assignmentTarget.tokens.begin(),
            assignmentTarget.tokens.end(),
            game,
            localeStack
        );
        
        string name = assignmentTarget.tokens.back().text;
        StoreValue(name, value, attrCont->Get(name).type, *attrCont);
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
        game.phases[phase.name] = phase;
    }
    else if (expr.type == ExpressionType::STACK_MOVE || expr.type == ExpressionType::STACK_MOVE_UNDER) {
        auto sourceStackExpr = expr.children[0];
        auto targetStackExpr = expr.children[1];

        Attr stackRef = ResolveTokensToAttr(targetStackExpr.children[0].tokens, game, localeStack);
        if (stackRef.type != AttributeType::STACK_REF) {
            throw RuntimeError("the target of a move must be a stack", *targetStackExpr.children[0].tokens.rbegin());
        }
        string targetName = stackRef.stackRef;

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
            Attr sourceStackRef = ResolveTokensToAttr(sourceStackExpr.tokens, game, localeStack);
            if (sourceStackRef.type != AttributeType::STACK_REF) {
                throw RuntimeError("the source of a move must be a stack", *sourceStackExpr.tokens.rbegin());
            }
            string sourceName = sourceStackRef.stackRef;
            
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

            // the target Expression type determines how we want to take cards from the source stack
            if (targetStackExpr.type == ExpressionType::STACK_SOURCE_TOP) {
                // take cards from the top of the source deck
                
                int limit = std::min(
                    static_cast<int>(source.cards.size()),
                    moveNumber
                );

                auto start = source.cards.end() - limit;
                auto end = source.cards.end();

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
                auto end = source.cards.begin() + (limit - 1);

                cardsTaken = vector<Card>(start, end);
                source.cards.erase(start, end);

                /**
                 * the code that takes the cards and puts them in the new stack
                 * assumes that the 'backmost' card is the one you
                 * would take first, whether from the bottom, or the top,
                 * so cardsTaken must be reversed in this case
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
	        vector<Card> randomPool;
	        string cardParentTypeName = sourceStackExpr.tokens[0].text;
	    
	        for (auto it: game.cards) {
	          if (it.second.parentName == cardParentTypeName) {
		    randomPool.push_back(it.second);
	          }
	        }

	        Stack& target = game.stacks[targetName];
	        vector<Card> cardsTaken;

	        for (int i=0; i<moveNumber; i++) {
	            cardsTaken.push_back(randomPool[rand()%randomPool.size()]);
	        }

	        if (expr.type == ExpressionType::STACK_MOVE) {
                std::reverse(cardsTaken.begin(), cardsTaken.end());
                target.cards.insert(target.cards.end(), cardsTaken.begin(), cardsTaken.end());
            }
            else if (expr.type == ExpressionType::STACK_MOVE_UNDER) {
                target.cards.insert(target.cards.begin(), cardsTaken.begin(), cardsTaken.end());
            }
        }
    }
    else if (expr.type == ExpressionType::PLAYERS_DECLARATION) {
        auto nPlayersExpression = FactorExpression(expr.children[0], game, localeStack);
        auto nPlayers = std::stoi(nPlayersExpression.tokens[0].text);

        for (int i=0; i<nPlayers; i++) {
            game.players.push_back(Player());
        }
    }
    else if (expr.type == ExpressionType::FOREACHPLAYER_DECLARATION) {
        Expression identExpression = expr.children.back();
        expr.children.pop_back();
        if (identExpression.tokens.size() != 1 || identExpression.tokens[0].type != TokenType::name) {
            throw RuntimeError("must specify a for current player in foreachplayer loop", expr.tokens[0]);
        }
        string identifier = identExpression.tokens[0].text;
        AttrCont forLoopAttrs;
        localeStack.push_back(std::move(forLoopAttrs));

        for (int i=0; i<game.players.size(); i++) {
            Attr playerRef;
            playerRef.playerRef = i;
            playerRef.type = AttributeType::PLAYER_REF;
            localeStack.back().Store(identifier, playerRef);
            for (auto child : expr.children) {
                RunExpression(child, game, ExpressionType::FOREACHPLAYER_DECLARATION, localeStack);
            }
        }
        localeStack.pop_back();
    }
    else if (expr.type == ExpressionType::DO_DECLARATION) {
        auto phaseName = expr.tokens[1].text;
        
        if (game.phases.find(phaseName) == game.phases.end())
        {
            throw RuntimeError("Unknown phase", expr.tokens[1]);
        }

        auto phase = game.phases[phaseName];
        
        for (int i = 0; i < phase.expression.children.size(); i++) {
            RunExpression(phase.expression.children[i], game, ExpressionType::DO_DECLARATION, localeStack);
        }
    }
    else if (expr.type == ExpressionType::ONPLACE_DECLARATION) {
        throw RuntimeError("onplace expressions are not yet implemented", expr.tokens[0]);
    }
    else if (expr.type == ExpressionType::WINER_DECLARATION) {
        Attr winnerAttr = ResolveTokensToAttr(expr.tokens, game, localeStack);
        if (winnerAttr.type != AttributeType::PLAYER_REF) {
            throw RuntimeError("Victory must be assigned with a player reference.", expr.tokens[0]);
        }

        game.winner = winnerAttr.playerRef;
    }
    else if (expr.type == ExpressionType::IF_DECLARATION) {
        auto booleanExpression = expr.children[1];
        auto block = expr.children[0];

        auto booleanResult = FactorExpression(booleanExpression, game, localeStack);
        bool result{ false };
        if (booleanResult.tokens[0].type == TokenType::name) {
            if (booleanResult.tokens[0].text == "true") {
                result = true;
            }
        }
        else if (booleanResult.tokens[0].type == TokenType::number) {
            int n = stoi(booleanResult.tokens[0].text);
            if (n) {
                result = true;
            }
        }
        else {
            throw RuntimeError("unsupported boolean expression", booleanExpression.tokens[0]);
        }

        if (result) {
            RunExpression(block, game, ExpressionType::IF_DECLARATION, localeStack);
        }

    } else {
        std::stringstream ss;
        ss << "Unsupported Expression: ";
        for (auto t : expr.tokens) {
            ss << t.text << " ";
        }
        throw RuntimeError(ss.str(), expr.tokens[0]);
    }
}
