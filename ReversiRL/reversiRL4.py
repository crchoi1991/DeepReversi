import socket
import random
import time
import numpy as np
import tensorflow as tf
from tensorflow import keras
from collections import deque
import os.path

class Game:
    cpPath = "training4/cp_{0:06}.ckpt"

    def __init__(self):
        # parameters
        self.alpha = 0.0005
        self.gamma = 0.95
        self.epsilon = 1
        self.epsilon_min = 0.001
        self.epsilon_decay = 0.999
        self.batch_size = 32

        # memory
        self.memory = deque([], maxlen=500)
        self.gameCount = 0

        # build model
        self.buildModel()

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.sock.connect(("127.0.0.1", 8888))
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
        reads = { "S" : 1, "T" : 64, "Q" : 4, "A" : 0, "R" : 64 }
        buf = b""
        try:
            cmd = self.sock.recv(1)
            if cmd == None or len(cmd) == 0: return "E", "Network closed"
        except socket.error: return "E", str(socket.error)
        cmd = cmd.decode("ascii")
        if cmd not in reads:
            return "E", f"Unknown command {cmd}"
        while len(buf) < reads[cmd]:
            try:
                t = self.sock.recv(reads[cmd]-len(buf))
                if t == None or len(t) == 0: return "E", "Network closed"
            except socket.error: return "E", str(socket.error)
            buf += t
        return cmd, buf.decode("ascii")

    def send(self, buf):
        self.sock.send(buf.encode("ascii"))

    def preRun(self, p):
        self.send("R%02d"%p)
        cmd, buf = self.recv()
        if cmd != "R": return False, None
        ref = (0.0, (self.turn==1)*2-1.0, (self.turn==2)*2-1.0, 0.0)
        st = np.array([ref[int(buf[i])] for i in range(64)])
        return True, st

    def onStart(self, buf):
        self.turn = int(buf[0])
        self.episode = []
        colors = ( "", "White", "Black" )
        print(f"Game {self.gameCount+1} {colors[self.turn]}")

    def onQuit(self, buf):
        self.gameCount += 1
        w = int(buf[:2])
        b = int(buf[2:])
        result = w-b if self.turn == 1 else b-w
        winText = ("You Lose!!", "Tie", "You Win!!")
        win = 1 if result == 0 else 2 if result > 0 else 0
        print(f"{winText[win]} W : {w}, B : {b}")
        reward = result + (win-1)*10
        for st, turn in self.episode[::-1]:
            self.memory.append((st, reward if turn==self.turn else -reward))
            reward *= self.gamma
        self.replay()
        return win, result

    def onTurn(self, buf):
        st, nst, p = self.action(buf)
        if p < 0: return False
        self.send("P%02d"%p)
        self.episode.append((st, self.turn^3))
        self.episode.append((nst, self.turn))
        print("Place (%d, %d)"%(p/8, p%8))
        return True

    def buildModel(self):
        self.model = keras.Sequential([
            keras.layers.Dense(128, input_dim = 64, activation="relu"),
            keras.layers.Dense(64, activation="relu"),
            keras.layers.Dense(32, activation="relu"),
            keras.layers.Dense(1, activation="linear")
        ])
        self.model.compile(loss="mean_squared_error",
            optimizer=keras.optimizers.Adam(learning_rate=self.alpha))

        # Load weights
        dir = os.path.dirname(Game.cpPath)
        latest = tf.train.latest_checkpoint(dir)
        if not latest: return
        print(f"Load weights {latest}")
        self.model.load_weights(latest)
        idx = latest.find("cp_")
        self.gameCount = int(latest[idx+3:idx+9])
        self.epsilon *= self.epsilon_decay**self.gameCount
        if self.epsilon < self.epsilon_min: self.epsilon = self.epsilon_min

    def action(self, board):
        hints = [i for i in range(64) if board[i] == "0"]
        ref = (0.0, (self.turn==2)*2-1.0, (self.turn==1)*2-1.0, 0.0)
        st = np.array([ref[int(board[i])] for i in range(64)])

        # if prbability is below epsilon, choose random place
        if np.random.rand() <= self.epsilon:
            r = hints[random.randrange(len(hints))]
            ret, nst = self.preRun(r)
            if not ret: return None, None, -1
            return st, nst, r

        # choose max value's hint place
        maxp, maxnst, maxv = -1, None, -10000
        for h in hints:
            ret, nst = self.preRun(h)
            if not ret: return None, None, -1
            v = self.model.predict(nst.reshape(1, 64))
            if v > maxv: maxp, maxnst, maxv = h, nst, v
        return st, maxnst, maxp

    def replay(self):
        if len(self.memory) < self.batch_size: return

        xarray = np.array([k[0] for k in self.memory])
        yarray = np.array([k[1] for k in self.memory])

        self.model.fit(xarray, yarray, epochs=10, batch_size=self.batch_size)

        # save checkpoint
        saveFile = Game.cpPath.format(self.gameCount)
        self.model.save_weights(saveFile)
        print(f"Save weights : {saveFile}")

        if self.epsilon > self.epsilon_min: self.epsilon *= self.epsilon_decay

quitFlag = False
winlose = [0, 0, 0]
game = Game()

while not quitFlag:
    if not game.connect(): break

    episode = []
    while True:
        cmd, buf = game.recv()
        if cmd == "E":
            print(f"Network Error!! : {buf}")
            break
        if cmd == "Q":
            w, r = game.onQuit(buf)
            winlose[w] += 1
            print(f"Wins: {winlose[2]}, Loses: {winlose[0]}, Ties: {winlose[1]}, {winlose[2]*100/(winlose[0]+winlose[1]+winlose[2]):.2f}%" )
            break
        if cmd == "A":
            print("Game Abort!!")
            break
        if cmd == "S": game.onStart(buf)
        elif cmd == "T":
            if not game.onTurn(buf): break

    game.close()
    time.sleep(1.0)

