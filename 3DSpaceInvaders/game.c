//compile with:
//tcc -mwindows -o game.exe game.c

//execute directly with:
//tcc -run game.c

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>


#define WIN_WIDTH 480 //window size
#define WIN_HEIGHT 480
#define IMGBUFF_WIDTH 256 //max bmp image size
#define IMGBUFF_HEIGHT 256

#define IMGOBJCNT 3 //ship.bmp, circle.bmp, and invader.bmp
#define OBJCNT 7 //the ship, the bullet, and 5 invaders
#define OPCNT 128 //max number of operations that can be done on an object

#define SHIPIMG 0 //image ids
#define CIRCLEIMG 1
#define INVADERIMG 2

#define SHIP 0 //object ids
#define BULLET 1
#define INVADER 2 //note: that 0-4 is added to INVADER
#define INVADERCNT 5

#define IMGOBJ -1 //object operations
#define HIDE 0
#define XROT 1
#define YROT 2
#define ZROT 3
#define XLOC 4
#define YLOC 5
#define ZLOC 6
#define XSZ 7
#define YSZ 8
#define ZSZ 9
#define END 0

#define TRUE 1
#define FALSE 0
#define ID_TIMER 1

int canvasWidth;
int canvasHeight;
int xCenter, yCenter;
BITMAPINFO pbmi[40];
BYTE canvas[(WIN_WIDTH*3 + WIN_WIDTH%4) * WIN_HEIGHT];

int imgObj[IMGOBJCNT][IMGBUFF_WIDTH * IMGBUFF_HEIGHT];
int objOps[OBJCNT][OPCNT];//object operations
float objOpValues[OBJCNT][OPCNT];//object operation values

//unit circle coordinates buffers
float hUcRotValues[OPCNT];
float vUcRotValues[OPCNT];


void loadImg(int imgObjNum, char *imgFile)
{
   int x, y;
   struct stat stat_p;
   FILE *bmpFile;
   int imgWidth;
   int headerSize;
   
   // does the file exist
   if (-1 == stat(imgFile, &stat_p))
   return;
   
   // open file for read
   bmpFile = fopen(imgFile,"rb");
   
   if (!bmpFile)
   return;
   
   //get image width
   //the image must be square and have a width with a power of 2
   //the header must be small enough relative to the image data
   imgWidth = pow(2, (int)(log(sqrt(stat_p.st_size/3))/log(2)));
   
   //read image file to buffer
   headerSize = stat_p.st_size - imgWidth * imgWidth * 3;
   fseek(bmpFile, headerSize+1, SEEK_CUR);
   for (y=IMGBUFF_HEIGHT/2-imgWidth/2; y < imgWidth+IMGBUFF_HEIGHT/2-imgWidth/2; y++)
   {
      for (x=IMGBUFF_WIDTH/2-imgWidth/2; x < imgWidth+IMGBUFF_WIDTH/2-imgWidth/2; x++)
      {
         if ((x >= 0) && (x < IMGBUFF_WIDTH) && (y >= 0) && (y < IMGBUFF_HEIGHT))
         imgObj[imgObjNum][IMGBUFF_WIDTH*y+x] = (int)fgetc(bmpFile)/128 * 255;
         
         fseek(bmpFile, 2, SEEK_CUR);
      }
   }
   
   fclose(bmpFile);
}


void rot(float *horiP, float *vertP, float degrees)
{
   float h, v;
   float hUc;
   float vUc;
   float hLine1, vLine1;
   float hLine2, vLine2;
   
   if (degrees != degrees) // check if NaN
   return;
   
   hUc = cos(degrees * (M_PI * 2.0 / 360.0));
   vUc = sin(degrees * (M_PI * 2.0 / 360.0));
   
   hLine1 = hUc;
   vLine1 = vUc;
   hLine2 = -vUc;
   vLine2 = hUc;

   h = *vertP * hLine2 + *horiP * vLine2;
   v = *horiP * vLine1 + *vertP * hLine1;
   *horiP = h;
   *vertP = v;
}


