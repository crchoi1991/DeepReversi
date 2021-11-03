import socket
import random
import time
import numpy as np
import tensorflow as tf
from tensorflow import keras
import os.path

class Game:
    def __init__(self):
        self.gameCount = 0
        # build model
        self.buildModel()

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.sock.connect(("127.0.0.1", 8791))
        except socket.error:
            print(f"Socket error : {socket.error.errno}")
            return False
        except socket.timeout:
            print("Socket timeout")
            time.sleep(1.0)
        return True

    def close(self):
        self.sock.close()

    def recv(self):
        buf = b""
        while len(buf) < 4:
            try:
                t = self.sock.recv(4-len(buf))
                if t == None or len(t) == 0: return "et", "Network closed"
            except socket.error: return "et", str(socket.error)
            buf += t
        needed = int(buf.decode("ascii"))
        buf = b""
        while len(buf) < needed:
            try:
                t = self.sock.recv(needed-len(buf))
                if t == None or len(t) == 0: return "et", "Network closed"
            except socket.error: return "et", str(socket.error)
            buf += t
        ss = buf.decode("ascii").split()
        if ss[0] == "ab": return "ab", "abort"
        return ss[0], ss[1]

    def send(self, buf):
        self.sock.send(buf.encode("ascii"))

    def preRun(self, p):
        self.send("%04d pr %4d"%(8, p))
        cmd, buf = self.recv()
        if cmd != "pr": return False, None
        ref = (0.0, (self.turn==1)*2-1.0, (self.turn==2)*2-1.0, 0.0)
        st = np.array([ref[int(buf[i])] for i in range(64)])
        return True, st

    def onStart(self, buf):
        self.turn = int(buf)
        self.episode = []
        colors = ( "", "White", "Black" )
        print(f"Game {self.gameCount+1} {colors[self.turn]}")

    def onQuit(self, buf):
        self.gameCount += 1
        w = int(buf[:2])
        b = int(buf[2:])
        result = w-b if self.turn == 1 else b-w
        winText = ("Lose", "Draw", "Win")
        win = (result == 0) + (result > 0)*2
        print(f"{winText[win]} W : {w}, B : {b}")
        return win, result

    def onBoard(self, buf):
        st, nst, p = self.action(buf)
        if p < 0: return False
        self.send("%04d pt %4d"%(8, p))
        self.episode.append((st, self.turn^3))
        self.episode.append((nst, self.turn))
        print("(%d, %d)"%(p/8, p%8), end="")
        return True

    def buildModel(self):
        self.model = keras.Sequential([
            keras.layers.Dense(1, input_dim=64, activation="linear"),
        ])
        self.model.compile(loss="mean_squared_error",
            optimizer=keras.optimizers.Adam())
        print(self.model.layers[0].get_weights()[0].shape)
        w = np.ones((64, 1))
        b = np.zeros(1)
        self.model.layers[0].set_weights([w.reshape(64, 1), b])
    
    def action(self, board):
        hints = [i for i in range(64) if board[i] == "0"]
        ref = (0.0, (self.turn==2)*2-1.0, (self.turn==1)*2-1.0, 0.0)
        st = np.array([ref[int(board[i])] for i in range(64)])

        # choose max value's hint place
        maxp, maxnst, maxv = -1000, None, -10000
        for h in hints:
            ret, nst = self.preRun(h)
            if not ret: return None, None, -1
            v = self.model.predict(nst.reshape(1, 64))
            if v[0,0] > maxv: maxp, maxnst, maxv = h, nst, v[0,0]
        return st, maxnst, maxp

quitFlag = False
winlose = [0, 0, 0]
game = Game()

while not quitFlag:
    if not game.connect(): break

    episode = []
    while True:
        cmd, buf = game.recv()
        if cmd == "et":
            print(f"Network Error!! : {buf}")
            break
        if cmd == "qt":
            w, r = game.onQuit(buf)
            winlose[w] += 1
            print(f"Wins: {winlose[2]}, Loses: {winlose[0]}, Draws: {winlose[1]}, {winlose[2]*100/(winlose[0]+winlose[1]+winlose[2]):.2f}%" )
            break
        if cmd == "ab":
            print("Game Abort!!")
            break
        if cmd == "st":
            game.onStart(buf)
        elif cmd == "bd":
            if not game.onBoard(buf): break

    game.close()
    time.sleep(1.0)

