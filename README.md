# Battler

### A language for card games

Battler is an interpreter and card-game engine, an example of a Snap game can 
be seen below.

The games currently run automatically without any player interaction, but that is just over the horizon after I move to
a bytecode implementation. It currently uses a tree-walk, which is hella slow.

Use this project's simple CMakeLists.txt to build it, although it doesn't link
with anything except for the C++ standard lib

Run the interpreter against a game file:
`.\Battler.exe game_file.txt`


### Eve Online Snap Example Game

```
# this game is an implementation of Snap, but with ships from Eve Online

game TestGame start

    # number of players
    players 2

    visiblestack InPlay
    
    # a hidden stack
    hiddenstack Draw
    
    # Now for some cards
    card Ship start
        # card attributes, we don't use any of these attributes
        int shield
        int armour
        int hull
        int attack
    end

    # child cards
    card Atron Ship start end
    card Kestral Ship start end
    card Rifter Ship start end
    card Apocolypse Ship start end
    card Comet Ship start end

    # move random cards of type Ship into a stack
    random Ship -> Draw top 100

    # move cards between stacks
    Draw -> InPlay top 2

    # a setup block, run once at the start of the game
    setup start
        foreachplayer p start
            privatestack p.Hand
            
            random Ship -> Draw top 10
            Draw ->_ p.Hand bottom 3
        end
    end

    # a phase declaration, like a function
    phase DrawPhase start
        Draw -> currentPlayer.Hand top 1
    end

    phase Place start
        # currentPlayer is set while a turn is executing
        currentPlayer.Hand -> InPlay top 1

        # Draw -> InPlay top 1

        # compare cards from positions in stacks
        if InPlay.top == InPlay.top-1 start
            # declare a wiener
            winneris currentPlayer
        end
    end

    # runs every turn
    turn start
        # run a phase
        do DrawPhase
        do Place
    end
end


```


If you run the above game in the interpreter, you will get something like this output, showing it executing each turn automatically until ther is a winner
```
running turn
turn 0 player 0
turn 1 player 1
turn 2 player 0
turn 3 player 1
turn 4 player 0
turn 5 player 1
turn 6 player 0
turn 7 player 1
turn 8 player 0
turn 9 player 1
the winner is player 1
```
