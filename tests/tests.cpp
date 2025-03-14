#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>

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
                "health = 15 * 1 + 5",
                "int defence",
            "end",
        
            "visiblestack a",
            "visiblestack b",
            
            "setup start",
                "random Parent -> a 1",
            "end",
        
            "turn start",
                "a -> b top 1",
                "random Parent ->_ b 1",
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
    EXPECT_EQ(child.attributes.Get("health").i, 90); // this is going to be wrong until we get operator presidence working
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
                "random Parent -> a 1",
            "end",
        
            "turn start",
                "choose a -> b 1",
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
                "random Parent -> a 20",
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
                "random Parent -> a 20",
            "end",
        
            "turn start",
                "choose a /> b top",
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
    
    Battler::StackTransferStateTracker waiting = p.m_stackTransferStateTracker;
    EXPECT_EQ(waiting.type, Battler::InputOperationType::CHOOSE_CARDS_FROM_SOURCE);
    EXPECT_EQ(waiting.transferType, Battler::StackTransferType::CUT);
    EXPECT_EQ(waiting.fixedDest, true);
    EXPECT_EQ(waiting.fixedSrc, true);
    EXPECT_EQ(waiting.srcStackID, 0);
    EXPECT_EQ(waiting.nExpected, -1);
    
    p.m_stackTransferStateTracker.cutPoint = 10;
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

TEST(CompilerTest, CutTestChoose)
{
    std::string line = "choose a /> b top";
    
    Battler::Program p;
    p.Compile({line});
    std::vector<Battler::Opcode> opcodes = p.opcodes();
    // TODO: this test isn't tesing anything
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
            "random Parent -> a 20",
            "random Parent -> b 20",
            "random Parent -> c 1"
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
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_SOURCE);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool.size(), 2);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[0], 0);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[1], 1);
    p.m_stackTransferStateTracker.srcStackID = 1;
    p.m_waitingForUserInteraction = false;

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
    EXPECT_FALSE(p.m_stackTransferStateTracker.srcTop);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_SOURCE);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool.size(), 2);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[0], 0);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[1], 1);
    p.m_stackTransferStateTracker.srcStackID = 0;
    Battler::Card newCardToBeOnTheBottom = p.game().stacks[0].cards[1];
    p.RunTurn(true);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 18);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 19);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 4);
    EXPECT_EQ(p.game().stacks[2].cards.begin()->UUID, newCardToBeOnTheBottom.UUID);
}

TEST(EndToEndTests, MoveToSelection)
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
                    "random Parent -> a 20",
                "end",

                "turn start",
                    "a -> b,c top 1",
                    "a ->_ c,b bottom 1",
                "end",
            "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    p.RunTurn();

    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_DESTINATION);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool.size(), 2);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[0], 1);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[1], 2);
    p.m_stackTransferStateTracker.dstStackID = 1;
    p.m_waitingForUserInteraction = false;

    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 0);
    Battler::Card cardToMove = *(p.game().stacks[0].cards.end()-1);
    p.RunTurn(true);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 19);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 1);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 0);
    EXPECT_EQ((p.game().stacks[1].cards.end()-1)->UUID, cardToMove.UUID);

    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_DESTINATION);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool.size(), 2);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[0], 2);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[1], 1);
    p.m_stackTransferStateTracker.dstStackID = 2;
    Battler::Card newCardToBeOnTheBottom = p.game().stacks[0].cards[0];
    p.RunTurn(true);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 18);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 1);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 1);
    EXPECT_EQ(p.game().stacks[2].cards.begin()->UUID, newCardToBeOnTheBottom.UUID);
    EXPECT_FALSE(p.m_waitingForUserInteraction);
}

