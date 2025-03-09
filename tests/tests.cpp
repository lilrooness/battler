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

TEST(CompilerTest, Compare_stackPosref_Card)
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

TEST(CompilerTest, ResolveCardAttributesFromStackPosition)
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

TEST(CompilerTest, LessThan)
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

TEST(CompilerTest, greatherThan)
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

TEST(CompilerTest, DeclareLooser)
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


TEST(CompilerTest, BracketExpression)
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

TEST(CompilerTest, TransferNamedCard)
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

TEST(CompilerTest, if_ifelse)
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

TEST(CompilerTest, if_ifelse_else)
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

TEST(CompilerTest, if_ifelse_NoMatchingBlock)
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