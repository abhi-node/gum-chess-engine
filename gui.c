// gui.c
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

#define WINDOW_SIZE    800
#define SQUARE_SIZE    (WINDOW_SIZE/8)

static SDL_Window   *window     = NULL;
static SDL_Renderer *renderer   = NULL;
static SDL_Texture  *textures[13] = { NULL };
static S_BOARD       board;
static int           selectedFrom = NO_SQ;
static Move          selMoves[256];
static int           selCount = 0;


void initBoard(int (*pieces)[BOARD_SQ_NUM]) {
    // Initialize every position to off board initially
    for (int sq = 0; sq < BOARD_SQ_NUM; sq++) {
        (*pieces)[sq] = OFFBOARD;
    }

    // 2) now fill in only the “real” 8×8 squares:
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int sq120 = (rank+2)*10 + (file+1);  // maps (0,0)=>21 up to (7,7)=>98
            (*pieces)[sq120] = EMPTY;         // or your starting piece
        }
    }
    //initialize white pieces
    (*pieces)[A1] = wR;
    (*pieces)[B1] = wN;
    (*pieces)[C1] = wB;
    (*pieces)[D1] = wQ;
    (*pieces)[E1] = wK;
    (*pieces)[F1] = wB;
    (*pieces)[G1] = wN;
    (*pieces)[H1] = wR;

    (*pieces)[A2] = wP;
    (*pieces)[B2] = wP;
    (*pieces)[C2] = wP;
    (*pieces)[D2] = wP;
    (*pieces)[E2] = wP;
    (*pieces)[F2] = wP;
    (*pieces)[G2] = wP;
    (*pieces)[H2] = wP;

    //initialize black pieces
    (*pieces)[A8] = bR;
    (*pieces)[B8] = bN;
    (*pieces)[C8] = bB;
    (*pieces)[D8] = bQ;
    (*pieces)[E8] = bK;
    (*pieces)[F8] = bB;
    (*pieces)[G8] = bN;
    (*pieces)[H8] = bR;

    (*pieces)[A7] = bP;
    (*pieces)[B7] = bP;
    (*pieces)[C7] = bP;
    (*pieces)[D7] = bP;
    (*pieces)[E7] = bP;
    (*pieces)[F7] = bP;
    (*pieces)[G7] = bP;
    (*pieces)[H7] = bP;
}

void printBoard(int pieces[BOARD_SQ_NUM]) {
    printf("   ");
    for (int i = 0; i < 8; i++) {
        printf("%c  ", 97+i);
    }
    printf("\n");
    for (int i = A1; i <= H8; i++) {
        if (i % 10 == 1) {
            printf("%d  ", (i/10)-1);
        }
        if ((i%10 < 1) || (i%10 > 8)) {
            continue;
        }
        char color;
        if (pieces[i] < 1) {
            color = '_';
        } else if (pieces[i] < 7) {
            color = 'w';
        } else {
            color = 'b';
        }
        printf("%c%c ",color,PIECE_CHARS[pieces[i]]);
        if (i % 10 == 8) {
            printf("\n");
        }
    }
}

int squareToValue(char file, char rank) {
    return (rank - 49)*10 + (file - 97) + A1;
}

Move parseMove(char SAN[99], S_BOARD board) {
    // Initialize all move variables to default values
    Move m;
    m.from = NO_SQ;
    m.to = NO_SQ;
    m.promotion = EMPTY;
    m.is_castle_kingside = false;
    m.is_castle_queenside = false;

    // Check if kingside castle
    if (strcmp(SAN, "O-O") == 0) {
        m.is_castle_kingside = true;
        return m;
    }

    // Check if queenside castle    
    if (strcmp(SAN, "O-O-O") == 0) {
        m.is_castle_queenside = true;
        return m;
    }

    // Check for promotion move
    if (strstr(SAN, "=") != NULL) {
        m.from = squareToValue(SAN[0], SAN[1]);
        m.to = squareToValue(SAN[2], SAN[3]);
        
        for (int i = 0; i < 13; i++) {
            if (SAN[5] == PIECE_CHARS[i]) {
                if (board.side == BLACK) {
                    m.promotion = i+6;
                } else {
                    m.promotion = i;
                }
                break;
            }
        }
        return m;
    }

    if (SAN[0] < 97) {
        m.from = squareToValue(SAN[1], SAN[2]);
        m.to = squareToValue(SAN[3], SAN[4]);
    } else {
        m.from = squareToValue(SAN[0], SAN[1]);
        m.to = squareToValue(SAN[2], SAN[3]);
    }
    
    return m;
    
}

