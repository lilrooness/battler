#pragma once

#include <vector>
#include <string>
#include <utility>
#include <cctype>
#include <cassert>

namespace Battler {
	enum class TokenType {
		assignment,
		equality,
		plus,
		minus,
		times,
		divide,
		lessthan,
		morethan,
		name,
		number,
		space,
		openbr,
		closebr,
		dquote,
		unknown,
		comma,
		dot,
		move,
		move_under,
		comment,
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