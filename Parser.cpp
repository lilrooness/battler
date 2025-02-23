#include "Parser.h"

namespace Battler {

	bool IsOperatorType(TokenType type) {
		return
			type == TokenType::divide
			|| type == TokenType::plus
			|| type == TokenType::minus
			|| type == TokenType::times
			|| type == TokenType::equality;
	}


	std::pair<Token, std::string::iterator> getNextToken(
		std::string::iterator begin,
		std::string::iterator end
	) {

		assert(begin != end);

		Token t;

		if (*begin == ' ') {
			t.type = TokenType::space;
			for (; begin != end && *begin == ' '; begin++) {
				t.text.push_back(*begin);
			}
			return make_pair(t, begin);
		}
		else if (*begin == '#') {
			t.type = TokenType::comment;
			t.text = '#';
			return make_pair(t, begin);
		}
		else if (*begin == '=') {
			t.text.push_back('=');
			if (begin + 1 != end && *(begin + 1) == '=') {
				t.text.push_back('=');
				t.type = TokenType::equality;
				return make_pair(t, begin + 2);
			}
			t.type = TokenType::assignment;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '+') {
			t.text.push_back('+');
			t.type = TokenType::plus;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '-') {
			t.text.push_back('-');

			if (begin + 1 != end && *(begin + 1) == '>') {
				t.text.push_back('>');

				if (begin + 2 != end && *(begin + 2) == '_') {
					t.text.push_back('_');
					t.type = TokenType::move_under;
					return make_pair(t, begin + 3);
				}

				t.type = TokenType::move;
				return make_pair(t, begin + 2);
			}
			t.type = TokenType::minus;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '*') {
			t.text.push_back('*');
			t.type = TokenType::times;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '/') {
            t.text.push_back('/');

            if (begin + 1 != end && *(begin + 1) == '>') {
                t.text.push_back('>');

                if (begin + 2 != end && *(begin + 2) == '_') {
                    t.text.push_back('_');
                    t.type = TokenType::cut_under;
                    return make_pair(t, begin + 3);
                }
                
                t.type = TokenType::cut;
                return make_pair(t, begin + 2);
            }
			t.type = TokenType::divide;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '<') {
			t.text.push_back('<');
			t.type = TokenType::lessthan;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '>') {
			t.text.push_back('>');
			t.type = TokenType::morethan;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '(') {
			t.text.push_back('(');
			t.type = TokenType::openbr;
			return make_pair(t, begin + 1);
		}
		else if (*begin == ')') {
			t.text.push_back(')');
			t.type = TokenType::closebr;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '"') {
			t.text.push_back('"');
			t.type = TokenType::dquote;
			return make_pair(t, begin + 1);
		}
		else if (*begin == ',') {
			t.text.push_back(',');
			t.type = TokenType::comma;
			return make_pair(t, begin + 1);
		}
		else if (*begin == '.') {
			t.text.push_back('.');
			t.type = TokenType::dot;
			return make_pair(t, begin + 1);
		}
		// the catch all unknown chars case
		else if (!std::isalnum(*begin)) {
			t.text.push_back(*begin);
			t.type = TokenType::unknown;
			return make_pair(t, begin + 1);
		}

		if (std::isdigit(*begin)) {
			t.type = TokenType::number;
			for (; begin != end && std::isdigit(*begin); begin++) {
				t.text.push_back(*begin);
			}
			return make_pair(t, begin);
		}

		t.type = TokenType::name;
		for (; begin != end && (std::isalnum(*begin) || *begin == '_'); begin++) {
			t.text.push_back(*begin);
		}

		return make_pair(t, begin);
	}

}
