#include "Chess.h"
#include "../Application.h"
#include <limits>
#include <cmath>
#include <map>

Chess::Chess()
{
    _grid = new Grid(8, 8);
    for(int i = 0; i < 64; i++) {
        knightBoards[i] = generateKnightMoveBitboard(i);
        kingBoards[i] = generateKingMoveBitboard(i);
    }
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
    startGame();
    _moves = generateAllMoves();
}
void Chess::endTurn()
{
    _gameOptions.currentTurnNo++;
	std::string startState = stateString();
	Turn *turn = new Turn;
	turn->_boardState = stateString();
	turn->_date = (int)_gameOptions.currentTurnNo;
	turn->_score = _gameOptions.score;
	turn->_gameNumber = _gameOptions.gameNumber;
	_turns.push_back(turn);
    _moves = generateAllMoves();
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
    ChessSquare* square = (ChessSquare *)&dst;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for(auto move : _moves) {
            if(move.to == squareIndex) {
                return true;
            }
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
std::vector<BitMove> Chess::generateAllMoves()
{
    // for(auto move : moves) {
    //     delete move;
    // }
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string state = stateString();

    constexpr uint64_t BitOne = 1;
    /*
        white pieces
    */
    uint64_t whitePawns = 0LL;
    uint64_t whiteKnights = 0LL;
    uint64_t whiteRooks = 0LL;
    uint64_t whiteBishops = 0LL;
    uint64_t whiteKing = 0LL;
    uint64_t whiteQueen = 0LL;
    /*
        black pieces
    */
    uint64_t blackPawns = 0LL;
    uint64_t blackKnights = 0LL;
    uint64_t blackRooks = 0LL;
    uint64_t blackBishops = 0LL;
    uint64_t blackKing = 0LL;
    uint64_t blackQueen = 0LL;

    for(int i = 0; i <64; i++) {
            if(state[i] == 'N') {
                whiteKnights |= BitOne << i;
            } else if (state[i] == 'P')  {
                whitePawns |= BitOne << i;
            } else if (state[i] == 'B')  {
                whiteBishops |= BitOne << i;
            } else if (state[i] == 'R')  {
                whiteRooks |= BitOne << i;
            } else if (state[i] == 'Q')  {
                whiteQueen |= BitOne << i;
            } else if (state[i] == 'K')  {
                whiteKing |= BitOne << i;
            } else if(state[i] == 'n') {
                blackKnights |= BitOne << i;
            } else if (state[i] == 'p')  {
                blackPawns |= BitOne << i;
            } else if (state[i] == 'b')  {
                blackBishops |= BitOne << i;
            } else if (state[i] == 'r')  {
                blackRooks |= BitOne << i;
            } else if (state[i] == 'q')  {
                blackQueen |= BitOne << i;
            } else if (state[i] == 'k')  {
                blackKing |= BitOne << i;
            }
        }
    if(getCurrentPlayer() == getPlayerAt(0)) { //white
        uint64_t occupancy = whiteKnights | whitePawns | whiteRooks | whiteBishops | whiteQueen | whiteKing;
        uint64_t enemyOccupancy = blackKnights | blackPawns | blackRooks | blackBishops | blackQueen | blackKing;
        generateKnightMoves(moves, whiteKnights, ~occupancy);
        generateKingMoves(moves, whiteKing, ~occupancy);
        generatePawnMoves(moves, whitePawns, ~(occupancy | enemyOccupancy), enemyOccupancy, 1);
    } else { //black
        uint64_t occupancy = blackKnights | blackPawns | blackRooks | blackBishops | blackQueen | blackKing;
        uint64_t enemyOccupancy = whiteKnights | whitePawns | whiteRooks | whiteBishops | whiteQueen | whiteKing;
        generateKnightMoves(moves, blackKnights, ~occupancy);
        generateKingMoves(moves, blackKing, ~occupancy);
        generatePawnMoves(moves, blackPawns, ~(occupancy | enemyOccupancy), enemyOccupancy, 0);
    }

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

    BitboardElement singleMoves = (playerColor == 1) ? (pawnBoard.getData() << 8) & emptySquares : (pawnBoard.getData() >> 8) & emptySquares;

    BitboardElement doubleMoves = (playerColor == 1) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares : ((singleMoves.getData() & Rank6) >> 8) & emptySquares;

    BitboardElement capturesLeft = (playerColor == 1) ? ((pawnBoard.getData() & NotAFile) << 7) & enemySquares : ((pawnBoard.getData() & NotAFile) >> 9) & enemySquares;
    BitboardElement capturesRight = (playerColor == 1) ? ((pawnBoard.getData() & NotAFile) << 9) & enemySquares : ((pawnBoard.getData() & NotAFile) >> 7) & enemySquares;
    
    int shiftForward = (playerColor == 1) ? 8: -8;
    int doubleShift = (playerColor == 1) ? 16: -16;
    int captureLeftShift = (playerColor == 1) ? 7: -9;
    int captureRightShift = (playerColor == 1) ? 9: -7;

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
