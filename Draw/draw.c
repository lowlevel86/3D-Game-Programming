//compile with:
//tcc -mwindows -o draw.exe draw.c

//execute directly with:
//tcc -run draw.c

#include <windows.h>
#include <stdio.h>
#include <math.h>


#define WIN_WIDTH 480 //window size
#define WIN_HEIGHT 320

#define TRUE 1
#define FALSE 0
#define ID_TIMER 1

int canvasWidth;
int canvasHeight;
BITMAPINFO pbmi[40];
BYTE canvas[(WIN_WIDTH*3 + WIN_WIDTH%4) * WIN_HEIGHT];


void clearCanvas()
{
   int i;
   int bytesWidth = canvasWidth * 3 + canvasWidth % 4;
   
   for (i=0; i < bytesWidth * canvasHeight; i++)
   {
      canvas[i] = 0x0;
   }
}


void putPix(int x, int y, int brightness)
{
   int bytesWidth = canvasWidth * 3 + canvasWidth % 4;
   
   if ((x >= 0) && (x < canvasWidth) && (y >= 0) && (y < canvasHeight))
   if (brightness > canvas[0+x*3+bytesWidth*y])
   {
      canvas[0+x*3+bytesWidth*y] = brightness;
      canvas[1+x*3+bytesWidth*y] = brightness;
      canvas[2+x*3+bytesWidth*y] = brightness;
   }
}


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
TCHAR szAppName[] = TEXT("Draw");

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
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
   wndclass.hbrBackground = 0;
   wndclass.lpszMenuName = NULL;
   wndclass.lpszClassName = szAppName;

   RegisterClass(&wndclass);
   hwnd = CreateWindow(szAppName, szAppName,
                       WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
                       0, 0, WIN_WIDTH, WIN_HEIGHT,
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
   static HDC hdc;
   static PAINTSTRUCT ps;
   static int dragLMouse = FALSE;
   static int xMouseLoc, yMouseLoc;
   static int pixBrightness = 255;
   
   
   if (WM_CREATE == message)
   {
      pbmi->bmiHeader.biSize = 40;
      pbmi->bmiHeader.biWidth = WIN_WIDTH;
      pbmi->bmiHeader.biHeight = WIN_HEIGHT;
      pbmi->bmiHeader.biPlanes = 1;
      pbmi->bmiHeader.biBitCount = 24;
      pbmi->bmiHeader.biCompression = BI_RGB;
      pbmi->bmiHeader.biSizeImage = WIN_WIDTH * WIN_HEIGHT;
      pbmi->bmiHeader.biXPelsPerMeter = 0;
      pbmi->bmiHeader.biYPelsPerMeter = 0;
      pbmi->bmiHeader.biClrUsed = 0;
      pbmi->bmiHeader.biClrImportant = 0;
      
      SetTimer(hwnd, ID_TIMER, 40, NULL);
      
      return 0;
   }
   
   if (WM_SIZE == message)
   {
      canvasWidth = LOWORD(lParam);
      canvasHeight = HIWORD(lParam);
      
      pbmi->bmiHeader.biWidth = canvasWidth;
      pbmi->bmiHeader.biHeight = canvasHeight;
      pbmi->bmiHeader.biSizeImage = canvasWidth * canvasHeight;
      
      return 0;
   }

   if (WM_TIMER == message)
   {
      if (dragLMouse)
      {
         putPix(xMouseLoc, yMouseLoc, pixBrightness);
         putPix(xMouseLoc+1, yMouseLoc, pixBrightness);
         putPix(xMouseLoc-1, yMouseLoc, pixBrightness);
         putPix(xMouseLoc, yMouseLoc+1, pixBrightness);
         putPix(xMouseLoc, yMouseLoc-1, pixBrightness);
      }
      
      InvalidateRect(hwnd, NULL, TRUE);
      UpdateWindow(hwnd);
      
      hdc = GetDC(hwnd);
      SetDIBitsToDevice(hdc, 0, 0, canvasWidth, canvasHeight,
                        0, 0, 0, canvasHeight, canvas, pbmi, DIB_RGB_COLORS);
      ReleaseDC(hwnd, hdc);
      
      return 0;
   }
   
   if (WM_LBUTTONDOWN == message)
   {
      dragLMouse = TRUE;
      return 0;
   }
   
   if (WM_LBUTTONUP == message)
   {
      dragLMouse = FALSE;
      return 0;
   }
   
   if (WM_RBUTTONDOWN == message)
   {
      clearCanvas();
      return 0;
   }
   
   if (WM_MOUSEMOVE == message)
   {
      xMouseLoc = LOWORD(lParam);
      yMouseLoc = canvasHeight-HIWORD(lParam);
      return 0;
   }
   
   if (WM_MOUSEWHEEL == message)
   {
      if (HIWORD(wParam) == 120)
      if (pixBrightness <= 240)
      pixBrightness += 15;
      
      if (HIWORD(wParam) == 65416)
      if (pixBrightness >= 15)
      pixBrightness -= 15;
      
      char winTextBuff[256];
      sprintf(winTextBuff, "Draw - Brightness:%i", pixBrightness);
      SetWindowText(hwnd, TEXT(winTextBuff));
      
      return 0;
   }
   
   if (WM_KEYDOWN == message)
   {
      //printf("keydown value %i\n", LOWORD(wParam));
      //fflush(stdout);
      
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
