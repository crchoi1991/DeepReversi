#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define	_CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include "GameCenter.h"
#include <stdio.h>
#include <stdlib.h>
#include <commdlg.h>
#include <winsock2.h>
#include <time.h>
#include "Reversi.h"

#define	WM_ACCEPT			(WM_USER+1)
#define	WM_CLIENT			(WM_USER+2)

#define	CSIZE	64
#define	XOFFSET	10
#define	YOFFSET	10
#define	WIDTH	(RSIZE * CSIZE + XOFFSET * 2 + 130)
#define	HEIGHT	(RSIZE * CSIZE + YOFFSET * 2)

#define	BUTTON_USER_GAME	(2001)

#define PLAYER_NOTASSIGN	0
#define PLAYER_NETWORK		1
#define PLAYER_USER			2
#define PLAYER_WAITCLOSE	3

// Global Variables:
HINSTANCE hInst;								// current instance
SOCKET hListen = INVALID_SOCKET;
SOCKET sockClient[2] = { INVALID_SOCKET, INVALID_SOCKET };
int players[2];
HDC memDC;
HWND startGame;
Reversi game;
bool gameRunning = false;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	ATOM MyRegisterClass(HINSTANCE hInstance);
	HWND InitInstance(HINSTANCE, int);
	srand((unsigned)time(0));

	//	WSA network initialize
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return FALSE;

	// Initialize global strings
	MyRegisterClass(hInstance);

	// Perform application initialization:
	HWND hWnd = InitInstance(hInstance, nCmdShow);
	if(!hWnd) return FALSE;

	//	Make listen socket
	hListen = socket(AF_INET, SOCK_STREAM, 0);
	if (hListen == INVALID_SOCKET) return FALSE;

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8791);
	bind(hListen, (SOCKADDR*)&addr, sizeof(addr));

	listen(hListen, 2);
	WSAAsyncSelect(hListen, hWnd, WM_ACCEPT, FD_ACCEPT);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	closesocket(hListen);

	WSACleanup();

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMECENTER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_GAMECENTER);
	wcex.lpszClassName = L"ReversiGC";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	RECT rt = { 0, 0, WIDTH, HEIGHT };
	AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, TRUE);

	HWND hWnd = CreateWindow(L"ReversiGC", L"Reversi Game Center", 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, rt.right - rt.left, rt.bottom - rt.top, 
		NULL, NULL, hInstance, NULL);

	if(!hWnd) return 0;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

int GetPlayer(int player)
{
	int cand;
	if(players[0] == PLAYER_NOTASSIGN && players[1] == PLAYER_NOTASSIGN) cand = rand()&1;
	else if(players[0] == PLAYER_NOTASSIGN) cand = 0;
	else cand = 1;
	players[cand] = player;
	return cand;
}

int Recv(int slot, char buf[])
{
	int len = 0;
	while(len < 4)
	{
		int r = recv(sockClient[slot], buf+len, 4-len, 0);
		if(r <= 0) return 0;
		len += r;
	}
	buf[len] = 0;
	int needed = atoi(buf);
	len = 0;
	while(len < needed)
	{
		int r = recv(sockClient[slot], buf+len, needed-len, 0);
		if(r <= 0) return 0;
		len += r;
	}
	buf[len] = 0;
	OutputDebugStringA(buf);
	return len;
}

void SendBoard(int slot)
{
	char buf[512];
	const char *board = game.GetBoard();
	sprintf(buf, "%04d bd ", RSIZE*RSIZE+4);
	for(int i = 0; i < RSIZE*RSIZE; i++) buf[i+8] = board[i]+'0';
	send(sockClient[slot], buf, RSIZE*RSIZE+8, 0);
}

void SendPreRun(int slot, char newBoard[])
{
	char buf[512];
	sprintf(buf, "%04d pr ", RSIZE*RSIZE+4);
	for(int i = 0; i < 64; i++) buf[i+8] = newBoard[i]+'0';
	send(sockClient[slot], buf, RSIZE*RSIZE+8, 0);
}

