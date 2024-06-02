#pragma once

#ifndef COMPILER_H
#define COMPILER_H

#include <vector>
#include <string>
#include <bitset>
#include <unordered_map>
#include <tuple>

#include "vm/game.h"

using std::vector;
using std::string;
using std::bitset;
using std::unordered_map;
using std::tuple;

enum class OpcodeType
{
	// Block headers
	GAME_BLK_HEADER,
	CARD_BLK_HEADER,
	SETUP_BLK_HEADER,
	PHASE_BLK_HEADER,
	TURN_BLK_HEADER,
	IF_BLK_HEADER,
	FOREACHPLAYER_BLK_HEADER,
	BLK_END,

	// Other things
	STACK_MOVE,
	ATTR_DECL,
	ATTR_ASSIGN,
	ATTR_DATA_TYPE,
	ATTR_DATA,
	DO_DECL,
	WINNER_DECL,

	NO_OP,
};

#define TYPE_CODE_T uint16_t
#define DATA_IX_T uint32_t
#define OPCODE_CONV_T uint64_t

// data type codes
const TYPE_CODE_T STRING_TC        = 0x01;
const TYPE_CODE_T INT_TC           = 0x02;
const TYPE_CODE_T BOOL_TC          = 0x03;
const TYPE_CODE_T FLOAT_TC         = 0x04;
const TYPE_CODE_T VISIBLE_STACK_TC = 0x05;
const TYPE_CODE_T HIDDEN_STACK_TC  = 0x06;
const TYPE_CODE_T PRIVATE_STACK_TC = 0x07;
const TYPE_CODE_T CARD_TC          = 0x08;

class Opcode
{
public:
	OpcodeType type;
	int blk_size;
	bitset<64> data;
};

class Program
{
private:
	//code
	vector<Opcode> m_opcodes;

	//data
	vector<string> m_strings;
	vector<int> m_ints;
	vector<bool> m_bools;

	// while compiling
	//unordered_map<string, tuple<TYPE_CODE_T, DATA_IX_T> > m_addresses;
public:
	void compile(Expression);
};

class CompileError {
public:
	Token t;
	string reason;
	CompileError(string reason, Token t) : reason{ reason }, t{ t } {}
};

#endif // !COMPILER_H