bool isKingInCheck(S_BOARD board) {

    int king_square = 0;
    for (int i = 0; i < BOARD_SQ_NUM; i++) {
        if (board.side == WHITE && board.pieces[i] == wK) {
            king_square = i;
        } else if (board.side == BLACK && board.pieces[i] == bK) {
            king_square = i;
        }
    }

    int start_square = king_square;
    
    // Knight Checks
    for (int i = 0; i < 8; i++) {
        start_square += KNIGHT_DIRS[i];
        if ((board.pieces[start_square] == wN && board.side == BLACK) || (board.pieces[start_square] == bN && board.side == WHITE)) {
            return true;
        }
        start_square = king_square;
    }

    // Pawn Checks (Corrected directions)
    if (board.side == WHITE) {
        if (board.pieces[start_square + 11] == bP || board.pieces[start_square + 9] == bP) {
            return true;
        }
    } else {
        if (board.pieces[start_square - 11] == wP || board.pieces[start_square - 9] == wP) {
            return true;
        }
    }

    // Bishop Checks
    for (int i = 0; i < 4; i++) {
        start_square += BISHOP_DIRS[i];
        while (board.pieces[start_square] != OFFBOARD) {
            if ((board.pieces[start_square] > 0 && board.pieces[start_square] < 7 && board.side == WHITE) || (board.pieces[start_square] >= 7 && board.side == BLACK)) {
                break;
            }
            if (((board.pieces[start_square] == wB || board.pieces[start_square] == wQ) && board.side == BLACK) || ((board.pieces[start_square] == bB || board.pieces[start_square] == bQ) && board.side == WHITE)) {
                return true;
            }
            if (board.pieces[start_square] != EMPTY) {
                break;
            }
            start_square += BISHOP_DIRS[i];
        }
        start_square = king_square;
    }

    // Rook Checks
    for (int i = 0; i < 4; i++) {
        start_square += ROOK_DIRS[i];
        while (board.pieces[start_square] != OFFBOARD) {
            if ((board.pieces[start_square] > 0 && board.pieces[start_square] < 7 && board.side == WHITE) || (board.pieces[start_square] >= 7 && board.side == BLACK)) {
                break;
            }
            if (((board.pieces[start_square] == wR || board.pieces[start_square] == wQ) && board.side == BLACK) || ((board.pieces[start_square] == bR || board.pieces[start_square] == bQ) && board.side == WHITE)) {
                return true;
            }
            if (board.pieces[start_square] != EMPTY) {
                break;
            }
            start_square += ROOK_DIRS[i];
        }
        start_square = king_square;
    }

    return false;
}

bool checkLegalPawn(Move m, S_BOARD board) {
    int color_mult = board.side == WHITE ? 1 : -1;
    bool enPassant = false;
    int move_diff = m.to - m.from;

    // Regular pawn moves
    if (move_diff == 10 * color_mult) {  // Single push
        if (board.pieces[m.to] != EMPTY) return false;
    } 
    else if (move_diff == 20 * color_mult) {  // Double push
        int middle_sq = m.from + 10 * color_mult;
        if ((board.side == WHITE && (m.from < A2 || m.from > H2)) || 
            (board.side == BLACK && (m.from < A7 || m.from > H7)) ||
            board.pieces[m.to] != EMPTY || 
            board.pieces[middle_sq] != EMPTY) {
            return false;
        }
    }
    // Capture moves (including en passant)
    else if (move_diff == 9 * color_mult || move_diff == 11 * color_mult) {
        // Regular capture check
        if (board.pieces[m.to] == EMPTY) {
            // En passant validation
            if (m.to != board.enPas) return false;
            int ep_pawn_sq = board.side == WHITE ? m.to - 10 : m.to + 10;
            if (board.pieces[ep_pawn_sq] != (board.side == WHITE ? bP : wP)) {
                return false;
            }
            enPassant = true;
        } 
        else {  // Regular capture
            bool valid_capture = (board.side == WHITE) ? 
                (board.pieces[m.to] >= bP) : 
                (board.pieces[m.to] <= wK);
            if (!valid_capture) return false;
        }
    } 
    else {
        return false;  // Invalid pawn move pattern
    }

    // Simulate move and check for exposed king
    int captured_piece = board.pieces[m.to];
    int original_ep = board.enPas;
    int ep_pawn_sq = board.side == WHITE ? m.to - 10 : m.to + 10; // Added for enPassant
    
    if (enPassant) {
        board.pieces[ep_pawn_sq] = EMPTY; // Corrected: remove the pawn at ep_pawn_sq
    }
    board.pieces[m.to] = board.pieces[m.from];
    board.pieces[m.from] = EMPTY;

    bool in_check = isKingInCheck(board);

    // Restore board state
    board.pieces[m.from] = board.pieces[m.to];
    board.pieces[m.to] = captured_piece;
    if (enPassant) {
        board.pieces[ep_pawn_sq] = (board.side == WHITE ? bP : wP); // Restore the pawn
    }
    board.enPas = original_ep;

    return !in_check;
}

static bool isPathClear(int from, int to, int dir, int pieces[BOARD_SQ_NUM]) {
    int sq = from + dir;
    while (sq != to) {
        if (pieces[sq] != EMPTY) return false;
        sq += dir;
    }
    return true;
}

