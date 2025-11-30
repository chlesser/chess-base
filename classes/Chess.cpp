#include "Chess.h"
#include "../Application.h"
#include "./MagicBitboards.h"
#include <limits>
#include <cmath>
#include <map>

#define WHITE 1
#define BLACK -1

Chess::Chess()
{
    _grid = new Grid(8, 8);

    initMagicBitboards();
    for(int i = 0; i < 64; i++) {
        knightBoards[i] = generateKnightMoveBitboard(i);
        kingBoards[i] = generateKingMoveBitboard(i);
    }
    for(int i = 0; i < 128; i++) {bitboardLookup[i] = 0;}

    bitboardLookup['P'] = WHITE_PAWNS;
    bitboardLookup['N'] = WHITE_KNIGHTS;
    bitboardLookup['B'] = WHITE_BISHOPS;
    bitboardLookup['R'] = WHITE_ROOKS;
    bitboardLookup['Q'] = WHITE_QUEENS;
    bitboardLookup['K'] = WHITE_KING;
    bitboardLookup['p'] = BLACK_PAWNS;
    bitboardLookup['n'] = BLACK_KNIGHTS;
    bitboardLookup['b'] = BLACK_BISHOPS;
    bitboardLookup['r'] = BLACK_ROOKS;
    bitboardLookup['q'] = BLACK_QUEENS;
    bitboardLookup['k'] = BLACK_KING;
    bitboardLookup['0'] = EMPTY_SQUARES;

}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    _currentPlayer = WHITE;
    _moves = generateAllMoves(stateString(), _currentPlayer);

    if(gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }
    startGame();

}
void Chess::endTurn()
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        square->setHighlighted(false);
    });
    _gameOptions.currentTurnNo++;
    _currentPlayer = -_currentPlayer;
    _moves = generateAllMoves(stateString(), _currentPlayer);
	std::string startState = stateString();
	Turn *turn = new Turn;
	turn->_boardState = stateString();
	turn->_date = (int)_gameOptions.currentTurnNo;
	turn->_score = _gameOptions.score;
	turn->_gameNumber = _gameOptions.gameNumber;
	_turns.push_back(turn);
    ClassGame::EndOfTurn();
}

