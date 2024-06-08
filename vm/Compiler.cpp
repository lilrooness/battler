#include "../Compiler.h"

#include "../expression.h"


vector<Opcode> Program::opcodes()
{
	return m_opcodes;
}

#define NAME_IS_LVALUE true
#define NAME_IS_RVALUE false
void Program::compile_name(vector<Token> tokens, bool lvalue)
{
	// we're dealing with just a name, no dot seperation
	if (tokens.size() == 1)
	{
		Opcode code;
		if (lvalue)
		{
			code.type = OpcodeType::L_VALUE;
		}
		else
		{
			code.type = OpcodeType::R_VALUE_REF;
		}
		
		DATA_IX_T string_index = m_strings.size();
		m_strings.push_back(tokens[0].text);
		code.data |= STRING_TC;
		code.data |= ((OPCODE_CONV_T)string_index << 32);
		m_opcodes.push_back(code);
	}
	// we're dealing with a dot seperated name
	else
	{
		for (auto token : tokens)
		{
			if (token.type == TokenType::name)
			{
				Opcode code;
				if (lvalue)
				{
					code.type = OpcodeType::L_VALUE_DOT_SEPERATED_REF_CHAIN;
				}
				else
				{
					code.type = OpcodeType::R_VALUE_DOT_SEPERATED_REF_CHAIN;
				}

				DATA_IX_T string_index = m_strings.size();
				m_strings.push_back(tokens[0].text);
				code.data |= STRING_TC;
				code.data |= ((OPCODE_CONV_T)string_index << 32);
				m_opcodes.push_back(code);
			}
		}
		Opcode endCode;
		endCode.type = OpcodeType::DOT_SEPERATED_REF_CHAIN_END;
		m_opcodes.push_back(endCode);
	}
}

void Program::factor_expression(Expression expr)
{
	if (expr.type == ExpressionType::FACTOR)
	{
		if (expr.tokens[0].type == TokenType::number)
		{
			Opcode code;
			code.type = OpcodeType::R_VALUE;
			DATA_IX_T int_index = m_ints.size();
			m_ints.push_back(stoi(expr.tokens[0].text));
			code.data |= INT_TC;
			code.data |= ((OPCODE_CONV_T)int_index << 32);
			m_opcodes.push_back(code);
		}
		else if (expr.tokens[0].type == TokenType::name && (expr.tokens[0].text == "true" || expr.tokens[0].text == "false"))
		{
			Opcode code;
			code.type = OpcodeType::R_VALUE;
			DATA_IX_T bool_index = m_bools.size();
			m_bools.push_back(expr.tokens[0].text == "true");
			code.data |= BOOL_TC;
			code.data |= ((OPCODE_CONV_T)bool_index << 32);
			m_opcodes.push_back(code);
		}
		else if (expr.tokens[0].type == TokenType::name)
		{
			compile_name(expr.tokens, NAME_IS_RVALUE);
		}

		return;
	}

	auto left_expr = expr.children[0];
	auto right_expr = expr.children[1];

	Opcode operation_opcode;

	if (expr.type == ExpressionType::ADDITION)
	{
		operation_opcode.type = OpcodeType::ADD;
	}
	else if (expr.type == ExpressionType::SUBTRACTION)
	{
		operation_opcode.type = OpcodeType::SUBTRACT;
	}
	else if (expr.type == ExpressionType::MULTIPLICATION)
	{
		operation_opcode.type = OpcodeType::MULTIPLY;
	}
	else if (expr.type == ExpressionType::DIVISION)
	{
		operation_opcode.type = OpcodeType::DIVIDE;
	}
	else if (expr.type == ExpressionType::EQUALITY_TEST)
	{
		operation_opcode.type = OpcodeType::COMPARE;
	}
	else
	{
		std::stringstream ss;
		ss << "Unsupported Operation: ";
		for (auto t : expr.tokens) {
			ss << t.text << " ";
		}
		throw CompileError(ss.str(), expr.tokens[0]);
	}

	m_opcodes.push_back(operation_opcode);
	factor_expression(left_expr);
	factor_expression(right_expr);
}

