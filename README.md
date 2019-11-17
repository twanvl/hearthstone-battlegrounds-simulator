Hearthstone Battlegrounds Battle Simulator
-----------------------------------------

A simulator for battles in the HS battlegrounds.
This program can quickly run over a battle many times, and give statistics on the results

Example output:

    Turn 8
    * 4/6 Cave Hydra
    * 8/2 Kaboom Bot
    * 10/4 Kaboom Bot
    * 11/6 Nightmare Amalgam
    * 2/2 Lightfang Enforcer
    * 4/6 Imp Gang Boss
    VS
    * 2/2 Kaboom Bot
    * 2/2 Kaboom Bot
    * 6/3 Cobalt Guardian, divine shield
    * 2/6 Security Rover
    * 4/2 Micro Machine
    * 1/5 Junkbot
    * 5/6 Psych-o-Tron, taunt, divine shield
    --------------------------------
    win: 2%, tie: 3%, lose: 94%
    mean score: -6.573, median score: -7
    percentiles: -14 -11 -9 -8 -8 -7 -6 -5 -4 -3 12
    actual outcome: -9, is at the 20-th percentile

This corresponds to the following board state:
![Example game](github_resources/run1-turn8.png)  
taken from the game at https://www.youtube.com/watch?v=TV0HSwbhasQ,

The score at the end is the number of stars of the remanining minions of the first player, or negative the stars of the second player.
So a positive score means the first player wins by that many stars, a negative score means that the first player loses.
This score corresponds to damage dealt or taken, excluding damage from the character's level.
The program reports mean and median of the scores, and the 0%, 10%, .., 100% percentiles

Usage
----

    hsbg run.txt

The input file consists of a series of commands to define the board state, and looks very similar to the output shown above.
See [examples/run1.txt](examples/run1.txt).

The program can also be used in interactive mode, by starting it without any arguments. Type `help` to get a list of commands:

    -- Defining the board
    board      = begin defining player board
    vs         = begin defining opposing board
    * <minion> = give the next minion
    HP <hero>  = tell that a hero power is used

    -- Running simulations
    actual <i> = tell about actual outcome (used in simulation display)
    run (<n>)  = run n simulations, report statistics (default: 1000)

    -- Stepping through a single battle
    show       = show the board state
    reset      = reset battle
    step       = do 1 attack step, or start if battle not started yet
    trace      = do steps until the battle ends
    back       = step backward. can be used to re-roll RNG
    
    -- Other
    info <msg> = show a message
    help       = show this help message
    quit       = quit the simulator
    
    -- Minion format
    Minions are specified as
      [attack/health] [golden] <name>, <buff>, <buff>, ..
    for example
     * 10/12 Nightmare Amalgam
     * Golden Murloc Tidecaller, poisonous, divine shield, taunt, windfury, +12 attack

A better user interface is a work in progress.

FAQ
----

Q: How do I put in a board state  
A: Currently in C++ code, see test.cpp.
The dream is to eventually make an image recognizer to do this automatically, but that might not be worth the effort.

Q: What about Bob's tavern?  
A: Currently only actual battles are simulated, the program doesn't know about buying, selling, leveling etc.

Q: What can I do with this?  
A: 
* You can see how lucky you are
* You can learn to better position your minions
* (future) you can see how well your board is expected to do at a certain turn of the game

Q: Known bugs  
A:
* There might be subtle differences in the order of triggers etc.

