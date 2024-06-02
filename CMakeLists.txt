cmake_minimum_required(VERSION 3.19)

project(Battler)

add_executable(Battler 
	Battler.cpp
	Expression.cpp
	vm/Compiler.cpp
	Parser.cpp
	InterpreterErrors.cpp
	Battler.h
	expression.h
	interpreter_errors.h
	Parser.h
	Compiler.h
	vm/game.h
	vm/run.h
)