void SendQuit()
{
	char buf[128];
	sprintf(buf, "%04d qt %02d%02d", 8, game.GetScores()[1], game.GetScores()[2]);
	for(int i = 0; i < 2; i++)
	{
		if(players[i] == PLAYER_NETWORK)
		{
			send(sockClient[i], buf, 12, 0);
			players[i] = PLAYER_WAITCLOSE;
		}
		else players[i] = PLAYER_NOTASSIGN;
	}
}

void SendAbort()
{
	char buf[128];
	sprintf(buf, "%04d ab ", 4);
	for(int slot = 0; slot < 2; slot++)
	{
		if(sockClient[slot] != INVALID_SOCKET)
			send(sockClient[slot], buf, 8, 0);
	}
}

void StartGame(HWND hWnd)
{
	for(int i = 0; i < 2; i++)
		if(players[i] == PLAYER_NOTASSIGN || players[i] == PLAYER_WAITCLOSE) return;
	EnableWindow(startGame, FALSE);
	game.Start();
	int slot = game.GetTurn() - 1;
	if(players[slot] == PLAYER_NETWORK) SendBoard(slot);
	gameRunning = true;
	listen(hListen, 0);
	InvalidateRect(hWnd, 0, FALSE);
}

void CloseGame(HWND hWnd)
{
	gameRunning = false;
	for(int slot = 0; slot < 2; slot++)
	{
		if(sockClient[slot] != INVALID_SOCKET) closesocket(sockClient[slot]);
		sockClient[slot] = INVALID_SOCKET;
		players[slot] = PLAYER_NOTASSIGN;
	}
	listen(hListen, 2);
	EnableWindow(startGame, TRUE);
	InvalidateRect(hWnd, 0, FALSE);
}

void OnClose(HWND hWnd)
{
	if(gameRunning)
	{
		SendAbort();
		CloseGame(hWnd);
	}
}

void OnQuit(HWND hWnd)
{
	SendQuit();
	CloseGame(hWnd);
}

void OnAccept(HWND hWnd)
{
	if(players[0] && players[1]) return;
	int cand = GetPlayer(PLAYER_NETWORK);
	game.SetPlayer(cand, PLAYER_NETWORK);
	
	SOCKADDR_IN addr;
	int len = sizeof(addr);
	sockClient[cand] = accept(hListen, (SOCKADDR*)&addr, &len);
	WSAAsyncSelect(sockClient[cand], hWnd, WM_CLIENT+cand, FD_READ | FD_CLOSE);
	char packet[16];
	sprintf(packet, "%04d st %4d", 8, cand+1);
	send(sockClient[cand], packet, 12, 0);

	StartGame(hWnd);
	InvalidateRect(hWnd, 0, FALSE);
}

void OnClient(HWND hWnd, int issue, int slot)
{
	if(sockClient[slot] == INVALID_SOCKET) return;
	if(issue == FD_CLOSE) { OnClose(hWnd); return; }
	char buf[1024];
	int len = Recv(slot, buf);
	char cmd[4];
	sscanf(buf, "%s", cmd);
	if(cmd[0] == 'p' && cmd[1] == 't')
	{
		int place = atoi(buf+4);
		int ret = game.Place(place);
		if(ret == -1) { SendAbort(); return; }
		if(ret == 0) { OnQuit(hWnd); return; }
		int nextSlot = game.GetTurn() - 1;
		if(players[nextSlot] == PLAYER_NETWORK) SendBoard(nextSlot);
		InvalidateRect(hWnd, 0, FALSE);
	}
	else if(cmd[0] == 'p' && cmd[1] == 'r')
	{
		int place = atoi(buf+4);
		char newBoard[64];
		if(!game.PreRun(place, newBoard)) { SendAbort(); return; }
		SendPreRun(slot, newBoard);
	}
	else OnClose(hWnd);
}

