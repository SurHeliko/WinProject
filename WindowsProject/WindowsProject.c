#define _CRT_SECURE_NO_WARNINGS
#include "framework.h"
#include "WindowsProject.h"

#define MAX_LOADSTRING 100
#define openButtonId 101
#define searchButtonId 102

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

UINT64 fileSize;
HWND hEditSearch, hEdit, hwndButton, hwndButtonSearch, hPrint;

HANDLE hFile, hFileMap;
PBYTE pFileView;

UINT64 offset = 0;
DWORD pos = 0;

SCROLLINFO scrInfo;
SYSTEM_INFO sysInfo;
DWORD dwBytesInBlock;
DWORD block = 0;
DWORD blockPrev;
DWORD bytesLeft;

int page = 30;
int scale;

ATOM                MyRegisterClass(HINSTANCE hInstance);
ATOM                MyRegisterPrintClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    WndPrintProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Разместите код здесь.

	// Инициализация глобальных строк
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WINDOWSPROJECT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	MyRegisterPrintClass(hInstance);

	// Выполнить инициализацию приложения:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT));

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

ATOM MyRegisterPrintClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndPrintProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT);
	wcex.lpszClassName = L"MyPrintClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

void updateViewOfFile()
{
	if (block == blockPrev) return;
	blockPrev = block;
	UnmapViewOfFile(pFileView);
	offset = dwBytesInBlock * block;
	bytesLeft = fileSize - dwBytesInBlock * block;
	if (dwBytesInBlock * block + dwBytesInBlock * 2 < fileSize)
		pFileView = (PBYTE)MapViewOfFile(hFileMap, FILE_MAP_READ, (DWORD)(offset >> 32), (DWORD)(offset & 0xFFFFFFFF), dwBytesInBlock * 2);
	else {
		pFileView = (PBYTE)MapViewOfFile(hFileMap, FILE_MAP_READ, (DWORD)(offset >> 32), (DWORD)(offset & 0xFFFFFFFF), bytesLeft);
	}
}

void print(HDC hdc, int localOffset)
{
	char buffer[200];
	int stringCharLen = 0, j = (scale * pos * 16);
	stringCharLen += sprintf(buffer + stringCharLen, "%08X: ", offset + scale * pos * 16 + localOffset);;
	for (__int64 i = localOffset + j; i < localOffset + j + 16; i++)
	{
		if (i < bytesLeft) stringCharLen += sprintf(buffer + stringCharLen, "%02X ", pFileView[i]);
		else {
			stringCharLen += sprintf(buffer + stringCharLen, "   ");
		}
	}
	stringCharLen += sprintf(buffer + stringCharLen, " | ");
	for (__int64 i = localOffset + j; i < localOffset + j + 16; i++)
	{
		if (i < bytesLeft && isprint(pFileView[i])) stringCharLen += sprintf(buffer + stringCharLen, "%C ", pFileView[i]);
		else {
			stringCharLen += sprintf(buffer + stringCharLen, ". ");
		}
	}
	stringCharLen += sprintf(buffer + stringCharLen, " ");
	TextOutA(hdc, 0, localOffset, buffer, stringCharLen);
}

void openFile(HWND hWnd)
{
	OPENFILENAME File;
	DWORD dwFileSizeHigh;
	int temp;
	blockPrev = -1;
	wchar_t szFile[1000];

	ZeroMemory(&File, sizeof(File));
	File.lStructSize = sizeof(OPENFILENAME);
	File.hwndOwner = NULL;
	File.lpstrFile = (LPWSTR)szFile;
	File.lpstrFile[0] = '\0';
	File.nMaxFile = sizeof(szFile);
	File.nFilterIndex = 1;
	File.lpstrFileTitle = NULL;
	File.nMaxFileTitle = 0;
	File.lpstrInitialDir = NULL;
	File.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	GetOpenFileName(&File);
	CloseHandle(hFile);
	CloseHandle(hFileMap);
	SetWindowTextW(hEdit, File.lpstrFile);

	hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	fileSize = GetFileSize(hFile, &dwFileSizeHigh);
	fileSize += (((INT64)dwFileSizeHigh) << 32);
	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

	GetSystemInfo(&sysInfo);
	dwBytesInBlock = sysInfo.dwAllocationGranularity;
	if (fileSize < sysInfo.dwAllocationGranularity)
		dwBytesInBlock = (DWORD)fileSize;
	updateViewOfFile();

	scale = 1;
	temp = (fileSize / 16);
	while (temp > INT_MAX)
	{
		temp = temp / 2;
		scale++;
	}
	scrInfo.nMax = temp - page + 1;
	scrInfo.cbSize = sizeof(scrInfo);
	scrInfo.fMask = SIF_ALL;
	scrInfo.nMin = 0;
	scrInfo.nPage = 0;
	scrInfo.nPos = 0;
	SetScrollInfo(hPrint, SB_VERT, &scrInfo, TRUE);
	UpdateWindow(hWnd);
}

