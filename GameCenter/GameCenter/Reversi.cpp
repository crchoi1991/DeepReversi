#define _CRT_SECURE_NO_WARNINGS
#include "FrameWork.h"
#include "Reversi.h"
#include <windows.h>
#include <stdio.h>
#include <string>

#define	CSIZE	64
#define	XOFFSET	10
#define	YOFFSET	10

Reversi::Reversi() : hint(0), turn(0)
{ 
	memset(board, 3, sizeof(board));
	scores[0] = scores[1] = scores[2] = 0;
	players[0] = players[1] = 0;
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
}

int Reversi::GetWidth()
{
	return RSIZE * CSIZE + XOFFSET * 2 + 130;
}

int Reversi::GetHeight()
{
	return RSIZE * CSIZE + YOFFSET * 2;
}

void Reversi::Draw(HDC hdc)
{
	HGDIOBJ oldObj = SelectObject(hdc, GetStockObject(DC_BRUSH));
	SelectObject(hdc, GetStockObject(DC_PEN));
	SetDCBrushColor(hdc, RGB(255, 255, 255));
	SetDCPenColor(hdc, RGB(255, 0, 0));
	Rectangle(hdc, 0, 0, GetWidth(), GetHeight());
	SetDCPenColor(hdc, RGB(0, 0, 0));
	SetDCBrushColor(hdc, RGB(240, 240, 150));
	Rectangle(hdc, XOFFSET, YOFFSET, XOFFSET+RSIZE*CSIZE+2, YOFFSET+RSIZE*CSIZE+2);
	for(int i = 0; i <= RSIZE; i++)
	{
		MoveToEx(hdc, XOFFSET + i*CSIZE, YOFFSET, 0);
		LineTo(hdc, XOFFSET + i*CSIZE, YOFFSET + RSIZE*CSIZE);
		MoveToEx(hdc, XOFFSET, YOFFSET + i*CSIZE, 0);
		LineTo(hdc, XOFFSET + RSIZE*CSIZE, YOFFSET + i*CSIZE);
	}
	for(int r = 0; r < RSIZE; r++)
	{
		for(int c = 0; c < RSIZE; c++)
		{
			int idx = r*RSIZE+c;
			if(board[idx] == 3) continue;
			SetDCBrushColor(hdc, (board[idx]==1)?RGB(255, 255, 255):RGB(100, 100, 120));
			if(board[idx] == 0) Rectangle(hdc, XOFFSET+CSIZE*c+5, YOFFSET+CSIZE*r+5, XOFFSET+CSIZE*c+CSIZE-5, YOFFSET+CSIZE*r+CSIZE-5);
			else
			{
				Ellipse(hdc, XOFFSET+CSIZE*c+5, YOFFSET+CSIZE*r+5, XOFFSET+CSIZE*c+CSIZE-3, YOFFSET+CSIZE*r+CSIZE-3);
				Ellipse(hdc, XOFFSET+CSIZE*c+4, YOFFSET+CSIZE*r+4, XOFFSET+CSIZE*c+CSIZE-4, YOFFSET+CSIZE*r+CSIZE-4);
				Ellipse(hdc, XOFFSET+CSIZE*c+3, YOFFSET+CSIZE*r+3, XOFFSET+CSIZE*c+CSIZE-5, YOFFSET+CSIZE*r+CSIZE-5);
			}
		}
	}
	char str[128];
	int len;
	const char *playerstr[3] = { "Wait", "Network", "User" };
	len = sprintf(str, "Player ¡Ü : %s", playerstr[players[0]]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+20, str, len); 
	len = sprintf(str, "Player ¡Û : %s", playerstr[players[1]]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+40, str, len); 
	len = sprintf(str, "Score  ¡Ü : %3d", scores[1]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+80, str, len); 
	len = sprintf(str, "Score  ¡Û : %3d", scores[2]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+100, str, len); 
	SelectObject(hdc, oldObj);
}

#if 0
const int d[4] = { -MSIZE, 1, MSIZE, -1 };
const int anti[4] = { 2, 3, 0, 1 };

bool MyMaze::Move(int dir)
{
	if (m_ucMaze[m_nCur] & 1 << dir)
	{
		if (m_bEditMode == false) return false;
		m_ucMaze[m_nCur] &= ~(1 << dir);
		m_ucMaze[m_nCur + d[dir]] &= ~(1 << anti[dir]);
	}
	m_nCur += d[dir];
	if (m_nStatus == FIRST_TARGET)
	{
		m_nScore[0]++;
		if (m_nCur == 135)
			m_nStatus = RETURN_HOME;
	}
	else if (m_nStatus == RETURN_HOME)
	{
		m_nScore[1]++;
		if (m_nCur == 240)
			m_nStatus = SECOND_TARGET;
	}
	else if (m_nStatus == SECOND_TARGET)
	{
		m_nScore[2] += m_ucMaze[m_nCur] >> 4;
		if (m_nCur == 135)
			m_nStatus = COMPLETE;
	}
	m_ucVisit[m_nCur] = m_nStatus;
}

#endif