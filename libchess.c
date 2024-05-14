#include "libchess.h"

void board_set(cell_t (*board)[8], piece_t (*chsmn)[8]) {
    cell_t  *tmp_cl;
    piece_t *tmp_pc;
    pcat_t pcat;
    pcol_t pcol;
    for (int i = 0, j; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            tmp_cl = &board[i][j];
            strncpy(tmp_cl->fr, (char[]){ 'a' + j, '8' - i, '\0' }, 3);
            if(i > 1 && i < 6) {
                tmp_cl->piece = NULL;
                continue;
            }
            pcat = (
                (i == 1 || i == 6) ? PAWN : (
                    (j == 0 || j == 7) ? ROOK :
                    (j == 1 || j == 6) ? KNIGHT :
                    (j == 2 || j == 5) ? BISHOP :
                    j == 3 ? QUEEN : KING
                )
            );
            pcol = i > 1; // 0 - 1 BLACK(0), 6 - 7 WHITE(1)
            tmp_pc = &chsmn[i % 4][j];
            INIT_MEN(tmp_pc, pcat, tmp_cl->fr, pcol);
            tmp_cl->piece = tmp_pc;
        }
    }
}

bool parse_move(const char *mv, move_t *mv_t, pcol_t pcol) {
    char hstg = ' ', frm[] = "  ", to[] = "  ", pc = mv[0];
    pcat_t pcat = PCAT(pc);
    const char *mv_ptr = mv + (pcat == PAWN ? 0 : 1);
    size_t len = strlen(mv_ptr);
    
    char *h_ptr = strrchr(mv_ptr, '=');
    if(h_ptr && len - (h_ptr - mv_ptr) > 1) {
        hstg = *(h_ptr + 1);
    }
    
    if (pc == '@' && len > 4) {
        if (mv_ptr[1] < '0' || mv_ptr[1] > '5') return false;
        strncpy(frm, mv_ptr + 2, 2);
        strncpy(to, mv_ptr + 4, 2);
        if (!(IN_RNG(frm) && IN_RNG(to))) return false;
        pcat = mv_ptr[1] - '0';
    } else if (len > 1 && pc != '@') {
        char c = mv_ptr[0];
        char *o_ptr = strchr(mv_ptr, 'O');
        char *x_ptr = strchr(mv_ptr, 'x');
        if (o_ptr) {
            if (strncmp(o_ptr, "O-O-O", 5) == 0) {
                strncpy(to, (pcol == WHITE ? "c1" : "c8"), 3);
            } else if(strncmp(o_ptr, "O-O", 2) == 0) {
                strncpy(to, (pcol == WHITE ? "g1" : "g8"), 3);
            } else {
                return false;
            }
            strncpy(frm, (pcol == WHITE ? "e1" : "e8"), 3);
            pcat = KING;
        } else if(x_ptr) {
            int x_indx = x_ptr - mv_ptr;
            if ((x_indx < 1 && pcat == PAWN)  || x_indx > 2) return false;
            if(x_indx == 0) {
                strncpy(to, mv_ptr + 1, 2);
            } else if(x_indx == 1) {
                frm[isdigit(c)] = c;
                strncpy(to, mv_ptr + 2, 2);
            } else {
                if(len - x_indx < 3) return false;
                strncpy(frm, mv_ptr, 2);
                strncpy(to, mv_ptr + 3, 2);
            }
        } else {
            if(len > 3 && isalnum(mv_ptr[2]) && isalnum(mv_ptr[3])) {
                    strncpy(frm, mv_ptr, 2);
                    strncpy(to, mv_ptr + 2, 2);
            } else if(len > 2 && isalnum(mv_ptr[2])) {
                frm[isdigit(c)] = c;
                strncpy(to, mv_ptr + 1, 2);
            } else {
                strncpy(to, mv_ptr, 2);
            }
        }
        if ((frm[0] != ' ' && (frm[0] < 'a' || frm[0] > 'h')) ||
            (frm[1] != ' ' && (frm[1] < '1' || frm[1] > '8')) || !IN_RNG(to)
        ) {
            return false;
        }
    } else {
        return false;
    }
    
    strncpy(mv_t->frm, frm, 3);
    strncpy(mv_t->to, to, 3);
    mv_t->pcat = pcat;
    mv_t->prmtd = PCAT(hstg);
    mv_t->ispromo = (pcat == PAWN && mv_t->prmtd != PAWN && mv_t->prmtd != KING);
    
    return true;
    
}

