#include "../Compiler.h"

#include "../expression.h"

void Program::compile(Expression expr)
{
	if (expr.type == ExpressionType::GAME_DECLARATION)
	{
		auto nameDecl = expr.children.back();
		
		DATA_IX_T name_index = m_strings.size();
		this->m_strings.push_back(nameDecl.tokens[0].text);

		Opcode code;
		code.type = OpcodeType::GAME_BLK_HEADER;
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

		string name = nameExpression.tokens[0].text;
		DATA_IX_T name_index = m_strings.size();
		m_strings.push_back(name);
		string type = typeExpression.tokens[0].text;

		Opcode code;
		code.type = OpcodeType::ATTR_DECL;
		code.data |= STRING_TC;
		code.data |= ((OPCODE_CONV_T)name_index << 32);
		m_opcodes.push_back(code);

		Opcode typeCode;
		code.type = OpcodeType::ATTR_DATA_TYPE;
				
		if (type == "int")
		{
			code.data |= INT_TC;
		}
		else if (type == "string")
		{
			code.data |= STRING_TC;
		}
		else if (type == "bool")
		{
			code.data |= BOOL_TC;
		}
		else if (type == "float")
		{
			code.data |= FLOAT_TC;
		}
		else if (type == "visiblestack")
		{
			code.data |= VISIBLE_STACK_TC;
		}
		else if (type == "hiddenstack")
		{
			code.data |= HIDDEN_STACK_TC;
		}
		else if (type == "privatestack")
		{
			code.data |= PRIVATE_STACK_TC;
		}
		else if (type == "card")
		{
			code.data |= CARD_TC;
		}

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
