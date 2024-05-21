from socket import _socket
from base64 import b64encode
from hashlib import sha1
import _thread, struct
from chessapi import game_loop

CONN_FDS = set()

def main():
    server = _socket.socket(_socket.AF_INET, _socket.SOCK_STREAM, 0)
    server.bind((b'', 5000))
    server.listen(32)
    
    print('server listening at [...]:5000')
    
    buff = bytearray(1024)
    
    http_res = bytearray(129)
    struct.pack_into('97s', http_res,  0,  b'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ')
    
    ws_accept = bytearray(60)
    struct.pack_into('36s', ws_accept, 24, b'258EAFA5-E914-47DA-95CA-C5AB0DC85B11')
    
    while True:
        try:
            fd, addr = server._accept()
            conn = _socket.socket(fileno=fd)
            nbytes = conn.recv_into(buff)
            
            ws_key_index = buff.find(b'Sec-WebSocket-Key: ', 0, nbytes)
            if ws_key_index == -1:
                conn.sendall(b'HTTP/1.1 502 Bad Gateway\r\n\r\n')
                conn.close()
                continue
            
            struct.pack_into('24s', ws_accept, 0,  struct.unpack_from('24s', buff, ws_key_index + 19)[0])
            
            struct.pack_into('28s', http_res, -32, b64encode(sha1(ws_accept).digest()))
            struct.pack_into('4s',  http_res, -4,  b'\r\n\r\n')
            conn.sendall(http_res)
            
            conn.sendall(b'\x81\x040;%s'%struct.pack('!H', fd))
            
            _thread.start_new_thread(game_loop, (fd, CONN_FDS))
            CONN_FDS.add(fd)
            conn.detach()
            
            print('connection from:', '%s:%d'%addr)
            
        except OSError as e:
            print(e.__class__.__name__, e)
            if conn:=locals().get('conn', None):
                conn.close()
        except KeyboardInterrupt:
            break
        
    server.close()
    
main()