bool king_safe(cell_t (*board)[8], piece_t (*chsmn)[8], move_t *prev_mv, move_t *cur_mv) {
    piece_t *tmp_pc, (*opp_mn)[8] = chsmn + !cur_mv->pcol * 2;
    char *kng_fr = cur_mv->pcat == KING ? cur_mv->to : chsmn[cur_mv->pcol * 3][4].fr;
    int fd, rd, rng, chks = 0;
    cell_t *tmp_cl;
    for (int i = 0, j, k; i < 2; i++) {
        for (j = 0; j < 8; j++) {
            if ((tmp_pc = &opp_mn[i][j])->captured) continue;
            fd = kng_fr[0] - tmp_pc->fr[0];
            rd = kng_fr[1] - tmp_pc->fr[1];
            
            if ((tmp_pc->pcat == PAWN && abs(fd) == 1 && abs(rd) == 1) ||
                (tmp_pc->pcat == BISHOP && (rng = abs(fd)) == abs(rd)) ||
                (tmp_pc->pcat == KNIGHT && ((abs(fd) == 1 && abs(rd) == 2) || (abs(fd) == 2 && abs(rd) == 1))) ||
                (tmp_pc->pcat == ROOK && ((fd == 0 && (rng = abs(rd)) > 0) || ((rng = abs(fd)) > 0 && rd == 0))) ||
                (tmp_pc->pcat == QUEEN && (((rng = abs(fd)) == abs(rd)) || ((fd == 0 && (rng = abs(rd)) > 0) || ((rng = abs(fd)) > 0 && rd == 0)))) ||
                (tmp_pc->pcat == KING && ((abs(fd) == 0 && abs(rd) == 1) || (abs(fd) == 1 && abs(rd) == 0) || (abs(fd) == 1 && abs(rd) == 1)))
            ) {
                chks++;
                if(tmp_pc->pcat != KING && strncmp(cur_mv->to, tmp_pc->fr, 2) == 0) {
                    chks--;
                } else if (tmp_pc->pcat == PAWN && prev_mv->kind == EPSNTHNT) {
                    tmp_pc = board['8' - cur_mv->to[1]][cur_mv->to[0] - 'a'].piece;
                    tmp_cl = &board['8' - (cur_mv->to[1] + (cur_mv->pcol == WHITE ? -1 : 1))][cur_mv->to[0] - 'a'];
                    if (!tmp_pc && tmp_cl->piece && tmp_cl->piece->pcol != cur_mv->pcol && strncmp(prev_mv->to, tmp_cl->fr, 2) == 0)
                        chks--;
                } else if (tmp_pc->pcat == BISHOP || tmp_pc->pcat == ROOK || tmp_pc->pcat == QUEEN) {
                    fd = fd / rng;
                    rd = rd / rng;
                    for (k = 1; k <= rng; k++) {
                        tmp_cl = &board['8' - (tmp_pc->fr[1] + rd * k)][(tmp_pc->fr[0] + fd * k) - 'a'];
                        if (k < rng && (!strncmp(tmp_cl->fr, cur_mv->to, 2) || (tmp_cl->piece && (strncmp(cur_mv->frm, tmp_cl->fr, 2) && strncmp(prev_mv->frm, tmp_cl->fr, 2))))) {
                            chks--;
                            break;
                        }
                    }
                }
            }
        }
    }
    return chks == 0;
}