// rotate using coordinates around a unit circle's circumference
void ucRot(float hUc, float vUc, float *hP, float *vP)
{
   float h, v;
   float hLine1, vLine1;
   float hLine2, vLine2;
   
   if (hUc != hUc) // check if NaN
   return;
   
   if (vUc != vUc) // check if NaN
   return;
   
   hLine1 = hUc;
   vLine1 = vUc;
   hLine2 = -vUc;
   vLine2 = hUc;
   
   h = *vP * hLine2 + *hP * vLine2;
   v = *hP * vLine1 + *vP * hLine1;
   
   *hP = h;
   *vP = v;
}


void applyObjOps(int objNum, int opNum, int x, int y, int *xDelta, int *yDelta, int *brightness)
{
   int i;
   float xPt, yPt, zPt;
   float perspctv = 240;
   float cameraLens = 220;//less than perspctv
   float cameraDistance = -600;
   xPt = x - IMGBUFF_WIDTH/2;
   yPt = y - IMGBUFF_HEIGHT/2;
   zPt = 0;
   
   for (i=opNum; i < OPCNT; i++)
   {
      if ((objOps[objNum][i] == END) || (objOps[objNum][i] == IMGOBJ))
      break;
      
      //rotation
      if (objOps[objNum][i] == XROT)
      ucRot(hUcRotValues[i], vUcRotValues[i], &yPt, &zPt);
      
      if (objOps[objNum][i] == YROT)
      ucRot(hUcRotValues[i], vUcRotValues[i], &xPt, &zPt);
      
      if (objOps[objNum][i] == ZROT)
      ucRot(hUcRotValues[i], vUcRotValues[i], &xPt, &yPt);
      
      //location
      if (objOps[objNum][i] == XLOC)
      xPt += objOpValues[objNum][i];
      
      if (objOps[objNum][i] == YLOC)
      yPt += objOpValues[objNum][i];
      
      if (objOps[objNum][i] == ZLOC)
      zPt += objOpValues[objNum][i];
      
      //size
      if (objOps[objNum][i] == XSZ)
      xPt *= objOpValues[objNum][i];
      
      if (objOps[objNum][i] == YSZ)
      yPt *= objOpValues[objNum][i];
      
      if (objOps[objNum][i] == ZSZ)
      yPt *= objOpValues[objNum][i];
   }
   
   //change size relative to distance from camera
   *xDelta = xPt * perspctv / (perspctv - zPt) + xCenter;
   *yDelta = yPt * perspctv / (perspctv - zPt) + yCenter;
   
   //change brightness relative to distance from camera
   if (zPt >= 0)
   *brightness = 128 + zPt/cameraLens * 127;
   else
   *brightness = 128 - zPt/cameraDistance * 127;
   
   if (zPt > cameraLens)
   *brightness = 0;
   
   if (zPt < cameraDistance)
   *brightness = 0;
}


void clearCanvas()
{
   int i;
   int bytesWidth = canvasWidth * 3 + canvasWidth % 4;
   
   for (i=0; i < bytesWidth * canvasHeight; i++)
   {
      canvas[i] = 0x0;
   }
}


