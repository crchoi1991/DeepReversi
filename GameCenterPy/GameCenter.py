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

FPS = 10 # frames per second to update the screen
ANIMATIONSPEED = 25 # integer from 1 to 100, higher is faster animation

HintTile, WhiteTile, BlackTile, EmptyTile = 0, 1, 2, 3
SpaceSize = 50
XMargin, YMargin = 10, 10
WindowWidth, WindowHeight = SpaceSize*8+XMargin*2, SpaceSize*8+YMargin*2

#              R    G    B
White      = (255, 255, 255)
Black      = (  0,   0,   0)
GREEN      = (  0, 155,   0)
BRIGHTBLUE = (  0,  50, 255)

TEXTBGCOLOR1 = BRIGHTBLUE
TEXTBGCOLOR2 = GREEN
GRIDLINECOLOR = Black
TEXTCOLOR = White
HintColor = (200, 200, 0)

def playerTurn(board):
    movexy = None
    while movexy == None:
        checkForQuit()
        for event in pygame.event.get():
            if event.type == MOUSEBUTTONUP:
                mousex, mousey = event.pos
                #if newGameRect.collidepoint( (mousex, mousey) ): return True
                movexy = getSpaceClicked(mousex, mousey)
                if movexy != None and not board[movexy] != 0: movexy = None

        # Draw the game board.
        drawBoard(board)
        #drawInfo(board, playerTile, computerTile, turn)

        # Draw the "New Game" and "Hints" buttons.
        #DISPLAYSURF.blit(newGameSurf, newGameRect)
        #DISPLAYSURF.blit(hintsSurf, hintsRect)

        MAINCLOCK.tick(FPS)
        pygame.display.update()

    # Make the move and end the turn.
    makeMove(board, playerTile, movexy[0], movexy[1], True)

def runGame():
    # Reset the board and game.
    mainBoard, hintCount = getNewBoard()
    turn = WhiteTile

    # Draw the starting board and ask the player what color they want.
    drawBoard(mainBoard)
    playerTile, computerTile = WhiteTile, BlackTile

    # Make the Surface and Rect objects for the "New Game" and "Hints" buttons
    newGameSurf = FONT.render('New Game', True, TEXTCOLOR, TEXTBGCOLOR2)
    newGameRect = newGameSurf.get_rect()
    newGameRect.topright = (WindowWidth - 8, 10)
    hintsSurf = FONT.render('Hints', True, TEXTCOLOR, TEXTBGCOLOR2)
    hintsRect = hintsSurf.get_rect()
    hintsRect.topright = (WindowWidth - 8, 40)

    while True: # main game loop
        # Keep looping for player and computer's turns.
        if turn == playerTile: playerTurn(mainBoard)
        else:
            # Draw the board.
            drawBoard(mainBoard)
            drawInfo(mainBoard, playerTile, computerTile, turn)

            # Draw the "New Game" and "Hints" buttons.
            DISPLAYSURF.blit(newGameSurf, newGameRect)
            DISPLAYSURF.blit(hintsSurf, hintsRect)

            # Make the move and end the turn.
            hints = [i for i in range(64) if board[i] == HintTile]
            p = random.choice(hints)
            makeMove(mainBoard, computerTile, p, True)
        turn ^= 3

    # Display the final score.
    drawBoard(mainBoard)
    scores = getScoreOfBoard(mainBoard)

    # Determine the text of the message to display.
    if scores[playerTile] > scores[computerTile]:
        text = 'You beat the computer by %s points! Congratulations!' % \
               (scores[playerTile] - scores[computerTile])
    elif scores[playerTile] < scores[computerTile]:
        text = 'You lost. The computer beat you by %s points.' % \
               (scores[computerTile] - scores[playerTile])
    else:
        text = 'The game was a tie!'

    textSurf = FONT.render(text, True, TEXTCOLOR, TEXTBGCOLOR1)
    textRect = textSurf.get_rect()
    textRect.center = (int(WindowWidth / 2), int(WindowHeight / 2))
    DISPLAYSURF.blit(textSurf, textRect)

    # Display the "Play again?" text with Yes and No buttons.
    text2Surf = BIGFONT.render('Play again?', True, TEXTCOLOR, TEXTBGCOLOR1)
    text2Rect = text2Surf.get_rect()
    text2Rect.center = (int(WindowWidth / 2), int(WindowHeight / 2) + 50)

    # Make "Yes" button.
    yesSurf = BIGFONT.render('Yes', True, TEXTCOLOR, TEXTBGCOLOR1)
    yesRect = yesSurf.get_rect()
    yesRect.center = (int(WindowWidth / 2) - 60, int(WindowHeight / 2) + 90)

    # Make "No" button.
    noSurf = BIGFONT.render('No', True, TEXTCOLOR, TEXTBGCOLOR1)
    noRect = noSurf.get_rect()
    noRect.center = (int(WindowWidth / 2) + 60, int(WindowHeight / 2) + 90)

    while True:
        # Process events until the user clicks on Yes or No.
        checkForQuit()
        for event in pygame.event.get(): # event handling loop
            if event.type == MOUSEBUTTONUP:
                mousex, mousey = event.pos
                if yesRect.collidepoint( (mousex, mousey) ):
                    return True
                elif noRect.collidepoint( (mousex, mousey) ):
                    return False
        DISPLAYSURF.blit(textSurf, textRect)
        DISPLAYSURF.blit(text2Surf, text2Rect)
        DISPLAYSURF.blit(yesSurf, yesRect)
        DISPLAYSURF.blit(noSurf, noRect)
        pygame.display.update()
        MAINCLOCK.tick(FPS)


