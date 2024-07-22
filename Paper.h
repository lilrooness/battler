#ifdef PAPER_EXPORTS
#define PAPER_API __declspec(dllexport)
#else
#define PAPER_API __declspec(dllimport)
#endif

typedef void(stack_move_callback_fun)(
	int from,
	int to,
	bool from_top,
	bool to_top,
	const int* cardIds,
	int nCards,
	void* data
);

class Program;

PAPER_API Program* Paper_newProgram();

PAPER_API void Paper_compile(Program* program, const char** lines, int size);

PAPER_API void Paper_load(Program* program);

PAPER_API void Paper_runSetup(Program* program);

PAPER_API void Paper_runTurn(Program* program);

PAPER_API void Paper_setStackMoveCallback(Program* program, stack_move_callback_fun* f, void* data);
