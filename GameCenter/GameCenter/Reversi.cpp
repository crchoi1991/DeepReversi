#define _CRT_SECURE_NO_WARNINGS
#include "FrameWork.h"
#include "Reversi.h"
#include <stdio.h>
#include <string>
#include "limittbl.h"

Reversi::Reversi() : hint(0), turn(0)
{ 
	memset(board, 3, sizeof(board));
	scores[0] = scores[1] = scores[2] = 0;
	players[0] = players[1] = 0;
	turn = 1;
}

void Reversi::Start()
{
	memset(board, 3, sizeof(board));
	board[3 * RSIZE + 3] = 1;
	board[3 * RSIZE + 4] = 2;
	board[4 * RSIZE + 3] = 2;
	board[4 * RSIZE + 4] = 1;
	board[2 * RSIZE + 4] = 0;
	board[3 * RSIZE + 5] = 0;
	board[4 * RSIZE + 2] = 0;
	board[5 * RSIZE + 3] = 0;
	scores[0] = 60;
	scores[1] = scores[2] = 2;
	hint = 4;
	turn = 1;
}

bool Reversi::Place(int p)
{
	static int dxy[8] = { -8, -7, 1, 9, 8, 7, -1, -9 };
	if(board[p] != 0) return false;
	board[p] = turn;
	for(int dir = 0; dir < 8; dir++)
	{
		int cv = 0, antiturn = turn^3, rv = 0;
		for(int i = 1; i <= limit[p][dir]; i++)
		{
			int np = p + dxy[dir]*i;
			if(board[np] == turn) { rv = cv; break; }
			if(board[np] != antiturn) break;
			cv++;
		}
		if(rv == 0) continue;
		for(int i = 1; i <= rv; i++) board[p+dxy[dir]*i] = turn;
	}
	for(int t = 0; t < 2; t++)
	{
		int hints = 0;
		turn ^= 3;
		scores[1] = scores[2] = 0;
		for(p = 0; p < 64; p++)
		{
			if(board[p] == 1 || board[p] == 2) { scores[board[p]]++; continue; }
			board[p] = 3;
			for(int dir = 0; dir < 8; dir++)
			{
				int cv = 0, antiturn = turn^3, rv = 0;
				for(int i = 1; i <= limit[p][dir]; i++)
				{
					int np = p + dxy[dir]*i;
					if(board[np] == turn) { rv = cv; break; }
					if(board[np] != antiturn) break;
					cv++;
				}
				if(rv)
				{
					hints++;
					board[p] = 0;
					break;
				}
			}
		}
		scores[0] = 64 - scores[1] - scores[2];
		if(hints) return true;
		if(!scores[0]) return false;
	}
	return false;
}

