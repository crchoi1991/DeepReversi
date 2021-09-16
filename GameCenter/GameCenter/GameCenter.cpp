#define _WINSOCK_DEPRECATED_NO_WARNINGS
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

#define	WIDTH		640
#define	HEIGHT		440

#define	BUTTON_USER_GAME	(2001)

#define PLAYER_NOTASSIGN	0
#define PLAYER_NETWORK		1
#define PLAYER_USER			2

// Global Variables:
HINSTANCE hInst;								// current instance
SOCKET hListen = INVALID_SOCKET;
SOCKET hClient[2] = { INVALID_SOCKET, INVALID_SOCKET };
int players[2];
HDC memDC;
HWND startGame;
Reversi game;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	srand(time(0));

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
	addr.sin_port = htons(8888);
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

	RECT rt = { 0, 0, Reversi::GetWidth(), Reversi::GetHeight() };
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
	int cand = (!players[0] && !players[1])? rand()&1:!players[1];
	players[cand] = player;
	return cand;
}

void OnAccept(HWND hWnd)
{
	if(players[0] && players[1]) return;
	int cand = GetPlayer(PLAYER_NETWORK);
	game.SetPlayer(cand, PLAYER_NETWORK);
	
	SOCKADDR_IN addr;
	int len = sizeof(addr);
	hClient[cand] = accept(hListen, (SOCKADDR*)&addr, &len);
	WSAAsyncSelect(hClient[cand], hWnd, WM_CLIENT+cand, FD_READ | FD_CLOSE);
	char c = cand+'1';
	send(hClient[cand], &c, 1, 0);

	if(players[0] && players[1])
	{
		EnableWindow(startGame, FALSE);
		game.Start();
	}
	InvalidateRect(hWnd, 0, FALSE);
}

void OnClient(HWND hWnd, int slot)
{
	if(hClient[slot] == INVALID_SOCKET) return;
	char buf[1024];
	int len = recv(hClient[slot], buf, 1024, 0);
	if(len <= 0)
	{
		closesocket(hClient[slot]);
		hClient[slot] = INVALID_SOCKET;
		InvalidateRect(hWnd, 0, FALSE);
		return;
	}
}

void OnCreate(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, Reversi::GetWidth(), Reversi::GetHeight());
	memDC = CreateCompatibleDC(hdc);
	SelectObject(memDC, hBitmap);
	ReleaseDC(hWnd, hdc);
	startGame = CreateWindow(L"BUTTON", L"User Game", 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		Reversi::GetWidth() - 120,
		Reversi::GetHeight() - 50,
		110, 30, hWnd, (HMENU)BUTTON_USER_GAME, 
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), 
		NULL);
	EnableWindow(startGame, TRUE);
}

int OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
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
			InvalidateRect(hWnd, 0, TRUE);
		}
	}
	else return DefWindowProc(hWnd, WM_COMMAND, wParam, lParam);
	return 0;
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
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			game.Draw(memDC);
			BitBlt(hdc, 0, 0, Reversi::GetWidth(), Reversi::GetHeight(), memDC, 0, 0, SRCCOPY);
			EndPaint(hWnd, &ps);
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
			OnClient(hWnd, message-WM_CLIENT);
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