bool checkLegalBishop(Move m, S_BOARD board) {
    int delta = m.to - m.from, dir = 0;
    for (int i = 0; i < 4; i++) {
        if (delta % BISHOP_DIRS[i] == 0 && delta / BISHOP_DIRS[i] > 0) {
            dir = BISHOP_DIRS[i];
            break;
        }
    }
    if (!dir) return false;

    // make sure all squares _between_ from and to are empty
    if (!isPathClear(m.from, m.to, dir, board.pieces)) return false;

    // ensure destination is not occupied by own piece (already done in checkLegal)
    // now simulate and test for check
    int cap = board.pieces[m.to];
    board.pieces[m.to]   = board.pieces[m.from];
    board.pieces[m.from] = EMPTY;

    bool ok = !isKingInCheck(board);

    // restore
    board.pieces[m.from] = board.pieces[m.to];
    board.pieces[m.to]   = cap;
    return ok;
}

bool checkLegalRook(Move m, S_BOARD board) {
    int delta = m.to - m.from, dir = 0;
    for (int i = 0; i < 4; i++) {
        if (delta % ROOK_DIRS[i] == 0 && delta / ROOK_DIRS[i] > 0) {
            dir = ROOK_DIRS[i];
            break;
        }
    }
    if (!dir) return false;
    if (!isPathClear(m.from, m.to, dir, board.pieces)) return false;

    int cap = board.pieces[m.to];
    board.pieces[m.to]   = board.pieces[m.from];
    board.pieces[m.from] = EMPTY;

    bool ok = !isKingInCheck(board);

    board.pieces[m.from] = board.pieces[m.to];
    board.pieces[m.to]   = cap;
    return ok;
}

bool checkLegalQueen(Move m, S_BOARD board) {
    if (checkLegalRook(m, board))   return true;
    if (checkLegalBishop(m, board)) return true;
    return false;
}

bool checkLegalKnight(Move m, S_BOARD board) {
    // Find direction of the knight and check if it is a legal direction for the knight
    bool valid = false;
    for (int i = 0; i < 8; i++) {
        if (m.to - m.from == KNIGHT_DIRS[i]) {
            valid = true;
            break;
        }
    }
    if (!valid) return false;

    int start = m.to;
    if (board.pieces[start] == OFFBOARD) {
        return false;
    }
    if ((board.pieces[start] > 0 && board.pieces[start] < 7 && board.side == WHITE) || (board.pieces[start] >= 7 && board.side == BLACK)) {
        return false;
    }

    int captured_piece = board.pieces[m.to];
    board.pieces[m.to] = board.pieces[m.from];
    board.pieces[m.from] = EMPTY;

    if (isKingInCheck(board)) {
        return false;
    }

    board.pieces[m.from] = board.pieces[m.to];
    board.pieces[m.to] = captured_piece;

    return true;
}

bool checkLegalKing(Move m, S_BOARD board) {
    // Find direction of the knight and check if it is a legal direction for the king
    bool valid = false;
    for (int i = 0; i < 8; i++) {
        if (m.to-m.from == KING_DIRS[i]) {
            valid = true;
            break;
        }
    }
    if (!valid) return false;
    int start = m.to;
    if (board.pieces[start] == OFFBOARD) {
        return false;
    }
    if ((board.pieces[start] > 0 && board.pieces[start] < bP && board.side == WHITE) || (board.pieces[start] >= bP && board.side == BLACK)) {
        return false;
    }

    int captured_piece = board.pieces[m.to];
    board.pieces[m.to] = board.pieces[m.from];
    board.pieces[m.from] = EMPTY;

    if (isKingInCheck(board)) {
        return false;
    }

    board.pieces[m.from] = board.pieces[m.to];
    board.pieces[m.to] = captured_piece;

    return true;
}

bool checkLegalQueensideCastle(S_BOARD board) {
    if ((board.bCastle != 0 && board.side == BLACK) || (board.wCastle != 0 && board.side == WHITE)) {
        return false;
    }

    if ((board.pieces[A1] != wR && board.side == WHITE) || (board.pieces[A8] != bR && board.side == BLACK)) {
        return false;
    }

    if ((board.side == WHITE && (board.pieces[B1] != EMPTY || board.pieces[C1] != EMPTY || board.pieces[D1] != EMPTY)) 
    || (board.side == BLACK && (board.pieces[B8] != EMPTY || board.pieces[C8] != EMPTY || board.pieces[D8] != EMPTY))) {
        return false;
    }

    int piece = board.side == WHITE ? wK : bK;
    board.pieces[board.side == WHITE ? E1 : E8] = EMPTY;
    board.pieces[board.side == WHITE ? D1 : D8] = piece;

    if (isKingInCheck(board)) {
        return false;
    }

    board.pieces[board.side == WHITE ? C1 : C8] = piece;
    board.pieces[board.side == WHITE ? D1 : D8] = board.side == WHITE ? wR : bR;

    if (isKingInCheck(board)) {
        return false;
    }

    board.pieces[board.side == WHITE ? E1 : E8] = piece;
    board.pieces[board.side == WHITE ? D1 : D8] = EMPTY;
    board.pieces[board.side == WHITE ? C1 : C8] = EMPTY;
    board.pieces[board.side == WHITE ? A1 : A8] = board.side == WHITE ? wR : bR;

    return true;
}

