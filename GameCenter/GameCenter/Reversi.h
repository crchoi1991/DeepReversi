#pragma once

#define	RSIZE	8

class Reversi
{
public:
	enum Status { READY, READY1, TURN_BLACK, TURN_WHITE, END };

public:
	/// <summary>
	///	Initialize reversi board
	/// </summary>
	void Init();
	/// <summary>
	///	Draw reversi board
	/// </summary>
	/// <param name="hdc">[IN] device context handle</param>
	void Draw(HDC hdc);
	Status GetStatus() const { return status; }
	void SetStatus(Status newStatus) { status = newStatus; }
	static int GetWidth();
	static int GetHeight();

private:
	unsigned char board[RSIZE * RSIZE];
	int scores[2];
	Status status;
};