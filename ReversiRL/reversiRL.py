import socket
import random
import keyboard
import numpy as np
import tensorflow as tf
from tensorflow import keras
from collections import deque

# parameters
alpha = 0.001
gamma = 0.95
epsilon = 1
epsilon_min = 0.001
epsilon_decay = 0.999
batch_size = 24

# memory
memory = deque([], maxlen=500)

reads = { "S" : 1, "T" : 64, "Q" : 4, "A" : 0 }
def recv(sock):
    buf = b""
    try:
        while True:
            cmd = sock.recv(1)
            if cmd == None: return "E", "Network closed"
            if len(cmd) == 1: break
        cmd = cmd.decode("ascii")
        if cmd not in reads:
            return "E", f"Unknown command {cmd}"
        while len(buf) < reads[cmd]:
            t = sock.recv(reads[cmd]-len(buf))
            if t == None: return "E", "Network closed"
            buf += t
    except socket.error:
        return "E", "Socket error"
    return cmd, buf.decode("ascii")

def OnQuit(buf):
    w = int(buf[:2])
    b = int(buf[2:])
    result = w-b if turn == 1 else b-w
    print(f"W: {w}, B: {b}")
    winText = ("You Lose!!", "Tie", "You Win!!")
    win = 1 if result == 0 else 2 if result > 0 else 0
    print(winText[win])
    return win, result

def BuildModel():
	model = keras.Sequential([
		keras.layers.Dense(64, activation="relu"),
		keras.layers.Dense(64, activation="relu"),
		keras.layers.Dense(64, activation="linear")
	])
	model.compile(loss="mean_squared_error",
		optimizer=keras.optimizers.Adam(learning_rate=alpha))
	return model
	
def Action(model, board, turn):
    hints = []
    state = np.zeros(64);
    for i in range(64):
        c = int(buf[i])
        if c == 0: 
            state[i] = -1.0
            hints.append(i)
        elif c == 3: state[i] = -1.0
        elif c == turn: state[i] = 1.0
        else: state[i] = 0.0
        predict = model.predict(state)
        if np.random.rand() <= epsilon:
            return state, predict, hints[random.randrange(len(hints))]
        return state, predict, np.argmax(predict[board==-1])

def Replay(model):
    minibatch = random.sample(memory, batch_size)
    x, y = [], []
    for state, predict, action, reward in minibatch:
        x.append(state)
        predict[action] = reward
        y.append(predict)
    model.fit(x, y)
    if epsilon > epsilon_min: epsilon *= epsilon_decay
	
quitFlag = False
winlose = [0, 0, 0]
model = BuildModel()

while not quitFlag:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try: sock.connect(("127.0.0.1", 8888))
    except socket.error: break
    except socket.timeout: continue

    cmd, buf = recv(sock)
    turn = int(buf[0])
    colors = ( "", "White", "Black" )
    print(f"Game {winlose[0]+winlose[1]+winlose[2]+1} Turn: {colors[turn]}")
    if cmd == "E": break

    episode = []
    while True:
        if keyboard.is_pressed("q"): quitFlag = True
        cmd, buf = recv(sock)
        if cmd == "E":
            print(f"Network Error!! {buf}")
            break
        if cmd == "Q":
            w, r = OnQuit(buf)
            winlose[w] += 1
            reward = w*1000 + r*100
            for e in episode[::-1]:
                memory.append((e[0], e[1], e[2], reward))
                reward *= gamma
            break
        if cmd == "A":
            print("Game Abort!!")
            break
        if cmd != "T":
            print(f"Unknown command {cmd}")
            break
        state, predict, p = Action(model, buf, turn)
        sock.send(("P%02d"%p).encode("ascii"))
        episode.append((state, predict, p))

    sock.close()
    Replay(model)

print(f"Wins: {winlose[1]}, Loses: {winlose[0]}")