void objsToCanvas()
{
   int i, j, x, y;
   int xDelta, yDelta, canvasDataLoc;
   int bytesWidth = canvasWidth * 3 + canvasWidth % 4;
   int brightness;
   int imgObjNum;
   
   for (i=0; i < OBJCNT; i++)
   {
      //convert rotations into unit circle coordinates
      for (j=0; j < OPCNT; j++)
      {
         if (objOps[i][j] == END)
         break;
         
         if ((objOps[i][j] == XROT) ||
             (objOps[i][j] == YROT) ||
             (objOps[i][j] == ZROT))
         {
            hUcRotValues[j] = cos(objOpValues[i][j] * (M_PI * 2.0 / 360.0));
            vUcRotValues[j] = sin(objOpValues[i][j] * (M_PI * 2.0 / 360.0));
         }
      }
      
      for (j=0; j < OPCNT; j++)
      {
         if (objOps[i][j] == END)// Don't draw if "END" or "HIDE"
         break;
      
         if (objOps[i][j] == IMGOBJ)
         {
            imgObjNum = objOpValues[i][j];
         
            for (y=0; y < IMGBUFF_HEIGHT; y++)
            {
               for (x=0; x < IMGBUFF_WIDTH; x++)
               {
                  if (imgObjNum < IMGOBJCNT)
                  brightness = imgObj[imgObjNum][IMGBUFF_WIDTH*y+x];
                  
                  if (brightness)
                  {
                     applyObjOps(i, j+1, x, y, &xDelta, &yDelta, &brightness);
                     
                     canvasDataLoc = xDelta * 3 + bytesWidth * yDelta;
                     
                     if ((xDelta >= 0) && (xDelta < canvasWidth) &&
                         (yDelta >= 0) && (yDelta < canvasHeight))
                     if (brightness > canvas[0+canvasDataLoc])//draw only if brighter
                     {
                        canvas[0+canvasDataLoc] = brightness;
                        canvas[1+canvasDataLoc] = brightness;
                        canvas[2+canvasDataLoc] = brightness;
                     }
                  }
               }
            }
         }
      }
   }
}