TEST(EndToEndTests, MoveFromAndToSelection)
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
            "random Parent -> a 20",
            "end",

            "turn start",
            "a,b,c -> a,b,c top 1",
            "end",
            "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    p.RunTurn();

    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_SOURCE);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool.size(), 3);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[0], 0);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[1], 1);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[2], 2);
    p.m_stackTransferStateTracker.srcStackID = 0;
    p.m_waitingForUserInteraction = false;

    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 0);
    Battler::Card cardToMove = *(p.game().stacks[0].cards.end()-1);
    p.RunTurn(true);


    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_DESTINATION);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool.size(), 3);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[0], 0);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[1], 1);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[2], 2);
    p.m_stackTransferStateTracker.dstStackID = 2;

    p.RunTurn(true);

    EXPECT_EQ(p.game().stacks[0].cards.size(), 19);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[2].cards.size(), 1);
    EXPECT_EQ((p.game().stacks[2].cards.end()-1)->UUID, cardToMove.UUID);
    EXPECT_FALSE(p.m_waitingForUserInteraction);
}

TEST(VMTtest, Compare_stackPosref_Card)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "card P start end",
            "card C P start end",
            "visiblestack a",
            "random P -> a 10",
            "if a.top == C start",
                "random P -> a 10",
                "random P -> a 10",
                "random P -> a 10",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks.begin()->second.cards.size(), 40);
}

TEST(VMTtest, ResolveCardAttributesFromStackPosition)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "card P start",
                "int x",
            "end",
            "card A P start",
                "x = 0",
            "end",
            "visiblestack a",
            "random P -> a 10",
            "if a.top.x == 0 start",
                "random P -> a 10",
                "random P -> a 10",
                "random P -> a 10",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks.begin()->second.cards.size(), 40);
}

TEST(VMTtest, LessThan)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "card P start",
                "int x",
            "end",
            "card A P start",
                "x = 10",
            "end",
            "visiblestack a",
            "random P -> a 10",
            "if a.top.x < 20 start",
                "random P -> a 10",
                "random P -> a 10",
                "random P -> a 10",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks.begin()->second.cards.size(), 40);
}

TEST(VMTtest, greatherThan)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "card P start",
                "int x",
            "end",
            "card A P start",
                "x = 10",
            "end",
            "visiblestack a",
            "random P -> a 10",
            "if a.top.x > 5 start",
                "random P -> a 10",
                "random P -> a 10",
                "random P -> a 10",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks.begin()->second.cards.size(), 40);
}

TEST(VMTtest, DeclareLooser)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "setup start end",
            "turn start",
                "looseris currentPlayer",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    p.RunTurn();
    EXPECT_EQ(p.game().winner, 1000000000);
}


TEST(VMTtest, BracketExpression)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "card P1 start",
                "int x",
            "end",
            "card A P1 start",
                "x = 20",
            "end",
            "card P2 start",
                "int x",
            "end",
            "card b P2 start",
                "x = 20",
            "end",
            "visiblestack a",
            "random P1 -> a 1",
            "random P2 -> a 1",

            "if a.top.x > (a.top-1).x - 1 start",
                "random P1 -> a 10",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks.begin()->second.cards.size(), 12);
}

TEST(VMTtest, TransferNamedCard)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "place A -> a 10"
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
    EXPECT_EQ(p.game().stacks[0].cards[0].ID, 0);
}

TEST(VMTtest, if_ifelse)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "card B start end",
            "place A -> a 1",
            "if a.top == B start",
                "place B -> a 19",
            "elseif a.top == A then",
                "place A -> a 9",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
    EXPECT_EQ(p.game().stacks[0].cards[0].ID, 0);
}

TEST(VMTtest, if_ifelse_else)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "card B start end",
            "card C start end",
            "place C -> a 1",
            "if a.top == B start",
                "place B -> a 19",
                "if a.top == B start end",
            "elseif a.top == A then",
                "place A -> a 9",
            "else",
                "place C -> a 9",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
    EXPECT_EQ(p.game().stacks[0].cards[0].ID, 2);
}

TEST(VMTtest, if_ifelse_NoMatchingBlock)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "card B start end",
            "card C start end",
            "place C -> a 1",
            "if a.top == B start",
                "place B -> a 19",
                "if a.top == B start end",
            "elseif a.top == A then",
                "place A -> a 9",
            "end",
            "place C -> a 1",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 2);
    EXPECT_EQ(p.game().stacks[0].cards[0].ID, 2);
}

