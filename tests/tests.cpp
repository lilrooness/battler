#include <gtest/gtest.h>
#include <vector>
#include <string>

#include "../Compiler.h"
#include "../expression.h"
#include "../vm/game.h"
#include "../interpreter_errors.h"

TEST(BasicGame, EndToEndTests)
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


TEST(ChooseMoveInstr, EndToEndTests)
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
