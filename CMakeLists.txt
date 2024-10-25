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

set(TESTS_DEFAULT OFF)
option(TESTS "enable testing" ${TESTS_DEFAULT})

if (TESTS)
	include(FetchContent)
	FetchContent_Declare(
	  googletest
	  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
	  DOWNLOAD_EXTRACT_TIMESTAMP true
	)
	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)

	enable_testing()
	add_executable(
		BattlerTester
		tests/tests.cpp

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

	target_link_libraries(BattlerTester GTest::gtest_main)

	include(GoogleTest)
	gtest_discover_tests(
		BattlerTester
		DISCOVERY_MODE PRE_TEST
	)
endif()

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
