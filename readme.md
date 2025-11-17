# General
This is a chess application developed for Graeme's CMPM123 Advanced Programming class. It is developed using visual studio code on a windows machine, programmed in c++ and utilizing the dearIMGUI library. Absolutely no generative AI was used in this course.

## Game Setup Implementation
### FEN String Implementation
The FEN string implementation was simple to add. It loops through the string, and adds the corresponding piece to both the state string and the physical game.

### King, Knight, Pawn Implementation
Lots of this implementation was inspired by the lectures from CMPM123. It utilizes bitboards for the application to run much quicker when minimax is implemented. All of the king and knight's possible positions are stored in a bitboard that is accessed by index at the end of the previous turn. Pawns work differently, their possible moves being selected via mass shifts at the end of the previous turn. There is no bitboard for them to store themselves in. Bitshifting is used with great frequency.