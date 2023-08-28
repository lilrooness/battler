#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "Parser.h"
#include "expression.h"
#include "vm/game.h"

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

            if (t.type != TokenType::space) {
                
                t.l = lines.size();
                t.c = std::distance(line.begin(), start);
                tokens.push_back(std::move(t));
            }
            
            start = resPair.second;
        }
    }

    try {

        Expression expr = GetExpression(tokens.begin(), tokens.end());
        TestEvaluateExpression(expr);

    } catch (UnexpectedTokenException e) {

        std::cout << "Error: unexpected token on line " << e.t.l << std::endl;
        std::cout << lines[e.t.l-1] << std::endl;
        for(int i=0; i<e.t.c; i++) {

         std::cout << "_";
        }
        std::cout << "^" << std::endl << std::endl;
        std::cout << "Reason: " << e.reason << std::endl;

        return 1;
    } catch (NameRedeclaredException e) {
        std::cout << "Error: redeclaration of name: " << e.name << std::endl;
        return 1;
    } catch (NoNameException e) {
        std::cout << "Error: " << e.name << " is not declared" << std::endl;
        return 1;
    }

    return 0;
}