bool checkLegalKingsideCastle(S_BOARD board) {
    if ((board.bCastle != 0 && board.side == BLACK) || (board.wCastle != 0 && board.side == WHITE)) {
        return false;
    }

    if ((board.pieces[H1] != wR && board.side == WHITE) || (board.pieces[H8] != bR && board.side == BLACK)) {
        return false;
    }

    if ((board.side == WHITE && (board.pieces[F1] != EMPTY || board.pieces[G1] != EMPTY)) 
    || (board.side == BLACK && (board.pieces[F8] != EMPTY || board.pieces[G8] != EMPTY))) {
        return false;
    }

    int piece = board.side == WHITE ? wK : bK;
    board.pieces[board.side == WHITE ? E1 : E8] = EMPTY;
    board.pieces[board.side == WHITE ? F1 : F8] = piece;

    if (isKingInCheck(board)) {
        return false;
    }

    board.pieces[board.side == WHITE ? G1 : G8] = piece;
    board.pieces[board.side == WHITE ? F1 : F8] = board.side == WHITE ? wR : bR;

    if (isKingInCheck(board)) {
        return false;
    }

    board.pieces[board.side == WHITE ? E1 : E8] = piece;
    board.pieces[board.side == WHITE ? F1 : F8] = EMPTY;
    board.pieces[board.side == WHITE ? G1 : G8] = EMPTY;
    board.pieces[board.side == WHITE ? H1 : H8] = board.side == WHITE ? wR : bR;

    return true;
}

bool checkLegal(Move m, S_BOARD board) {
    // Castle logic
    if (m.is_castle_queenside) {
        return checkLegalQueensideCastle(board);
    }

    if (m.is_castle_kingside) {
        return checkLegalKingsideCastle(board);
    }

    // If move is out of bounds, then it is illegal
    if (m.to < 0 || m.to >= 120 || board.pieces[m.to] == OFFBOARD || m.from < 0 || m.from >= 120 || board.pieces[m.from] == OFFBOARD || board.pieces[m.from] == EMPTY) {
        return false;
    }

    // If the turn does not match the piece that is moving, again it is illegal
    if ((board.pieces[m.from] < 7 && board.side == BLACK) || (board.pieces[m.from] >= 7 && board.side == WHITE)) {
        return false;
    }

    // If the turn does not match the piece that is moving, again it is illegal
    if ((board.pieces[m.to] > 0 && board.pieces[m.to] < 7 && board.side == WHITE) || (board.pieces[m.to] >= 7 && board.side == BLACK)) {
        return false;
    }

    char piece = PIECE_CHARS[board.pieces[m.from]];

    switch (piece) {
    // Pawn Legality Checks
        case 'P':  
            return checkLegalPawn(m, board);
            break;
    // Bishop Legality Checks
        case 'B':
            return checkLegalBishop(m, board);
            break;
    // Knight Legality Checks
        case 'N':
            return checkLegalKnight(m, board);
            break;
    // Rook Legality Checks
        case 'R':  
            return checkLegalRook(m, board);
            break;
    // Queen Legality Checks
        case 'Q':
            return checkLegalQueen(m, board);
            break;
    // King Legality Checks
        case 'K':
            return checkLegalKing(m, board);
            break;
        default:
            return false;
            break;
    }

    return false;
}

