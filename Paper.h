#ifdef PAPER_EXPORTS
#define PAPER_API __declspec(dllexport)
#else
#define PAPER_API __declspec(dllimport)
#endif

#include <vector>
#include <string>

class Program;

PAPER_API Program* Paper_newProgram();

PAPER_API void Paper_compile(Program* program, const char** lines, int size);

PAPER_API void Paper_runSetup(Program* program);

PAPER_API void Paper_runTurn(Program* program);
