import socket
import random
import time
import numpy as np
import tensorflow as tf
from tensorflow import keras
from collections import deque
import os.path

class Game:
    path = "training1/cp_{0:06}.ckpt"
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
            print(f"Socket error {socket.error}")
            return False
        except socket.timeout:
            print("Socket timeout")
            time.sleep(1.0)
        return True

    def close(self):
        self.sock.close()

    def recv(self):
        reads = { "S" : 1, "T" : 64, "Q" : 4, "A" : 0 }
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
        self.sock.send(buf.encode("ascii")
                       
    def onStart(self, buf):
        self.turn = int(buf[0])
        self.episode = []
        colors = ( "", "White", "Black" )
        print(f"Game {self.gameCount+1} {colors[turn]}")

    def onQuit(buf):
        self.gameCount += 1
        w = int(buf[:2])
        b = int(buf[2:])
        result = w-b if turn == 1 else b-w
        winText = ("You Lose!!", "Tie", "You Win!!")
        win = 1 if result == 0 else 2 if result > 0 else 0
        print(f"{winText[win]} W : {w}, B : {b}")
        reward = result
        for nst, p in self.episode[::-1]:
            memory.append((nst, p, reward))
            reward *= gamma
        game.replay()
        return win, result

    def onTurn(self, buf):
        nst, p = self.action(model, buf, turn)
        self.send("P%02d"%p)
        self.episode.append((nst, p))
        print("Place (%d, %d)"%(p/8, p%8))

    def buildModel(self):
        self.model = keras.Sequential([
            keras.layers.Dense(256, input_dim = 64, activation="relu"),
            keras.layers.Dense(256, activation="relu"),
            keras.layers.Dense(128, activation="relu"),
            keras.layers.Dense(1, activation="linear")
        ])
        self.model.compile(loss="mean_squared_error",
            optimizer=keras.optimizers.Adam(learning_rate=alpha))

        # Load weights
        dir = os.path.dirname(path)
        latest = tf.train.latest_checkpoint(dir)
        if not latest: return
        print(f"Load weights {latest}")
        self.model.load_weights(latest)
        idx = latest.find("cp_")
        self.gameCount = int(latest[idx+3:idx+9])
        self.epsilon *= self.epsilon_decay**self.gameCount
        if self.epsilon < self.epsilon_min: self.epsilon = self.epsilon_min
	
    def action(self, board, turn):
        hints = [i for i in range(64) if board[i] is 0]

        if np.random.rand() <= epsilon:
            r = hints[random.randrange(len(hints))]
            nst = self.preRun(r)
            return nst, r

        maxp, maxnst, maxv = 0, None, -10000
        for h in hints:
            nst = self.preRun(h)
            v = model.predict(nst)
            if v > maxv: maxp, maxnst, maxv = h, nst, v
        return maxnst, maxp

    def replay(self):
        if len(self.memory) < batch_size: return
        minibatch = random.sample(memory, batch_size)
        x, y = [], []
        for state, predict, action, reward in minibatch:
            x.append(state)
            if len(predict) != 64:
                print("Error in predict: %s"%predict)
                return
            for i in range(64):
                predict[i] *= 1.5**(reward if i is action else -reward)
                predict[i] = max(2.0, predict[i])
            y.append(predict)
        xarray = np.array(x)
        yarray = np.array(y)

        self.model.fit(xarray, yarray, epochs=5)

        # save checkpoint
        learningCount += 1
        model.save_weights(path.format(learningCount))
        print("Save weights : %s"%path.format(learningCount))

        if self.epsilon > self.epsilon_min: self.epsilon *= self.epsilon_decay

quitFlag = False
winlose = [0, 0, 0]
game = Game()

while not quitFlag:
    if not game.connect(): break
    
    episode = []
    while True:
        cmd, buf = game.recv(sock)
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
        elif cmd == "T": game.onTurn(buf)

    game.close()
    time.sleep(1.0)

print(f"Wins: {winlose[2]}, Loses: {winlose[0]}")
