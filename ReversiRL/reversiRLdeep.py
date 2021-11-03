import socket
import random
import time
import numpy as np
import tensorflow as tf
from tensorflow import keras
import os.path

class Game:
    cpPath = "training_deep/cp_{0:06}.ckpt"

    def __init__(self):
        # parameters
        self.alpha = 0.0001
        self.gamma = 1.0
        self.epsilon = 1
        self.epsilon_min = 0.001
        self.epsilon_decay = 0.999
        self.batch_size = 32
        self.epochs = 4
        self.sampleSize = 256

        # memory
        self.memSize = 4096
        self.memory = [None]*self.memSize
        self.memp = 0
        self.choose = 0.5
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
        reward = [win/2, 1-win/2]
        for st, turn in self.episode[::-1]:
            rw = reward[turn!=self.turn]
            if rw <= 0.5 and np.random.rand() > self.choose: continue
            self.memory[self.memp%self.memSize] = (st, rw)
            self.memp += 1
            reward[0] *= self.gamma
            reward[1] *= self.gamma
        self.replay()
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
            keras.layers.Dense(512, input_dim=64, activation="sigmoid"),
            keras.layers.Dense(256, activation="relu"),
            keras.layers.Dense(128, activation="relu"),
            keras.layers.Dense(64, activation="relu"),
            keras.layers.Dense(1, activation="sigmoid")
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
        if self.memp < self.sampleSize: return

        if self.memp < self.memSize:
            sample = random.sample(self.memory[:self.memp], self.sampleSize)
        else:
            sample = random.sample(self.memory, self.sampleSize)
        xarray = np.array([k[0] for k in sample])
        yarray = np.array([k[1] for k in sample])

        r = self.model.fit(xarray, yarray,
                       epochs=self.epochs,
                       batch_size=self.batch_size)

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