void makeMove(Move m, S_BOARD *board) {
    // TODO: Add make move functionality
    // - Add special case for enpassant
    // - Set enpas value, and adjust castle logic if king moves or castle happens

    if (m.is_castle_kingside) {
        (*board).pieces[board->side == WHITE ? E1 : E8] = EMPTY;
        (*board).pieces[board->side == WHITE ? F1 : F8] = board->side == WHITE ? wR : bR;
        (*board).pieces[board->side == WHITE ? G1 : G8] = board->side == WHITE ? wK : bK;
        (*board).pieces[board->side == WHITE ? H1 : H8] = EMPTY;

        if (board->side == WHITE) {
            board->wCastle = 1;
        } else {
            board->bCastle = 1;
        }

        board->enPas = 0;
        board->side = board->side == WHITE ? BLACK : WHITE;
        return;
    }

    if (m.is_castle_queenside) {
        (*board).pieces[board->side == WHITE ? E1 : E8] = EMPTY;
        (*board).pieces[board->side == WHITE ? D1 : D8] = board->side == WHITE ? wR : bR;
        (*board).pieces[board->side == WHITE ? C1 : C8] = board->side == WHITE ? wK : bK;
        (*board).pieces[board->side == WHITE ? A1 : A8] = EMPTY;

        if (board->side == WHITE) {
            board->wCastle = 1;
        } else {
            board->bCastle = 1;
        }

        board->enPas = 0;
        board->side = board->side == WHITE ? BLACK : WHITE;
        return;
    }

    char piece = PIECE_CHARS[(*board).pieces[m.from]];

    if (piece == 'P' && ((m.to-m.from) == 20 || (m.to-m.from) == -20)) {   
        (*board).pieces[m.to] = (*board).pieces[m.from];
        (*board).pieces[m.from] = EMPTY;

        board->enPas = m.to;
        board->side = board->side == WHITE ? BLACK : WHITE;
        return;
    }

    // EnPassant Logic
    if (piece == 'P' && (*board).pieces[m.to] == EMPTY) {
        (*board).pieces[m.to] = (*board).pieces[m.from];
        (*board).pieces[m.from] = EMPTY;
        if (board->side == WHITE) {
            (*board).pieces[m.from+((m.to-m.from)-10)] = EMPTY;
        } else {
            (*board).pieces[m.from+((m.to-m.from)+10)] = EMPTY;
        }
        

        board->enPas = 0;
        board->side = board->side == WHITE ? BLACK : WHITE;
        return;
    }

    if (piece == 'P' && ((m.to >= A8 && m.to <= H8) || (m.to >= A1 && m.to <= H1))) {
        (*board).pieces[m.to] = m.promotion;
        (*board).pieces[m.from] = EMPTY;

        board->enPas = 0;
        board->side = board->side == WHITE ? BLACK : WHITE;
        return;
    }

    

    if (piece == 'K') {
        if (board->side == WHITE) {
            board->wCastle = 1;
        } else {
            board->bCastle = 1;
        }
    }

    (*board).pieces[m.to] = (*board).pieces[m.from];
    (*board).pieces[m.from] = EMPTY;

    board->enPas = 0;
    board->side = board->side == WHITE ? BLACK : WHITE;
}

void addPawnMove(S_BOARD *board, int from, int to, Move *moves, int *count, bool isCapture) {
    int promoRank = (board->side == WHITE) ? 7 : 0; // Corrected promotion ranks
    if ((to/10 - 2) == promoRank) {
        int promotions[] = {(board->side == WHITE) ? wQ : bQ, 
                           (board->side == WHITE) ? wR : bR,
                           (board->side == WHITE) ? wB : bB,
                           (board->side == WHITE) ? wN : bN};
        for (int i = 0; i < 4; i++) {
            Move m = {from, to, promotions[i], isCapture, false, false};
            if (checkLegal(m, *board)) {
                moves[(*count)++] = m;
            }
        }
    } else {
        Move m = {from, to, EMPTY, isCapture, false, false};
        if (checkLegal(m, *board)) {
            moves[(*count)++] = m;
        }
    }
}