bool can_move(cell_t (*board)[8], piece_t (*chsmn)[8], move_t *prev_mv, move_t *cur_mv, cell_t *frm, cell_t *to, bool dry_run, char *gmerr_msg) {
    strcpy(gmerr_msg, "move illegal");
    if (!frm || !frm->piece ||
        frm->piece->pcol != cur_mv->pcol||
        (to->piece && to->piece->pcol == cur_mv->pcol)
    ) {
        return false;
    }
    int fd = to->fr[0] - frm->fr[0];
    int rd = to->fr[1] - frm->fr[1];
    int rng;
    
    if (frm->piece->pcat == PAWN) {
        if ((fd >= -1 && fd <= 1 && abs(rd) == 1) || (fd == 0 && abs(rd) == 2 && !frm->piece->moved)) {
            if (fd != 0 && !to->piece && prev_mv->kind == EPSNTHNT) {
                cell_t *tmp_cl = &board['8' - (to->fr[1] + (cur_mv->pcol == WHITE ? -1 : 1))][to->fr[0] - 'a'];
                if (!tmp_cl->piece || (tmp_cl->piece && tmp_cl->piece->pcol == cur_mv->pcol) || strncmp(prev_mv->to, tmp_cl->fr, 2) != 0) return false;
                if(!dry_run) {
                    tmp_cl->piece->captured = true;
                    tmp_cl->piece = NULL;
                    cur_mv->kind = EPSNT;
                    strcpy(gmerr_msg, "");
                    return true;
                }
            } else if((fd == 0 && to->piece) || (fd != 0 && !to->piece) ||
                (cur_mv->pcol == WHITE && rd < 0) || (cur_mv->pcol == BLACK && rd > 0)) return false;
            if(!dry_run)
                cur_mv->kind = abs(rd) > 1 ? EPSNTHNT : NRM;
        } else 
            return false;
    } else if ((frm->piece->pcat == BISHOP && !((rng = abs(fd)) == abs(rd))) ||
        (frm->piece->pcat == KNIGHT && !((abs(fd) == 1 && abs(rd) == 2) || (abs(fd) == 2 && abs(rd) == 1))) ||
        (frm->piece->pcat == ROOK && !((fd == 0 && (rng = abs(rd)) > 0) || ((rng = abs(fd)) > 0 && rd == 0))) ||
        (frm->piece->pcat == QUEEN && !(((rng = abs(fd)) == abs(rd)) || ((fd == 0 && (rng = abs(rd)) > 0) || ((rng = abs(fd)) > 0 && rd == 0))))
    ) {
        return false;
    } else if (frm->piece->pcat == KING) {
        if (!frm->piece->moved && ((rd == 0 && fd == 2 && !chsmn[cur_mv->pcol * 3][7].moved) ||
            (rd == 0 && fd == -2 && !chsmn[cur_mv->pcol * 3][0].moved))
        ) {
            cell_t *tmp_cl;
            move_t tmp_mv;
            memcpy((void*)&tmp_mv, (void*)cur_mv, sizeof(tmp_mv));
            int rng =  (fd > 0) ? 2 : 3;
            for (int i = 0; i <= rng; i++) {
                tmp_cl = &board['8' - frm->fr[1]][(frm->fr[0] + fd / 2 * i) - 'a'];
                strncpy(tmp_mv.to, tmp_cl->fr, 3);
                if (!king_safe(board, chsmn, prev_mv, &tmp_mv))  return false;
            }
            if(!dry_run)
                cur_mv->kind = (fd > 0) ? OO : OOO;
        } else if (!((fd == 0 && abs(rd) == 1) || (abs(fd) == 1 && rd == 0) || (abs(fd) == 1 && abs(rd) == 1)))
            return false;
    }
    if (frm->piece->pcat == BISHOP || frm->piece->pcat == ROOK || frm->piece->pcat == QUEEN) {
        fd = fd / rng;
        rd = rd / rng;
        cell_t *tmp_cl;
        for (int i = 1; i <= rng; i++) {
            tmp_cl = &board['8' - (frm->fr[1] + rd * i)][(frm->fr[0] + fd * i) - 'a'];
            if (tmp_cl->piece && i < rng) return false;
        }
    }
    strcpy(gmerr_msg, "");
    return true;
}

