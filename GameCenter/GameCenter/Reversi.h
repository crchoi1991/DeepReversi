#pragma once

#define	RSIZE	8

class Reversi
{
public:
	Reversi();
	/// <summary>
	///	Start game
	/// </summary>
	void Start();
	int GetTurn() const { return turn; }
	void SetPlayer(int idx, int p) { players[idx] = p; }
	const char *GetBoard() const { return board; }
	const int *GetScores() const { return scores; }
	void Place(int p);

private:
	char board[RSIZE * RSIZE];
	int scores[3], hint;
	int turn;					//	1: White, 2: Black
	int players[2];
};