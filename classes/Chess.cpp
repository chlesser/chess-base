#include "Chess.h"
#include "../Application.h"
#include "./PieceSquare.h"
#include "./GameState.h"
#include <limits>
#include <chrono>
#include <array>
#include <iomanip>
#include <cmath>
#include <map>

#define WHITE 1
#define BLACK -1

static int actualPS[128][64];

static const std::array<int, 128> evaluateScores = []() {
    std::array<int, 128> scores{};
    scores['P'] = 100; scores['p'] = -100;    // Pawns
    scores['N'] = 200; scores['n'] = -200;    // Knights
    scores['B'] = 230; scores['b'] = -230;    // Bishops
    scores['R'] = 400; scores['r'] = -400;    // Rooks
    scores['Q'] = 900; scores['q'] = -900;    // Queens
    scores['K'] = 2000; scores['k'] = -2000;  // Kings
    scores['0'] = 0;                       // Empty squares
    return scores;
}();
static const std::array<int *, 128> pieceSquareTables = []() {
    std::array<int *, 128> tables{};
    tables['P'] = (int *)&wPawnTable;
    tables['N'] = (int *)&wKnightTable;
    tables['B'] = (int *)&wBishopTable;
    tables['R'] = (int *)&wRookTable;
    tables['Q'] = (int *)&wQueenTable;
    tables['K'] = (int *)&wKingTable;
    tables['p'] = (int *)&bPawnTable;
    tables['n'] = (int *)&bKnightTable;
    tables['b'] = (int *)&bBishopTable;
    tables['r'] = (int *)&bRookTable;
    tables['q'] = (int *)&bQueenTable;
    tables['k'] = (int *)&bKingTable;
    tables['0'] = (int *)&emptyTable;
    return tables;
}();

Chess::Chess()
{
    _grid = new Grid(8, 8);
    _gameState = GameState();

    std::memset(actualPS, 0, sizeof(actualPS));
    const char pieces[] = { 'P', 'N', 'B', 'R', 'Q', 'K'};
    for(int p = 0; p < 6; p++) {
        int score = evaluateScores[(unsigned char)pieces[p]];
        for(int sq = 0; sq < 64; sq++) {
            int finalW = pieceSquareTables[(unsigned char)pieces[p]][sq] + score;
            int finalB = pieceSquareTables[(unsigned char)tolower(pieces[p])][sq] - score;
            actualPS[(unsigned char)pieces[p]][sq] = finalW;
            actualPS[(unsigned char)(pieces[p] + 32)][sq] = finalB;
            actualPS[pieces[p]][sq] = finalW;
            actualPS[(unsigned char)tolower(pieces[p])][sq] = finalB;
        }
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

    _gameState.init(initialStateString().c_str(), WHITE);
    _moves = _gameState.generateAllMoves();

    if(gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }
    startGame();

}
void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Update GameState with the new board state from the visual representation
    std::string boardState = stateString();
    std::memcpy(_gameState.state, boardState.c_str(), 64);
    
    Game::bitMovedFromTo(bit, src, dst);
}

void Chess::endTurn()
{
    std::cout << "Ending turn for player " << getCurrentPlayer()->playerNumber() << std::endl;
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        square->setHighlighted(false);
    });
    _gameOptions.currentTurnNo++;
    _currentPlayer = -_currentPlayer;
    
    // Flip color first, then generate moves for the new player
    _gameState.color = (_gameState.color == WHITE) ? BLACK : WHITE;
    _moves = _gameState.generateAllMoves();
	std::string startState = stateString();
	Turn *turn = new Turn;
	turn->_boardState = stateString();
	turn->_date = (int)_gameOptions.currentTurnNo;
	turn->_score = _gameOptions.score;
	turn->_gameNumber = _gameOptions.gameNumber;
    _gameState.init(stateString().c_str(), _gameState.color);
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
            //I need to switch the players and change the game state
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
int negInf = -1000000;
void Chess::updateAI() 
{
    const auto searchStart = std::chrono::steady_clock::now();
    _countMoves = 0;
    int bestVal = negInf;
    BitMove bestMove;

    _gameState.init(stateString().c_str(), _gameState.color);

    for(auto move : _moves) {
        _gameState.pushMove(move);
        int moveVal = -negamax(_gameState, 5, negInf, -negInf);
        // Undo the move 
        _gameState.popState();

        if(moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    if(bestVal != negInf) {
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - searchStart).count();
        const double boardsPerSecond = seconds > 0.0 ? static_cast<double>(_countMoves) / seconds : 0.0;
        std::cout << "Moves checked: " << _countMoves
                << " (" << std::fixed << std::setprecision(2) << boardsPerSecond
                << " boards/s)" << std::defaultfloat << std::endl;
        
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

#define FLIP(x) (x ^ 56)

int Chess::evaluateBoard(const GameState& gameState) {
    int value = 0;
    for(int square = 0; square < 64; square++) {
        const unsigned char ch = gameState.state[square];
        value += actualPS[ch][square];
    }
    // Don't multiply by color - negamax already handles perspective
    return value * (gameState.color);
}


//
// player is the current player's number (AI or human)
//
int Chess::negamax(GameState& gameState, int depth, int alpha, int beta)
{
    _countMoves++;
    // Check if AI wins, human wins, or draw
    if(depth == 0) { 
        // A winning state is a loss for the player whose turn it is.
        // The previous player made the winning move.
        return evaluateBoard(gameState); 
    }
    auto newMoves = gameState.generateAllMoves();
    int bestVal = negInf; // Min value
    BitMove bestMove;
    std::stable_partition(newMoves.begin(), newMoves.end(), [](const BitMove& m){ // prioritize captures, recommended online
        return (m.flags & IsCapture) != 0;
    });
    for(const auto& move : newMoves) {
        gameState.pushMove(move);
        bestVal = std::max(bestVal, -negamax(gameState, depth - 1, -beta, -alpha));
        // Undo the move
        gameState.popState();
        // alpha beta cut-off
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;
        }
    }

    return bestVal;
}