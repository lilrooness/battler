#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "Battler.h"
#include "Parser.h"
#include "Interpreter.h"

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
                tokens.push_back(t);
            }
            
            start = resPair.second;
        }
    }

    BattlerGame game;
    try {

        game = LoadGameFromTokens(tokens);
    } catch (UnexpectedTokenException e) {

        std::cout << "Error: unexpected token on line " << e.t.l << std::endl;
        std::cout << lines[e.t.l-1] << std::endl;
        for(int i=0; i<e.t.c; i++) {

         std::cout << "_";
        }
        std::cout << "^" << std::endl << std::endl;
        std::cout << "Reason: " << e.reason << std::endl;

        return 1;
    }

    return 0;
}