TEST(VMTest, if_else)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "card B start end",
            "place B -> a 1",
            "if a.top == B start",
                "place B -> a 19",
            "else",
                "place A -> a 9",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[0].cards[0].ID, 1);
}

TEST(VMTtest, test_ifelseblock_insideTurn)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "card B start end",

            "setup start",
                "place B -> a 1",
            "end",

            "turn start",
                "if a.top == B start",
                    "place B -> a 19",
                "else",
                    "place A -> a 9",
                "end",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    p.RunSetup();
    p.RunTurn();

    EXPECT_EQ(p.game().stacks[0].cards.size(), 20);
    EXPECT_EQ(p.game().stacks[0].cards[0].ID, 1);
}

TEST(VMTtest, transferFromEmptyStack)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "visiblestack b",

            "a -> b top 1",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 0);
}

TEST(VMTtest, testCardFromEmptyStack)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "visiblestack b",

            "if a.bottom == b.bottom start end",
            "if a.top == b.top start end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
    EXPECT_EQ(p.game().stacks[0].cards.size(), 0);
}

TEST(VMTtest, compareTwoStackPositionReferences)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "place A -> a 3",

            "if a.top == (a.top-2) start",
                "place A -> a 2",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 5);
}

TEST(VMTest, nesetedIfElseInIfElse)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "card C start end",
            "card One C start end",
            "card Two C start end",

            "hiddenstack a",
            "visiblestack b",

            // "random C -> a 50",
            "place Two -> b 5",
            "place Two -> a 50",

            "turn start",
                "if a.top == Two start",
                    "if b.size > 2 start",
                        "a -> b top 1",
                    "elseif b.size == 100 then",
                        "a -> b top 10",
                    "else",
                        "a -> b top 100",
                    "end",
                "elseif a.top == One then",
                    "a -> b top 1000",
                "end",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run(true);
    EXPECT_EQ(p.RunTurn(), 0);
    EXPECT_EQ(p.game().stacks[1].cards.size(), 6);
}

TEST(VMTtest, TestExactSequence)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "card A start end",
            "card B start end",
            "card C start end",
            "card D start end",

            "place D -> a 1",
            "place C -> a 1",
            "place B -> a 1",
            "place A -> a 1",

            "if a == [A B C D] start",
                "place A -> a 2",
            "end",
        "end"
    };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 6);
}

TEST(VMTtest, testStartSequence)
{
    auto lines = std::vector<std::string>() =
     {
         "game test start",
             "visiblestack a",
             "card A start end",
             "card B start end",
             "card C start end",
             "card D start end",

             "place D -> a 5",
             "place C -> a 1",
             "place B -> a 1",
             "place A -> a 1",

             "if a == [A B C D :] start",
                "place A -> a 2",
             "end",
         "end"
     };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
}

TEST(VMTtest, testWildcardSequence)
{
    auto lines = std::vector<std::string>() =
     {
         "game test start",
             "visiblestack a",
             "card A start end",
             "card B start end",
             "card C start end",
             "card D start end",

             "place D -> a 5",
             "place A -> a 1",
             "place B -> a 1",
             "place A -> a 1",

             "if a == [A B _ D :] start",
                "place A -> a 2",
             "end",
         "end"
     };

    Battler::Program p;
    p.Compile(lines);
    p.Run();
    EXPECT_EQ(p.game().stacks[0].cards.size(), 10);
}

