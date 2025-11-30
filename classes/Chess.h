#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"

constexpr int pieceSize = 80;
enum AllBitBoards {
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    WHITE_ALL_PIECES,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};
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

    std::vector<BitMove> generateAllMoves(const std::string& state, int playerColor);

    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares);
    BitboardElement generateKnightMoveBitboard(int index);

    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares);
    BitboardElement generateKingMoveBitboard(int index);

    void generateBishopMoves(std::vector<BitMove>& moves, BitboardElement bishopBoard, uint64_t occupancy, uint64_t friendlies);
    void generateRookMoves(std::vector<BitMove>& moves, BitboardElement rookBoard, uint64_t occupancy, uint64_t friendlies);
    void generateQueenMoves(std::vector<BitMove>& moves, BitboardElement queenBoard, uint64_t occupancy, uint64_t friendlies);

    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemySquares, int playerColor);
    void addPawnBitboardMovesToList(std::vector<BitMove>& moves, BitboardElement bitboard, int shift);

    Grid* getGrid() override { return _grid; }
    void updateAI() override;
    int evaluateBoard(const std::string& state);
    int negamax(std::string& state, int depth, int playerColor, int alpha, int beta);

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    int _currentPlayer;
    int _countMoves;
    BitboardElement knightBoards[64];
    BitboardElement kingBoards[64];
    BitboardElement _bitboards[e_numBitboards];
    int bitboardLookup[128];
    std::vector<BitMove> _moves;
    Grid* _grid;
};