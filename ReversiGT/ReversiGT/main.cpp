#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>

#include "limittbl.h"

#if defined(_WIN32)
#define	getch	_getch
#endif

#define	LEVEL_EASY		(1)
#define	LEVEL_HARD		(5)

#define	USE_SCOREBOARD	(1)
#define	USE_PURE		(2)

struct Gameboard
{
	unsigned char board[64];
	unsigned score[3];		//	0 : empty 1 : white 2 : black
	unsigned hint;
};

static Gameboard Initboard =
{
	{
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 0, 3, 3, 3,
		3, 3, 3, 1, 2, 0, 3, 3,
		3, 3, 0, 2, 1, 3, 3, 3,
		3, 3, 3, 0, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3
	},
	60, 2, 2, 4
};

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

void DisplayBoard(Gameboard &board);
void PutBoard(Gameboard &board, unsigned int index, unsigned int turn);
unsigned GetOptimal(Gameboard &board, unsigned turn, unsigned depth = 3, unsigned method = USE_SCOREBOARD);

unsigned GetLevel();
unsigned GetPlayerTurn();
unsigned GetPlayerChoice(Gameboard &board);

int main(int argc, char *argv[])
{
	Gameboard board = Initboard;
	unsigned depth0, depth1, depth2;

	unsigned level = GetLevel();

	if(level == 1)
	{
		depth0 = LEVEL_EASY;
		depth1 = LEVEL_EASY+2;
		depth2 = LEVEL_EASY+4;
	}
	else
	{
		depth0 = LEVEL_HARD;
		depth1 = LEVEL_HARD+2;
		depth2 = LEVEL_HARD+4;
	}

	unsigned playerturn = GetPlayerTurn();

	DisplayBoard(board);

	for( unsigned turn = 1 ; ; turn ^= 3)
	{
		unsigned place;

		if(board.hint == 0)
		{
			place = 64;
		}
		else if(turn == playerturn)
		{
			place = GetPlayerChoice(board);
		}
		else
		{
			unsigned depth;
			unsigned scoremethod;
			if(board.score[0] > 50)
			{
				depth = depth0;
				scoremethod = USE_SCOREBOARD;
			}
			else if(board.score[0] > depth2)
			{
				depth = depth1;
				scoremethod = USE_SCOREBOARD;
			}
			else
			{
				depth = board.score[0];
				scoremethod = USE_PURE;
			}
			place = GetOptimal(board, turn, depth, scoremethod);
		}

		PutBoard(board, place, turn);

		DisplayBoard(board);

		if(!board.score[0] || !board.score[1] || !board.score[2])
			break;
	}
	
	printf("¡Ü : ¡Û = %d : %d\n", board.score[1], board.score[2]);

	while(getchar() != '\n') ;

	return 0;
}

unsigned GetLevel()
{
	unsigned ch;
	printf("Select game level : 1. Easy, 2. Hard: ");
	for( ; ; )
	{
		ch = getch();
		if(ch == '1' || ch == '2')
			break;
	}
	putchar('\n');
	return ch - '0';
}

unsigned GetPlayerTurn()
{
	unsigned ch;
	printf("Select your turn : 1. ¡Ü(first), 2. ¡Û(Second) 3. None: ");
	for( ; ; )
	{
		ch = getch();
		if(ch == '1' || ch == '2' || ch == '3')
			break;
	}
	putchar('\n');
	return ch - '0';
}

unsigned GetPlayerChoice(Gameboard &board)
{
	unsigned place;

	for( ; ; )
	{
		char str[512];

		fgets(str, 512, stdin);

		if(str[0] >= 'a' && str[0] <= 'h' && str[1] >= '1' && str[1] <= '8')
		{
			place = (str[0] - 'a')*8 + (str[1]-'1');
			if(board.board[place] == 0)
				break;
		}
		printf("Incorrect input\n");
	}
	return place;
}

#define	MINVAL (-1000)
#define	MAXVAL (1000)
#define	MAXDEPTH	(20)

unsigned GetOptimal(Gameboard &board, unsigned turn, unsigned maxdepth, unsigned method)
{
	unsigned nodenum = 0;

	if(!board.hint)
		return 64;
	
	if(board.hint == 1)
	{
		for( unsigned ui = 0 ; ui < 64 ; ++ui)
			if(board.board[ui] == 0)
				return ui;
	}

	Gameboard boards[MAXDEPTH];
	unsigned slot[MAXDEPTH];
	unsigned optindex[MAXDEPTH];
	int score[MAXDEPTH];
	unsigned depth = 0;
	int nodescore;

	boards[0] = board;
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
				for(unsigned r = 0 ; r < 64 ; r++)
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
		
//		for(unsigned r = 0 ; r < maxdepth ; ++r)
//			printf("[%c%c(%d)]", slot[r]/8 + 'a', slot[r]%8 + '1', score[r]);
//		printf("%d\n", nodescore);

		slot[depth]++;
	}

	return optindex[0];
}

void DisplayBoard(Gameboard &board)
{
	unsigned int ui, uj;

	system("cls");
	printf("     1    2    3    4    5    6    7    8     ¡Ü : ¡Û = %d : %d\n", board.score[1], board.score[2]);
	for(ui = 0 ; ui < 8 ; ui++)
	{
		printf("   +----+----+----+----+----+----+----+----+\n");
		printf(" %c |", ui+'a');
		for(uj = 0 ; uj < 8 ; uj++)
		{
			unsigned char v = board.board[ui*8+uj];
			if(v == 1)
				printf(" ¡Ü |");
			else if(v == 2)
				printf(" ¡Û |");
			else if(v == 0)
				printf(" ¡Ø |");
			else
				printf("    |");
		}
		putchar('\n');
	}
	printf("   +----+----+----+----+----+----+----+----+\n");
}

void PutBoard(Gameboard &board, unsigned int index, unsigned int turn)
{
	unsigned int ui;
	unsigned int r;
	unsigned int anti = (turn ^ 0x3);
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
#if 0
void MakeLimits()
{
	FILE *fp = fopen("limittbl.h", "w");
	unsigned limit[8];
	unsigned index, x, y;

	fprintf(fp, "unsigned limit[64][8] = \n{\n");
	for(index = 0 ; index < 64 ; index++ )
	{
		//	Get limit values of 8 axis
		x = index%8;
		y = index/8;
		limit[0] = y;
		limit[1] = (y > 7-x)?7-x:y;
		limit[2] = 7-x;
		limit[3] = (7-y > 7-x)?7-x:7-y;
		limit[4] = 7-y;
		limit[5] = (7-y > x)?x:7-y;
		limit[6] = x;
		limit[7] = (y > x)?x:y;
		fprintf(fp, "\t{ %d, %d, %d, %d, %d, %d, %d, %d }", limit[0], limit[1], limit[2], limit[3], limit[4], limit[5], limit[6], limit[7]);
		if(index != 63)
			fprintf(fp, ",\n");
	}
	fprintf(fp, "};");

	fclose(fp);
}
#endif