void generateLegalMoves(S_BOARD *board, Move *moves, int *moveCount) {
    *moveCount = 0;

    // Generate castling moves if allowed
    if (board->side == WHITE && board->wCastle == 0) {
        Move kingside = {E1, G1, EMPTY, false, true, false};
        if (checkLegal(kingside, *board)) {
            moves[(*moveCount)++] = kingside;
        }
        Move queenside = {E1, C1, EMPTY, false, false, true};
        if (checkLegal(queenside, *board)) {
            moves[(*moveCount)++] = queenside;
        }
    } else if (board->side == BLACK && board->bCastle == 0) {
        Move kingside = {E8, G8, EMPTY, false, true, false};
        if (checkLegal(kingside, *board)) {
            moves[(*moveCount)++] = kingside;
        }
        Move queenside = {E8, C8, EMPTY, false, false, true};
        if (checkLegal(queenside, *board)) {
            moves[(*moveCount)++] = queenside;
        }
    }

    // Generate moves for each piece
    for (int from = 0; from < BOARD_SQ_NUM; from++) {
        if (board->pieces[from] == OFFBOARD || board->pieces[from] == EMPTY)
            continue;
        if ((board->side == WHITE && board->pieces[from] >= bP) || 
            (board->side == BLACK && board->pieces[from] <= wK))
            continue;

        int p = board->pieces[from];
        
        // Generate pawn moves
        if (p == wP || p == bP) {
            int dir = (p == wP) ? 10 : -10;
            int startRank = (p == wP) ? 1 : 6; // Corrected start ranks for double push
            
            // Single push
            int to = from + dir;
            if (board->pieces[to] == EMPTY) {
                addPawnMove(board, from, to, moves, moveCount, false);
                
                // Double push from start rank
                if (((from/10 - 2) == startRank) && board->pieces[to + dir] == EMPTY) {
                    Move m = {from, to + dir, EMPTY, false, false, false};
                    if (checkLegal(m, *board)) {
                        moves[(*moveCount)++] = m;
                    }
                }
            }
            
            // Captures (including en passant)
            int targets[] = {from + dir + 1, from + dir - 1};
            for (int i = 0; i < 2; i++) {
                to = targets[i];
                if (board->pieces[to] == OFFBOARD) continue;

                bool isCapture = (board->pieces[to] != EMPTY) || (to == board->enPas);
                if (isCapture) {
                    addPawnMove(board, from, to, moves, moveCount, true);
                }
            }
        }
        // Generate knight moves
        else if (p == wN || p == bN) {
            for (int i = 0; i < 8; i++) {
                int to = from + KNIGHT_DIRS[i];
                if (board->pieces[to] == OFFBOARD) continue;
                if (board->pieces[to] != EMPTY) {
                    bool ownPiece = (board->side == WHITE) ? 
                        (board->pieces[to] <= wK) : (board->pieces[to] >= bP);
                    if (ownPiece) continue;
                }
                

                Move m = {from, to, EMPTY, board->pieces[to] != EMPTY, false, false};
                if (checkLegal(m, *board)) {
                    moves[(*moveCount)++] = m;
                }
            }
        }
        // Generate sliding moves (Bishop/Rook/Queen)
        else if (p == wB || p == bB || p == wR || p == bR || p == wQ || p == bQ) {
            int dirs[8], dirCount;
            if (p == wB || p == bB) {
                dirs[0] = 11; dirs[1] = 9; dirs[2] = -9; dirs[3] = -11;
                dirCount = 4;
            } else if (p == wR || p == bR) {
                dirs[0] = 10; dirs[1] = 1; dirs[2] = -1; dirs[3] = -10;
                dirCount = 4;
            } else { // Queen
                dirs[0] = 11; dirs[1] = 10; dirs[2] = 9; dirs[3] = 1; 
                dirs[4] = -1; dirs[5] = -9; dirs[6] = -10; dirs[7] = -11;
                dirCount = 8;
            }
            
            for (int d = 0; d < dirCount; d++) {
                int to = from;
                while (1) {
                    to += dirs[d];
                    if (board->pieces[to] == OFFBOARD) break;

                    if (board->pieces[to] != EMPTY) {
                        bool ownPiece = (board->side == WHITE) ? 
                            (board->pieces[to] <= wK) : (board->pieces[to] >= bP);
                        if (ownPiece) break;
                    }

                    Move m = {from, to, EMPTY, board->pieces[to] != EMPTY, false, false};
                    if (checkLegal(m, *board)) {
                        moves[(*moveCount)++] = m;
                    }

                    if (board->pieces[to] != EMPTY) break; // Stop after capturing
                }
            }
        }
        // Generate king moves
        else if (p == wK || p == bK) {
            for (int i = 0; i < 8; i++) {
                int to = from + KING_DIRS[i];
                if (board->pieces[to] == OFFBOARD) continue;

                if (board->pieces[to] != EMPTY) {
                    bool ownPiece = (board->side == WHITE) ? 
                        (board->pieces[to] <= wK) : (board->pieces[to] >= bP);
                    if (ownPiece) continue;
                }
                
                Move m = {from, to, EMPTY, board->pieces[to] != EMPTY, false, false};
                if (checkLegal(m, *board)) {
                    moves[(*moveCount)++] = m;
                }
            }
        }
    }
}



double Evaluate(S_BOARD board) {
    double score = 0;
    for (int i = A1; i < H8; i++) {
        if (board.pieces[i] == EMPTY || board.pieces[i] == OFFBOARD) {
            continue;
        }
        int mult = board.pieces[i] < bP ? 1 : -1;
        switch (PIECE_CHARS[board.pieces[i]]) {
            case 'P':  
                score+= 1*mult;
                score+= mult*0.09*PawnEval[board.side == WHITE ? 0 : 1][i];
                break;
            case 'B':
                score+= 3.1*mult;
                break;
            case 'N':
                score+= 3*mult;
                break;
            case 'R':  
                score+= 5*mult;
                break;
            case 'Q':
                score+= 9*mult;
                break;
            default:
                break;
        }
    }
    Move legalMoves[256];
    int moveCount = 0;
    generateLegalMoves(&board, legalMoves, &moveCount);
    board.side = board.side == WHITE ? BLACK : WHITE;

    int moveCount2 = 0;
    generateLegalMoves(&board, legalMoves, &moveCount2);
    board.side = board.side == WHITE ? BLACK : WHITE;

    score += (board.side == WHITE ? 1 : -1)*((moveCount-moveCount2)/(moveCount2+moveCount)) * 4;
    return score*(board.side==WHITE ? 1 : -1);
}

static StateInfo makeMoveUndoable(Move m, S_BOARD *b) {
    StateInfo st = {
      .captured   = b->pieces[m.to],
      .ep_old     = b->enPas,
      .wCast_old  = b->wCastle,
      .bCast_old  = b->bCastle
    };
    makeMove(m, b);
    return st;
}

