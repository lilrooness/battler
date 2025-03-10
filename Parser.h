#pragma once

#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>

namespace Battler {
	enum TokenType {
		assignment,
		equality,
		plus,
		minus,
		times,
		divide,
		lessthan,
		greaterthan,
		name,
		number,
		space,
		openbr,
		closebr,
		open_sq_br,
		close_sq_br,
		dquote,
		unknown,
		comma,
		dot,
		move,
		move_under,
        cut,
        cut_under,
		comment,
        underscore,
        colon,
	};


	struct Token {
		TokenType type;
		int l;
		int c;
		std::string text;
	};

	bool IsOperatorType(TokenType type);
	std::pair<Token, std::string::iterator> getNextToken(
		std::string::iterator begin,
		std::string::iterator end
	);

}