void Chess::FENtoBoard(const std::string& fen) {
    const std::unordered_map <char, int> pieceCodes = {
        {'P', 1}, {'N', 2}, {'B', 3},
        {'R', 4}, {'Q', 5}, {'K', 6}
    };
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
    int x = 0;
    int y = 0;
    for(int i = 0; i < fen.length(); i++) {
        const char current = fen[i];
        if(current == 47) {x = 0; y++;} //break
        else if(current >= 49 && current <= 57) //empty numbers
            x += current - 48;
        else if(current >= 66 && current <= 82) { //lowercase letters
            ChessPiece newPiece = ChessPiece(pieceCodes.at(current));
            Bit* piece = PieceForPlayer(1, newPiece);
            ChessSquare* square = _grid->getSquare(x, y);
            piece->setPosition(square->getPosition());
            square->setBit(piece);
            piece->setGameTag(newPiece + 128);
            x++;
        } else if (current >= 98 && current <= 122) { //uppercase numbers
            char newCode = fen[i] - 32; //converting to our standard number
            ChessPiece newPiece = ChessPiece(pieceCodes.at(newCode));
            Bit* piece = PieceForPlayer(0, newPiece);
            ChessSquare* square = _grid->getSquare(x, y);
            piece->setPosition(square->getPosition());
            square->setBit(piece);
            piece->setGameTag(newPiece);
            x++;
        }
        if(x == 8 && y == 7)
            break;
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    bool spacesAvailable = false;
    if (pieceColor == currentPlayer) {
        ChessSquare* square = (ChessSquare *)&src;
        int squareIndex = square->getSquareIndex();
        for(auto move : _moves) {
            if(move.from == squareIndex) {
                spacesAvailable = true;
                auto dest = _grid->getSquareByIndex(move.to);
                dest->setHighlighted(true);
            }
        }
    }
    return spacesAvailable;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* dstSquare = (ChessSquare *)&dst;
    ChessSquare* srcSquare = (ChessSquare *)&src;

    if (!srcSquare || !dstSquare) return false;

    int dstIndex = dstSquare->getSquareIndex();
    int srcIndex = srcSquare->getSquareIndex();
    for(auto move : _moves) {
        if(move.to == dstIndex && move.from == srcIndex) {
            return true;
        }
    }
    return false;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

std::vector<BitMove> Chess::generateAllMoves(const std::string& state, int playerColor)
{
    //std::cout <<"I think I am " << (playerColor == 1 ? "White!" : "Black!") << std::endl;
    // for(auto move : moves) {
    //     delete move;
    // }
    std::vector<BitMove> moves;
    moves.reserve(32);

    for(int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for(int i = 0; i < 64; i++) {
        int bitIndex = bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
    }

    _bitboards[WHITE_ALL_PIECES] = _bitboards[WHITE_PAWNS].getData() | _bitboards[WHITE_BISHOPS].getData() | _bitboards[WHITE_ROOKS].getData() | _bitboards[WHITE_KNIGHTS].getData() | _bitboards[WHITE_QUEENS].getData() |  _bitboards[WHITE_KING].getData(); 
    _bitboards[BLACK_ALL_PIECES] = _bitboards[BLACK_PAWNS].getData() | _bitboards[BLACK_BISHOPS].getData() | _bitboards[BLACK_ROOKS].getData() | _bitboards[BLACK_KNIGHTS].getData() | _bitboards[BLACK_QUEENS].getData() |  _bitboards[BLACK_KING].getData(); 
    _bitboards[OCCUPANCY] = _bitboards[WHITE_ALL_PIECES].getData() | _bitboards[BLACK_ALL_PIECES].getData();

    int bitIndex = playerColor == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int oppBitIndex = playerColor == WHITE ? BLACK_PAWNS : WHITE_PAWNS;

    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], ~_bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateKingMoves(moves, _bitboards[WHITE_KING + bitIndex], ~_bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + bitIndex].getData());
    generatePawnMoves(moves, _bitboards[WHITE_PAWNS + bitIndex], ~_bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + oppBitIndex].getData(), playerColor);

    return moves;
}
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(knightBoards[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}
void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitboardElement bishopBoard, uint64_t occupancy, uint64_t friendlies) {
    bishopBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getBishopAttacks(fromSquare, occupancy) & ~friendlies);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}
void Chess::generateRookMoves(std::vector<BitMove>& moves, BitboardElement rookBoard, uint64_t occupancy, uint64_t friendlies) {
    rookBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getRookAttacks(fromSquare, occupancy) & ~friendlies);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}
void Chess::generateQueenMoves(std::vector<BitMove>& moves, BitboardElement queenBoard, uint64_t occupancy, uint64_t friendlies) {
    queenBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(getQueenAttacks(fromSquare, occupancy) & ~friendlies);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}
void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares) {
    kingBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(kingBoards[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}
void Chess::generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares, int playerColor) {
    constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL);
    constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL);
    constexpr uint64_t Rank3(0x0000000000FF0000ULL);
    constexpr uint64_t Rank6(0x0000FF0000000000ULL);

    BitboardElement singleMoves = (playerColor == WHITE) ? (pawnBoard.getData() << 8) & emptySquares : (pawnBoard.getData() >> 8) & emptySquares;

    BitboardElement doubleMoves = (playerColor == WHITE) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares : ((singleMoves.getData() & Rank6) >> 8) & emptySquares;

    BitboardElement capturesLeft = (playerColor == WHITE) ? ((pawnBoard.getData() & NotAFile) << 7) & enemySquares : ((pawnBoard.getData() & NotAFile) >> 9) & enemySquares;
    BitboardElement capturesRight = (playerColor == WHITE) ? ((pawnBoard.getData() & NotHFile) << 9) & enemySquares : ((pawnBoard.getData() & NotHFile) >> 7) & enemySquares;
    
    int shiftForward = (playerColor == WHITE) ? 8: -8;
    int doubleShift = (playerColor == WHITE) ? 16: -16;
    int captureLeftShift = (playerColor == WHITE) ? 7: -9;
    int captureRightShift = (playerColor == WHITE) ? 9: -7;

    addPawnBitboardMovesToList(moves, singleMoves, shiftForward);
    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);
    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}
