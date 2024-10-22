#include "../Compiler.h"

#include "../expression.h"
#include "../interpreter_errors.h"

namespace Battler {

vector<Opcode> Program::opcodes()
{
	return m_opcodes;
}

void Program::Compile(vector<string> lines)
{
	for (std::string line : lines)
	{
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
				m_tokens.push_back(std::move(t));
			}

			start = resPair.second;
		}
	}

	m_rootExpression = GetExpression(m_tokens.begin(), m_tokens.end());
	compile_expression(m_rootExpression);
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
				m_strings.push_back(token.text);
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

void Program::compile_expression(Expression expr)
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
			compile_expression(e);
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
		m_setup_index = m_opcodes.size()-1;
		for (auto e : expr.children)
		{
			compile_expression(e);
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
		m_turn_index = m_opcodes.size()-1;
		for (auto e : expr.children)
		{
			compile_expression(e);
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
		m_phase_indexes[phase_name] = m_opcodes.size()-1;
		for (auto e : expr.children)
		{
			compile_expression(e);
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
		compile_name(expr.tokens, NAME_IS_LVALUE);
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
		compile_expression(block);
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
		end.type = OpcodeType::FOREACHPLAYER_BLK_END;

		DATA_IX_T string_index = m_strings.size();
		m_strings.push_back(eachPlayerLoopVarName);
		forEachHeaderCode.data |= STRING_TC;
		forEachHeaderCode.data |= ((OPCODE_CONV_T)string_index << 32);

		m_opcodes.push_back(forEachHeaderCode);
		for (auto e : expr.children)
		{
			compile_expression(e);
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
			factor_expression(targetStackExpr.children[1]);
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
			factor_expression(targetStackExpr.children[1]);
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

		if (nameTokens.size() == 2)
		{
			std::stringstream ss;
			ss << name << ":" << nameTokens[1].text;
			name = ss.str();
		}

		DATA_IX_T name_index = m_strings.size();
		m_strings.push_back(name);

		Opcode code;
		code.type = OpcodeType::CARD_BLK_HEADER;
		code.data |= STRING_TC;
		code.data |= ((OPCODE_CONV_T)name_index << 32);
		m_opcodes.push_back(code);

		expr.children.pop_back();

		for (auto e : expr.children) {
			compile_expression(e);
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
		else if (type == "playerref")
		{
			typeCode.data |= PLAYER_REF_TC;
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

int Program::Run(bool load)
{
	
	while (m_game.winner == -1 && m_current_opcode_index < m_opcodes.size())
	{
		Opcode code = m_opcodes[m_current_opcode_index];
		if (run(code, load) == -1)
		{
			return -1;
		}
	}
	return 0;
}

int Program::RunSetup()
{
	bool done = false;
	int depth_store = m_depth;

	m_current_opcode_index = m_setup_index;

	while (!done)
	{
		Opcode code = m_opcodes[m_current_opcode_index];
		
		if (run(code, false) == -1)
		{
			return -1;
		}

		if (code.type == OpcodeType::BLK_END && m_depth == depth_store)
		{
			done = true;
		}
	}
	return 0;
}

int Program::RunTurn()
{
	bool done = false;
	int depth_store = m_depth;

	AttrCont currentPlayerAttrCont;
    Attr currentPlayerAttr;
    currentPlayerAttr.type = AttributeType::PLAYER_REF;
    currentPlayerAttr.playerRef = m_game.currentPlayerIndex;
    currentPlayerAttrCont.Store("currentPlayer", currentPlayerAttr);

	locale_stack().push_back(currentPlayerAttrCont);
	
	m_current_opcode_index = m_turn_index;

	while (!done)
	{
		Opcode code = m_opcodes[m_current_opcode_index];

		if (run(code, false) == -1)
		{
			return -1;
		}

		if (code.type == OpcodeType::BLK_END && m_depth == depth_store)
		{
			done = true;
		}
	}

	locale_stack().pop_back();
	m_game.currentPlayerIndex = (currentPlayerAttr.playerRef + 1) % (m_game.players.size());

	return 0;
}

int Program::run(Opcode code, bool load)
{
	if (code.type == OpcodeType::GAME_BLK_HEADER)
	{
		m_depth++;
		if (!m_proc_mode_stack.empty())
		{
			cout << "Error: Cannot define game" << endl;
			return -1;
		}
		m_proc_mode_stack.push_back(PROC_MODE::GAME);
		m_locale_stack.push_back(AttrCont());
		DATA_IX_T name_idx = (code.data & uint64_t(0xFFFF0000)) >> 32;
		m_game.name = m_strings[name_idx];
		m_block_name_stack.push_back(m_strings[name_idx]);
		m_current_opcode_index += 1;
	}
	else if (code.type == OpcodeType::BLK_END)
	{
		
		if (m_proc_mode_stack.empty())
		{
			cout << "'end' without a matching 'start'" << endl;
			return -1;
		}

		m_depth--;

		if (m_proc_mode_stack.back() == PROC_MODE::CARD)
		{
			Card card;
			card.attributes = m_locale_stack.back();
			card.ID = m_game.cards.size();
			// name sequence is NAME:PARENT_NAME or just NAME
			string nameSequence = m_block_name_stack.back();

			card.name = get_card_name(nameSequence);
			card.parentName = get_card_parent_name(nameSequence);

			m_game.cards[card.name] = card;
			m_current_opcode_index += 1;
		}
		else if (m_proc_mode_stack.back() == PROC_MODE::PHASE)
		{
			m_locale_stack.pop_back();
			assert(!m_locale_stack.empty());
			assert(m_locale_stack.back().Contains("__INDEX_STORE"));
			Attr index_store = m_locale_stack.back().Get("__INDEX_STORE");
			assert(index_store.type == AttributeType::INT);
			m_current_opcode_index = index_store.i;
		}
		else
		{
			m_current_opcode_index += 1;
		}

		// never pop the root level attributes
		if (m_proc_mode_stack.back() != PROC_MODE::GAME)
		{
			m_locale_stack.pop_back();
		}
		m_proc_mode_stack.pop_back();
		m_block_name_stack.pop_back();
		
	}
	else if (code.type == OpcodeType::IF_BLK_HEADER)
	{
		m_current_opcode_index++;
		bool enter_block = resolve_bool_expression();

		if (enter_block)
		{
			m_depth++;
			m_proc_mode_stack.push_back(PROC_MODE::IF);
			m_block_name_stack.push_back("__IF");
			AttrCont cont;
			m_locale_stack.push_back(cont);
		}
		else
		{
			m_current_opcode_index--;
			ignore_block();
		}
	}
	else if (code.type == OpcodeType::FOREACHPLAYER_BLK_HEADER)
	{
		m_depth++;
		AttrCont cont;
		Attr counter;
		counter.playerRef = 0;
		counter.type = AttributeType::PLAYER_REF;
		cont.Store("p", counter);

		Attr loop_start_index;
		loop_start_index.type = AttributeType::INT;
		loop_start_index.i = m_current_opcode_index + 1;
		cont.Store("__LOOP_START_INDEX", loop_start_index);

		m_proc_mode_stack.push_back(PROC_MODE::FOREACH);
		m_block_name_stack.push_back("__FOREACHPLAYER");
		m_locale_stack.push_back(cont);
		m_current_opcode_index++;
	}
	else if (code.type == OpcodeType::FOREACHPLAYER_BLK_END)
	{
		auto playerRef = m_locale_stack.back().Get("p");
		assert(playerRef.type == AttributeType::PLAYER_REF);

		if (playerRef.playerRef == m_game.players.size() - 1)
		{
			m_depth--;
			m_locale_stack.pop_back();
			m_proc_mode_stack.pop_back();
			m_block_name_stack.pop_back();
			m_current_opcode_index++;
		}
		else
		{
			Attr newPlayerRef;
			newPlayerRef.type = AttributeType::PLAYER_REF;
			newPlayerRef.playerRef = playerRef.playerRef + 1;
			m_locale_stack.back().Store("p", newPlayerRef);

			auto loopStartIndex = m_locale_stack.back().Get("__LOOP_START_INDEX");
			assert(loopStartIndex.type == AttributeType::INT);
			int jmp_location = loopStartIndex.i;
			m_current_opcode_index = jmp_location;
		}
	}
	else if (code.type == OpcodeType::SETUP_BLK_HEADER)
	{
		if (load)
		{
			ignore_block();
		}
		else
		{
			m_depth++;
			m_proc_mode_stack.push_back(PROC_MODE::SETUP);
			m_block_name_stack.push_back("__SETUP");
			m_locale_stack.push_back(AttrCont());
			m_current_opcode_index++;
		}
	}
	else if (code.type == OpcodeType::PHASE_BLK_HEADER)
	{
		if (load)
		{
			ignore_block();
		}
		else
		{
			m_depth++;
			m_proc_mode_stack.push_back(PROC_MODE::PHASE);
			m_block_name_stack.push_back("__PHASE");
			m_locale_stack.push_back(AttrCont());
			m_current_opcode_index++;
		}
	}
	else if (code.type == OpcodeType::TURN_BLK_HEADER)
	{
		if (load)
		{
			ignore_block();
		}
		else
		{
			m_depth++;
			m_proc_mode_stack.push_back(PROC_MODE::TURN);
			m_block_name_stack.push_back("__TURN");
			m_locale_stack.push_back(AttrCont());
			m_current_opcode_index++;
		}
	}
	else if (code.type == OpcodeType::CARD_BLK_HEADER)
	{
		m_depth++;
		DATA_IX_T name_idx = (code.data & (OPCODE_CONV_T(0xFF) << 32)) >> 32;

		
		m_proc_mode_stack.push_back(PROC_MODE::CARD);
		m_block_name_stack.push_back(m_strings[name_idx]);

		string parentName = get_card_parent_name(m_strings[name_idx]);

		if (parentName.size() > 0 && m_game.cards.find(parentName) != m_game.cards.end())
		{
			AttrCont attrs = m_game.cards.find(parentName)->second.attributes;
			m_locale_stack.push_back(attrs);
		}
		else
		{
			m_locale_stack.push_back(AttrCont());
		}

		m_current_opcode_index++;
	}
	else if (code.type == OpcodeType::DO_DECL)
	{
		DATA_IX_T name_idx = (code.data & (OPCODE_CONV_T(0xFF) << 32)) >> 32;
		string block_name = m_strings[name_idx];

		AttrCont cont;
		Attr index_store_attr;
		index_store_attr.type = AttributeType::INT;
		index_store_attr.i = m_current_opcode_index + 1;
		cont.Store("__INDEX_STORE", index_store_attr);
		m_locale_stack.push_back(cont);
		
		m_current_opcode_index = m_phase_indexes[block_name];
	}
	else if (code.type == OpcodeType::STACK_SOURCE_RANDOM_CARD_TYPE)
	{
		m_current_opcode_index++;
		vector<string> card_type;
		read_name(card_type, m_opcodes[m_current_opcode_index].type);

		assert(card_type.size() == 1);

		auto destination_opcode_type = m_opcodes[m_current_opcode_index].type;
		m_current_opcode_index++;
		
		vector<string> destination_stack_identifier;
		read_name(destination_stack_identifier, m_opcodes[m_current_opcode_index].type);
		int move_amount = resolve_number_expression();

		Stack* dst = get_stack_ptr(destination_stack_identifier);
		auto matching_cards = m_game.get_cards_of_type(card_type[0]);
		
		vector<Card> cardsTaken;
		for (int i = 0; i < move_amount; i++)
		{
			cardsTaken.push_back(matching_cards[rand() % matching_cards.size()]);
		}

		if (destination_opcode_type == OpcodeType::STACK_DEST_TOP)
		{
			std::reverse(cardsTaken.begin(), cardsTaken.end());
			dst->cards.insert(dst->cards.end(), cardsTaken.begin(), cardsTaken.end());
		}
		else if (destination_opcode_type == OpcodeType::STACK_DEST_BOTTOM)
		{
			dst->cards.insert(dst->cards.begin(), cardsTaken.begin(), cardsTaken.end());
		}
	}
	else if (code.type == OpcodeType::STACK_SOURCE_TOP || code.type == OpcodeType::STACK_SOURCE_BOTTOM)
	{
		m_current_opcode_index++;
		vector<string> source_name;
		read_name(source_name, m_opcodes[m_current_opcode_index].type);

		auto destination_opcode_type = m_opcodes[m_current_opcode_index].type;
		m_current_opcode_index++;

		vector<string> dest_name;
		read_name(dest_name, m_opcodes[m_current_opcode_index].type);
		int move_amount = resolve_number_expression();

		Stack* source = get_stack_ptr(source_name);
		Stack* dst = get_stack_ptr(dest_name);

		vector<Card> cardsTaken;

		vector<int> forCallbackReport_cardsTaken;
		bool forCallbackReport_sourceTop = false;
		bool forCallbackReport_destTop = false;
		int forCallbackReport_src = source->ID;
		int forCallbackReport_dst = dst->ID;

		if (code.type == OpcodeType::STACK_SOURCE_BOTTOM)
		{
			forCallbackReport_sourceTop = false;
			int limit = std::min(
				static_cast<int>(source->cards.size()),
				move_amount
			);

			auto start = source->cards.end() - limit;
			auto end = source->cards.end();

			cardsTaken = vector<Card>(start, end);
			source->cards.erase(start, end);

			/**
			 * the code that takes the cards and puts them in the new stack
			 * assumes that the 'backmost' card is the one you
			 * would take first, whether from the bottom, or the top,
			 * so cardsTaken must be reversed in this case
			*/
			std::reverse(cardsTaken.begin(), cardsTaken.end());
		}
		else if (code.type == OpcodeType::STACK_SOURCE_TOP)
		{
			forCallbackReport_sourceTop = true;
			int limit = std::min(
				static_cast<int>(source->cards.size()),
				move_amount
			);

			auto start = source->cards.end() - limit;
			auto end = source->cards.end();

			cardsTaken = vector<Card>(start, end);
			source->cards.erase(start, end);
		}

		if (destination_opcode_type == OpcodeType::STACK_DEST_TOP)
		{
			forCallbackReport_destTop = true;
			std::reverse(cardsTaken.begin(), cardsTaken.end());
			dst->cards.insert(dst->cards.end(), cardsTaken.begin(), cardsTaken.end());
		}
		else if (destination_opcode_type == OpcodeType::STACK_DEST_BOTTOM)
		{
			forCallbackReport_destTop = false;
			dst->cards.insert(dst->cards.begin(), cardsTaken.begin(), cardsTaken.end());
		}

		for (Card c : cardsTaken)
		{
			forCallbackReport_cardsTaken.push_back(c.ID);
		}

		call_stack_move_callback(
			forCallbackReport_src,
			forCallbackReport_dst,
			forCallbackReport_sourceTop,
			forCallbackReport_destTop,
			forCallbackReport_cardsTaken.data(),
			forCallbackReport_cardsTaken.size()
		);
	}
	else if (code.type == OpcodeType::ATTR_DECL)
	{
		m_current_opcode_index++;
		vector<string> names;
		read_name(names, m_opcodes[m_current_opcode_index].type);

		auto typeOpcode = m_opcodes[m_current_opcode_index];
		assert(typeOpcode.type == OpcodeType::ATTR_DATA_TYPE);
		TYPE_CODE_T typeCode = typeOpcode.data & TYPE_CODE_T_MASK;
		m_current_opcode_index++;

		Attr a;
		auto attrType = s_type_code_to_attribute_type(typeCode);

		if (attrType == AttributeType::UNDEFINED)
		{
			cout << "undefined attribute type " << typeCode << endl;
			return -1;
		}

		a.type = attrType;

		if (attrType == AttributeType::STACK_REF)
		{
			
			Stack newStack;
			a.stackRef = m_game.stacks.size();

			if (typeCode == VISIBLE_STACK_TC)
			{
				newStack.t = StackType::VISIBLE;
			}
			else if (typeCode == HIDDEN_STACK_TC)
			{
				newStack.t = StackType::HIDDEN;
			}
			else if (typeCode == PRIVATE_STACK_TC)
			{
				newStack.t = StackType::PRIVATE;
			}
			
			newStack.ID = m_game.stacks.size();
			m_game.stacks[a.stackRef] = newStack;
		}

		if (names.size() == 1)
		{
			m_locale_stack.back().Store(names[0], a);
		}
		else
		{
			AttrCont* cont = GetObjectAttrContPtrFromIdentifier(names.begin(), names.end() - 1);
			// TODO: Fix bug where nested stack attrs delcarations such as hiddenstack p.hand 
			//       are overwritten in the game's stack store using their last name
			cont->Store(names.back(), a);
		}
	}
	else if (code.type == OpcodeType::PLAYERS_L_VALUE)
	{
		m_current_opcode_index++;
		int n_players = resolve_number_expression();

		for (int i = 0; i < n_players; i++) {
			m_game.players.push_back(Player());
		}
	}
	else if (code.type == OpcodeType::L_VALUE_DOT_SEPERATED_REF_CHAIN || code.type == OpcodeType::L_VALUE)
	{
		vector<string> names;
		read_name(names, code.type);

		Attr* attrPtr = get_attr_ptr(names);

		if (code.type == OpcodeType::R_VALUE)
		{
			throw ("Cannot assign from an rvalue reference yet");
		}
		else if (attrPtr->type == AttributeType::INT)
		{
			int value = resolve_number_expression();
			attrPtr->i = value;
		}
		else if (attrPtr->type == AttributeType::BOOL)
		{
			bool value = resolve_bool_expression();
			attrPtr->b = value;
		}
		else if (attrPtr->type == AttributeType::STRING)
		{
			string value = resolve_string_expression();
			attrPtr->s = value;
		}
		else if (attrPtr->type == AttributeType::FLOAT)
		{
			float value = resolve_float_expression();
			attrPtr->f = value;
		}
		else if (attrPtr->type == AttributeType::PLAYER_REF)
		{
			attrPtr->playerRef = resolve_expression_to_attr().playerRef;
		}
		else
		{
			throw VMError("Unsupported lvalue type, not sure how we got here.");
		}
	}
	else if (code.type == OpcodeType::WINNER_DECL)
	{
		m_current_opcode_index++;

		vector<string> names;
		read_name(names, m_opcodes[m_current_opcode_index].type);

		Attr attr = get_attr_rvalue(names);
		if (attr.type != AttributeType::PLAYER_REF)
		{
			throw VMError("You must declare a winner with a playerRef");
		}
		m_game.winner = attr.playerRef;
	}
	else
	{
		cout << "encounterd unknown opcode type " << int(code.type) << endl;
		return -1;
	}

	return 0;
}

void Program::ignore_block()
{
	int depth = 1;

	while (depth > 0)
	{
		m_current_opcode_index++;
		auto type = m_opcodes[m_current_opcode_index].type;
		if (type == OpcodeType::CARD_BLK_HEADER)
			depth++;
		
		if (type == OpcodeType::FOREACHPLAYER_BLK_HEADER)
			depth++;

		if (type == OpcodeType::GAME_BLK_HEADER)
			depth++;

		if (type == OpcodeType::IF_BLK_HEADER)
			depth++;

		if (type == OpcodeType::PHASE_BLK_HEADER)
			depth++;

		if (type == OpcodeType::SETUP_BLK_HEADER)
			depth++;

		if (type == OpcodeType::TURN_BLK_HEADER)
			depth++;

		if (type == OpcodeType::BLK_END)
			depth--;

		if (type == OpcodeType::FOREACHPLAYER_BLK_END)
			depth--;
	}
	m_current_opcode_index++;
}

void Program::read_name(vector<string>& names, OpcodeType nameType)
{
	int idx = m_current_opcode_index;
	while (m_opcodes[idx].type == nameType)
	{
		auto nameOpcode = m_opcodes[idx];
		TYPE_CODE_T _string_typecode = nameOpcode.data & TYPE_CODE_T_MASK;
		assert(_string_typecode == STRING_TC);
		DATA_IX_T stringNameIdx = (nameOpcode.data & (OPCODE_CONV_T(0xFF) << 32)) >> 32;
		names.push_back(m_strings[stringNameIdx]);
		idx++;
	}

	if (m_opcodes[idx].type == OpcodeType::DOT_SEPERATED_REF_CHAIN_END)
	{
		idx++;
	}
	m_current_opcode_index = idx;
}

int Program::resolve_number_expression()
{
	if (m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE)
	{
		auto int_value_opcode = m_opcodes[m_current_opcode_index];

		TYPE_CODE_T type = (int_value_opcode.data & TYPE_CODE_T_MASK);

		assert(type == INT_TC);

		DATA_IX_T data_index = (int_value_opcode.data & (OPCODE_CONV_T(0xFF) << 32)) >> 32;
		
		m_current_opcode_index++;
		return m_ints[data_index];	
	}
	if (m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE_DOT_SEPERATED_REF_CHAIN)
	{
		vector<string> names;
		read_name(names, m_opcodes[m_current_opcode_index].type);

		Attr attrPtr = get_attr_rvalue(names);
		
		assert(attrPtr.type == AttributeType::INT);

		return attrPtr.i;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::MULTIPLY)
	{
		m_current_opcode_index++;
		int left = resolve_number_expression();
		int right = resolve_number_expression();

		return left * right;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::ADD)
	{
		m_current_opcode_index++;
		int left = resolve_number_expression();
		int right = resolve_number_expression();

		return left + right;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::SUBTRACT)
	{
		m_current_opcode_index++;
		int left = resolve_number_expression();
		int right = resolve_number_expression();

		return left - right;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::DIVIDE)
	{
		m_current_opcode_index++;
		int left = resolve_number_expression();
		int right = resolve_number_expression();

		return left / right;
	}
	else
	{

		throw VMError("Unable to resolve direct rvalue in any way other than a number entry, like '5'");
	}

	return 0;
}

bool Program::resolve_bool_expression()
{
	if (m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE)
	{
		auto bool_value_opcode = m_opcodes[m_current_opcode_index];

		TYPE_CODE_T type = (bool_value_opcode.data & TYPE_CODE_T_MASK);

		assert(type == BOOL_TC);

		DATA_IX_T data_index = (bool_value_opcode.data & (OPCODE_CONV_T(0xFF) << 32)) >> 32;

		m_current_opcode_index++;
		return m_bools[data_index];
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE_DOT_SEPERATED_REF_CHAIN)
	{
		vector<string> names;
		read_name(names, m_opcodes[m_current_opcode_index].type);

		Attr attr = get_attr_rvalue(names);
		if (attr.type != AttributeType::BOOL)
		{
			throw VMError("Attribute must be a bool to be a boolean expression");
		}

		return attr.b;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::COMPARE)
	{
		m_current_opcode_index++;
		auto left = resolve_expression_to_attr();
		auto right = resolve_expression_to_attr();

		return compare_attrs(left, right);
	}
	else
	{
		throw VMError("unsupported reference type used in boolean expression");
	}
}

bool Program::compare_attrs(Attr a, Attr b)
{
	if (a.type != b.type)
	{
		throw VMError("Unable to compare attributes of different type");
	}

	switch (a.type)
	{
	case AttributeType::BOOL:
		return a.b == b.b;
	case AttributeType::CARD_REF:
		return a.cardRef == b.cardRef;
	case AttributeType::FLOAT:
		return a.f == b.f;
	case AttributeType::INT:
		return a.i == b.i;
	case AttributeType::PHASE_REF:
		return a.phaseRef == b.phaseRef;
	case AttributeType::PLAYER_REF:
		return a.playerRef == b.playerRef;
	case AttributeType::STACK_POSITION_REF:
	{
		std::get<0>(a.stackPositionRef) == std::get<0>(b.stackPositionRef) && std::get<1>(a.stackPositionRef) == std::get<1>(b.stackPositionRef);
		auto stack_a = &m_game.stacks[std::get<0>(a.stackPositionRef)];
		int pos_ref_a = std::get<1>(a.stackPositionRef);

		auto stack_b = &m_game.stacks[std::get<0>(b.stackPositionRef)];
		int pos_ref_b = std::get<1>(b.stackPositionRef);

		if (pos_ref_a < 0 || pos_ref_b < 0)
		{
			return false;
		}

		auto card_a = &stack_a->cards[pos_ref_a];
		auto card_b = &stack_b->cards[pos_ref_b];

		return card_a->name == card_b->name && card_a->parentName == card_b->parentName;
	}
	case AttributeType::STACK_REF:
		return a.stackRef == b.stackRef;
	case AttributeType::STRING:
		return a.s == b.s;
	default:
		throw VMError("this type cannot be compared");
	}
}

Attr Program::add_attrs(Attr a, Attr b)
{
	throw VMError("Joe needs to implement add_attrs");
}

Attr Program::multiply_attrs(Attr a, Attr b)
{
	throw VMError("Joe needs to implement multiply_attrs");
}

Attr Program::divide_attrs(Attr a, Attr b)
{
	throw VMError("Joe needs to implement divide_attrs");
}

Attr Program::subtract_attrs(Attr a, Attr b)
{

	Attr r;
	switch (a.type)
	{
	case AttributeType::FLOAT:
		r.type = AttributeType::FLOAT;
		if (b.type == AttributeType::FLOAT)
		{
			r.f = a.f - b.f;
		}
		else if (b.type == AttributeType::INT)
		{
			r.f = a.f - b.i;
		}
		else
		{
			throw VMError("You cannot subtract this type from a float");
		}
		break;

	case AttributeType::INT:
		r.type = AttributeType::INT;
		if (b.type == AttributeType::FLOAT)
		{
			r.i = a.i - b.f;
		}
		else if (b.type == AttributeType::INT)
		{
			r.i = a.i - b.i;
		}
		else
		{
			throw VMError("You cannot subtract this type from an int");
		}
		break;
	case AttributeType::PLAYER_REF:
		r.type = AttributeType::PLAYER_REF;
		if (b.type == AttributeType::INT)
		{
			b.playerRef = (a.playerRef - b.i) % m_game.players.size();
		}
		else
		{
			throw VMError("only integers can be subtracted from player references");
		}
		break;
	case AttributeType::STACK_POSITION_REF:
		//return std::get<0>(a.stackPositionRef) == std::get<0>(b.stackPositionRef) && std::get<1>(a.stackPositionRef) == std::get<1>(b.stackPositionRef);
		r.type = AttributeType::STACK_POSITION_REF;
		if (b.type == AttributeType::INT)
		{
			r.stackPositionRef = std::tuple<int, int>(std::get<0>(a.stackPositionRef), std::get<1>(a.stackPositionRef) - b.i);
		}
		else
		{
			throw VMError("only integers can be subtracted from stack position references");
		}
		break;
	default:
		throw VMError("this type cannot be subtracted from");
	}

	return r;
}

Attr Program::resolve_expression_to_attr()
{
	if (m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE)
	{
		auto value_opcode = m_opcodes[m_current_opcode_index];

		TYPE_CODE_T type = (value_opcode.data & TYPE_CODE_T_MASK);
		DATA_IX_T data_index = (value_opcode.data & (OPCODE_CONV_T(0xFF) << 32)) >> 32;

		Attr a;
		switch (type)
		{
		case INT_TC:
			a.type = AttributeType::INT;
			a.i = m_ints[data_index];
			break;
		case STRING_TC:
			a.type = AttributeType::STRING;
			a.s = m_strings[data_index];
			break;
		case BOOL_TC:
			a.type = AttributeType::BOOL;
			a.b = m_bools[data_index];
			break;
		default:
			throw VMError("Unsupported rvalue type");
		}

		m_current_opcode_index++;
		return a;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE_DOT_SEPERATED_REF_CHAIN || m_opcodes[m_current_opcode_index].type == OpcodeType::R_VALUE_REF)
	{
		vector<string> names;
		read_name(names, m_opcodes[m_current_opcode_index].type);

		Attr attr = get_attr_rvalue(names);
		return attr;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::COMPARE)
	{
		bool res = resolve_bool_expression();

		Attr a;
		a.type = AttributeType::BOOL;
		a.b = res;

		return a;
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::SUBTRACT)
	{
		m_current_opcode_index++;
		Attr left = resolve_expression_to_attr();
		Attr right = resolve_expression_to_attr();

		return subtract_attrs(left, right);
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::ADD)
	{
		m_current_opcode_index++;
		Attr left = resolve_expression_to_attr();
		Attr right = resolve_expression_to_attr();

		return add_attrs(left, right);
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::DIVIDE)
	{
		m_current_opcode_index++;
		Attr left = resolve_expression_to_attr();
		Attr right = resolve_expression_to_attr();

		return divide_attrs(left, right);
	}
	else if (m_opcodes[m_current_opcode_index].type == OpcodeType::MULTIPLY)
	{
		m_current_opcode_index++;
		Attr left = resolve_expression_to_attr();
		Attr right = resolve_expression_to_attr();

		return multiply_attrs(left, right);
	}
	else
	{
		throw VMError("Expression -> attr conversion is unsupported for this expression type");
	}
}

string Program::resolve_string_expression()
{
	throw VMError("Cannot yet resolve string expressions");
}

float Program::resolve_float_expression()
{
	throw VMError("Cannot yet resolve float expressions");
}

Game Program::game()
{
	return m_game;
}

AttributeType Program::s_type_code_to_attribute_type(TYPE_CODE_T t)
{
	switch (t)
	{
	case STRING_TC:
		return AttributeType::STRING;
	case INT_TC:
		return AttributeType::INT;
	case BOOL_TC:
		return AttributeType::BOOL;
	case FLOAT_TC:
		return AttributeType::FLOAT;
	case VISIBLE_STACK_TC:
		return AttributeType::STACK_REF;
	case HIDDEN_STACK_TC:
		return AttributeType::STACK_REF;
	case PRIVATE_STACK_TC:
		return AttributeType::STACK_REF;
	case CARD_TC:
		return AttributeType::CARD_REF;
	case PLAYER_REF_TC:
		return AttributeType::PLAYER_REF;
	default:
		return AttributeType::UNDEFINED;
	}
}

AttrCont* Program::GetGlobalObjectAttrContPtr(AttrCont& cont, string name) {
	if (!cont.Contains(name)) {
		throw VMError("this attribute does not exist ");
	}

	if (cont.Get(name).type == AttributeType::CARD_REF) {
		return &m_game.cards[cont.Get(name).cardRef].attributes;
	}

	if (cont.Get(name).type == AttributeType::PLAYER_REF) {
		return &m_game.players[cont.Get(name).playerRef].attributes;
	}

	if (cont.Get(name).type == AttributeType::PHASE_REF) {
		return &m_game.phases[cont.Get(name).phaseRef].attributes;
	}

	if (cont.Get(name).type == AttributeType::STACK_REF) {
		return &m_game.stacks[cont.Get(name).stackRef].attributes;
	}

	throw VMError("this type cannot have attributes ");
}


AttrCont* Program::GetObjectAttrContPtrFromIdentifier(vector<string>::iterator namesBegin, vector<string>::iterator namesEnd) {
	assert(namesBegin!= namesEnd);

	auto namesItr = namesBegin;

	string firstName = *namesItr;

	AttrCont* current;
	bool found = false;

	for (int i = m_locale_stack.size() - 1; !found && i >= 0; i--) {
		if (m_locale_stack[i].Contains(firstName)) {
			current = GetGlobalObjectAttrContPtr(m_locale_stack[i], *namesBegin);
			found = true;
		}
	}


	if (!found) {
		throw VMError("Could not find attribute");
	}

	++namesItr;

	while (namesItr != namesEnd) {
		if (current->Contains(*namesItr)) {
			current = GetGlobalObjectAttrContPtr(*current, *namesItr);
		}
		else {
			throw VMError(*namesItr + " not found in " + firstName);
		}

		++namesItr;
	}

	return current;
}

Attr Program::get_attr_rvalue(vector<string>& names)
{
	assert(names.size() > 0);

	bool shallowName = names.size() == 1;

	Attr currentAttr;
	bool found = false;

	auto nameItr = names.begin();


	if (shallowName && m_game.cards.find(*nameItr) != m_game.cards.end())
	{
		Attr tmp;
		tmp.type = AttributeType::CARD_REF;
		tmp.cardRef = *nameItr;
		return tmp;
	}
	

	for (auto& locale : m_locale_stack)
	{
		if (locale.Contains(*nameItr))
		{
			if (shallowName)
			{
				return locale.Get(*nameItr);
			}

			found = true;
			currentAttr = locale.Get(*nameItr);
		}
	}

	if (shallowName && !found)
	{
		throw VMError("variable does not exist");
	}

	if (!found && m_game.cards.find(*nameItr) != m_game.cards.end())
	{
		found = true;
		currentAttr.type = AttributeType::CARD_REF;
		currentAttr.cardRef = *nameItr;
	}

	if (!found)
	{
		throw VMError("variable does not exist");
	}

	nameItr++;

	while (nameItr != names.end())
	{
		if (currentAttr.type == AttributeType::CARD_REF)
		{
			if (m_game.cards[currentAttr.cardRef].attributes.Contains(*nameItr))
			{
				if (nameItr == names.end() - 1)
				{
					return m_game.cards[currentAttr.cardRef].attributes.Get(*nameItr);
				}

				currentAttr = m_game.cards[currentAttr.cardRef].attributes.Get(*nameItr);
			}
			else
			{
				throw VMError("This card does not contain attribute");
			}
		}
		else if (currentAttr.type == AttributeType::STACK_REF)
		{

			if (*nameItr == "top" || *nameItr == "bottom")
			{
				Attr tmp;
				tmp.type = AttributeType::STACK_POSITION_REF;

				if (*nameItr == "top")
				{
					int topIndex = m_game.stacks[currentAttr.stackRef].cards.size() - 1;
					tmp.stackPositionRef = std::tuple< int, int>(currentAttr.stackRef, topIndex);
				}
				else
				{
					int bottomIndex = 0;
					tmp.stackPositionRef = std::tuple< int, int>(currentAttr.stackRef, bottomIndex);
				}

				return tmp;
			}
			else if (*nameItr == "size")
			{
				Attr tmp;
				tmp.type = AttributeType::INT;

				tmp.i = m_game.stacks[currentAttr.stackRef].cards.size();

				return tmp;
			}

			if (m_game.stacks[currentAttr.stackRef].attributes.Contains(*nameItr))
			{
				if (nameItr == names.end() - 1)
				{
					return m_game.stacks[currentAttr.stackRef].attributes.Get(*nameItr);
				}

				currentAttr = m_game.stacks[currentAttr.stackRef].attributes.Get(*nameItr);
			}
			else
			{
				cout << "This stack does not contain attribute " << *nameItr << endl;
				throw VMError("This stack does not contain attribute");
			}
		}
		else if (currentAttr.type == AttributeType::PLAYER_REF)
		{
			if (m_game.players[currentAttr.playerRef].attributes.Contains(*nameItr))
			{
				if (nameItr == names.end() - 1)
				{
					return m_game.players[currentAttr.playerRef].attributes.Get(*nameItr);
				}

				currentAttr = m_game.players[currentAttr.playerRef].attributes.Get(*nameItr);
			}
			else
			{
				throw VMError("This player does not contain attribute");
			}
		}

		nameItr++;
	}
}

Attr* Program::get_attr_ptr(vector<string>& names)
{

	assert(names.size() > 0);

	bool shallowName = names.size() == 1;

	Attr currentAttr;
	bool found = false;

	auto nameItr = names.begin();


	if (shallowName && m_game.cards.find(*nameItr) != m_game.cards.end())
	{
		throw VMError("Cannot create an lvalue reference to a card type");
	}

	for (auto& locale : m_locale_stack)
	{
		if (locale.Contains(*nameItr))
		{
			if (shallowName)
			{
				return &locale.Get(*nameItr);
			}
			
			found = true;
			currentAttr = locale.Get(*nameItr);
		}
	}

	if (shallowName && !found)
	{
		throw VMError("variable does not exist");
	}

	if (!found && m_game.cards.find(*nameItr) != m_game.cards.end())
	{
		found = true;
		currentAttr.type = AttributeType::CARD_REF;
		currentAttr.cardRef = *nameItr;
	}

	if (!found)
	{
		throw VMError("variable does not exist");
	}

	nameItr++;

	while (nameItr != names.end())
	{
		if (currentAttr.type == AttributeType::CARD_REF)
		{
			if (m_game.cards[currentAttr.cardRef].attributes.Contains(*nameItr))
			{
				if (nameItr == names.end()-1)
				{
					return &m_game.cards[currentAttr.cardRef].attributes.Get(*nameItr);
				}

				currentAttr = m_game.cards[currentAttr.cardRef].attributes.Get(*nameItr);
			}
			else
			{
				throw VMError("This card does not contain attribute");
			}
		}
		else if (currentAttr.type == AttributeType::STACK_REF)
		{

			if (*nameItr == "top" || *nameItr == "bottom")
			{
				throw VMError("Cannot create lvalue reference from stack position pointer");
			}

			if (m_game.stacks[currentAttr.stackRef].attributes.Contains(*nameItr))
			{
				if (nameItr == names.end() - 1)
				{
					return &m_game.stacks[currentAttr.stackRef].attributes.Get(*nameItr);
				}

				currentAttr = m_game.stacks[currentAttr.stackRef].attributes.Get(*nameItr);
			}
			else
			{
				cout << "This stack does not contain attribute " << *nameItr << endl;
				throw VMError("This stack does not contain attribute");
			}
		}
		else if (currentAttr.type == AttributeType::PLAYER_REF)
		{
			if (m_game.players[currentAttr.playerRef].attributes.Contains(*nameItr))
			{
				if (nameItr == names.end() - 1)
				{
					return &m_game.players[currentAttr.playerRef].attributes.Get(*nameItr);
				}

				currentAttr = m_game.players[currentAttr.playerRef].attributes.Get(*nameItr);
			}
			else
			{
				throw VMError("This player does not contain attribute");
			}
		}

		nameItr++;
	}
}

string Program::get_card_parent_name(string nameSequence)
{

	auto delimPos = nameSequence.find(":");

	if (delimPos != string::npos)
	{
		return nameSequence.substr(delimPos + 1);
	}
	
	return "";
}

string Program::get_card_name(string nameSequence)
{
	auto delimPos = nameSequence.find(":");

	if (delimPos != string::npos)
	{
		return nameSequence.substr(0, delimPos);
	}

	return nameSequence;
}

Stack* Program::get_stack_ptr(vector<string>& stack_identifier)
{
	assert(stack_identifier.size() != 0);

	Stack* src = nullptr;

	Attr* stackAttr = get_attr_ptr(stack_identifier);
	assert(stackAttr->type == AttributeType::STACK_REF);
	auto stackId = stackAttr->stackRef;
	if (m_game.stacks.find(stackId) == m_game.stacks.end())
	{
		throw VMError("could not find stack with name: " + stack_identifier.back());
	}
	src = &m_game.stacks[stackId];

	assert(src != nullptr);

	return src;
}
vector<AttrCont>& Program::locale_stack()
{
	return m_locale_stack;
}

void Program::SetStackMoveCallbackFun(stack_move_callback_fun* fun, void* data)
{
	m_stack_move_callback = fun;
	m_stack_callback_data = data;
}

void Program::call_stack_move_callback(int from, int to, bool fromTop, bool toTop, const int* cardIds, int nCards)
{
	if (m_stack_move_callback != nullptr)
	{
		m_stack_move_callback(from, to, fromTop, toTop, cardIds, nCards, m_stack_callback_data);
	}
}

}