void search()
{
	if (!pFileView) return;
	DWORD dwSizeHigh;
	UINT64 i;
	int n = 0;
	char buffer[9], buffer2[5] = { '0','0','0','0','\0' }, buffer3[5] = { '0','0','0','0','\0' };
	GetWindowTextA(hEditSearch, buffer, 9);
	memcpy(buffer2, buffer, 4);
	memcpy(buffer3, &buffer[4], 4);
	dwSizeHigh = strtol(buffer2, NULL, 16);
	if (buffer3[n] == -52) i = strtol(buffer2, NULL, 16);
	else {
		i = strtol(buffer3, NULL, 16);
		while (buffer3[n] != '\0')
			n++;
		i += dwSizeHigh * (pow(16, n));
	}

	if (i > fileSize) {
		SetWindowTextW(hEditSearch, L"ERROR");
		return;
	}
	scrInfo.nPos = i/16;
	pos = scrInfo.nPos;
	block = pos / (dwBytesInBlock / 16);
	pos = pos - dwBytesInBlock * block / 16;
	SetScrollInfo(hPrint, SB_VERT, &scrInfo, TRUE);
	updateViewOfFile();
	SendMessage(hPrint, WM_PAINT, NULL, NULL);
}

LRESULT CALLBACK WndPrintProc(HWND hPrint, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_VSCROLL:
	{
		GetScrollInfo(hPrint, SB_VERT, &scrInfo);
		switch (LOWORD(wParam))
		{
		case SB_LINEUP:
		{
			scrInfo.nPos -= 1;
			pos = scrInfo.nPos;
			block = pos / (dwBytesInBlock / 16);
			pos = pos - dwBytesInBlock * block / 16;
		}
		break;
		case SB_LINEDOWN:
		{
			scrInfo.nPos += 1;
			pos = scrInfo.nPos;
			block = pos / (dwBytesInBlock / 16);
			pos = pos - dwBytesInBlock * block / 16;
		}
		break;
		case SB_THUMBTRACK:
		{
			scrInfo.nPos = scrInfo.nTrackPos;
			pos = scrInfo.nPos;
			block = pos / (dwBytesInBlock / 16);
			pos = pos - dwBytesInBlock * block / 16;
		}
		break;
		}
		SetScrollInfo(hPrint, SB_VERT, &scrInfo, TRUE);
		updateViewOfFile();
		SendMessage(hPrint, WM_PAINT, NULL, NULL);
	}
	break;
	case WM_PAINT:
	{
		if (!pFileView) return;
		HDC hdc = GetDC(hPrint);
		SelectObject(hdc, GetStockObject(ANSI_FIXED_FONT));
		int localOffset = 0;
		for (int i = 0; i < page; i++)
		{
			if (localOffset + pos < fileSize) print(hdc, localOffset);
			localOffset += 16;
		}
		ReleaseDC(hPrint, hdc);
	}
	break;
	default:
		return DefWindowProc(hPrint, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
	{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		MoveWindow(hEdit, width - 550, 10, 400, 20, TRUE);
		MoveWindow(hEditSearch, width - 550, 32, 400, 20, TRUE);
		MoveWindow(hwndButton, width - 150, 10, 130, 20, TRUE);
		MoveWindow(hwndButtonSearch, width - 150, 32, 130, 20, TRUE);
		UpdateWindow(hWnd);
	}
	break;
	case WM_CREATE:
	{
		hEditSearch = CreateWindow(TEXT("EDIT"), NULL,
			WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_LEFT,
			0, 0, 0, 0,
			hWnd, NULL, hInst, NULL);

		hEdit = CreateWindow(TEXT("EDIT"), NULL,
			WS_CHILD | WS_VISIBLE | WS_BORDER |
			ES_LEFT,
			0, 0, 0, 0,
			hWnd, NULL, hInst, NULL);

		hPrint = CreateWindow(L"MyPrintClass", NULL,
			WS_CHILD | WS_VISIBLE |
			ES_LEFT | ES_READONLY,
			100, 100, 1000, 500,
			hWnd, NULL, hInst, NULL);

		hwndButtonSearch = CreateWindow(L"BUTTON", L"Найти", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			0, 0, 0, 0, hWnd, (HMENU)searchButtonId, hInst, NULL);

		hwndButton = CreateWindow(L"BUTTON", L"Открыть", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			0, 0, 0, 0, hWnd, (HMENU)openButtonId, hInst, NULL);

		SendMessage(hEdit, EM_SETLIMITTEXT, 8, 0);
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case searchButtonId:
			search();
			break;
		case openButtonId:
			openFile(hWnd);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
	{
		CloseHandle(hFile);
		CloseHandle(hFileMap);
		PostQuitMessage(0);
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
