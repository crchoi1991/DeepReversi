#include "FrameWork.h"
#include "Reversi.h"
#include <windows.h>
#include <stdio.h>

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
	scores[0] = scores[1] = 0;
	status = READY;
}

int Reversi::GetWidth()
{
	return RSIZE * CSIZE + XOFFSET * 2 + 100;
}

int Reversi::GetHeight()
{
	return RSIZE * CSIZE + YOFFSET * 2;
}

void Reversi::Draw(HDC hdc)
{
	for(int i = 0; i <= RSIZE; i++)
	{
		MoveToEx(hdc, XOFFSET + i*CSIZE, YOFFSET, 0);
		LineTo(hdc, XOFFSET + i*CSIZE, YOFFSET + RSIZE*CSIZE);
		MoveToEx(hdc, XOFFSET, YOFFSET + i*CSIZE, 0);
		LineTo(hdc, XOFFSET + RSIZE*CSIZE, YOFFSET + i*CSIZE);
	}
#if 0
	int xdiff[4][2] = { { 0, CSIZE }, { CSIZE, CSIZE }, { 0, CSIZE }, { 0, 0 } };
	int ydiff[4][2] = { { 0, 0 }, { 0, CSIZE }, { CSIZE, CSIZE }, { 0, CSIZE } };

	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (m_ucMaze[i] & (1 << j))
			{
				MoveToEx(hdc, XOFFSET + (i % MSIZE) * CSIZE + xdiff[j][0], YOFFSET + (i / MSIZE) * CSIZE + ydiff[j][0], 0);
				LineTo(hdc, XOFFSET + (i % MSIZE) * CSIZE + xdiff[j][1], YOFFSET + (i / MSIZE) * CSIZE + ydiff[j][1]);
			}
		}
		COLORREF c = 0;
		if (i == 240) c = RGB(255, 0, 0);
		else if (i == 135) c = RGB(0, 0, 255);
		else if (m_nStatus == SECOND_TARGET || m_bEditMode) c = (m_ucVisit[i] == SECOND_TARGET) ? RGB(255, 200, 200) : 0xffffff;
		else if (m_ucVisit[i] == FIRST_TARGET) c = RGB(255, 255, 200);
		else if (m_ucVisit[i] == RETURN_HOME) c = RGB(200, 200, 255);
		else if (m_ucVisit[i] == SECOND_TARGET) c = RGB(255, 200, 200);
		if (c)
		{
			HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
			HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
			SetDCBrushColor(hdc, c);
			SetBkColor(hdc, c);
			Rectangle(hdc, XOFFSET + (i % MSIZE) * CSIZE + 2, YOFFSET + (i / MSIZE) * CSIZE + 2,
				XOFFSET + (i % MSIZE) * CSIZE + CSIZE - 2, YOFFSET + (i / MSIZE) * CSIZE + CSIZE - 2);
			SelectObject(hdc, oldBrush);
			SelectObject(hdc, oldPen);
			if (m_nStatus == SECOND_TARGET || m_bEditMode)
			{
				char s = '0' + (m_ucMaze[i] >> 4);
				TextOutA(hdc, XOFFSET + (i % MSIZE) * CSIZE + 8, YOFFSET + (i / MSIZE) * CSIZE + 3, &s, 1);
			}
		}
	}

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