void make_move(cell_t (*board)[8], piece_t (*chsmn)[8], move_t *prev_mv, const char *mv, pcol_t *pcol, gmstat_t *gmstatus, char *gmerr_msg) {
    move_t cur_mv;
    if(!parse_move(mv, &cur_mv, *pcol)) {
        strcpy(gmerr_msg, "format unmatch");
        *gmstatus = ERROR;
        return;
    }
    cur_mv.pcol = *pcol;
    cur_mv.kind = NRM;
    
    *gmstatus = PLAY;
    
    cell_t *frm, *to = &board['8' - cur_mv.to[1]][cur_mv.to[0] - 'a'];
    
    if(cur_mv.frm[0] != ' ' && cur_mv.frm[1] != ' ') {
        frm = &board['8' - cur_mv.frm[1]][cur_mv.frm[0] - 'a'];
    } else {
        /* relative frm fr search start */
        cell_t  *tmp_cfrm;
        piece_t *tmp_pfrm;
        char f = cur_mv.frm[0];
        char r = cur_mv.frm[1];
        int nfrc = 0, fc = 0, rc = 0; // counters
        for (int i = 0, j; i < 2; i++) {
            for (j = 0; j < 8; j++) {
                tmp_pfrm = &(chsmn + *pcol * 2)[i][j];
                tmp_cfrm = &board['8' - tmp_pfrm->fr[1]][tmp_pfrm->fr[0] - 'a'];
                
                if (tmp_pfrm->captured || !tmp_cfrm->piece || tmp_pfrm->pcat != cur_mv.pcat ||
                    !can_move(board, chsmn, prev_mv, &cur_mv, tmp_cfrm, to, true, gmerr_msg)
                ) {
                    continue;
                }
                
                if(f == ' ' && r == ' ') {
                    frm = tmp_cfrm;
                    nfrc++;
                } else if(f == tmp_pfrm->fr[0]) {
                    frm = tmp_cfrm;
                    fc++;
                } else if(r == tmp_pfrm->fr[1]) {
                    frm = tmp_cfrm;
                    rc++;
                }
            }
        }
        if(nfrc + fc + rc == 0) {
            strcpy(gmerr_msg, "move illegal");
            *gmstatus = ERROR;
            return;
        }
        else if(nfrc > 1 || fc > 1 || rc > 1) {
            strcpy(gmerr_msg, "many routes");
            *gmstatus = ERROR;
            return;
        }
        /* relative frm fr search end */
    }
    strncpy(cur_mv.frm, frm->fr, 3);
    
    bool canmove = can_move(board, chsmn, prev_mv, &cur_mv, frm, to, false, gmerr_msg);
    
    if(!canmove) {
        *gmstatus = ERROR;
        return;
    }
    
    bool kingsafe = king_safe(board, chsmn, prev_mv, &cur_mv);
    
    if (kingsafe) {
        if (frm->piece->pcat == KING && cur_mv.kind != NRM) {
            cell_t *tmp_cfrm = &board[*pcol * 7][(cur_mv.kind == OO ? 7 : 0)];
            cell_t *tmp_cto = &board[*pcol * 7][(cur_mv.kind == OO ? 5 : 3)];
            if(!tmp_cfrm->piece) {
                strcpy(gmerr_msg, "move illegal");
                *gmstatus = ERROR;
                return;
            }
            tmp_cto->piece = tmp_cfrm->piece;
            strncpy(tmp_cfrm->piece->fr, tmp_cto->fr, 3);
            tmp_cfrm->piece->moved = true;
            tmp_cfrm->piece = NULL;
        } else if(frm->piece->pcat == PAWN) {
            bool prmtbl = to->fr[1] == '1' || to->fr[1] == '8';
            if(cur_mv.ispromo && prmtbl) {
                frm->piece->pcat = cur_mv.prmtd;
                cur_mv.pcat = cur_mv.prmtd;
            } else if (prmtbl) {
                frm->piece->pcat = QUEEN;
                cur_mv.pcat = QUEEN;
                cur_mv.prmtd = QUEEN;
            }
        }
        
        if (to->piece) {
            to->piece->captured = true;
        }
        strncpy(frm->piece->fr, to->fr, 3);
        to->piece = frm->piece;
        frm->piece->moved = true;
        
        prev_mv->pcat = frm->piece->pcat;
        prev_mv->kind = cur_mv.kind;
        prev_mv->prmtd = cur_mv.prmtd;
        prev_mv->ispromo = cur_mv.ispromo;
        strncpy(prev_mv->frm, frm->fr, 3);
        strncpy(prev_mv->to, to->fr, 3);
        
        /* mate check start */
        bool ismate = true;
        
        cell_t *tmp_cfrm, *tmp_cto;
        
        piece_t *tmp_pfrm;
        
        pcol_t tmp_pcol = (*pcol == WHITE) ? BLACK : WHITE;
        
        cur_mv.pcol = tmp_pcol;
        cur_mv.kind = NRM;
        
        int p_frd[][2] = { {0, 1}, {1, 1}, {-1, 1}, {0, 2}, {0, -1}, {1, -1}, {-1, -1}, {0, -2} };
        int n_frd[][2] = { {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1} };
        int q_frd[][2] = { {-1, -1}, {-1, 1}, {1, -1}, {1, 1}, {-1, 0}, {0, -1}, {0, 1}, {1, 0} };
        int (*frd)[2];
        
        for (int i = 0, j, k; i < 2; i++) {
            for (j = 0; j < 8; j++) {
                tmp_pfrm = &(chsmn + tmp_pcol * 2)[i][j];
                
                if(tmp_pfrm->captured) continue;
                
                cur_mv.pcat = tmp_pfrm->pcat;
                char rng;
                
                if(tmp_pfrm->pcat == PAWN) {
                    rng = (tmp_pfrm->moved) ? 3 : 4;
                    frd = p_frd + (tmp_pcol == WHITE ? 0 : 4);
                } else if (tmp_pfrm->pcat ==BISHOP || tmp_pfrm->pcat == ROOK) {
                    rng = 4;
                    frd = q_frd + (tmp_pfrm->pcat == BISHOP ? 0 : 4);
                } else {
                    rng = 8;
                    frd = (tmp_pfrm->pcat == KNIGHT ? n_frd : q_frd);
                }
                
                for (k = 0; k < rng; k++) {
                    char f = tmp_pfrm->fr[0] + frd[k][0];
                    char r = tmp_pfrm->fr[1] + frd[k][1];
                    
                    if (f < 'a' || f > 'h' || r < '1' || r > '8') continue;
                    
                    tmp_cfrm = &board['8' - tmp_pfrm->fr[1]][tmp_pfrm->fr[0] - 'a'];
                    tmp_cto = &board['8' - r][f - 'a'];
                    
                    if (tmp_cto->piece && tmp_cto->piece->pcol == tmp_pcol) continue;
                    
                    strncpy(cur_mv.frm, tmp_cfrm->fr, 3);
                    strncpy(cur_mv.to, tmp_cto->fr, 3);
                    
                    if (can_move(board, chsmn, prev_mv, &cur_mv, tmp_cfrm, tmp_cto, true, gmerr_msg) &&
                        king_safe(board, chsmn, prev_mv, &cur_mv)
                    ) {
                        ismate = false;
                        goto finalize;
                    }
                }
            }
        }
        finalize:
        do {
            char *kng_fr = (chsmn + tmp_pcol * 2)[tmp_pcol * 1][4].fr;
            
            strncpy(cur_mv.frm, kng_fr, 3);
            strncpy(cur_mv.to, kng_fr, 3);
            cur_mv.pcat = KING;
            
            bool opp_ks = king_safe(board, chsmn, prev_mv, &cur_mv);
            if(!opp_ks) {
                *gmstatus = CHECK;
            }
            if(ismate) {
                if (opp_ks)
                    *gmstatus = STLMATE;
                else
                    *gmstatus = CHKMATE;
            }
            /* mate check end */
            
            frm->piece = NULL;
            
            *pcol = (*pcol == WHITE) ? BLACK : WHITE;
        } while (false);
    } else {
        strcpy(gmerr_msg, "king unsafe");
        *gmstatus = ERROR;
    }
}