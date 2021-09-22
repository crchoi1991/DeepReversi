import socket
import random
import keyboard

reads = { "S" : 1, "T" : 64, "Q" : 4 }
def recv(sock):
    buf = b""
    try:
        cmd = sock.recv(1)
        cmd = cmd.decode("ascii")
        while len(buf) < reads[cmd]:
            buf += sock.recv(reads[cmd]-len(buf))
    except socket.error:
        return "E", None
    return cmd, buf.decode("ascii")

def OnQuit(buf):
    w = int(buf[:2])
    b = int(buf[2:])
    print(f"W: {w}, B: {b}")
    winText = ("You Lose!!", "You Win!!", "Tie")
    win = 2 if w == b else turn == 1 if w > b else turn == 2
    print(winText[win])
    return win
    
quitFlag = False
winlose = [0, 0, 0]

while not quitFlag:
    print(f"Game {winlose[0]+winlose[1]+winlose[2]+1}")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try: sock.connect(("127.0.0.1", 8888))
    except socket.error: break
    except socket.timeout: break

    cmd, buf = recv(sock)
    turn = int(buf[0])
    print(f"Turn: {turn}")
    if cmd == "E": break

    while True:
        if keyboard.is_pressed("q"): quitFlag = True
        cmd, buf = recv(sock)
        if cmd == "E": break
        if cmd == "Q":
            w = OnQuit(buf)
            winlose[w] += 1
            break
        hints = []
        board = []
        for i in range(64):
            board.append(int(buf[i]))
            if board[-1] == 0:
                hints.append(i)
        p = random.randrange(len(hints))
        sock.send(("P%02d"%hints[p]).encode("ascii"))

    sock.close()

print(f"Wins: {winlose[1]}, Loses: {winlose[0]}")
