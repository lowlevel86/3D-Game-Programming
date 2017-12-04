//compile with:
//tcc -mwindows -o platformer.exe platformer.c

//execute directly with:
//tcc -run platformer.c

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>


#define WIN_WIDTH 640 //window size
#define WIN_HEIGHT 400
#define IMGBUFF_WIDTH 256 //max bmp image size
#define IMGBUFF_HEIGHT 256

#define IMGOBJCNT 4
#define OBJCNT 9
#define OPCNT 128 //max number of operations that can be done on an object

#define ROBOTIMG 0 //image id
#define WHEELIMG 1
#define CHECKEREDIMG 2
#define CATBORGIMG 3
#define ROBOT 0//object id
#define PLATFORM 1
#define PLATFORMCNT 5
#define CATBORG 6
#define CATBORGCNT 3

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


inline double round(double val)
{    
   return floor(val + 0.5);
}


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
   float perspctv = 350;
   float cameraLens = 200;//less than perspctv
   float cameraDistance = -200;
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
   *xDelta = round(xPt * perspctv / (perspctv - zPt) + xCenter);
   *yDelta = round(yPt * perspctv / (perspctv - zPt) + yCenter);
   
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


int zCollision(float *xPt, float *yPt, float *zPt,
               float xLoc, float yLoc, float *zCollisionLoc)
{
   float xPt0Pt1, zPt0Pt1;
   float xPt1Pt2, zPt1Pt2;
   float xPt2Pt3, zPt2Pt3;
   float xPt3Pt0, zPt3Pt0;
   float zCollisionLocA;
   float zCollisionLocB;
   
   //extend or contract 2 parallel edges of the quad to the same y-plane as "yLoc"
   xPt0Pt1 = xPt[0] - (yPt[0] - yLoc) / (yPt[1] - yPt[0]) * (xPt[1] - xPt[0]);
   zPt0Pt1 = zPt[0] - (yPt[0] - yLoc) / (yPt[1] - yPt[0]) * (zPt[1] - zPt[0]);
   
   xPt1Pt2 = xPt[1] - (yPt[1] - yLoc) / (yPt[2] - yPt[1]) * (xPt[2] - xPt[1]);
   zPt1Pt2 = zPt[1] - (yPt[1] - yLoc) / (yPt[2] - yPt[1]) * (zPt[2] - zPt[1]);
   
   xPt2Pt3 = xPt[2] - (yPt[2] - yLoc) / (yPt[3] - yPt[2]) * (xPt[3] - xPt[2]);
   zPt2Pt3 = zPt[2] - (yPt[2] - yLoc) / (yPt[3] - yPt[2]) * (zPt[3] - zPt[2]);
   
   xPt3Pt0 = xPt[3] - (yPt[3] - yLoc) / (yPt[0] - yPt[3]) * (xPt[0] - xPt[3]);
   zPt3Pt0 = zPt[3] - (yPt[3] - yLoc) / (yPt[0] - yPt[3]) * (zPt[0] - zPt[3]);
   
   //extend or contract a horizontal line to the same x-plane as "xLoc"
   zCollisionLocA = zPt0Pt1 - (xPt0Pt1 - xLoc) / (xPt2Pt3 - xPt0Pt1) * (zPt2Pt3 - zPt0Pt1);
   zCollisionLocB = zPt1Pt2 - (xPt1Pt2 - xLoc) / (xPt3Pt0 - xPt1Pt2) * (zPt3Pt0 - zPt1Pt2);
   
   if (zCollisionLocA == zCollisionLocA)// check if NaN
   *zCollisionLoc = zCollisionLocA;
   
   if (zCollisionLocB == zCollisionLocB)
   *zCollisionLoc = zCollisionLocB;
   
   //return true if collision
   if (((zPt0Pt1 == zPt0Pt1) && (zPt1Pt2 == zPt1Pt2)) &&
       ((xLoc >= xPt0Pt1) && (xLoc <= xPt2Pt3) ||
        (xLoc >= xPt2Pt3) && (xLoc <= xPt0Pt1)) &&
       ((xLoc >= xPt1Pt2) && (xLoc <= xPt3Pt0) ||
        (xLoc >= xPt3Pt0) && (xLoc <= xPt1Pt2)))
   return TRUE;
   
   if ((zPt0Pt1 != zPt0Pt1) &&
      ((xLoc >= xPt[0]) && (xLoc <= xPt[1]) ||
       (xLoc >= xPt[1]) && (xLoc <= xPt[0])) &&
      ((yLoc >= yPt[1]) && (yLoc <= yPt[2]) ||
       (yLoc >= yPt[2]) && (yLoc <= yPt[1])))
   return TRUE;
   
   if ((zPt1Pt2 != zPt1Pt2) &&
      ((xLoc >= xPt[1]) && (xLoc <= xPt[2]) ||
       (xLoc >= xPt[2]) && (xLoc <= xPt[1])) &&
      ((yLoc >= yPt[0]) && (yLoc <= yPt[1]) ||
       (yLoc >= yPt[1]) && (yLoc <= yPt[0])))
   return TRUE;
   
   return FALSE;
}


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
TCHAR szAppName[] = TEXT("Platformer");

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
   static int i, j, opInc;
   
   static int leftKeyDown = FALSE;
   static int rightKeyDown = FALSE;
   static int upKeyDown = FALSE;
   static int downKeyDown = FALSE;
   static int spacebarDown = FALSE;
   
   static float xWorldLoc = 0;
   static float yWorldLoc = 0;
   static float zWorldLoc = -200;
   static float xCameraRot = 30;
   static float zCameraRot = 0;
   
   static float robotSz = 64;
   static float zRobotRot = 0;
   static float xMoveRobot = 0;
   static float yMoveRobot = 0;
   static float zMoveRobot = 0;
   static float zWheelRot = 0;
   
   static float zCollisionLoc;
   static int inPlatformBounds;
   static int onPlatform;
   
   static int jump = FALSE;
   static int jumpDuration = 10;//number of frames
   static int jumpDurationInc = 0;
   static float jumpSpeed = 30;
   
   //platform size, rotation, and location
   static float xPlatformSz[PLATFORMCNT] = {4, 4, 4, 4, 4};
   static float yPlatformSz[PLATFORMCNT] = {4, 4, 4, 4, 4};
   
   static float xPlatformRot[PLATFORMCNT] = {0, -20, -20,   0,   0};
   static float yPlatformRot[PLATFORMCNT] = {0,   0,   0, -20, -20};
   static float zPlatformRot[PLATFORMCNT] = {0, -45, -45, -45, -45};
   
   static float xPlatformLoc[PLATFORMCNT] = {0, -200, -300,  200,  300};
   static float yPlatformLoc[PLATFORMCNT] = {0, -300, -600, -300, -600};
   static float zPlatformLoc[PLATFORMCNT] = {0,    0,    0,    0,    0};
   
   //"catborg" rotation and location
   static float zCatborgRot[CATBORGCNT] = {0, 120, 240};
   
   static float xCatborgLoc[CATBORGCNT] = {0,   -475,  475};
   static float yCatborgLoc[CATBORGCNT] = {150, -675, -675};
   static float zCatborgLoc[CATBORGCNT] = {100,  100,  100};
   
   
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
      
      loadImg(CHECKEREDIMG, "checkered.bmp");
      loadImg(ROBOTIMG, "robot.bmp");
      loadImg(WHEELIMG, "wheel.bmp");
      loadImg(CATBORGIMG, "catborg.bmp");
      
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
      if ((upKeyDown) || (downKeyDown))
      {
         zRobotRot = zCameraRot;
         
         if (upKeyDown)
         {
            yMoveRobot = 5;
            zWheelRot += 5;
         }
         
         if (downKeyDown)
         {
            yMoveRobot = -5;
            zWheelRot -= 5;
         }
         
         xMoveRobot = 0;
         zMoveRobot = 0;
      }
      else
      {
         xMoveRobot = 0;
         yMoveRobot = 0;
         zMoveRobot = 0;
      }
      
      if (leftKeyDown)
      zCameraRot -= 5;
      
      if (rightKeyDown)
      zCameraRot += 5;
      
      if ((onPlatform) && (spacebarDown))
      jump = TRUE;
      
      if (spacebarDown == FALSE)
      jump = FALSE;
      
      if (onPlatform)
      jumpDurationInc = 0;
      
      if ((jump) && (jumpDurationInc < jumpDuration))
      {
         zWorldLoc -= jumpSpeed;
         jumpDurationInc++;
      }
      
      //calculate the location of the world relative to the robot
      rot(&xMoveRobot, &yMoveRobot, -zCameraRot);//z rotate
      xWorldLoc -= xMoveRobot;
      yWorldLoc -= yMoveRobot;
      
      
      onPlatform = FALSE;
      
      for (i=0; i < PLATFORMCNT; i++)
      {
         #define PLATFORMHALF 32
         float xPlatformCorner[4] = {PLATFORMHALF, -PLATFORMHALF, -PLATFORMHALF, PLATFORMHALF};
         float yPlatformCorner[4] = {PLATFORMHALF, PLATFORMHALF, -PLATFORMHALF, -PLATFORMHALF};
         float zPlatformCorner[4] = {0, 0, 0, 0};
         
         //scale, rotate, and move a quad that represets a platform 
         for (j=0; j < 4; j++)
         {
            xPlatformCorner[j] *= xPlatformSz[i];
            yPlatformCorner[j] *= yPlatformSz[i];
            rot(&yPlatformCorner[j], &zPlatformCorner[j], xPlatformRot[i]);//x rotate
            rot(&xPlatformCorner[j], &zPlatformCorner[j], yPlatformRot[i]);//y rotate
            rot(&xPlatformCorner[j], &yPlatformCorner[j], zPlatformRot[i]);//z rotate
            xPlatformCorner[j] += xPlatformLoc[i];
            yPlatformCorner[j] += yPlatformLoc[i];
            zPlatformCorner[j] += zPlatformLoc[i];
            xPlatformCorner[j] += xWorldLoc;
            yPlatformCorner[j] += yWorldLoc;
         }
         
         inPlatformBounds = zCollision(xPlatformCorner, yPlatformCorner,
                                       zPlatformCorner, 0, 0, &zCollisionLoc);
         
         if ((inPlatformBounds) &&
             (zWorldLoc < -(zCollisionLoc + (float)1/3*robotSz)) &&//is robot at the right height
             (zWorldLoc > -(zCollisionLoc + (float)2/3*robotSz)))
         {
            onPlatform = TRUE;
            break;
         }
      }
      
      if (onPlatform)
      zWorldLoc = -(zCollisionLoc + (float)1/2*robotSz);//note: robot is always at location 0
      else
      zWorldLoc += 8;
      
      //reset if fell
      if (zWorldLoc > 200)
      {
         xWorldLoc = 0;
         yWorldLoc = 0;
         zWorldLoc = -200;
      }
      
      
      //////////////////////////////////////DRAW AND OBJECT CONTROL
      opInc = 0;
      chgObj(ROBOT, opInc++, IMGOBJ, ROBOTIMG);//draw robot
      chgObj(ROBOT, opInc++, ZLOC, 2);
      chgObj(ROBOT, opInc++, XROT, 90-5);
      chgObj(ROBOT, opInc++, ZROT, zCameraRot-zRobotRot);
      chgObj(ROBOT, opInc++, XROT, -70);
      
      chgObj(ROBOT, opInc++, IMGOBJ, ROBOTIMG);
      chgObj(ROBOT, opInc++, ZLOC, -2);
      chgObj(ROBOT, opInc++, XROT, 90-5);
      chgObj(ROBOT, opInc++, ZROT, zCameraRot-zRobotRot);
      chgObj(ROBOT, opInc++, XROT, -70);
      
      chgObj(ROBOT, opInc++, IMGOBJ, WHEELIMG);//draw robot wheel
      chgObj(ROBOT, opInc++, ZROT, zWheelRot);
      chgObj(ROBOT, opInc++, XSZ, 0.9);
      chgObj(ROBOT, opInc++, YSZ, 0.9);
      chgObj(ROBOT, opInc++, YROT, 90);
      chgObj(ROBOT, opInc++, XLOC, 5);
      chgObj(ROBOT, opInc++, ZLOC, -16);
      chgObj(ROBOT, opInc++, ZROT, zCameraRot-zRobotRot);
      chgObj(ROBOT, opInc++, XROT, -70);
      
      chgObj(ROBOT, opInc++, IMGOBJ, WHEELIMG);
      chgObj(ROBOT, opInc++, ZROT, zWheelRot);
      chgObj(ROBOT, opInc++, YROT, 90);
      chgObj(ROBOT, opInc++, XLOC, 2);
      chgObj(ROBOT, opInc++, ZLOC, -16);
      chgObj(ROBOT, opInc++, ZROT, zCameraRot-zRobotRot);
      chgObj(ROBOT, opInc++, XROT, -70);
      
      chgObj(ROBOT, opInc++, IMGOBJ, WHEELIMG);
      chgObj(ROBOT, opInc++, ZROT, zWheelRot);
      chgObj(ROBOT, opInc++, YROT, 90);
      chgObj(ROBOT, opInc++, XLOC, -2);
      chgObj(ROBOT, opInc++, ZLOC, -16);
      chgObj(ROBOT, opInc++, ZROT, zCameraRot-zRobotRot);
      chgObj(ROBOT, opInc++, XROT, -70);
      
      chgObj(ROBOT, opInc++, IMGOBJ, WHEELIMG);
      chgObj(ROBOT, opInc++, ZROT, zWheelRot);
      chgObj(ROBOT, opInc++, XSZ, 0.9);
      chgObj(ROBOT, opInc++, YSZ, 0.9);
      chgObj(ROBOT, opInc++, YROT, 90);
      chgObj(ROBOT, opInc++, XLOC, -5);
      chgObj(ROBOT, opInc++, ZLOC, -16);
      chgObj(ROBOT, opInc++, ZROT, zCameraRot-zRobotRot);
      chgObj(ROBOT, opInc++, XROT, -70);
      chgObj(ROBOT, opInc++, END, 0);
      
      
      for (i=0; i < PLATFORMCNT; i++)
      {
         opInc = 0;
         chgObj(PLATFORM+i, opInc++, IMGOBJ, CHECKEREDIMG);//draw platforms
         chgObj(PLATFORM+i, opInc++, XSZ, xPlatformSz[i]);
         chgObj(PLATFORM+i, opInc++, YSZ, yPlatformSz[i]);
         chgObj(PLATFORM+i, opInc++, XROT, xPlatformRot[i]);
         chgObj(PLATFORM+i, opInc++, YROT, yPlatformRot[i]);
         chgObj(PLATFORM+i, opInc++, ZROT, zPlatformRot[i]);
         chgObj(PLATFORM+i, opInc++, XLOC, xPlatformLoc[i]);
         chgObj(PLATFORM+i, opInc++, YLOC, yPlatformLoc[i]);
         chgObj(PLATFORM+i, opInc++, ZLOC, zPlatformLoc[i]);
         
         chgObj(PLATFORM+i, opInc++, XLOC, xWorldLoc);
         chgObj(PLATFORM+i, opInc++, YLOC, yWorldLoc);
         chgObj(PLATFORM+i, opInc++, ZLOC, zWorldLoc);
         
         chgObj(PLATFORM+i, opInc++, ZROT, zCameraRot);
         chgObj(PLATFORM+i, opInc++, XROT, -70);
         chgObj(PLATFORM+i, opInc++, END, 0);
      }
      
      
      for (i=0; i < CATBORGCNT; i++)
      {
         opInc = 0;
         chgObj(CATBORG+i, opInc++, IMGOBJ, CATBORGIMG);//draw catborg
         chgObj(CATBORG+i, opInc++, XROT, 90);
         chgObj(CATBORG+i, opInc++, ZROT, zCatborgRot[i]);
         chgObj(CATBORG+i, opInc++, XLOC, xCatborgLoc[i]);
         chgObj(CATBORG+i, opInc++, YLOC, yCatborgLoc[i]);
         chgObj(CATBORG+i, opInc++, ZLOC, zCatborgLoc[i]);
         chgObj(CATBORG+i, opInc++, XLOC, xWorldLoc);
         chgObj(CATBORG+i, opInc++, YLOC, yWorldLoc);
         chgObj(CATBORG+i, opInc++, ZLOC, zWorldLoc);
         
         chgObj(CATBORG+i, opInc++, ZROT, zCameraRot);
         chgObj(CATBORG+i, opInc++, XROT, -70);
         chgObj(CATBORG+i, opInc++, END, 0);
      }
      
      
      //don't draw objects that are far away
      for (i=0; i < PLATFORMCNT; i++)
      {
         opInc = 0;
         if ((fabs(xPlatformLoc[i] + xWorldLoc) > 400) ||
             (fabs(yPlatformLoc[i] + yWorldLoc) > 400) ||
             (fabs(zPlatformLoc[i] + zWorldLoc) > 400))
         chgObj(PLATFORM+i, opInc++, HIDE, 0);
      }
      
      for (i=0; i < CATBORGCNT; i++)
      {
         opInc = 0;
         if ((fabs(xCatborgLoc[i] + xWorldLoc) > 400) ||
             (fabs(yCatborgLoc[i] + yWorldLoc) > 400) ||
             (fabs(zCatborgLoc[i] + zWorldLoc) > 400))
         chgObj(CATBORG+i, opInc++, HIDE, 0);
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
      if (LOWORD(wParam) == 37)//left arrow
      leftKeyDown = FALSE;
      
      if (LOWORD(wParam) == 39)//right arrow
      rightKeyDown = FALSE;
      
      if (LOWORD(wParam) == 38)//right arrow
      upKeyDown = FALSE;
      
      if (LOWORD(wParam) == 40)//down arrow
      downKeyDown = FALSE;
      
      if (LOWORD(wParam) == 32)//spacebar
      spacebarDown = FALSE;
   }
   
   if (WM_KEYDOWN == message)
   {
      //printf("keydown value %i\n", LOWORD(wParam));
      //fflush(stdout);
      
      if (LOWORD(wParam) == 37)//left arrow
      leftKeyDown = TRUE;
      
      if (LOWORD(wParam) == 39)//right arrow
      rightKeyDown = TRUE;
      
      if (LOWORD(wParam) == 38)//up arrow
      upKeyDown = TRUE;
      
      if (LOWORD(wParam) == 40)//down arrow
      downKeyDown = TRUE;
      
      if (LOWORD(wParam) == 32)//spacebar
      spacebarDown = TRUE;
      
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
