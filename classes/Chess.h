#pragma once

#include "Game.h"
#include "Grid.h"
#include "GameState.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;
class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    void setBoardFromFEN(const std::string& fen);
    BitMove getLastAIMove() const { return _lastAIMove; }
    std::string getFEN() const;

    // Get current player color (WHITE=1, BLACK=-1)
    int getCurrentPlayerColor() const { return _currentPlayer; }

    // you can make this variable private, it's just grouped with the public methods for convenience
    BitMove _lastAIMove;  // Stores the last move calculated by AI (for tournament)


    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;
    void endTurn() override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;


    Grid* getGrid() override { return _grid; }
    void updateAI() override;
    int evaluateBoard(const GameState& gameState);
    int negamax(GameState& gameState, int depth, int alpha, int beta);

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    int _currentPlayer;
    int _countMoves;
    std::vector<BitMove> _moves;
    GameState _gameState;
    Grid* _grid;
};