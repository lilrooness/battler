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