void OnLButtonDown(HWND hWnd, int x, int y)
{
	int slot = game.GetTurn() - 1;
	if(players[slot] != PLAYER_USER) return;
	int nx = (x-XOFFSET)/CSIZE, ny = (y-YOFFSET)/CSIZE;
	if(nx < 0 || nx >= 8 || ny < 0 || ny >= 8) return;
	int p = ny*8 + nx;
	if(game.GetBoard()[p] != 0) return;
	if(!game.Place(p)) { OnQuit(hWnd); return; }
	slot = game.GetTurn() - 1;
	if(players[slot] == PLAYER_NETWORK) SendBoard(slot);
	InvalidateRect(hWnd, 0, FALSE);
}

void OnCreate(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
	memDC = CreateCompatibleDC(hdc);
	SelectObject(memDC, hBitmap);
	ReleaseDC(hWnd, hdc);
	startGame = CreateWindow(L"BUTTON", L"User Game", 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		WIDTH - 120, HEIGHT - 50, 110, 30, 
		hWnd, (HMENU)BUTTON_USER_GAME, 
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
		NULL);
	EnableWindow(startGame, TRUE);
}

int OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	// Parse the menu selections:
	if(wmId == IDM_ABOUT) DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
	else if(wmId == IDM_EXIT) DestroyWindow(hWnd);
	else if(wmId == BUTTON_USER_GAME)
	{
		if(wmEvent == BN_CLICKED && players[0] != PLAYER_USER && players[1] != PLAYER_USER)
		{
			int cand = GetPlayer(PLAYER_USER);
			game.SetPlayer(cand, PLAYER_USER);
			EnableWindow(startGame, FALSE);
			StartGame(hWnd);
			InvalidateRect(hWnd, 0, TRUE);
		}
	}
	else return (int)DefWindowProc(hWnd, WM_COMMAND, wParam, lParam);
	return 0;
}

void OnPaint(HWND hWnd)
{
	void Draw(HDC hdc);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	Draw(memDC);
	BitBlt(hdc, 0, 0, WIDTH, HEIGHT, memDC, 0, 0, SRCCOPY);
	EndPaint(hWnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
		{
			OnCreate(hWnd);
			break;
		}
		case WM_COMMAND:
		{
			return OnCommand(hWnd, wParam, lParam);
			break;
		}
		case WM_PAINT:
		{
			OnPaint(hWnd);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			OnLButtonDown(hWnd, LOWORD(lParam), HIWORD(lParam));
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_ACCEPT:
			OnAccept(hWnd);
			break;
		case WM_CLIENT:
		case WM_CLIENT+1:
			OnClient(hWnd, LOWORD(lParam), message-WM_CLIENT);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
	if(message == WM_INITDIALOG) return TRUE;
	if(message == WM_COMMAND)
	{
		if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

void Draw(HDC hdc)
{
	const char *board = game.GetBoard();
	const int *scores = game.GetScores();
	HGDIOBJ oldObj = SelectObject(hdc, GetStockObject(DC_BRUSH));
	SelectObject(hdc, GetStockObject(DC_PEN));
	SetDCBrushColor(hdc, RGB(255, 255, 255));
	SetDCPenColor(hdc, RGB(255, 0, 0));
	Rectangle(hdc, 0, 0, WIDTH, HEIGHT);
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
			if(board[idx] == 0) Rectangle(hdc, XOFFSET+CSIZE*c+15, YOFFSET+CSIZE*r+15, XOFFSET+CSIZE*c+CSIZE-15, YOFFSET+CSIZE*r+CSIZE-15);
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
	const char *playerstr[] = { "Wait", "Network", "User", "Closing" };
	len = sprintf(str, "Player �� : %s", playerstr[players[0]]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+20, str, len); 
	len = sprintf(str, "Player �� : %s", playerstr[players[1]]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+40, str, len); 
	len = sprintf(str, "Score  �� : %3d", scores[1]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+80, str, len); 
	len = sprintf(str, "Score  �� : %3d", scores[2]);
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+100, str, len); 
	len = sprintf(str, "Current Turn : %s", game.GetTurn()==1?"��":"��");
	TextOutA(hdc, XOFFSET*2 + RSIZE*CSIZE, YOFFSET+180, str, len); 
	SelectObject(hdc, oldObj);
}
