// Player.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <CommDlg.h>
#include "Player.h"
#include "SceneFile.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name


//////////////////////////////////////////////////////////////////////////


HWND g_hWnd				= NULL;
SimContext* g_pContext	= NULL;
CSceneFile* g_SceneFile	= NULL;
BOOL				g_bFullscreen = 0;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void				ParseCmdLine(LPTSTR lpCmdLine);
void				ToggleFullscreen();
void				OpenScene();

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PLAYER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PLAYER));

	g_SceneFile = new CSceneFile();
	g_SceneFile->OnInit();

	ParseCmdLine(lpCmdLine);

	// Now we're ready to receive and process Windows messages.
	bool bGotMsg;
	msg.message = WM_NULL;
	PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );

	while( WM_QUIT != msg.message )
	{
		// Use PeekMessage() so we can use idle time to render the scene. 
		bGotMsg = ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );

		if( bGotMsg )
		{
			// Translate and dispatch the message
			if ( !TranslateAccelerator(msg.hwnd, hAccelTable, &msg) )
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// Render a frame during idle time (no messages are waiting)
			if (g_pContext)
			{
				g_pContext->OnFrame();
			}
		}
	}

	return (int) msg.wParam;
}

void ParseCmdLine(LPTSTR lpCmdLine)
{
	//MessageBox(NULL, lpCmdLine, L"", MB_OK);

	XString str = lpCmdLine;

	int len = str.findFirst(L'-');

	while (len >= 0)
	{
		XString tmp;

		int next = str.findNext(L'-', len+1);

		if (next >= 0)
			tmp = str.subString(len+1, next-len-1);
		else
			tmp = str.subString(len+1, str.size()-len-1);

		len = next;

		tmp.trim();

		if (tmp.equalsi(L"f"))
		{
			ToggleFullscreen();
		}
		else
		{
			XString ext;
			getFileNameExtension(ext, tmp);
			if (ext.equalsi(L".scn"))
			{
				if (isFileExist(tmp))
					g_SceneFile->OnOpen(tmp);
				else
					g_SceneFile->OnOpen(getStartPath() + tmp);
			}
		}
	}
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PLAYER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	//wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PLAYER);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   g_hWnd = hWnd;
   CreateSimContext(hWnd, &g_pContext);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	if (g_pContext)
		g_pContext->PostMessage(message, wParam, lParam);

	switch (message)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'F':
			ToggleFullscreen();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
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
		case ID_FILE_OPEN:
			OpenScene();
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		SAFE_RELEASE(g_pContext);
		return DefWindowProc(hWnd, message, wParam, lParam);
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ToggleFullscreen()
{
	//g_pContext->GetRenderContext()->ToggleFullScreen();

	static RECT rcRestore;

	g_bFullscreen = !g_bFullscreen;

	if (g_bFullscreen)
	{
		::GetWindowRect(g_hWnd, &rcRestore);

		RECT screenRect;
		::GetWindowRect( ::GetDesktopWindow(), &screenRect );

		int width = (screenRect.right - screenRect.left) * g_pContext->GetRenderContext()->GetNumOutputs();
		int height = screenRect.bottom - screenRect.top;

		LONG st = GetWindowLong(g_hWnd, GWL_STYLE);
		st &= ~WS_CAPTION;
		st &= ~WS_THICKFRAME;
		SetWindowLong(g_hWnd, GWL_STYLE, st);

		//Resize the window
		::SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, width, height, 0/*SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE*/);
		::SetFocus(g_hWnd);

		//g_pContext->GetRenderContext()->ToggleFullScreen();
	}
	else
	{
		//g_pContext->GetRenderContext()->ToggleFullScreen();

		//Resize the window
		int width = rcRestore.right - rcRestore.left;
		int height = rcRestore.bottom - rcRestore.top;

		LONG st = GetWindowLong(g_hWnd, GWL_STYLE);
		st |= WS_CAPTION;
		st |= WS_THICKFRAME;
		SetWindowLong(g_hWnd, GWL_STYLE, st);

		::SetWindowPos(g_hWnd, HWND_NOTOPMOST, rcRestore.left, rcRestore.top, width, height, NULL);
		::SetFocus(g_hWnd);
	}
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_SIGNATURE);
			XString str = g_pContext->GetAttributeManager()->GetString(L"HW_Signature");
			SetWindowText(hEdit, str.c_str());
		}
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

void OpenScene()
{
	WCHAR szFileName[MAX_PATH] = L"";
	OPENFILENAME file = { 0 };
	file.lStructSize = sizeof(file);

	file.lpstrFile = szFileName;
	file.nMaxFile = MAX_PATH;
	//file.lpstrFilter = L"Tera File(*.tera)\0*.tera\0All Files\0*.*\0\0"; 
	//file.lpstrFilter = L"Tera File(*.scn)\0*.scn\0All Files\0*.*\0\0";
	file.lpstrFilter = L"Movie File(*.avi,*.mov,*mp4)\0*.avi;*.mov;*.mp4\0All Files\0*.*\0\0";
	file.nFilterIndex = 1;//默认选择第一个

	// 弹出打开文件的对话框
	if(::GetOpenFileName(&file))
	{
		//::SetWindowText(::GetDlgItem(hDlg, IDC_FILE), szFileName);
		XString ext;
		getFileNameExtension(ext, szFileName);

		g_SceneFile->OnOpen(szFileName);
	}
}
