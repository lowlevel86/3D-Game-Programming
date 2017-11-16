//compile with:
//tcc -mwindows -o dbgwin.exe dbgwin.c

#include <windows.h>
#include <stdio.h>


#define window_width 760
#define window_height 260
#define font_size 18
#define terminal_buffer_size 1024*4

#define ID_EDIT 1
#define ID_TIMER 2

char usageText[] =
{
"This program reads and displays standard output from an executable.\r\n\
\r\n\
Command line usage example:\r\n\
\r\n\
>executable.exe | dbgwin.exe\r\n\
\r\n\
The executable if written in c must be compiled with such functions:\r\n\
\r\n\
printf(\"hello world!\\n\"); // To see if an event occured\r\n\
printf(\"x:%i y:%f\\n\", xInt, yFloat); // To print the value of variables\r\n\
fflush(stdout); // To view output in the middle of a program\r\n\
"
};
 
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

TCHAR szAppName[] = TEXT("Debug Window");

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;
 
	if (!RegisterClass(&wndclass))
	return 0;

	hwnd = CreateWindow(szAppName, szAppName,
	                    WS_OVERLAPPEDWINDOW,
	                    0, 0,
	                    window_width, window_height,
	                    NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd); 

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;
	static HFONT hFont;
	
   static CHAR chBuf[terminal_buffer_size]; 
   static DWORD dwRead;
   static HANDLE hStdin, hStdout; 
   static BOOL bSuccess; 
 
   if (WM_CREATE == message)
	{
		hFont = CreateFont(font_size, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Courier New"));

		hwndEdit = CreateWindow(TEXT("edit"), NULL,
		                        WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		                        WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
		                        0, 0, 0, 0, hwnd, (HMENU)ID_EDIT, ((LPCREATESTRUCT)lParam)-> hInstance, NULL);

		SendMessageW(hwndEdit, WM_SETFONT, (WPARAM)hFont, 0);
		
		SetWindowText(hwndEdit, usageText);
		
		SetTimer(hwnd, ID_TIMER, 1, NULL);
		
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
		hStdin = GetStdHandle(STD_INPUT_HANDLE); 
		
		if ((hStdout == INVALID_HANDLE_VALUE) || (hStdin == INVALID_HANDLE_VALUE))
      KillTimer(hwnd, ID_TIMER);
 
		return 0;
	}
	
	if (WM_TIMER == message)
	{
		// Read from standard input and stop on error or no data.
		bSuccess = ReadFile(hStdin, chBuf, terminal_buffer_size, &dwRead, NULL); 
	
      if (bSuccess && dwRead)
		{
			SetWindowText(hwndEdit, chBuf);
			SendMessage(hwndEdit, WM_VSCROLL, SB_BOTTOM, 0);
		}
		else
		{
			KillTimer(hwnd, ID_TIMER);
		}
	}
 
   if (WM_SIZE == message)
	{
		MoveWindow(hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	}

   if (WM_KEYDOWN == message)
   {
      if (LOWORD(wParam) == 27)//Esc
      {
         KillTimer(hwnd, ID_TIMER);
         PostQuitMessage(0);
      }
      
      return 0;
   }
   
   if (WM_DESTROY == message)
   {
      KillTimer(hwnd, ID_TIMER);
		PostQuitMessage(0);
		return 0;
	}
	
	return DefWindowProc(hwnd, message, wParam, lParam);
}
