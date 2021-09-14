#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <winsock2.h>

#include "limittbl.h"

#if defined(_WIN32)
#define	getch	_getch
#define kbhit	_kbhit
#endif

#define	LEVEL_HARD		(5)

#define	USE_SCOREBOARD	(1)
#define	USE_PURE		(2)

static int ScoreBoard[64] =
{
	10,  1,  3,  2,  2,  3,  1, 10,
	 1, -5, -1, -1, -1, -1, -5,  1,
	 3, -1,  0,  0,  0,  0, -1,  3,
	 2, -1,  0,  0,  0,  0, -1,  2,
	 2, -1,  0,  0,  0,  0, -1,  2,
	 3, -1,  0,  0,  0,  0, -1,  3,
	 1, -5, -1, -1, -1, -1, -5,  1,
	10,  1,  3,  2,  2,  3,  1, 10
};

class Game
{
public:
	Game() : sock(INVALID_SOCKET) { }
	~Game() { if(sock != INVALID_SOCKET) closesocket(sock); }
	int GetHint() const { return hint; }
	bool IsGameOver() const { return !scores[0] || !scores[1] || !scores[2]; }
	bool Ready()
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);

		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		addr.sin_port = htons(8888);
		if( connect(sock, (SOCKADDR *)&addr, sizeof(addr)) != 0 ) return false;

		char buf[512];
		int len = recv(sock, buf, 512, 0);
		if(len <= 0) return false;
		buf[len] = 0;

		turnColor = atoi(buf);
		printf("turn : %s\n", turn?"White":"Black");

		return true;
	}
	bool RunTurn()
	{
		char buf[512];
		for(int len = 0; len < 64; )
		{
			int slen = recv(sock, buf+len, 512-len, 0);
			if(slen <= 0) return false;
			len += slen;
		}
		memset(scores, 0, sizeof(scores));
		hint = 0;
		for(int i = 0; i < 64; i++)
		{
			board[i] = buf[i]-'0';
			if(buf[i] == '0') { scores[0]++; hints[hint++] = i; }
			else if(buf[i] == '3') scores[0]++;
			else scores[buf[i]-'0']++;
		}
		int depth, method;
		if(scores[0] > 50) depth = LEVEL_HARD, method = USE_SCOREBOARD;
		else if(scores[0] > LEVEL_HARD+4) depth = LEVEL_HARD+2, method = USE_SCOREBOARD;
		else depth = scores[0], method = USE_PURE;
		int choice = GetOptimal(depth, method);
	}
	int GetOptimal(int depth, int method);
private:
	char board[64], hints[64];
	int scores[3];		//	0 : empty 1 : white 2 : black
	int hint, turn;
	int turnColor;
	SOCKET sock;
};

int main(int argc, char *argv[])
{
	bool PlayGame();
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2), &wsa);

	while(PlayGame());

	WSACleanup();
}

bool PlayGame()
{
	Game game;
	if(!game.Ready()) return false;

	bool continueFlag = true;
	while(game.IsGameOver())
	{
		if(kbhit() && getch() == 'q') continueFlag = false;
		if(!game.RunTurn()) return false;
	}
	
	return continueFlag;
}

#define	MINVAL		(-1000)
#define	MAXVAL		(1000)
#define	MAXDEPTH	(20)

struct Gameboard { char board[64]; int score[3], hint; };
void PutBoard(Gameboard &board, int index, int turn);

int Game::GetOptimal(int maxdepth, int method)
{
	int nodenum = 0;

	if(!hint) return 64;
	if(hint == 1) return hints[0];

	Gameboard boards[MAXDEPTH];
	int slot[MAXDEPTH];
	int optindex[MAXDEPTH];
	int score[MAXDEPTH];
	int depth = 0;
	int nodescore;

	memcpy(boards[0].board, board, sizeof(board));
	boards[0].score[0] = scores[0];
	boards[0].score[1] = scores[1];
	boards[0].score[2] = scores[2];
	boards[0].hint = hint;
	slot[0] = 0;

	score[0] = (turn == 1)? MINVAL : MAXVAL;	
	
	for( ; ; )
	{
		//	Get next hint slot
		while(slot[depth] < 64)
		{
			if(boards[depth].board[slot[depth]] == 0)
				break;
			slot[depth]++;
		}

		if(slot[depth] < 64 || (boards[depth].hint == 0 && slot[depth] == 64))
		{
			boards[depth+1] = boards[depth];
			PutBoard(boards[depth+1], slot[depth], turn);
			depth++; turn ^= 3;
			score[depth] = (turn == 1)? MINVAL : MAXVAL;
			slot[depth] = 0;
			nodenum++;

			if(depth != maxdepth)
			{
				continue;
			}

			if(method == USE_SCOREBOARD)
			{
				nodescore = 0;
				for(int r = 0 ; r < 64 ; r++)
				{
					if(boards[depth].board[r] == 1)
						nodescore += ScoreBoard[r];
					else if(boards[depth].board[r] == 2)
						nodescore -= ScoreBoard[r];
				}
			}
			else
			{
				nodescore = boards[depth].score[1]-boards[depth].score[2];
			}
		}
		else if(depth == 0)
		{
			break;
		}
		else
		{
			nodescore = score[depth];
		}

		//	upward pass
		bool bFlag = false;
		if(turn == 2)	//	max check
		{
			if(score[depth-1] >= nodescore)
				bFlag = true;
		}
		else	//	min check
		{
			if(score[depth-1] <= nodescore)
				bFlag = true;
		}

		//	up a step
		depth--;
		turn ^= 3;

		//	Success
		if(!bFlag)
		{
			score[depth] = nodescore;
			optindex[depth] = slot[depth];

			if(depth > 0)
			{
				bFlag = false;
				if(turn == 2)	//	max check
				{
					if(score[depth-1] >= nodescore)
						bFlag = true;
				}
				else	//	min check
				{
					if(score[depth-1] <= nodescore)
						bFlag = true;
				}

				//	2nd ancester fail
				if(bFlag)
				{
					depth--;
					turn ^= 3;
				}
			}
		}
		
		slot[depth]++;
	}

	return optindex[0];
}

void PutBoard(Gameboard &board, int index, int turn)
{
	int ui, r, anti = (turn ^ 0x3);
	static int dxy[8] = { -8, -7, 1, 9, 8, 7, -1, -9 };

	if(index != 64)
	{
		board.board[index] = turn;
		board.score[0]--;
		board.score[turn]++;
		
		for(ui = 0 ; ui < 8 ; ui++)
		{
			if(limit[index][ui] == 0) continue;
			r = index + dxy[ui];
			int k;
			for( k = 1 ; k < limit[index][ui] ; ++k)
			{
				if(board.board[r] != anti)
					break;
				r += dxy[ui];
			}
			if(board.board[r] == turn)
			{
				while(--k)
				{
					r -= dxy[ui];
					board.board[r] = turn;
					board.score[turn]++;
					board.score[anti]--;
				}
			}
		}
	}

	//	clear hint slots
	board.hint = 0;
	for(index = 0 ; index < 64 ; index++ )
	{
		if(board.board[index] != 3 && board.board[index] != 0)
			continue;

		//	clear
		board.board[index] = 3;

		for(ui = 0 ; ui < 8 ; ui++)
		{
			r = index + dxy[ui];
			int k;
			for( k = 1 ; k < limit[index][ui] ; k++)
			{
				if(board.board[r] != turn)
					break;
				r += dxy[ui];
			}
			if(k != 1 && board.board[r] == anti)
			{
				board.board[index] = 0;
				board.hint++;
				break;
			}
		}
	}
}