static void undoMove(StateInfo st, Move m, S_BOARD *b) {
    // --- handle castling undo ---
    if (m.is_castle_kingside) {
        // King went E1→G1 (or E8→G8), rook went H1→F1 (or H8→F8)
        if (m.from == E1) {
            // White
            b->pieces[E1] = wK;
            b->pieces[G1] = EMPTY;
            b->pieces[H1] = wR;
            b->pieces[F1] = EMPTY;
        } else {
            // Black
            b->pieces[E8] = bK;
            b->pieces[G8] = EMPTY;
            b->pieces[H8] = bR;
            b->pieces[F8] = EMPTY;
        }
    }
    else if (m.is_castle_queenside) {
        // King went E1→C1 (or E8→C8), rook went A1→D1 (or A8→D8)
        if (m.from == E1) {
            // White
            b->pieces[E1] = wK;
            b->pieces[C1] = EMPTY;
            b->pieces[A1] = wR;
            b->pieces[D1] = EMPTY;
        } else {
            // Black
            b->pieces[E8] = bK;
            b->pieces[C8] = EMPTY;
            b->pieces[A8] = bR;
            b->pieces[D8] = EMPTY;
        }
    }
    // --- all other moves (including promotions & captures) ---
    else {
        b->pieces[m.from] = b->pieces[m.to];
        b->pieces[m.to]   = st.captured;
    }

    // restore state fields
    b->enPas   = st.ep_old;
    b->wCastle = st.wCast_old;
    b->bCastle = st.bCast_old;
    // flip side back
    b->side    = (b->side == WHITE ? BLACK : WHITE);
}

void orderMoves(Move (*moves)[256], int moveCount, S_BOARD board) {
    for(int i=1;i<moveCount;i++){
        if ((*moves)[i].is_capture && !(*moves)[i-1].is_capture) {
          Move tmp = (*moves)[i];
          (*moves)[i] = (*moves)[i-1];
          (*moves)[i-1] = tmp;
        }
    }
}

double AlphaBetaSearch(int depth, double alpha, double beta, S_BOARD* board, bool isRoot) {
    if (depth == 0)
        return Evaluate(*board);

    Move legalMoves[256];
    int moveCount = 0;
    generateLegalMoves(board, legalMoves, &moveCount);
    orderMoves(&legalMoves, moveCount, *board);

    for (int i = 0; i < moveCount; i++) {
        StateInfo st = makeMoveUndoable(legalMoves[i], board);
        double val = -AlphaBetaSearch(depth - 1, -beta, -alpha, board, false);
        undoMove(st, legalMoves[i], board);

        if (val >= beta) {
            return beta;
        }
        if (val > alpha) {
            alpha = val;
            if (isRoot) board->bestMove = legalMoves[i];
        }
    }
    return alpha;
}

bool isKingCheckmated(S_BOARD board) {
    Move legalMoves[256];
    int moveCount = 0;
    generateLegalMoves(&board, legalMoves, &moveCount);
    return moveCount == 0;
}


