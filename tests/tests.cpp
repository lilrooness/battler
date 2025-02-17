#include <gtest/gtest.h>
#include <vector>
#include <string>

#include "../Compiler.h"
#include "../expression.h"
#include "../vm/game.h"
#include "../interpreter_errors.h"

TEST(EndToEndTests, BasicGame)
{
    auto lines = std::vector<std::string>() = {
        "game Test start",
            "players 1"
            "card Parent start",
                "int health",
                "int attack",
            "end",
            "card Child Parent start",
                "health = 15",
                "int defence",
            "end",
        
            "visiblestack a",
            "visiblestack b",
            
            "setup start",
                "random Parent -> a top 1",
            "end",
        
            "turn start",
                "a -> b top 1",
                "random Parent ->_ b top 1",
            "end",
        
        "end"
    };
    
    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    
    EXPECT_EQ(p.game().cards.size(), 2);
    EXPECT_EQ(p.game().name, "Test");
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
    
    auto cards = p.game().cards;
    
    auto parentCardIt = cards.find("Parent");
    EXPECT_NE(parentCardIt, cards.end());
    
    auto childCardIt = cards.find("Child");
    EXPECT_NE(childCardIt, cards.end());
    
    Battler::Card parent = parentCardIt->second;
    Battler::Card child = childCardIt->second;
    
    EXPECT_EQ(parent.name, "Parent");
    EXPECT_TRUE(parent.attributes.Contains("health"));
    EXPECT_TRUE(parent.attributes.Contains("attack"));
    EXPECT_FALSE(parent.attributes.Contains("defence"));
    EXPECT_EQ(parent.attributes.Get("health").i, 0);
    EXPECT_EQ(parent.attributes.Get("attack").i, 0);
    EXPECT_EQ(parent.ID, 0);
    EXPECT_EQ(parent.UUID, -1);
    
    EXPECT_EQ(child.name, "Child");
    EXPECT_TRUE(child.attributes.Contains("health"));
    EXPECT_TRUE(child.attributes.Contains("attack"));
    EXPECT_TRUE(child.attributes.Contains("defence"));
    EXPECT_EQ(child.attributes.Get("health").i, 15);
    EXPECT_EQ(child.attributes.Get("attack").i, 0);
    EXPECT_EQ(child.attributes.Get("defence").i, 0);
    EXPECT_EQ(child.ID, 1);
    EXPECT_EQ(parent.UUID, -1);
    
    p.RunSetup();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 1);
    EXPECT_EQ(p.game().stacks[0].cards[0].UUID, 1);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    
    p.RunTurn();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 2);
    EXPECT_EQ(p.game().stacks[1].cards[1].UUID, 1);
    EXPECT_EQ(p.game().stacks[1].cards[0].UUID, 2);    
}


TEST(EndToEndTests, ChooseMoveInstr)
{
    auto lines = std::vector<std::string>() = {
        "game Test start",
            "players 1"
            "card Parent start",
                "int health",
                "int attack",
            "end",
            "card Child Parent start",
                "health = 15",
                "int defence",
            "end",
        
            "visiblestack a",
            "visiblestack b",
            
            "setup start",
                "random Parent -> a top 1",
            "end",
        
            "turn start",
                "choose a -> b top 1",
            "end",
        
        "end"
    };
    
    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    
    p.RunTurn();
    
    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 1);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    EXPECT_FALSE(p.AddCardToWaitingInput(p.game().stacks[0].cards[0]));
    
    EXPECT_FALSE(p.m_waitingForUserInteraction);
    p.RunTurn(true);
    
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 1);
    
    p.RunTurn();
    // the source stack should be empty, so we should not expect any more user input
    EXPECT_FALSE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 1);
}

TEST(EndToEndTests, CutTest)
{
    auto lines = std::vector<std::string>() = {
        "game Test start",
            "players 1"
            "card Parent start",
                "int health",
                "int attack",
            "end",
            "card Child Parent start",
                "health = 15",
                "int defence",
            "end",
        
            "visiblestack a",
            "visiblestack b",
            
            "setup start",
                "random Parent -> a top 20",
            "end",
        
            "turn start",
                "a /> b top 10",
            "end",
        
        "end"
    };
    
    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    
    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    
    p.RunTurn();
    
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 10);
}

TEST(ParserTest, CutTest)
{
    std::string line = "a /> b top 5";
    
    Battler::Program p;
    p._Parse({line});
    
    auto tokens = p._Tokens();
    EXPECT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, Battler::TokenType::name);
    EXPECT_EQ(tokens[1].type, Battler::TokenType::cut);
    EXPECT_EQ(tokens[2].type, Battler::TokenType::name);
    EXPECT_EQ(tokens[3].type, Battler::TokenType::name);
    EXPECT_EQ(tokens[4].type, Battler::TokenType::number);
}

TEST(ExpressionTest, CutTest)
{
    std::string line = "a /> b top 5";
    
    Battler::Program p;
    p._Parse({line});
    auto tokens = p._Tokens();
    auto tokensBegin = tokens.begin();
    Battler::Expression expr = Battler::GetExpression(tokensBegin, tokens.end());
    
    EXPECT_EQ(expr.children.size(), 2);
}

TEST(CompilerTest, CutTest)
{
    std::string line = "a /> b top 5";
    
    Battler::Program p;
    p.Compile({line});
    std::vector<Battler::Opcode> opcodes = p.opcodes();
}

