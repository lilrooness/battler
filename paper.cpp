#include "paper.h"
#include "Compiler.h"

Program* Paper_newProgram()
{
	Program* p = new Program();
	
	return p;
}

void Paper_compile(Program* program, std::vector<std::string> lines)
{
	program->Compile(lines);
}

void Paper_runSetup(Program* program)
{
	program->RunSetup();
}

void Paper_runTurn(Program* program)
{
	program->RunTurn();
}
