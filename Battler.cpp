#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "Parser.h"
#include "expression.h"
#include "vm/game.h"
#include "vm/run.h"

#include "Compiler.h"

void TestEvaluateExpression(Expression &expr) {
    using std::cout;
    using std::endl;

    for (auto t : expr.tokens) {
        cout << t.text << " ";
    }
    cout << endl;

    for (auto child : expr.children) {
        TestEvaluateExpression(child);
    }
}

std::string GetErrorString(std::string errorTypeString, std::string reason, Token t, vector<string> lines) {
    std::stringstream ss;
    ss << "Error: " << errorTypeString << " on line " << t.l << std::endl;
    ss << lines[t.l-1] << std::endl;
    for(int i=0; i<t.c; i++) {

        ss << "_";
    }
    ss << "^" << std::endl << std::endl;
    ss << "Reason: " << reason << std::endl;

    return ss.str();
}

int main(int argc, char* argv[]) {

    if (argc < 2) {

        std::cout<< "Please call like this: \"battler.exe path/to/main/game/file.battler\"" << std::endl;
        return 1;
    }

    std::ifstream is;
    is.open(argv[1]);

    if (!is.is_open()) {

        std::cout << "Could not find file " << argv[1] << std::endl;
        return 1;
    }

    std::vector<Token> tokens;
    std::vector<std::string> lines;


    std::string line;
    while(std::getline(is, line)) {

        lines.push_back(line);
        auto start = line.begin();
        auto end = line.end();
        
        while (start != end) {

            auto resPair = getNextToken(start, end);
            Token t = resPair.first;

            if (t.type == TokenType::comment) {
                break;
            }

            if (t.type != TokenType::space) {
                
                t.l = lines.size();
                t.c = std::distance(line.begin(), start);
                tokens.push_back(std::move(t));
            }
            
            start = resPair.second;
        }
    }

    Program program;
    try {

        Expression expr = GetExpression(tokens.begin(), tokens.end());

        //Game game;
        //vector<AttrCont> localeStack {};
        //std::cout << "Initalizing . . ." << std::endl;
        //RunExpression(expr, game, ExpressionType::UNKNOWN, localeStack);

        //std::cout << "running " << game.name <<  " setup block" << std::endl;
        //for (int i = 0; i < game.setup.children.size(); i++)
        //{
        //    RunExpression(game.setup.children[i], game, ExpressionType::UNKNOWN, localeStack);
        //}
        //game.Print();

        //std::cout << "running turn" << std::endl;
        //auto currentPlayerAttr = Attr{ AttributeType::PLAYER_REF };
        //currentPlayerAttr.playerRef = 0;
        //auto turnAttrs =  AttrCont();
        //turnAttrs.Store("currentPlayer", currentPlayerAttr);
        //localeStack.push_back(turnAttrs);

        //int turn = 0;
        //while (game.winner < 0) {
        //    localeStack.back().Get("currentPlayer").playerRef = turn % game.players.size();
        //    cout << "turn " << turn++ << " player " << localeStack.back().Get("currentPlayer").playerRef << endl;
        //    for (int i = 0; i < game.turn.children.size(); i++)
        //    {
        //        RunExpression(game.turn.children[i], game, ExpressionType::UNKNOWN, localeStack);
        //    }
        //}

        //cout << "the winner is player " << game.winner << std::endl;

        //localeStack.pop_back();


        std::cout << "Compiling game file" << std::endl;
        program.compile(expr);

        std::cout << "Loading Compiled Game" << std::endl;
        program.run(true);

        std::cout << "Running game setup" << std::endl;
        program.run_setup();

        std::cout << "Running ONE turn" << std::endl;
        AttrCont currentPlayerAttrCont;
        Attr currentPlayerAttr;
        currentPlayerAttr.type = AttributeType::PLAYER_REF;
        currentPlayerAttr.playerRef = program.game().currentPlayerIndex;
        currentPlayerAttrCont.Store("currentPlayer", currentPlayerAttr);

        program.locale_stack().push_back(currentPlayerAttrCont);
        program.run_turn();
        program.locale_stack().pop_back();


    } catch (VMError e) {
        std::cout << "VM Error" << e.reason << endl;
        return 1;
    } catch (UnexpectedTokenException e) {
        std::cout << GetErrorString("unexpected token", e.reason, e.t, lines) << endl;
        return 1;
    } catch (NameRedeclaredException e) {
        std::cout << "Error: redeclaration of name: " << e.name << std::endl;
        return 1;
    } catch (NoNameException e) {
        std::cout << "Error: " << e.name << " is not declared" << std::endl;
        return 1;
    } catch (RuntimeError e) {
        std::cout << GetErrorString("runtime error", e.reason, e.t, lines) << endl;
        return 1;
    } catch (OperationError e) {
        std::cout << e.reason << endl;
        return 1;
    }
    catch (CompileError e) {
        std::cout << GetErrorString("compiler error", e.reason, e.t, lines) << endl;
    }

    return 0;
}