TEST(EndToEndTests, ChooseCutTest)
{
    auto lines = std::vector<std::string>() = {
        "game Test start",
            "players 1"
            "card Parent start",
                "int health",
                "int attack",
            "end",
            "card Child Parent start",
                "health = 15",
                "int defence",
            "end",
        
            "visiblestack a",
            "visiblestack b",
            
            "setup start",
                "random Parent -> a top 20",
            "end",
        
            "turn start",
                "choose a /> b",
            "end",
        
        "end"
    };
    
    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    
    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    
    p.RunTurn();
    
    EXPECT_TRUE(p.m_waitingForUserInteraction);
    
    Battler::CardInputWait waiting = p.m_card_input_wait;
    EXPECT_EQ(waiting.type, Battler::InputOperationType::CUT);
    EXPECT_EQ(waiting.fixedDest, true);
    EXPECT_EQ(waiting.fixedSrc, true);
    EXPECT_EQ(waiting.srcStackID, 0);
    EXPECT_EQ(waiting.dstStackID, 1);
    EXPECT_EQ(waiting.dstTop, true);
    EXPECT_EQ(waiting.nExpected, 0);
    
    p.m_card_input_wait.cutPoint = 10;
    p.RunTurn(true);
    
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 10);
}

TEST(ParserTest, CutTestChoose)
{
    std::string line = "choose a /> b";
    
    Battler::Program p;
    p._Parse({line});
    
    auto tokens = p._Tokens();
    EXPECT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, Battler::TokenType::name);
    EXPECT_EQ(tokens[1].type, Battler::TokenType::name);
    EXPECT_EQ(tokens[2].type, Battler::TokenType::cut);
    EXPECT_EQ(tokens[3].type, Battler::TokenType::name);
}

TEST(ExpressionTest, CutTestChoose)
{
    std::string line = "choose a /> b";
    
    Battler::Program p;
    p._Parse({line});
    auto tokens = p._Tokens();
    auto tokensBegin = tokens.begin();
    Battler::Expression expr = Battler::GetExpression(tokensBegin, tokens.end());
    
    EXPECT_EQ(expr.children.size(), 2);
    EXPECT_EQ(expr.children[1].children.size(), 1);
    EXPECT_EQ(expr.children[1].type, Battler::ExpressionType::STACK_CUT_SOURCE_TOP);
}

TEST(CompilerTest, CutTestChoose)
{
    std::string line = "choose a /> b";
    
    Battler::Program p;
    p.Compile({line});
    std::vector<Battler::Opcode> opcodes = p.opcodes();
    // TODO: this test isn't tesing anything
}

TEST(ExpressionTest, MoveFromSelection)
{
    std::string line = "a,b -> c top 1";

    Battler::Program p;
    p._Parse({ line });
    auto tokens = p._Tokens();
    auto tokensBegin = tokens.begin();
    Battler::Expression expr = Battler::GetExpression(tokensBegin, tokens.end());

    EXPECT_EQ(expr.children.size(), 2);
    EXPECT_EQ(expr.children[0].children.size(), 0);
    EXPECT_EQ(expr.children[0].type, Battler::ExpressionType::STACK_MOVE_SOURCE_MULTI);
    EXPECT_EQ(expr.children[1].children.size(), 2);
    EXPECT_EQ(expr.children[1].children[0].type, Battler::ExpressionType::IDENTIFIER);
    EXPECT_EQ(expr.children[1].children[1].type, Battler::ExpressionType::FACTOR);
}

TEST(EndToEndTests, MoveFromSelection)
{
    auto lines = std::vector<std::string>() = {
    "game Test start",
        "players 1"
        "card Parent start",
            "int health",
            "int attack",
        "end",
        "card Child Parent start",
            "health = 15",
            "int defence",
        "end",

        "visiblestack a",
        "visiblestack b",
        "visiblestack c",

        "setup start",
            "random Parent -> a top 20",
            "random Parent -> b top 20",
            "random Parent -> c top 1"
        "end",

        "turn start",
            "a,b -> c top 1",
            "a,b ->_ c bottom 2",
        "end",

    "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    p.RunTurn();

    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_TRUE(p.m_card_input_wait.srcTop);
    EXPECT_EQ(p.m_card_input_wait.type, Battler::InputOperationType::CHOOSE_SOURCE);
    EXPECT_EQ(p.m_card_input_wait.sourceStackSelectionPool.size(), 2);
    EXPECT_EQ(p.m_card_input_wait.sourceStackSelectionPool[0], 0);
    EXPECT_EQ(p.m_card_input_wait.sourceStackSelectionPool[1], 1);
    p.m_card_input_wait.srcStackID = 1;

    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 1);
    Battler::Card cardToMove = *(p.game().stacks[1].cards.end()-1);
    p.RunTurn(true);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 19);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 2);
    EXPECT_EQ((p.game().stacks[2].cards.end()-1)->UUID, cardToMove.UUID);

    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_FALSE(p.m_card_input_wait.srcTop);
    EXPECT_EQ(p.m_card_input_wait.type, Battler::InputOperationType::CHOOSE_SOURCE);
    EXPECT_EQ(p.m_card_input_wait.sourceStackSelectionPool.size(), 2);
    EXPECT_EQ(p.m_card_input_wait.sourceStackSelectionPool[0], 0);
    EXPECT_EQ(p.m_card_input_wait.sourceStackSelectionPool[1], 1);
    p.m_card_input_wait.srcStackID = 0;
    Battler::Card newCardToBeOnTheBottom = p.game().stacks[0].cards[1];
    p.RunTurn(true);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 18);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 19);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 4);
    EXPECT_EQ(p.game().stacks[2].cards.begin()->UUID, newCardToBeOnTheBottom.UUID);
}
