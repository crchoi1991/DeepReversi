import socket
import random
import time
import numpy as np
import tensorflow as tf
from tensorflow import keras
from collections import deque
import os.path

class Game:
    cpPath = "training1/cp_{0:06}.ckpt"

    def __init__(self):
        # parameters
        self.alpha = 0.001
        self.gamma = 0.95
        self.epsilon = 1
        self.epsilon_min = 0.001
        self.epsilon_decay = 0.999
        self.batch_size = 64

        # memory
        self.memory = deque([], maxlen=1000)
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
        t = str(self.turn)
        state = np.zeros(64)
        for i in range(64):
            if buf[i] == "0": state[i] = 0
            else: state[i] =  1 if buf[i] == t else -1
        return True, state

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
        reward = result
        for nst, p in self.episode[::-1]:
            self.memory.append((nst, p, reward))
            reward *= self.gamma
        self.replay()
        return win, result

    def onTurn(self, buf):
        nst, p = self.action(buf)
        if p < 0: return False
        self.send("P%02d"%p)
        self.episode.append((nst, p))
        print("Place (%d, %d)"%(p/8, p%8))
        return True

    def buildModel(self):
        self.model = keras.Sequential([
            keras.layers.Dense(256, input_dim = 64, activation="relu"),
            keras.layers.Dense(256, activation="relu"),
            keras.layers.Dense(128, activation="relu"),
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

        # if prbability is below epsilon, choose random place
        if np.random.rand() <= self.epsilon:
            r = hints[random.randrange(len(hints))]
            ret, nst = self.preRun(r)
            if not ret: return None, -1
            return nst, r

        # choose max value's hint place
        maxp, maxnst, maxv = -1, None, -10000
        for h in hints:
            ret, nst = self.preRun(h)
            if not ret: return None, -1
            v = self.model.predict(nst.reshape(1, 64))
            if v > maxv: maxp, maxnst, maxv = h, nst, v
        return maxnst, maxp

    def replay(self):
        if len(self.memory) < self.batch_size: return
        minibatch = random.sample(self.memory, self.batch_size)

        xarray = np.array([k[0] for k in minibatch])
        yarray = np.array([k[2] for k in minibatch])

        self.model.fit(xarray, yarray, epochs=5)

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
            break
        if cmd == "A":
            print("Game Abort!!")
            break
        if cmd == "S": game.onStart(buf)
        elif cmd == "T":
            if not game.onTurn(buf): break

    game.close()
    time.sleep(1.0)

print(f"Wins: {winlose[2]}, Loses: {winlose[0]}")