void Program::compile(Expression expr)
{
	if (expr.type == ExpressionType::GAME_DECLARATION)
	{
		auto nameDecl = expr.children.back();
		
		DATA_IX_T name_index = m_strings.size();
		this->m_strings.push_back(nameDecl.tokens[0].text);

		Opcode start;
		Opcode end;
		end.type = OpcodeType::BLK_END;
		start.type = OpcodeType::GAME_BLK_HEADER;
		start.data |= STRING_TC;
		start.data |= ((OPCODE_CONV_T)name_index << 32);

		m_opcodes.push_back(start);
		expr.children.pop_back();
		for (auto e : expr.children) {
			compile(e);
		}
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::SETUP_DECLARATION)
	{
		Opcode start;
		Opcode end;
		start.type = OpcodeType::SETUP_BLK_HEADER;
		end.type = OpcodeType::BLK_END;
		
		m_opcodes.push_back(start);
		m_setup_index = m_opcodes.size();
		for (auto e : expr.children)
		{
			compile(e);
		}
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::TURN_DECLARATION)
	{
		Opcode start;
		Opcode end;
		start.type = OpcodeType::TURN_BLK_HEADER;
		end.type = OpcodeType::BLK_END;

		m_opcodes.push_back(start);
		m_turn_index = m_opcodes.size();
		for (auto e : expr.children)
		{
			compile(e);
		}
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::PHASE_DECLARATION)
	{
		Opcode start;
		Opcode end;
		start.type = OpcodeType::PHASE_BLK_HEADER;
		end.type = OpcodeType::BLK_END;

		string phase_name = expr.tokens[0].text;
		DATA_IX_T phase_name_index = m_strings.size();
		m_strings.push_back(phase_name);
		start.data |= STRING_TC;
		start.data |= ((OPCODE_CONV_T)phase_name_index << 32);

		m_opcodes.push_back(start);
		m_phase_indexes[phase_name] = m_opcodes.size();
		for (auto e : expr.children)
		{
			compile(e);
		}
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::DO_DECLARATION)
	{
		Opcode code;
		code.type = OpcodeType::DO_DECL;
		
		string phase_name = expr.tokens[1].text;
		DATA_IX_T phase_name_index = m_strings.size();
		m_strings.push_back(phase_name);
		code.data |= STRING_TC;
		code.data |= ((OPCODE_CONV_T)phase_name_index << 32);

		m_opcodes.push_back(code);
	}
	else if (expr.type == ExpressionType::WINER_DECLARATION)
	{
		Opcode code;
		code.type = OpcodeType::WINNER_DECL;
		
		m_opcodes.push_back(code);
		compile_name(expr.tokens, NAME_IS_RVALUE);
	}
	else if (expr.type == ExpressionType::IF_DECLARATION)
	{
		auto booleanExpression = expr.children[1];
		auto block = expr.children[0];

		Opcode ifHeader;
		ifHeader.type = OpcodeType::IF_BLK_HEADER;
		Opcode end;
		end.type = OpcodeType::BLK_END;
		
		if (booleanExpression.type != ExpressionType::FACTOR && booleanExpression.type != ExpressionType::EQUALITY_TEST)
		{
			std::stringstream ss;
			ss << "Not a Boolean Expression: ";
			for (auto t : expr.tokens) {
				ss << t.text << " ";
			}
			throw CompileError(ss.str(), expr.tokens[0]);
		}

		m_opcodes.push_back(ifHeader);
		factor_expression(booleanExpression);
		compile(block);
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::FOREACHPLAYER_DECLARATION)
	{
		Expression identExpression = expr.children.back();
		expr.children.pop_back();

		string eachPlayerLoopVarName = identExpression.tokens[0].text;

		Opcode forEachHeaderCode;
		Opcode end;
		forEachHeaderCode.type = OpcodeType::FOREACHPLAYER_BLK_HEADER;
		end.type = OpcodeType::BLK_END;

		DATA_IX_T string_index = m_strings.size();
		m_strings.push_back(eachPlayerLoopVarName);
		forEachHeaderCode.data |= STRING_TC;
		forEachHeaderCode.data |= ((OPCODE_CONV_T)string_index << 32);

		m_opcodes.push_back(forEachHeaderCode);
		for (auto e : expr.children)
		{
			compile(e);
		}
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::STACK_MOVE || expr.type == ExpressionType::STACK_MOVE_UNDER)
	{
		auto sourceStackExpr = expr.children[0];
		auto targetStackExpr = expr.children[1];

		if (sourceStackExpr.type == ExpressionType::STACK_MOVE_RANDOM_SOURCE)
		{
			Opcode sourceOpcode;
			Opcode destinationOpcode;

			sourceOpcode.type = OpcodeType::STACK_SOURCE_RANDOM_CARD_TYPE;
			if (expr.type == ExpressionType::STACK_MOVE)
			{
				destinationOpcode.type = OpcodeType::STACK_DEST_TOP;
			}
			else
			{
				destinationOpcode.type = OpcodeType::STACK_DEST_BOTTOM;
			}

			m_opcodes.push_back(sourceOpcode);
			compile_name(sourceStackExpr.tokens, NAME_IS_LVALUE);
			m_opcodes.push_back(destinationOpcode);
			compile_name(targetStackExpr.children[0].tokens, NAME_IS_RVALUE);
		}
		else if (sourceStackExpr.type == ExpressionType::STACK_MOVE_SOURCE)
		{
			Opcode sourceOpcode;
			Opcode destOpcode;

			if (expr.type == ExpressionType::STACK_MOVE)
			{
				destOpcode.type = OpcodeType::STACK_DEST_TOP;
			}
			else
			{
				destOpcode.type = OpcodeType::STACK_DEST_BOTTOM;
			}

			if (targetStackExpr.type == ExpressionType::STACK_SOURCE_TOP)
			{
				sourceOpcode.type = OpcodeType::STACK_SOURCE_TOP;
			}
			else if (targetStackExpr.type == ExpressionType::STACK_SOURCE_BOTTOM)
			{
				sourceOpcode.type = OpcodeType::STACK_SOURCE_BOTTOM;
			}
			else if (targetStackExpr.type == ExpressionType::STACK_SOURCE_CHOOSE)
			{
				sourceOpcode.type = OpcodeType::STACK_SOURCE_CHOOSE;
			}

			m_opcodes.push_back(sourceOpcode);
			compile_name(sourceStackExpr.tokens, NAME_IS_LVALUE);
			m_opcodes.push_back(destOpcode);
			compile_name(targetStackExpr.children[0].tokens, NAME_IS_RVALUE);
		}
	}
	else if (expr.type == ExpressionType::ATTR_ASSIGNMENT)
	{
		auto rightHandSide = expr.children.back();
		auto assignmentTarget = *(expr.children.end() - 2);

		compile_name(assignmentTarget.tokens, NAME_IS_LVALUE);
		factor_expression(rightHandSide);
	}
	else if (expr.type == ExpressionType::PLAYERS_DECLARATION)
	{
		Opcode code;
		code.type = OpcodeType::PLAYERS_L_VALUE;
		m_opcodes.push_back(code);
		auto numPlayersExpression = expr.children[0];
		factor_expression(numPlayersExpression);
	}
	else if (expr.type == ExpressionType::CARD_DECLARATION)
	{
		auto nameTokens = expr.children.back().tokens;
		string name = nameTokens[0].text;
		DATA_IX_T name_index = m_strings.size();
		m_strings.push_back(name);

		Opcode code;
		code.type = OpcodeType::CARD_BLK_HEADER;
		code.data |= STRING_TC;
		code.data |= ((OPCODE_CONV_T)name_index << 32);
		m_opcodes.push_back(code);

		expr.children.pop_back();

		for (auto e : expr.children) {
			compile(e);
		}

		Opcode end;
		end.type = OpcodeType::BLK_END;
		m_opcodes.push_back(end);
	}
	else if (expr.type == ExpressionType::ATTR_DECLARATION)
	{
		auto nameExpression = expr.children.back();
		expr.children.pop_back();

		auto typeExpression = expr.children.back();
		expr.children.pop_back();
		
		Opcode attrDeclCode;
		attrDeclCode.type = OpcodeType::ATTR_DECL;
		
		Opcode typeCode;
		typeCode.type = OpcodeType::ATTR_DATA_TYPE;

		string type = typeExpression.tokens[0].text;
		if (type == "int")
		{
			typeCode.data |= INT_TC;
		}
		else if (type == "string")
		{
			typeCode.data |= STRING_TC;
		}
		else if (type == "bool")
		{
			typeCode.data |= BOOL_TC;
		}
		else if (type == "float")
		{
			typeCode.data |= FLOAT_TC;
		}
		else if (type == "visiblestack")
		{
			typeCode.data |= VISIBLE_STACK_TC;
		}
		else if (type == "hiddenstack")
		{
			typeCode.data |= HIDDEN_STACK_TC;
		}
		else if (type == "privatestack")
		{
			typeCode.data |= PRIVATE_STACK_TC;
		}
		else if (type == "card")
		{
			typeCode.data |= CARD_TC;
		}

		m_opcodes.push_back(attrDeclCode);
		compile_name(nameExpression.tokens, NAME_IS_LVALUE);
		m_opcodes.push_back(typeCode);
	}
	else
	{
		std::stringstream ss;
		ss << "Unsupported Expression: ";
		for (auto t : expr.tokens) {
			ss << t.text << " ";
		}
		throw CompileError(ss.str(), expr.tokens[0]);
	}
}
