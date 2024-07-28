#include <stdlib.h>

#include "paper.h"
#include "Compiler.h"

Program* Paper_newProgram()
{
	Program* p = new Program();
	
	return p;
}

void Paper_compile(Program* program, const char** lines, int size)
{
	std::vector<std::string> v_lines;
	for (int i = 0; i < size; i++)
	{
		auto s = std::string(lines[i]);
		v_lines.push_back(s);
	}

	program->Compile(v_lines);
}

void Paper_load(Program* program)
{
	program->Run(true);
}

void Paper_runSetup(Program* program)
{
	program->RunSetup();
}

void Paper_runTurn(Program* program)
{
	program->RunTurn();
}

void Paper_setStackMoveCallback(Program* program, stack_move_callback_fun* f, void* data)
{
	program->SetStackMoveCallbackFun(f, data);
}

int Paper_getStack(Program* program, int id, Paper_Stack* stack_out)
{
	auto stackItr = program->game().stacks.find(id);

	if (stackItr == program->game().stacks.end())
	{
		return -1;
	}

	stack_out->id = stackItr->second.ID;
	stack_out->ncards = stackItr->second.cards.size();

	Paper_Card* cards = (Paper_Card*) malloc(sizeof(Paper_Card) * stack_out->ncards);

	for (int i = 0; i < stack_out->ncards; i++)
	{
		Card card = stackItr->second.cards[i];

		cards[i].id = card.ID;

		cards[i].name = (char*) malloc(sizeof(char) * card.name.size());
		cards[i].parentName = (char*)malloc(sizeof(char) * card.parentName.size());
		strcpy_s(cards[i].name, card.name.size(), card.name.c_str());
		strcpy_s(cards[i].parentName, card.parentName.size(), card.parentName.c_str());
	}

	return 0;
}

int Paper_getCard(Program* program, int id, Paper_Card* card_out)
{
	Card* card = nullptr;

	for (auto entry : program->game().cards)
	{
		if (entry.second.ID == id)
		{
			card = &entry.second;
			break;
		}
	}

	if (card == nullptr)
	{
		return -1;
	}

	card_out = (Paper_Card*)malloc(sizeof(Paper_Card));

	card_out->id = card->ID;

	card_out->name = (char*)malloc(sizeof(char) * card->name.size());
	card_out->parentName = (char*)malloc(sizeof(char) * card->parentName.size());
	strcpy_s(card_out->name, card->name.size(), card->name.c_str());
	strcpy_s(card_out->parentName, card->parentName.size(), card->parentName.c_str());

	return 0;
}

Paper_Card* Paper_getAllCards(Program* program, int* ncards_out)
{
	*ncards_out = program->game().cards.size();

	Paper_Card* cards = (Paper_Card*)malloc(sizeof(Paper_Card) * *ncards_out);

	for (int i = 0; i < *ncards_out; i++)
	{
		Card c = program->game().cards[0];
		cards[i].id = c.ID;

		cards[i].name = (char*) malloc(sizeof(char*) * c.name.size());
		cards[i].parentName = (char*)malloc(sizeof(char*) * c.parentName.size());
		strcpy_s(cards[i].name, c.name.size(), c.name.c_str());
		strcpy_s(cards[i].parentName, c.parentName.size(), c.parentName.c_str());
	}

	return cards;
}

int* Paper_getAllStackIDs(Program* program, int* nstacks_out)
{
	*nstacks_out = program->game().stacks.size();

	int* stackIds = (int*)malloc(sizeof(int) * *nstacks_out);

	int i = 0;
	for (auto s : program->game().stacks)
	{
		stackIds[i++] = s.second.ID;
	}

	return stackIds;
}
