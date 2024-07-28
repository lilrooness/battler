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

typedef struct {
	int id;
	char* name;
	char* parentName;
} Paper_Card;

typedef struct {
	int id;
	int ncards;
	Paper_Card* cards;
} Paper_Stack;

class Program;

PAPER_API Program* Paper_newProgram();

PAPER_API void Paper_compile(Program* program, const char** lines, int size);

PAPER_API void Paper_load(Program* program);

PAPER_API void Paper_runSetup(Program* program);

PAPER_API void Paper_runTurn(Program* program);

PAPER_API void Paper_setStackMoveCallback(Program* program, stack_move_callback_fun* f, void* data);

PAPER_API int Paper_getStack(Program* program, int id, Paper_Stack* stack_out);

PAPER_API int Paper_getCard(Program* program, int id, Paper_Card* card_out);

PAPER_API Paper_Card* Paper_getAllCards(Program* program, int* ncards_out);

PAPER_API int* Paper_getAllStackIDs(Program* program, int* nstacks_out);
