cmake_minimum_required(VERSION 3.19)

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

add_library(paper
	SHARED
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
	Paper.h
)

install(
	TARGETS Battler paper
)
install(
	FILES Paper.h DESTINATION include
)
