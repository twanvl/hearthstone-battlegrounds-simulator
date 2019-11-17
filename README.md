Hearthstone Battlegrounds Battle Simulator
-----------------------------------------

A simulator for battles in the HS battlegrounds.
This program can quickly run over a battle many times, and give statistics on the results

Example output:

    2/2 Lightfang Enforcer (*****)
    24/26 Cave Hydra (****)
    4/4 Managerie Magician (****)
    2/4 Brann Bronzebeard (*****)
    36/31 [TD] Nightmare Amalgam (**)
    16/18 Imp Gang Boss (***)
    39/27 [TD] Golden Kaboom Bot (**)
      VS
    7/10 [T] Foe Reaper 4000 (******)
    36/31 [TP] Golden Nightmare Amalgam (**)
    7/7 Golden Kindly Grandmother (**)
    5/2 Rat Pack (**)
    7/7 Rat Pack (**)
    5/5 Pre-nerf Mama Bear (******)
    6/10 [T] Golden Vulgar Homunculus (*)
    ----------------------------------
    win: 865, tie: 0, lose: 135
    mean score: 8.313
    median score: 10
    percentiles: -11 -3 5 9 9 10 11 11 12 15 20
    ----------------------------------

The score at the end is the number of stars of the remanining minions of the first player, or negative the stars of the second player. This corresponds to damage dealt or negative damage taken, excluding damage from the character's level.

The marks after the minion names indicate abilities (T for taunt, D for divine shield, P for poisonous). The *s indicate the amount of stars a minion is worth.

A better user interface is a work in progress.