//index is 0-63
BitboardElement Chess::generateKnightMoveBitboard(int index) {
    BitboardElement board = 0ULL;
    int rank = index / 8;
    int file = index % 8;

    std::pair<int,int> knightOffsets[] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };

    constexpr uint64_t oneBit = 1;
    for(auto [dr, df] : knightOffsets) {
        int r = rank + dr, f = file + df;
        if(r >= 0 && r < 8 && f >= 0 && f < 8) {
            board |= oneBit << (r * 8 + f);
        }
    }
    return board;
}
BitboardElement Chess::generateKingMoveBitboard(int index) {
    BitboardElement board = 0ULL;
    int rank = index / 8;
    int file = index % 8;

    std::pair<int,int> kingOffsets[] = {
        {1, 1}, {-1, -1}, {-1, 1}, {1, -1},
        {0, 1}, {0, -1}, {-1, 0}, {1, 0}
    };

    constexpr uint64_t oneBit = 1;
    for(auto [dr, df] : kingOffsets) {
        int r = rank + dr, f = file + df;
        if(r >= 0 && r < 8 && f >= 0 && f < 8) {
            board |= oneBit << (r * 8 + f);
        }
    }
    return board;
}
void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, BitboardElement bitboard, int shift) {
    if(bitboard.getData() == 0)
        return;
    bitboard.forEachBit([&](int toSquare) {
        int fromSquare = toSquare - shift;
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}
int negInf = -1000000;
void Chess::updateAI() 
{
    int bestVal = negInf;
    BitMove bestMove;
    std::string state = stateString();
    _countMoves = 0;

    for(auto move : _moves) {
        int srcSquare = move.from;
        int dstSquare = move.to;

        char oldDst = state[dstSquare];
        char srcPce = state[srcSquare];
        state[dstSquare] = srcPce;
        state[srcSquare] = '0';
        int moveVal = -negamax(state, 4, WHITE, negInf, -negInf);
        // Undo the move
        state[dstSquare] = oldDst;
        state[srcSquare] = srcPce;

        if(moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    if(bestVal != negInf) {
        std::cout <<"Moves checked: " << _countMoves << std::endl;
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);
        Bit* bit = src.bit();
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}

static std::map<char, int> evaluateScores = {
    {'P', 100}, {'p', -100},    // Pawns
    {'N', 200}, {'n', -200},    // Knights
    {'B', 230}, {'b', -230},    // Bishops
    {'R', 400}, {'r', -400},    // Rooks
    {'Q', 900}, {'q', -900},    // Queens
    {'K', 2000}, {'k', -2000},  // Kings
    {'0', 0}                     // Empty squares
};

int Chess::evaluateBoard(const std::string& state) {
    int value = 0;
    for(char ch : state) {
        value += evaluateScores[ch];
    }
    return value;
}


//
// player is the current player's number (AI or human)
//
int Chess::negamax(std::string& state, int depth, int playerColor, int alpha, int beta)
{
    _countMoves++;
    // Check if AI wins, human wins, or draw
    if(depth == 0) { 
        // A winning state is a loss for the player whose turn it is.
        // The previous player made the winning move.
        return evaluateBoard(state) * playerColor; 
    }

    auto newMoves = generateAllMoves(state, playerColor);

    int bestVal = negInf; // Min value
    BitMove bestMove;
    for(auto move : newMoves) {
        int srcSquare = move.from;
        int dstSquare = move.to;

        char oldDst = state[dstSquare];
        char srcPce = state[srcSquare];
        state[dstSquare] = srcPce;
        state[srcSquare] = '0';
        bestVal = std::max(bestVal, -negamax(state, depth - 1, -playerColor, -beta, -alpha));
        // Undo the move
        state[dstSquare] = oldDst;
        state[srcSquare] = srcPce;
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;  // Beta cutoff
        }
    }

    return bestVal;
}