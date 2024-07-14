#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "Parser.h"
#include "expression.h"
#include "vm/game.h"

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

    srand(time(NULL));

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

        std::cout << "Compiling game file" << std::endl;
        program.compile(expr);

        std::cout << "Loading Compiled Game" << std::endl;
        program.run(true);

        std::cout << "Running game setup" << std::endl;
        program.run_setup();
        
        while (program.game().winner == -1)
        {
            program.run_turn();
        }
        
        cout << "The winner is " << program.game().winner << endl;

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
    } catch (OperationError e) {
        std::cout << e.reason << endl;
        return 1;
    }
    catch (CompileError e) {
        std::cout << GetErrorString("compiler error", e.reason, e.t, lines) << endl;
    }

    return 0;
}

