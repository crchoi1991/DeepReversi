#include "framework.h"
#include "GameCenter.h"
#include <stdio.h>
#include <commdlg.h>
#include <winsock2.h>
#include "Reversi.h"

#define	WM_ACCEPT			(WM_USER+1)
#define	WM_CLIENT			(WM_USER+2)

#define	WIDTH		640
#define	HEIGHT		440

// Global Variables:
HINSTANCE hInst;								// current instance
SOCKET hListen = INVALID_SOCKET;
SOCKET hClient[2] = { INVALID_SOCKET, INVALID_SOCKET };
bool bListenWait = true;
HDC memDC;
Reversi game;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	//	WSA network initialize
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return FALSE;

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

	listen(hListen, 1);
	WSAEventSelect(hListen, hWnd, FD_ACCEPT);

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

void OnAccept(HWND hWnd)
{
	SOCKADDR_IN addr;
	int len = sizeof(addr);
	if(game.GetStatus() > Reversi::READY1) return;
	if(game.GetStatus() == Reversi::READY)
	{
		hClient[0] = accept(hListen, (SOCKADDR*)&addr, &len);
		game.SetStatus(Reversi::READY1);
	}
	else
	{
		hClient[1] = accept(hListen, (SOCKADDR*)&addr, &len);
		WSAEventSelect(hClient[0], hWnd, FD_READ | FD_CLOSE);
		WSAEventSelect(hClient[1], hWnd, FD_READ | FD_CLOSE);
	}

	InvalidateRect(hWnd, 0, FALSE);
}

void OnClient(HWND hWnd)
{
#if 0
	if (hClient == INVALID_SOCKET) return;
	char buf[1024];
	int len = recv(hClient, buf, 1024, 0);
	if (len <= 0)
	{
		closesocket(hClient);
		hClient = INVALID_SOCKET;
		return;
	}

	int dir = -1;
	if (buf[0] == 'N') dir = 0;
	else if (buf[0] == 'E') dir = 1;
	else if (buf[0] == 'S') dir = 2;
	else if (buf[0] == 'W') dir = 3;
	bool v = (dir != -1) ? kMaze.Move(dir) : true;
	if (v == false) send(hClient, "0 0 0 0 0", 9, 0);
	else
	{
		Sleep(100);
		const char* buf = kMaze.GetCell();
		send(hClient, buf, 9, 0);
		InvalidateRect(hWnd, 0, FALSE);
	}
#endif
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_CREATE:
		{
			HDC hdc = GetDC(hWnd);
			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 620, 440);
			memDC = CreateCompatibleDC(hdc);
			SelectObject(memDC, hBitmap);
			ReleaseDC(hWnd, hdc);
			break;
		}
		case WM_COMMAND:
		{
			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		}
		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);
			HGDIOBJ oldObj = SelectObject(memDC, GetStockObject(NULL_PEN));
			Rectangle(memDC, 0, 0, WIDTH, HEIGHT);
			SelectObject(memDC, oldObj);
			game.Draw(memDC);
			BitBlt(hdc, 0, 0, WIDTH, HEIGHT, memDC, 0, 0, SRCCOPY);
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
			OnClient(hWnd);
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