// Initialize SDL2 + window + renderer
static bool init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
        return false;
    }
    window = SDL_CreateWindow("Chess GUI",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_SIZE, WINDOW_SIZE, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

// Load all piece textures
static bool load_textures(void) {
    const char* files[13] = {
        "",
        "sprites/wP.png","sprites/wN.png","sprites/wB.png",
        "sprites/wR.png","sprites/wQ.png","sprites/wK.png",
        "sprites/bP.png","sprites/bN.png","sprites/bB.png",
        "sprites/bR.png","sprites/bQ.png","sprites/bK.png"
    };
    for (int i = 1; i <= 12; i++) {
        SDL_Surface *surf = IMG_Load(files[i]);
        if (!surf) {
            SDL_Log("IMG_Load %s failed: %s", files[i], IMG_GetError());
            return false;
        }
        textures[i] = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (!textures[i]) {
            SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
            return false;
        }
    }
    return true;
}

// Cleanup SDL resources
static void cleanup(void) {
    for (int i = 1; i <= 12; i++) if (textures[i]) SDL_DestroyTexture(textures[i]);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window)   SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

// Convert 120-index to GUI row,col
static bool sq120_to_rc(int sq, int *row, int *col) {
    int r = (sq / 10) - 2;
    int c = (sq % 10) - 1;
    if (r < 0 || r > 7 || c < 0 || c > 7) return false;
    *row = 7 - r;
    *col = c;
    return true;
}

// Convert GUI row,col to 120-index
static int rc_to_sq120(int row, int col) {
    int r = 7 - row;
    return (r + 2) * 10 + (col + 1);
}

// Render board, highlights, and pieces
static void render_board(void) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // Draw squares
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            SDL_Rect sq = { c * SQUARE_SIZE, r * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE };
            bool light = ((r + c) & 1) == 0;
            SDL_SetRenderDrawColor(renderer,
                light ? 240 : 181,
                light ? 217 : 136,
                light ? 181 :  99,
                255);
            SDL_RenderFillRect(renderer, &sq);
        }
    }

    // Highlight legal moves (blue)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 100);
    for (int i = 0; i < selCount; i++) {
        Move m = selMoves[i];
        int rr, cc;
        // King/capture target
        if (sq120_to_rc(m.to, &rr, &cc)) {
            SDL_Rect hl = { cc * SQUARE_SIZE, rr * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE };
            SDL_RenderFillRect(renderer, &hl);
        }
        // Rook landing square if castling
        if (m.is_castle_kingside || m.is_castle_queenside) {
            int rookTo = m.is_castle_kingside
                ? (board.side == WHITE ? F1 : F8)
                : (board.side == WHITE ? D1 : D8);
            if (sq120_to_rc(rookTo, &rr, &cc)) {
                SDL_Rect hl2 = { cc * SQUARE_SIZE, rr * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE };
                SDL_RenderFillRect(renderer, &hl2);
            }
        }
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // Highlight selected square (green)
    if (selectedFrom != NO_SQ) {
        int sr, sc;
        if (sq120_to_rc(selectedFrom, &sr, &sc)) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);
            SDL_Rect hl = { sc * SQUARE_SIZE, sr * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE };
            SDL_RenderFillRect(renderer, &hl);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }

    // Draw pieces
    for (int sq = 0; sq < BOARD_SQ_NUM; sq++) {
        int p = board.pieces[sq];
        if (p > EMPTY && p <= bK) {
            int rr, cc;
            if (!sq120_to_rc(sq, &rr, &cc)) continue;
            SDL_Rect dst = { cc * SQUARE_SIZE, rr * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE };
            SDL_RenderCopy(renderer, textures[p], NULL, &dst);
        }
    }

    SDL_RenderPresent(renderer);
}

// Map mouse x,y to 120-index
static int square_from_mouse(int x, int y) {
    if (x < 0 || x >= WINDOW_SIZE || y < 0 || y >= WINDOW_SIZE) return NO_SQ;
    return rc_to_sq120(y / SQUARE_SIZE, x / SQUARE_SIZE);
}

int main(void) {
    if (!init_sdl() || !load_textures()) {
        cleanup();
        return 1;
    }

    // Initialize board state
    board.side      = WHITE;
    board.enPas     = 0;
    board.wCastle   = board.bCastle = 0;
    initBoard(&board.pieces);

    bool quit = false;
    SDL_Event e;
    Move allMoves[256];
    int allCount;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    // Cancel selection
                    selectedFrom = NO_SQ;
                    selCount     = 0;
                } else {
                    int sq = square_from_mouse(e.button.x, e.button.y);
                    if (sq == NO_SQ) break;

                    bool moved = false;

                    // --- White’s human move ---
                    if (selectedFrom == NO_SQ) {
                        int pc = board.pieces[sq];
                        bool myPiece = (pc != EMPTY && pc != OFFBOARD) &&
                                       ((board.side == WHITE && pc < bP) ||
                                        (board.side == BLACK && pc >= bP));
                        if (myPiece) {
                            selectedFrom = sq;
                            selCount = 0;
                            generateLegalMoves(&board, allMoves, &allCount);
                            for (int i = 0; i < allCount; i++)
                                if (allMoves[i].from == selectedFrom)
                                    selMoves[selCount++] = allMoves[i];
                        }
                    } else {
                        for (int i = 0; i < selCount; i++) {
                            if (selMoves[i].to == sq) {
                                makeMove(selMoves[i], &board);
                                moved = true;
                                break;
                            }
                        }
                        // re‑select if clicked another own piece
                        if (!moved) {
                            int pc = board.pieces[sq];
                            bool myPiece = (pc != EMPTY && pc != OFFBOARD) &&
                                           ((board.side == WHITE && pc < bP) ||
                                            (board.side == BLACK && pc >= bP));
                            if (myPiece) {
                                selectedFrom = sq;
                                selCount = 0;
                                generateLegalMoves(&board, allMoves, &allCount);
                                for (int i = 0; i < allCount; i++)
                                    if (allMoves[i].from == selectedFrom)
                                        selMoves[selCount++] = allMoves[i];
                            } else {
                                selectedFrom = NO_SQ;
                                selCount     = 0;
                            }
                        }
                    }

                    // --- Black’s engine reply ---
                    if (moved && board.side == BLACK) {
                        // run a depth‑3 α–β search (you can adjust depth or bounds)
                        AlphaBetaSearch(4, -10000.0, 10000.0, &board, true);
                        Move ai = board.bestMove;
                        makeMove(ai, &board);
                    }

                    if (moved) {
                        selectedFrom = NO_SQ;
                        selCount     = 0;
                    }
                }
            }
        }

        render_board();
        SDL_Delay(16);
    }

    cleanup();
    return 0;
}