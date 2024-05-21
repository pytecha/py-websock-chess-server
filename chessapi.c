#define Py_SSIZE_T_CLEAN
#include <Python.h>
#include <sys/socket.h>
#include "libchess.h"

#define BUFF_SIZE 16

// !contextual (all)
#define PARSE_DATA \
    {\
        memcpy(mask, buff_in + 2, 4);\
        int i = 6, j = 0, k = i + length;\
        for(; i < k; i++, j++) {\
            buff_out[j] = buff_in[i] ^ mask[j % 4];\
        }\
        buff_out[j] = '\0';\
    }

// !contextual (buff_in)
#define SEND_DATA(fd, msg, len) { buff_in[0] = 0x81; buff_in[1] = len; memcpy(buff_in + 2, msg, len); send(fd, buff_in, len + 2, 0); }

// !contextual(buff_in, clnts_set)
#define BROADCAST(msg, len) \
    {\
        Py_BLOCK_THREADS\
        PyObject *iter = PyObject_GetIter(clnts_set);\
        if (iter != NULL) {\
            buff_in[0] = 0x81; buff_in[1] = len;\
            memcpy(buff_in + 2, msg, len);\
            PyObject *item; int fd;\
            while ((item = PyIter_Next(iter)) != NULL) {\
                fd = PyLong_AsLong(item);\
                send(fd, buff_in, len + 2, 0);\
                Py_DECREF(item);\
            }\
            Py_DECREF(iter);\
        }\
        Py_UNBLOCK_THREADS\
    }

// !contextual(clnts_set, host_fd)
#define CLOSE_HOST \
    {\
        PyObject *result = PyObject_CallMethod(clnts_set, "discard", "i", host_fd);\
        Py_XDECREF(result);\
        close(host_fd);\
        Py_RETURN_NONE;\
    }

PyObject* game_loop(PyObject *self, PyObject *args) {
    PyObject *clnts_set;
    int host_fd;
    PyArg_ParseTuple(args, "iO", &host_fd, &clnts_set);
    
    Py_BEGIN_ALLOW_THREADS
    uint8_t buff_in[BUFF_SIZE], buff_out[BUFF_SIZE - 4], mask[4];
    uint8_t host_fd_lb = (host_fd & 0xff00) >> 8, host_fd_rb = host_fd & 0xff;
    int32_t nbytes, length;
    
    game_init_lab:
    while (true) {
        nbytes = recv(host_fd, buff_in, BUFF_SIZE, 0);
        if (nbytes < 8  || (buff_in[0] & 0x0f) == 0x8 || nbytes > 14) break;
        length = buff_in[1] & 0x7f;
        PARSE_DATA
        if (strncmp((char*)buff_out, "0;", 2) == 0) {
            buff_out[0] = '1';
            BROADCAST(buff_out, length)
        } else if (strncmp((char*)buff_out, "1;", 2) == 0) {
            buff_out[0] = '2';
            int opnt_fd = (buff_out[length - 2] << 8) | buff_out[length - 1];
            buff_out[length - 2] = host_fd_lb;
            buff_out[length - 1] = host_fd_rb;
            SEND_DATA(opnt_fd, buff_out, length)
        } else if (strncmp((char*)buff_out, "2;", 2) == 0 || strncmp((char*)buff_out, "3;", 2) == 0) {
            int opnt_fd = (buff_out[length - 2] << 8) | buff_out[length - 1];
            pcol_t play_as = buff_out[2] - '0';
            if (buff_out[0] == '2') {
                buff_out[0] = '3';
                SEND_DATA(host_fd, buff_out, length)
                // to opnt
                buff_out[2] = "01"[!play_as];
                buff_out[length - 2] = host_fd_lb;
                buff_out[length - 1] = host_fd_rb;
                SEND_DATA(opnt_fd, buff_out, length)
            }
            cell_t  board[8][8];
            piece_t chsmn[4][8];
            pcol_t  turn = WHITE;
            gmstat_t gmstatus;
            move_t prev_mv;
            char gmerr_msg[20];
            
            board_set(board, chsmn);
            
            while (true) {
                nbytes = recv(host_fd, buff_in, BUFF_SIZE, 0);
                if (nbytes < 8  || (buff_in[0] & 0x0f) == 0x8 || nbytes > 14) {
                    /*uint8_t msg[] = { '5', ';', '7', ';', host_fd_lb, host_fd_rb };
                    BROADCAST(msg, 6)
                    sleep(1);*/
                    SEND_DATA(opnt_fd, "5;7", 3)
                    Py_BLOCK_THREADS
                    CLOSE_HOST
                    
                }
                length = buff_in[1] & 0x7f;
                PARSE_DATA
                if (strncmp((char*)buff_out, "4;", 2) == 0) {
                    make_move(board, chsmn, &prev_mv, (const char*)(buff_out + 2), &turn, &gmstatus, gmerr_msg);
                    char status[8];
                    sprintf(status, "5;%d%d%d%d", prev_mv.kind, prev_mv.prmtd, prev_mv.ispromo, gmstatus);
                    
                    if (play_as != turn) {
                        SEND_DATA(host_fd, status, 6)
                        if (gmstatus < 4) {
                            char move_stat[12];
                            memcpy(move_stat, buff_out, 8);
                            strncpy(move_stat + 8, status + 2, 4);
                            SEND_DATA(opnt_fd, move_stat, 12)
                        }
                    }
                    if (gmstatus == 2 || gmstatus == 3) goto game_init_lab;
                } else if (strncmp((char*)buff_out, "5;", 2) == 0) {
                    if (buff_out[length-1] != '.') {
                        SEND_DATA(opnt_fd, buff_out, length)
                    }
                    if (buff_out[2] != '9' && buff_out[2] != 'a') goto game_init_lab;
                }
            }
        }
    }
    Py_END_ALLOW_THREADS
    CLOSE_HOST
}

static PyMethodDef api_funcs[] = {
    {"game_loop", game_loop, METH_VARARGS, "runs game loop"},
    { NULL, NULL, 0, NULL }
};

static PyModuleDef chessapi = {
    PyModuleDef_HEAD_INIT,
    "chessapi",
    "simple chessapi module",
    -1,
    api_funcs
};

PyMODINIT_FUNC PyInit_chessapi() {
    return PyModule_Create(&chessapi);
}