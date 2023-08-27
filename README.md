# Battler

### A language for card games

Battler is an card-game interpreter, an example of a "color snap" game can 
be seen below.

Use this project's simple CMakeLists.txt to build it, although it doesn't link
with anything except for the C++ standard lib

Run the interpreter against a game file:
`.\Battler.exe game_file.txt`

if you try to run the `game_file.txt` currently, you may get a "not implemented yet" error like this one:

```
Error: unexpected token on line 35
    phase DrawPhase start
____^

Reason: phase declarations are not implemented yet ... :(
```

the current game_file.txt is an experiment and doesn't really make sense as a game. Some Eve Ships are in there, but only because that's what was in my head at the time :)


### Color Snap Example Game

```
game ColorSnap start

    visiblestack InPlay
    hiddenstack Draw

    players 2

    card Color start
    end

    card red Color start end
    card yellow Color start end
    card green Color start end
    card blue Color start end
    card circle Color start end

    setup start

        foreach player p start
            privatestack p.Hand
            
            random Color -> Draw top 10
            Draw -> p.Hand top 3
        end
    end

    phase DrawPhase start

        Draw -> me.hand top 1
    end

    phase Place start

        onplace InPlay c start
            if c == InPlay.top-1 start
                winneris me
            end
        end

        me.hand -> InPlay choose 1
    end

    turn start
        do Draw
        do Place
    end

end

```