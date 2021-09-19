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
	/// <summary>
	///	Draw reversi board
	/// </summary>
	/// <param name="hdc">[IN] device context handle</param>
	void Draw(HDC hdc);
	int GetTurn() const { return turn; }
	void SetPlayer(int idx, int p) { players[idx] = p; }
	const char *GetBoard() const { return board; }
	const int *GetScores() const { return scores; }
	void Place(int p, int turn);
	static int GetWidth();
	static int GetHeight();

private:
	char board[RSIZE * RSIZE];
	int scores[3], hint;
	int turn;
	int players[2];
};