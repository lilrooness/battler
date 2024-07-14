# Battler

## A language for card games

Battler is an interpreter and card-game engine, an example of a Snap game can 
be seen below.

- [x] PlainText to AST parser
- [x] AST to bytecode compiler
- [x] Bytecode interpreter can run test game file
- [x] Interpreter runnable from command line
- [ ] Full Bytecode interpreter implementation
- [ ] Library published
- [ ] Godot Plugin
- [ ] Python Plugin

### Building this project
Use this project's simple CMakeLists.txt to build it, although it doesn't link
with anything except for the C++ standard lib

### Running a script
Run the interpreter against a game file:
`.\Battler.exe game_file.txt`


### Eve Online Snap Example Game

```
# this game is an implementation of Snap, but with ships
# from Eve Online

# it's a lot bigger than it has to be

game TestGame start

    visiblestack InPlay
    
    # a hidden stack
    hiddenstack Draw

    # number of players
    

    # stacks can have member attributes
    int Draw.x

    # member attributes can even be other stacks!
    privatestack Draw.h

    # and they can have attributes too!
    int Draw.h.y
    int Draw.h.a
    int Draw.h.b
    
    players 2

    Draw.h.b = 6

    int someVarName
    someVarName = 5*5
    
    # cards!
    card Ship start
        # card attributes
        int shield
        int armour
        int hull
        int attack
    end

    # child cards
    card Atron Ship start
        shield = 300 + 1
        armour = 350 * 2 - 1
        hull = 400 + 2 * 3
        attack = 50 + 60 * 6 - 5 / 4
    end
    
    # you dont have to set the attributes
    card Kestral Ship start end
    card Rifter Ship start end
    card Apocolypse Ship start end
    card Comet Ship start end

    # move random cards into a stack
    random Ship -> Draw top 100

    # move cards between stacks
    Draw -> InPlay top 2


    # a setup block, run once at the beggining of the game
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
        currentPlayer.Hand -> InPlay top 1

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
Compiling game file
Loading Compiled Game
Running game setup
The winner is 0
```
