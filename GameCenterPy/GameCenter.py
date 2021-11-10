import socket   # Network connection을 위한 디스크립터
import select   # Network event 선택자
import time     # 시간 관련된 내용
import math     # math module
import os.path  # file 또는 directory 검사용
import random
import sys
import pygame
import copy
from pygame.locals import *

FPS = 10
SpaceSize = 50
XMargin, YMargin = 10, 10
WindowWidth, WindowHeight = SpaceSize*8+XMargin+120, SpaceSize*8+YMargin+40

White = (255, 255, 255)
Black = (0, 0, 0)
GridColor = (0, 0, 0)
TextColor = (255, 255, 255)
HintColor = (0, 50, 255)

def drawBoard():
    # Draw background of board.
    displaySurf.blit(bgImage, bgImage.get_rect())

    # Draw grid lines of the board.
    for i in range(8+1):
        x = i * SpaceSize + XMargin
        pygame.draw.line(displaySurf, GridColor, (x, YMargin), (x, YMargin+8*SpaceSize))
        y = i * SpaceSize + YMargin
        pygame.draw.line(displaySurf, GridColor, (XMargin, y), (XMargin+8*SpaceSize, y))

    # Draw the Black & White tiles or hint spots.
    for i in range(64):
        cx, cy = getCenter(i)
        if board[i] == 1:
            pygame.draw.circle(displaySurf, White, (cx, cy), SpaceSize//2-4)
        elif board[i] == 2:
            pygame.draw.circle(displaySurf, Black, (cx, cy), SpaceSize//2-4)
        elif board[i] == 0:
            pygame.draw.rect(displaySurf, HintColor, (cx-8, cy-8, 16, 16))

def drawInfo():
    # Draws scores and whose turn it is at the bottom of the screen.
    scores = getScores()
    colors = ( "", "White", "Black" )
    str = f"White: {scores[0]:2}   Black: {scores[1]:2} Turn: {colors[turn]}"
    scoreSurf = normalFont.render(str, True, TextColor)
    scoreRect = scoreSurf.get_rect()
    scoreRect.bottomleft = (10, WindowHeight - 5)
    displaySurf.blit(scoreSurf, scoreRect)
    if players[1] != 'user' and players[2] != 'user':
        displaySurf.blit(userGameSurf, userGameRect)

def getCenter(p):
    return XMargin+(p%8)*SpaceSize+SpaceSize//2, YMargin+(p//8)*SpaceSize+SpaceSize//2

def getFlipTiles(board, p, t):
    dxy = ((1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1))
    x, y = p%8, p//8
    flips = []
    for dx, dy in dxy:
        isFlip = False
        tf = []
        nx, ny = x+dx, y+dy
        while nx >= 0 and nx < 8 and ny >= 0 and ny < 8:
            np = ny*8+nx
            if board[np] == t:
                isFlip = True
                break
            if board[np] != t^3: break
            tf.append(np)
            nx, ny = nx+dx, ny+dy
        if isFlip: flips+=tf
    return flips

def flipTiles(p):
    flips = getFlipTiles(board, p, turn)
    print(flips)
    for rgb in range(0, 255, 50):
        color = tuple([rgb]*3) if turn == 1 else tuple([255-rgb]*3)
        for t in flips:
            cx, cy = getCenter(t)
            pygame.draw.circle(displaySurf, color, (cx, cy), SpaceSize//2 - 4)
        pygame.display.update()
        time.sleep(1/FPS)
    for t in flips: board[t] = turn

def getHints():
    hc = 0
    for i in range(64):
        if board[i] == 1 or board[i] == 2: continue
        board[i] = 3
        ft = getFlipTiles(i, turn)
        if len(ft) >= 1:
            board[i] = 0
            hc += 1
    return hc

def getClickPosition(x, y):
    x, y = (x-XMargin)//SpaceSize, (y-YMargin)//SpaceSize
    return None if x < 0 or x >= 8 or y < 0 or y >= 8 else y*8+x

def newBoard():
    board = [3]*64
    board[27], board[28], board[35], board[36] = 1, 2, 2, 1
    board[20], board[29], board[34], board[43] = 0, 0, 0, 0
    return board, 4

def getScores():
    wscore, bscore = 0, 0
    for i in range(64):
        if board[i] == 1: wscore += 1
        elif board[i] == 2: bscore += 1
    return (wscore, bscore)

def sendReady():
    if players[turn] == 'user': return
    mesg = "0068 bd "
    for i in range(64): mesg += str(board[i])
    print(f"sendReady() : {mesg}")
    players[turn].send(mesg.encode())

def sendPrerun(p):
    global turn
    pboard = [k for k in board]
    if pboard[p] != 0: return False
    pboard[p] = turn
    ft = getFlipTiles(pboard, p, turn)
    for f in ft: pboard[f] = turn
    getHints(pboard, turn^3)
    return True

def place(p):
    global turn, hintCount
    if board[p] != 0: return False
    board[p] = turn
    flipTiles(p)
    turn ^= 3
    hintCount = getHints()
    if hintCount > 0:
        sendReady()
        return True
    turn ^= 3
    hintCount = getHints()
    if hintCount > 0:
        sendReady()
        return True
    onQuitGame()
    return False

def onStartGame():
    global board, hintCount, turn
    board, hintCount = newBoard()
    turn = 1
    for i in range(1, 3):
        if players[i] == 'user': continue
        mesg = f"0008 st 000{i}"
        players[i].send(mesg.encode())
    sendReady()

def onQuitGame():
    board, hintCount = newBoard()
    w, b = getScores()
    for i in range(1, 3):
        if players[i] == 'user': continue
        mesg = f"0008 qt {w:02}{b:02}"
        players[i].send(mesg.encode())
    players = [0, None, None]

def onUserGame():
    if players[0]==2 or players[1] == 'user' or players[2] == 'user': return
    while True:
        c = random.randrange(1, 3)
        if players[c] == None: break
    players[c] = 'user'
    players[0] += 1
    if players[0] == 2: onStartGame()

def onUser(x, y):
    p = getClickPosition(x, y)
    if p == None or board[p] != 0: return
    place(p)
    
def onConnect(sock):
    cSock, addr = sock.accept()
    print(f"Connection from {addr}")
    readSocks.append(cSock)
    if players[0]==2:
        cSock.close()
        return
    while True:
        c = random.randrange(1, 3)
        if players[c] == None: break
    players[c] = cSock
    players[0] += 1
    if players[0] == 2: onStartGame()
            
def onRecv(sock):
    buf = sock.recv(4)
    len = int(buf.decode())
    buf = sock.recv(len).decode()
    ss = buf.split()
    if ss[0] == 'ab': onAbort()
    elif ss[0] == 'pt':
        p = int(ss[1])
        place(p)
    elif ss[0] == 'pr':
        p = int(ss[1])
        sendPrerun(p)
    
def onIdle():
    drawBoard()
    drawInfo()
    pygame.display.update()
    # Process events
    for event in pygame.event.get():
        if event.type == QUIT or (event.type == KEYUP and event.key == K_ESCAPE):
            return False
        elif event.type == MOUSEBUTTONUP:
            mousex, mousey = event.pos
            if userGameRect.collidepoint( (mousex, mousey) ): onUserGame()
            elif players[turn] == 'user': onUser(mousex, mousey)
    return True

# Create a socket for listen
listenSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listenSock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# Bind address to listen socket
listenSock.bind(('', 8791))

# Listen socket
listenSock.listen(5)

# Connected client sockets
readSocks = [listenSock]

# Initialize PyGame
pygame.init()
displaySurf = pygame.display.set_mode((WindowWidth, WindowHeight))
pygame.display.set_caption("Game Center")
normalFont = pygame.font.Font('freesansbold.ttf', 16)
bigFont = pygame.font.Font('freesansbold.ttf', 32)

# Set up the background image.
boardImage = pygame.image.load('board.png')
boardImage = pygame.transform.smoothscale(boardImage, (8 * SpaceSize, 8 * SpaceSize))
boardImageRect = boardImage.get_rect()
boardImageRect.topleft = (XMargin, YMargin)
bgImage = pygame.image.load('bg.png')
bgImage = pygame.transform.smoothscale(bgImage, (WindowWidth, WindowHeight))
bgImage.blit(boardImage, boardImageRect)

# Setup buttons
userGameSurf = normalFont.render("User Game", True, TextColor)
userGameRect = userGameSurf.get_rect()
userGameRect.topright = (WindowWidth - 8, 10)

# Create board
board, hintCount = newBoard()
players = [ 0, None, None ]
turn = 1

# Run the main game.
while True:
    reads, _, _ = select.select(readSocks, [], [], 1/FPS)
    for s in reads:
        if s == listenSock: onConnect(s)
        else: onRecv(s)
    if not onIdle(): break
pygame.quit()
