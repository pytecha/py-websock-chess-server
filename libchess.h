#pragma once
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// pc: piece_t*, ct: pcat_t, rf: fr, cl: pcol_t
#define INIT_MEN(pc, ct, rf, cl) {(pc)->pcat=(ct); strncpy(pc->fr, (rf), 3); \
    (pc)->pcol=(cl); (pc)->captured=false; (pc)->moved=false;}

// pc: char
#define PCAT(pc) ((pc) == 'R' ? ROOK : (pc) == 'N' ? KNIGHT : (pc) == 'B' ? BISHOP : (pc) == 'Q' ? QUEEN : (pc) == 'K' ? KING : PAWN);

// x: char[3] fr
#define IN_RNG(x) (((x)[0] >= 'a' && (x)[0] <= 'h' && (x)[1] >= '1' && (x)[1] <= '8'))

typedef enum { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN } pcat_t;

typedef enum { PLAY, CHECK, CHKMATE, STLMATE, MADRAW, RESIGN, TMOUT, LEAVE, ERROR } gmstat_t;

typedef enum { BLACK, WHITE } pcol_t;

typedef struct {
    pcat_t pcat;
    pcat_t prmtd;
    pcol_t pcol;
    bool ispromo;
    char frm[3];
    char to[3];
    enum { NRM, EPSNT, OO, OOO, EPSNTHNT } kind;
} move_t;

typedef struct {
    pcat_t pcat;
    pcol_t pcol;
    char fr[3];
    bool moved;
    bool captured;
} piece_t;

typedef struct {
    char fr[3];
    piece_t *piece;
} cell_t;

void board_set(cell_t (*board)[8], piece_t (*chsmn)[8]);
void make_move(cell_t (*board)[8], piece_t (*chsmn)[8], move_t *prev_mv, const char *mv, pcol_t *pcol, gmstat_t *gmstatus, char *gmerr_msg);