void chgObj(int obj, int opNum, int op, float opValue)
{
   if ((obj >= OBJCNT) || (opNum >= OPCNT))
   return;
   
   objOps[obj][opNum] = op;
   objOpValues[obj][opNum] = opValue;
}


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
TCHAR szAppName[] = TEXT("Space Invaders");

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
   static int i;
   static int opInc;
   
   static int rotUp = FALSE;
   static int rotDown = FALSE;
   static int rotLeft = FALSE;
   static int rotRight = FALSE;
   static float xShipRot = 0;
   static float yShipRot = 0;
   static int animatePointer = 0;
   
   static int fireBullet = FALSE;
   static float bulletDistance = 0;
   static float xBulletRot = 0;
   static float yBulletRot = 0;
   static float xBulletLoc = 0;
   static float yBulletLoc = 0;
   static float zBulletLoc = 0;
   static float rotBullet = 0;
   
   static float rotInvader = 0;
   static float invaderSpeed = 0.2;
   static float xInvaderLoc[INVADERCNT];
   static float yInvaderLoc[INVADERCNT];
   static float zInvaderLoc[INVADERCNT];
   
   static int resetGame = TRUE;
   static int gameOver = FALSE;
   static char endGameStr[256];
   static int scoreNum = 0;
   
   
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
      
      loadImg(SHIPIMG, "ship.bmp");
      loadImg(CIRCLEIMG, "circle.bmp");
      loadImg(INVADERIMG, "invader.bmp");
      
      return 0;
   }
   
   if (WM_SIZE == message)
   {
      canvasWidth = LOWORD(lParam);
      canvasHeight = HIWORD(lParam);
      xCenter = canvasWidth / 2;
      yCenter = canvasHeight / 2;
      
      pbmi->bmiHeader.biWidth = canvasWidth;
      pbmi->bmiHeader.biHeight = canvasHeight;
      pbmi->bmiHeader.biSizeImage = canvasWidth * canvasHeight;
      
      return 0;
   }

   if (WM_TIMER == message)
   {
      //////////////////////////////////////GAME LOGIC
      if (resetGame)
      {
         xShipRot = 0;
         yShipRot = 0;
         
         bulletDistance = 0;
         fireBullet = FALSE;
         
         //move invaders out away from ship
         for (i=0; i < INVADERCNT; i++)
         {
            xInvaderLoc[i] = 0;
            yInvaderLoc[i] = 400;
            zInvaderLoc[i] = 0;
            rot(&yInvaderLoc[i], &zInvaderLoc[i], rand() % 360);
            rot(&xInvaderLoc[i], &zInvaderLoc[i], rand() % 360);
         }
         
         invaderSpeed = 0.2;
         
         scoreNum = 0;
         resetGame = FALSE;
      }
      
      //rotate ship
      if (rotUp)
      xShipRot += 4;
      if (rotDown)
      xShipRot -= 4;
      if (rotLeft)
      yShipRot -= 4;
      if (rotRight)
      yShipRot += 4;
      
      
      if (fireBullet)
      bulletDistance += 15;
      
      //find bullet location
      xBulletLoc = 0;
      yBulletLoc = bulletDistance;
      zBulletLoc = 0;
      rot(&yBulletLoc, &zBulletLoc, xBulletRot);//x rotate
      rot(&xBulletLoc, &zBulletLoc, yBulletRot);//y rotate
      
      
      for (i=0; i < INVADERCNT; i++)
      {
         //move invaders closer to ship (location 0)
         float invaderToShipDist;
         invaderToShipDist = sqrt(pow(sqrt(pow(xInvaderLoc[i],2)+
                                           pow(yInvaderLoc[i],2)),2)+
                                           pow(zInvaderLoc[i],2));
         
         xInvaderLoc[i] *= (invaderToShipDist-invaderSpeed) / invaderToShipDist;
         yInvaderLoc[i] *= (invaderToShipDist-invaderSpeed) / invaderToShipDist;
         zInvaderLoc[i] *= (invaderToShipDist-invaderSpeed) / invaderToShipDist;
         
         //game over if an invader reaches the ship
         if (invaderToShipDist <= 30)
         {
            gameOver = TRUE;
            sprintf(endGameStr, "SCORE: %i   Press Enter To Play", scoreNum);
            SetWindowText(hwnd, TEXT(endGameStr));
         }
         
         float bulletToInvaderDist;
         bulletToInvaderDist = sqrt(pow(sqrt(pow(xBulletLoc-xInvaderLoc[i],2)+
                                             pow(yBulletLoc-yInvaderLoc[i],2)),2)+
                                             pow(zBulletLoc-zInvaderLoc[i],2));
         
         //reset invader if the bullet hits the invader
         if (fireBullet)
         if (gameOver == FALSE)
         if (bulletToInvaderDist <= 30)
         {
            xInvaderLoc[i] = 0;
            yInvaderLoc[i] = 400;
            zInvaderLoc[i] = 0;
            rot(&yInvaderLoc[i], &zInvaderLoc[i], rand() % 360);//x rotate
            rot(&xInvaderLoc[i], &zInvaderLoc[i], rand() % 360);//y rotate
            invaderSpeed += 0.003;
            
            bulletDistance = 0;
            
            scoreNum++;
            fireBullet = FALSE;
         }
      }
      
      //reset bullet after distance
      if (bulletDistance >= 400)
      {
         bulletDistance = 0;
         fireBullet = FALSE;
      }
      
      //animate bullet, invader, and pointer
      rotBullet += 5;
      rotInvader += 8;
      animatePointer = (animatePointer + 3) % 50;
      
      
      //////////////////////////////////////DRAW AND OBJECT CONTROL
      opInc = 0;
      chgObj(BULLET, opInc++, IMGOBJ, CIRCLEIMG);//draw bullet
      chgObj(BULLET, opInc++, XSZ, 0.4);
      chgObj(BULLET, opInc++, YSZ, 0.4);
      chgObj(BULLET, opInc++, XROT, rotBullet);
      chgObj(BULLET, opInc++, YROT, rotBullet);
      chgObj(BULLET, opInc++, XLOC, xBulletLoc);
      chgObj(BULLET, opInc++, YLOC, yBulletLoc);
      chgObj(BULLET, opInc++, ZLOC, zBulletLoc);
      
      chgObj(BULLET, opInc++, IMGOBJ, CIRCLEIMG);
      chgObj(BULLET, opInc++, XSZ, 0.4);
      chgObj(BULLET, opInc++, YSZ, 0.4);
      chgObj(BULLET, opInc++, XROT, 90);
      chgObj(BULLET, opInc++, XROT, rotBullet);
      chgObj(BULLET, opInc++, YROT, rotBullet);
      chgObj(BULLET, opInc++, XLOC, xBulletLoc);
      chgObj(BULLET, opInc++, YLOC, yBulletLoc);
      chgObj(BULLET, opInc++, ZLOC, zBulletLoc);
      
      chgObj(BULLET, opInc++, IMGOBJ, CIRCLEIMG);
      chgObj(BULLET, opInc++, XSZ, 0.4);
      chgObj(BULLET, opInc++, YSZ, 0.4);
      chgObj(BULLET, opInc++, YROT, 90);
      chgObj(BULLET, opInc++, XROT, rotBullet);
      chgObj(BULLET, opInc++, YROT, rotBullet);
      chgObj(BULLET, opInc++, XLOC, xBulletLoc);
      chgObj(BULLET, opInc++, YLOC, yBulletLoc);
      chgObj(BULLET, opInc++, ZLOC, zBulletLoc);
      chgObj(BULLET, opInc++, END, 0);
      
      
      opInc = 0;
      chgObj(SHIP, opInc++, IMGOBJ, SHIPIMG);//draw ship
      chgObj(SHIP, opInc++, XSZ, 2.5);
      chgObj(SHIP, opInc++, XROT, xShipRot);
      chgObj(SHIP, opInc++, YROT, yShipRot);
      
      for (i=0; i < 3; i++)
      {
         chgObj(SHIP, opInc++, IMGOBJ, SHIPIMG);//pointer
         chgObj(SHIP, opInc++, XSZ, 0.3);
         chgObj(SHIP, opInc++, YSZ, 0.3);
         chgObj(SHIP, opInc++, YLOC, 50+50*i+animatePointer);
         chgObj(SHIP, opInc++, XROT, xShipRot);
         chgObj(SHIP, opInc++, YROT, yShipRot);
      }
      
      chgObj(SHIP, opInc++, IMGOBJ, SHIPIMG);
      chgObj(SHIP, opInc++, YROT, 90);
      chgObj(SHIP, opInc++, XROT, xShipRot);
      chgObj(SHIP, opInc++, YROT, yShipRot);
      
      chgObj(SHIP, opInc++, IMGOBJ, SHIPIMG);
      chgObj(SHIP, opInc++, XSZ, 0.6);
      chgObj(SHIP, opInc++, YSZ, 0.6);
      chgObj(SHIP, opInc++, YROT, 90);
      chgObj(SHIP, opInc++, XLOC, 70);
      chgObj(SHIP, opInc++, YLOC, -25);
      chgObj(SHIP, opInc++, XROT, xShipRot);
      chgObj(SHIP, opInc++, YROT, yShipRot);
      
      chgObj(SHIP, opInc++, IMGOBJ, SHIPIMG);
      chgObj(SHIP, opInc++, XSZ, 0.6);
      chgObj(SHIP, opInc++, YSZ, 0.6);
      chgObj(SHIP, opInc++, YROT, 90);
      chgObj(SHIP, opInc++, XLOC, -70);
      chgObj(SHIP, opInc++, YLOC, -25);
      chgObj(SHIP, opInc++, XROT, xShipRot);
      chgObj(SHIP, opInc++, YROT, yShipRot);
      chgObj(SHIP, opInc++, END, 0);
      
      
      for (i=0; i < INVADERCNT; i++)
      {
         opInc = 0;
         chgObj(INVADER+i, opInc++, IMGOBJ, INVADERIMG);//draw invaders
         chgObj(INVADER+i, opInc++, ZLOC, -1);
         chgObj(INVADER+i, opInc++, YROT, rotInvader);
         chgObj(INVADER+i, opInc++, XLOC, xInvaderLoc[i]);
         chgObj(INVADER+i, opInc++, YLOC, yInvaderLoc[i]);
         chgObj(INVADER+i, opInc++, ZLOC, zInvaderLoc[i]);
         
         chgObj(INVADER+i, opInc++, IMGOBJ, INVADERIMG);
         chgObj(INVADER+i, opInc++, ZLOC, 1);
         chgObj(INVADER+i, opInc++, YROT, rotInvader);
         chgObj(INVADER+i, opInc++, XLOC, xInvaderLoc[i]);
         chgObj(INVADER+i, opInc++, YLOC, yInvaderLoc[i]);
         chgObj(INVADER+i, opInc++, ZLOC, zInvaderLoc[i]);
         chgObj(INVADER+i, opInc++, END, 0);
      }
      
      
      if (gameOver)
      {
         chgObj(BULLET, 0, HIDE, 0);//hide all objects
         chgObj(SHIP, 0, HIDE, 0);
         for (i=0; i < INVADERCNT; i++)
         {
            chgObj(INVADER+i, 0, HIDE, 0);
         }
         
         opInc = 0;
         chgObj(INVADER, opInc++, IMGOBJ, INVADERIMG);//draw invader
         chgObj(INVADER, opInc++, ZLOC, -1);
         chgObj(INVADER, opInc++, YROT, rotInvader);
         chgObj(INVADER, opInc++, ZLOC, 200);
         
         chgObj(INVADER, opInc++, IMGOBJ, INVADERIMG);
         chgObj(INVADER, opInc++, ZLOC, 1);
         chgObj(INVADER, opInc++, YROT, rotInvader);
         chgObj(INVADER, opInc++, ZLOC, 200);
         chgObj(INVADER, opInc++, END, 0);
      }
      
      clearCanvas();
      objsToCanvas();//draw objects to screen
      
      InvalidateRect(hwnd, NULL, TRUE);
      UpdateWindow(hwnd);
      
      hdc = GetDC(hwnd);
      SetDIBitsToDevice(hdc, 0, 0, canvasWidth, canvasHeight,
                        0, 0, 0, canvasHeight, canvas, pbmi, DIB_RGB_COLORS);
      ReleaseDC(hwnd, hdc);
      
      return 0;
   }
   
   if (WM_KEYUP == message)
   {
      if (LOWORD(wParam) == 38)
      rotUp = FALSE;
      if (LOWORD(wParam) == 40)
      rotDown = FALSE;
      if (LOWORD(wParam) == 37)
      rotLeft = FALSE;
      if (LOWORD(wParam) == 39)
      rotRight = FALSE;
      
      if (LOWORD(wParam) == 32)//spacebar
      if (bulletDistance == 0)
      fireBullet = FALSE;
      
      return 0;
   }
   
   if (WM_KEYDOWN == message)
   {
      //printf("keydown value %i\n", LOWORD(wParam));
      //fflush(stdout);
      
      if (LOWORD(wParam) == 38)
      rotUp = TRUE;
      if (LOWORD(wParam) == 40)
      rotDown = TRUE;
      if (LOWORD(wParam) == 37)
      rotLeft = TRUE;
      if (LOWORD(wParam) == 39)
      rotRight = TRUE;
      
      if (LOWORD(wParam) == 32)//spacebar
      if (bulletDistance == 0)
      {
         fireBullet = TRUE;
         xBulletRot = xShipRot;
         yBulletRot = yShipRot;
      }
      
      if (LOWORD(wParam) == 13)//Enter
      {
         gameOver = FALSE;
         resetGame = TRUE;
         SetWindowText(hwnd, TEXT("Space Invaders"));
      }
      
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
