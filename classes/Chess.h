#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;
    void endTurn() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    std::vector<BitMove> generateAllMoves();

    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares);
    BitboardElement generateKnightMoveBitboard(int index);

    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares);
    BitboardElement generateKingMoveBitboard(int index);

    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares, int playerColor);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, BitboardElement bitboard, int shift);

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    BitboardElement knightBoards[64];
    BitboardElement kingBoards[64];
    std::vector<BitMove> _moves;
    Grid* _grid;
};