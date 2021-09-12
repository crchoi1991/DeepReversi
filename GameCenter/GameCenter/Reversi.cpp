#define _CRT_SECURE_NO_WARNINGS
#include "FrameWork.h"
#include "Reversi.h"
#include <windows.h>
#include <stdio.h>
#include <string>

#define	CSIZE	64
#define	XOFFSET	10
#define	YOFFSET	10

void Reversi::Init()
{
	ZeroMemory(board, sizeof(board));
	board[3 * RSIZE + 3] = 1;
	board[3 * RSIZE + 4] = 2;
	board[4 * RSIZE + 3] = 2;
	board[4 * RSIZE + 4] = 1;
	scores[0] = scores[1] = 2;
	status = READY;
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
			if(board[idx] == 0) continue;
			SetDCBrushColor(hdc, (board[idx]==1)?RGB(255, 255, 255):RGB(100, 100, 120));
			Ellipse(hdc, XOFFSET+CSIZE*c+5, YOFFSET+CSIZE*r+5, XOFFSET+CSIZE*c+CSIZE-3, YOFFSET+CSIZE*r+CSIZE-3);
			Ellipse(hdc, XOFFSET+CSIZE*c+4, YOFFSET+CSIZE*r+4, XOFFSET+CSIZE*c+CSIZE-4, YOFFSET+CSIZE*r+CSIZE-4);
			Ellipse(hdc, XOFFSET+CSIZE*c+3, YOFFSET+CSIZE*r+3, XOFFSET+CSIZE*c+CSIZE-5, YOFFSET+CSIZE*r+CSIZE-5);
		}
	}
	char str[128];
	int len;
	len = sprintf(str, "Score(White) : %3d", scores[0]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET, str, len); 
	len = sprintf(str, "Score(White) : %3d", scores[1]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+20, str, len); 
	SelectObject(hdc, oldObj);
#if 0
	HGDIOBJ oldObj = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, XOFFSET + (m_nCur % MSIZE) * CSIZE + 2, YOFFSET + (m_nCur / MSIZE) * CSIZE + 2,
		XOFFSET + (m_nCur % MSIZE) * CSIZE + CSIZE - 2, YOFFSET + (m_nCur / MSIZE) * CSIZE + CSIZE - 2);
	SelectObject(hdc, oldObj);

	Rectangle(hdc, CSIZE * MSIZE + XOFFSET + 10, YOFFSET, CSIZE * MSIZE + XOFFSET + 200, YOFFSET + 80);
	char str[128];
	int len;
	SetBkColor(hdc, 0xffffff);
	len = sprintf(str, "X : %d, Y : %d", m_nCur % MSIZE, MSIZE - m_nCur / MSIZE - 1);
	TextOutA(hdc, CSIZE * MSIZE + XOFFSET + 20, YOFFSET + 10, str, len);
	len = sprintf(str, "1st %d, Ret %d, 2nd %d", m_nScore[0], m_nScore[1], m_nScore[2]);
	TextOutA(hdc, CSIZE * MSIZE + XOFFSET + 20, YOFFSET + 30, str, len);
	len = sprintf(str, "Total %d", m_nScore[0] * 5 + m_nScore[1] + m_nScore[2]);
	TextOutA(hdc, CSIZE * MSIZE + XOFFSET + 20, YOFFSET + 50, str, len);

	TextOutA(hdc, CSIZE * MSIZE + XOFFSET + 40, YOFFSET + 360, "Algorithm of KPU 2015", 21);
#endif
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