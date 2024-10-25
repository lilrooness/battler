cmake_minimum_required(VERSION 3.19)
SET(CMAKE_CXX_STANDARD 17)

project(Battler)

add_executable(Battler 
	Battler.cpp
	Expression.cpp
	vm/Compiler.cpp
	Parser.cpp
	InterpreterErrors.cpp
	vm/game.cpp
	Battler.h
	expression.h
	interpreter_errors.h
	Parser.h
	Compiler.h
	vm/game.h
)

if (WIN32)
	add_library(paper
		SHARED
		Expression.cpp
		vm/Compiler.cpp
		Parser.cpp
		InterpreterErrors.cpp
		vm/game.cpp
		paper.cpp
		Battler.h
		expression.h
		interpreter_errors.h
		Parser.h
		Compiler.h
		vm/game.h
		paper.h
	)

	target_compile_definitions(paper PRIVATE PAPER_EXPORTS=1)

	install(
		TARGETS Battler paper
	)
	install(
		FILES 
		paper.h
		DESTINATION include
	)
endif()
