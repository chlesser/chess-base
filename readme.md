# General
This is a chess application developed for Graeme's CMPM123 Advanced Programming class. It is developed using visual studio code on a windows machine, programmed in c++ and utilizing the dearIMGUI library. Absolutely no generative AI was used in this course.

## Game Setup Implementation
### FEN String Implementation
The FEN string implementation was simple to add. It loops through the string, and adds the corresponding piece to both the state string and the physical game.

### King, Knight, Pawn Implementation
Lots of this implementation was inspired by the lectures from CMPM123. It utilizes bitboards for the application to run much quicker when minimax is implemented. All of the king and knight's possible positions are stored in a bitboard that is accessed by index at the end of the previous turn. Pawns work differently, their possible moves being selected via mass shifts at the end of the previous turn. There is no bitboard for them to store themselves in. Bitshifting is used with great frequency.

### Bishop, Rook, Queen Implementation
Once again, lots of this implementation was inspired by Graeme's lectures. The magic bitboard class was incredibly interesting, utilizing what appears to be as close to magic as we can get in the modern world. It is quite impressive nonetheless! What followed was a relatively straightforward implementation, mimicking my previous style.

### AI Setup
I am including this as a seperate section due to what I did. I refactored generating moves to completely remove branching statements, as Graeme did in lecture. However, I still don't know if I am tracing player color correctly. Integration with established code is difficult to work around, as sometimes it is 1/-1 for color, or 0/1 for sprite visualization, or -1/1 for ai calculation. While I am frustrated, the program is working properly. I also switched my bitboards into a global enumerator, at Graeme's suggestion.

### AI Implementation
negamax continues to prove simple, and yet every time I mess it up slightly. Not much to be said here, as the implementation of these functions was relatively smooth after I had setup everything in the previous section. I will make a note of passing in WHITE rather than HUMAN_PLAYER or AI_PLAYER to the initial call. This threw me for a loop for a while, but I elected to use its current state as opposed to one of the system's player determinators. Then, I cross referenced Graeme's document and saw the same result. Delightful all around. Alpha Beta pruning is as magical as ever, though it doesn't do too much without iterative deepening. It can consistently go to depth 4 with little thinking time, and I have not tested release mode.

### Evaluation function
The evaluation function simply goes off of piece value as of right now. Playing it, I can only beat it 60% of the time, which is a terrifying thought of what is to come. In the next iteration, I hope to implement a more appropriate scoring metric using piece square tables.

### To-Do
The last things to do on this assignment are:
- Iterative Deepening
- Piece Square Tables
- Castling
- En passant
- Pawn Promotion