def translateBoardToPixelCoord(x, y):
    return XMargin+x*SpaceSize+SpaceSize//2, YMargin+y*SpaceSize+SpaceSize//2

def animateTileChange(tilesToFlip, tileColor, additionalTile):
    additionalTileColor = White if tileColor == WhiteTile else Black
    additionalTileX, additionalTileY = translateBoardToPixelCoord(additionalTile[0], additionalTile[1])
    pygame.draw.circle(DISPLAYSURF, additionalTileColor, (additionalTileX, additionalTileY), SpaceSize//2-4)
    pygame.display.update()

    for rgbValues in range(0, 255, int(ANIMATIONSPEED * 2.55)):
        if tileColor == WhiteTile:
            color = tuple([rgbValues] * 3) # rgbValues goes from 0 to 255
        elif tileColor == BlackTile:
            color = tuple([255 - rgbValues] * 3) # rgbValues goes from 255 to 0

        for x, y in tilesToFlip:
            centerx, centery = translateBoardToPixelCoord(x, y)
            pygame.draw.circle(DISPLAYSURF, color, (centerx, centery), int(SpaceSize / 2) - 4)
        pygame.display.update()
        MAINCLOCK.tick(FPS)
        checkForQuit()

def drawBoard(board):
    # Draw background of board.
    DISPLAYSURF.blit(BGIMAGE, BGIMAGE.get_rect())

    # Draw grid lines of the board.
    for i in range(8+1):
        x = i * SpaceSize + XMargin
        pygame.draw.line(DISPLAYSURF, GRIDLINECOLOR, (x, YMargin), (x, YMargin+8*SpaceSize))
        y = i * SpaceSize + YMargin
        pygame.draw.line(DISPLAYSURF, GRIDLINECOLOR, (XMargin, y), (XMargin+8*SpaceSize, y))

    # Draw the Black & White tiles or hint spots.
    for i in range(64):
        centerx, centery = translateBoardToPixelCoord(i%8, i//8)
        if board[i] == WhiteTile:
            pygame.draw.circle(DISPLAYSURF, White, (centerx, centery), SpaceSize//2-4)
        elif board[i] == BlackTile:
            pygame.draw.circle(DISPLAYSURF, Black, (centerx, centery), SpaceSize//2-4)
        elif board[i] == HintTile:
            pygame.draw.rect(DISPLAYSURF, HintColor, (centerx-8, centery-8, 16, 16))

def getSpaceClicked(mousex, mousey):
    x, y = (mousex-XMargin)//SpaceSize, (mousey-YMargin)//SpaceSize
    return None if x < 0 or x >= 8 or y < 0 or y >= 8 else y*8+x

def drawInfo(board, playerTile, computerTile, turn):
    # Draws scores and whose turn it is at the bottom of the screen.
    scores = getScoreOfBoard(board)
    scoreSurf = FONT.render("Player Score: %s    Computer Score: %s    %s's Turn" % (str(scores[playerTile]), str(scores[computerTile]), turn.title()), True, TEXTCOLOR)
    scoreRect = scoreSurf.get_rect()
    scoreRect.bottomleft = (10, WindowHeight - 5)
    DISPLAYSURF.blit(scoreSurf, scoreRect)

def getNewBoard():
    board = [EmptyTile]*64
    board[27], board[28], board[35], board[36] = WhiteTile, BlackTile, BlackTile, WhiteTile
    board[20], board[29], board[34], board[43] = HintTile, HintTile, HintTile, HintTile
    return board, 4

def getBoardWithValidMoves(board, tile):
    # Returns a new board with hint markings.
    dupeBoard = copy.deepcopy(board)

    for x, y in getValidMoves(dupeBoard, tile):
        dupeBoard[x][y] = HINT_TILE
    return dupeBoard


def getScoreOfBoard(board):
    xscore, oscore = 0, 0
    for i in range(64):
        if board[i] == WhiteTile: xscore += 1
        elif board[i] == BlackTile: oscore += 1
    return {WhiteTile:xscore, BlackTile:oscore}


def makeMove(board, tile, p, realMove=False):
    if board[p] != HintTile: return False
    board[xstart][ystart] = tile
    animateTileChange(tilesToFlip, tile, p)
    for i in tilesToFlip: board[i] = tile
    return True

def checkForQuit():
    for event in pygame.event.get((QUIT, KEYUP)): # event handling loop
        if event.type == QUIT or (event.type == KEYUP and event.key == K_ESCAPE):
            pygame.quit()
            sys.exit()


global MAINCLOCK, DISPLAYSURF, FONT, BIGFONT, BGIMAGE

pygame.init()
MAINCLOCK = pygame.time.Clock()
DISPLAYSURF = pygame.display.set_mode((WindowWidth, WindowHeight))
pygame.display.set_caption("Game Center")
FONT = pygame.font.Font('freesansbold.ttf', 16)
BIGFONT = pygame.font.Font('freesansbold.ttf', 32)

# Set up the background image.
boardImage = pygame.image.load('flippyboard.png')
# Use smoothscale() to stretch the board image to fit the entire board:
boardImage = pygame.transform.smoothscale(boardImage, (8 * SpaceSize, 8 * SpaceSize))
boardImageRect = boardImage.get_rect()
boardImageRect.topleft = (XMargin, YMargin)
BGIMAGE = pygame.image.load('flippybackground.png')
# Use smoothscale() to stretch the background image to fit the entire window:
BGIMAGE = pygame.transform.smoothscale(BGIMAGE, (WindowWidth, WindowHeight))
BGIMAGE.blit(boardImage, boardImageRect)

# Run the main game.
while True:
    if runGame() == False: break