TEST(GameTest, MatchesSequence)
{
    Battler::Stack s;
    Battler::Card c1;
    c1.ID = 1;

    Battler::Card c2;
    c2.ID = 2;

    Battler::Card c3;
    c3.ID = 3;

    Battler::Card c4;
    c4.ID = 4;

    Battler::Card c5;
    c5.ID = 5;

    Battler::Card c6;
    c6.ID = 6;

    Battler::Card c7;
    c7.ID = 7;

    s.cards = {c1, c2, c3, c4, c5, c6, c7};

    // we want C1 to be at the top of the deck, which is the end of the vector
    std::reverse(s.cards.begin(), s.cards.end());

    // matches awkward sequence [: 1 2 3 : 6 _]
    std::vector<Battler::CardMatcher> sequence1 = {
        {Battler::CardMatcherType::REST, 0},
        {Battler::CardMatcherType::ID, 1},
        {Battler::CardMatcherType::ID, 2},
        {Battler::CardMatcherType::ID, 3},
        {Battler::CardMatcherType::REST, 0},
        {Battler::CardMatcherType::ID, 6},
        {Battler::CardMatcherType::ANY, 6}
    };
    EXPECT_TRUE(s.MatchesSequence(sequence1));

    // does not match awkward sequence [: 1 2 3 : 7 6]
    std::vector<Battler::CardMatcher> sequence2 = {
        {Battler::CardMatcherType::REST, 0},
        {Battler::CardMatcherType::ID, 1},
        {Battler::CardMatcherType::ID, 2},
        {Battler::CardMatcherType::ID, 3},
        {Battler::CardMatcherType::REST, 0},
        {Battler::CardMatcherType::ID, 7},
        {Battler::CardMatcherType::ANY, 6}
    };
    EXPECT_FALSE(s.MatchesSequence(sequence2));

    // [: 1 :] stack contains c1
    std::vector<Battler::CardMatcher> sequence3 = {
        {Battler::CardMatcherType::REST, 0},
        {Battler::CardMatcherType::ID, 1},
        {Battler::CardMatcherType::REST, 0}
    };
    EXPECT_TRUE(s.MatchesSequence(sequence3));

    // [: 10 :] stack does not contain c10 (c10 does not exist)
    std::vector<Battler::CardMatcher> sequence4 = {
        {Battler::CardMatcherType::REST, 0},
        {Battler::CardMatcherType::ID, 10},
        {Battler::CardMatcherType::REST, 0}
    };
    EXPECT_FALSE(s.MatchesSequence(sequence4));

    // matches [1 2 3 4 5 6 7 :]
    std::vector<Battler::CardMatcher> sequence5 = {
        {Battler::CardMatcherType::ID, 1},
        {Battler::CardMatcherType::ID, 2},
        {Battler::CardMatcherType::ID, 3},
        {Battler::CardMatcherType::ID, 4},
        {Battler::CardMatcherType::ID, 5},
        {Battler::CardMatcherType::ID, 6},
        {Battler::CardMatcherType::ID, 7},
        {Battler::CardMatcherType::REST, 0},
    };
    EXPECT_TRUE(s.MatchesSequence(sequence5));

}

TEST(VMTtest, testStackTransferWhereClause)
{
    auto lines = std::vector<std::string>() =
    {
        "game test start",
            "visiblestack a",
            "visiblestack b",
            "visiblestack c",
            "visiblestack d",

            "card A start end",
            "card B start end",

            "place A -> b 2",
            "place B -> d 3",

            "setup start end",

            "turn start",
                "a,b -> c,d top 1 where from{from == [A A]}, to{to == [B :]}",
            "end",
        "end"
    };
    Battler::Program p;
    p.Compile(lines);
    EXPECT_EQ(p.Run(true), 0);
    int WAITING_FOR_INTERACTION_RETURN = 1;
    EXPECT_EQ(p.RunTurn(), WAITING_FOR_INTERACTION_RETURN);

    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_SOURCE);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool.size(), 1);
    EXPECT_EQ(p.m_stackTransferStateTracker.sourceStackSelectionPool[0], 1);
    p.m_stackTransferStateTracker.srcStackID = 1;
    p.m_waitingForUserInteraction = false;

    EXPECT_EQ(p.RunTurn(true), WAITING_FOR_INTERACTION_RETURN);
    EXPECT_TRUE(p.m_waitingForUserInteraction);
    EXPECT_EQ(p.m_stackTransferStateTracker.type, Battler::InputOperationType::CHOOSE_DESTINATION);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool.size(), 1);
    EXPECT_EQ(p.m_stackTransferStateTracker.destinationStackSelectionPool[0], 3);
    p.m_stackTransferStateTracker.dstStackID = 3;
    p.m_waitingForUserInteraction = false;
    EXPECT_EQ(p.RunTurn(true), 0);

    EXPECT_EQ(p.game().stacks[3].cards.size(), 4);

}

