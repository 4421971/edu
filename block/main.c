//////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "bitmap.h"

//////////////////////////////////////////////////////////////////////////

static long CALLBACK wndProc( HWND, unsigned int, WPARAM, LPARAM );
static HWND createDraw2DWindow();

//////////////////////////////////////////////////////////////////////////

static void appInit();
static void appRun();
static void appExit();

//////////////////////////////////////////////////////////////////////////

HWND gWnd = NULL;
HDC gDC = NULL;

//////////////////////////////////////////////////////////////////////////

#define WND_WIDTH 640
#define WND_HEIGHT 960
WORD* wnd_pixels;  // 窗口像素缓存

//////////////////////////////////////////////////////////////////////////

typedef struct s_block
{
	int type;
	int dir;
	int x; 
	int y;                                                                                                                                                                      
}block_t; 

bitmap_t* block_bmp = NULL;
block_t* g_block = NULL;

const int g_background_width = WND_WIDTH / 32;
const int g_background_height = WND_HEIGHT / 32;
int* g_background = NULL;

#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define MOVE_DOWN 2

static unsigned long last_time = 0;

//////////////////////////////////////////////////////////////////////////

block_t* CreateNewBlock();
void ChangeBlockDir(block_t*b);
int GetBlockWidth(block_t*b);
int GetBlockHeight(block_t*b);
int* GetBlockData(block_t*b);
int MoveBlock(block_t*b, int dir);
void MergeBlockTobackground(block_t*b);
void CleanWholeRows();
void DrawBlock(block_t*b);
void DrawBackground();

//////////////////////////////////////////////////////////////////////////

void blt(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y);
void blt_alpha(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y);
void blt_mask(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y, unsigned short color);
void blt_trans(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y, unsigned char alpha);

//////////////////////////////////////////////////////////////////////////

/*
====================
main
====================
*/ 
int main(int argc, char** argv)
{
	WNDCLASSEX wcex;
	MSG msg;

	// register the window class
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)wndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= GetModuleHandle(NULL);
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)WHITE_BRUSH;
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= TEXT("Draw2DWindow");
	wcex.hIconSm		= 0;
	if (RegisterClassEx(&wcex) == 0)
	{
		return -1;
	}

	appInit();

	// the message loop	
	memset(&msg,0,sizeof(msg));
	while(msg.message != WM_QUIT) 
	{
		if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} 
		else 
		{
			appRun();
			Sleep(30);
		}
	}

	appExit();

	// unregister the window class
	UnregisterClass(TEXT("Draw2DWindow"), GetModuleHandle(NULL));

	return 0;
}

//////////////////////////////////////////////////////////////////////////

HWND createDraw2DWindow()	
{
	HWND hwnd;
	unsigned int ex_style;
	unsigned int style;
	RECT rc;
	int screen_width, screen_height, wnd_width, wnd_height;
	int x, y;

	// 设置窗口风格
	ex_style = 0;
	style = WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX;

	// 得到屏幕的宽和高
	screen_width = GetSystemMetrics(SM_CXSCREEN);
	screen_height = GetSystemMetrics(SM_CYSCREEN);

	// 根据窗口风格调整矩形, 调整窗口，加上边框
	rc.top = 0;							
	rc.left = 0;
	rc.right = WND_WIDTH;			
	rc.bottom = WND_HEIGHT;
	AdjustWindowRect(&rc, style, FALSE);

	// clamp window dimensions to screen size
	wnd_width = (rc.right-rc.left < screen_width)? rc.right-rc.left : screen_width;	
	wnd_height = (rc.bottom-rc.top < screen_height)? rc.bottom-rc.top : screen_height;

	// 计算窗口的起始位置，保证窗口居中
	x = (screen_width - wnd_width) / 2;
	y = (screen_height - wnd_height) / 2;
	if(x > screen_width - wnd_width) x = screen_width - wnd_width;
	if(y > screen_height - wnd_height) y = screen_height - wnd_height;	

	// 创建窗口
	hwnd = CreateWindowEx(ex_style, TEXT("Draw2DWindow"), TEXT("操作（左、右、下、空格、F1）"),style, x,y, wnd_width,wnd_height,NULL,NULL,GetModuleHandle(NULL), NULL);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);
	return hwnd;
}

long CALLBACK wndProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;		
	case WM_KEYDOWN:
		{
			if( wParam == VK_ESCAPE ) { PostQuitMessage(0); break; }
			switch(wParam)
			{
			case VK_UP:
				{		
				}
				break;
			case VK_DOWN:
				{
					if(MoveBlock(g_block, MOVE_DOWN) == 0) 
					{
						// 不能向下移动则合并方块
						MergeBlockTobackground(g_block);
						if(g_block != NULL) { free(g_block); g_block = NULL; }
						// 创建新的方块
						g_block = CreateNewBlock();
					}
				}
				break;
			case VK_LEFT:
				{
					MoveBlock(g_block, MOVE_LEFT);
				}
				break;
			case VK_RIGHT:
				{
					MoveBlock(g_block, MOVE_RIGHT);
				}
				break;
			case VK_F1:
				{
					if(g_background)
					{
						memset(g_background, 0, g_background_width * g_background_height * sizeof(int));
					}
				}
				break;
			case VK_SPACE:
				{
					ChangeBlockDir(g_block);
				}
				break;
			}
		}	
  		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
/*
====================
appInit
====================
*/
static void appInit()
{
	srand((unsigned)time(0)); 
	last_time = timeGetTime();

	// 创建窗口，获得可以对窗口绘图的dc
	gWnd = createDraw2DWindow();
	if (gWnd == NULL) return;
	gDC = GetDC(gWnd);
	if (gDC == NULL) return;
	// 创建窗口的像素缓存，所有图片先绘制到这个缓存上，最后再贴在窗口上
	wnd_pixels =(WORD*)malloc(WND_WIDTH*WND_HEIGHT*sizeof(unsigned short));
	memset(wnd_pixels, 0xff, WND_WIDTH*WND_HEIGHT*sizeof(unsigned short));  // 16位	

	// 创建方块背景图
	g_background = (int*)malloc(g_background_width * g_background_height * sizeof(int));
	memset(g_background, 0, g_background_width * g_background_height * sizeof(int));

	block_bmp = load_bmp("block.bmp");

	g_block = CreateNewBlock();
}

/*
====================
appRun
====================
*/
static void appRun()
{
	BITMAPV4HEADER bmp_header;

	// 每秒向下移动一格
	{	
		unsigned long cur_time = timeGetTime();
		if( cur_time - last_time > 1000 )
		{
			last_time = cur_time;
			if(MoveBlock(g_block, MOVE_DOWN) == 0) 
			{
				// 不能向下移动则合并方块
				MergeBlockTobackground(g_block);
				if(g_block != NULL) { free(g_block); g_block = NULL; }
				// 创建新的方块
				g_block = CreateNewBlock();
			}
		}
	}	

	memset(wnd_pixels, 0xff, WND_WIDTH*WND_HEIGHT*sizeof(unsigned short));
	DrawBackground();
	DrawBlock(g_block);

	// Fill out the bitmap info
	bmp_header.bV4Size = sizeof(bmp_header);
	bmp_header.bV4Width = WND_WIDTH;
	bmp_header.bV4Height = -WND_HEIGHT;
	bmp_header.bV4Planes = 1;
	bmp_header.bV4BitCount = 16;
	bmp_header.bV4V4Compression = BI_BITFIELDS;
	bmp_header.bV4SizeImage = 0;
	bmp_header.bV4XPelsPerMeter = 0;
	bmp_header.bV4YPelsPerMeter = 0;
	bmp_header.bV4ClrUsed = 0;
	bmp_header.bV4ClrImportant = 0;
	bmp_header.bV4RedMask = 0xF800;
	bmp_header.bV4GreenMask = 0x07E0;	
	bmp_header.bV4BlueMask = 0x001F;
	StretchDIBits(gDC, 0, 0, WND_WIDTH, WND_HEIGHT, 0, 0, WND_WIDTH, WND_HEIGHT, wnd_pixels, (LPBITMAPINFO)&bmp_header, DIB_RGB_COLORS, SRCCOPY);
}

/*
====================
appExit
====================
*/
static void appExit()
{
	if (block_bmp)
	{
		if (block_bmp->pixels)
		{
			free(block_bmp->pixels);
		}
		free(block_bmp);
		block_bmp = NULL;
	}

	if (g_background != NULL) { free(g_background); g_background = NULL; }
	if (wnd_pixels != NULL) { free(wnd_pixels); wnd_pixels = NULL; }

	if (gWnd)
	{
		// 释放dc, 与GetDC配对，如果不调用进程的GDI对象会不断增加
		if (gDC)
		{
			ReleaseDC(gWnd, gDC);
			gDC = NULL;
		}
		DestroyWindow(gWnd);
		gWnd = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

block_t* CreateNewBlock()
{
	block_t* b = (block_t*)malloc(sizeof(block_t));
	b->type = rand() % 5;
	b->dir = 0;
	b->x = (g_background_width - GetBlockWidth(b)) / 2;
	b->y = 0;
	return b;
}

void ChangeBlockDir(block_t*b)
{
	block_t temp;
	int w, h;
	int* data = NULL;
	memcpy(&temp, b, sizeof(block_t));
 	switch (b->type) 
	{
	case 0:
		{
			temp.dir = ++temp.dir % 4;
		}
		break;
	case 1:
		{
			temp.dir = ++temp.dir % 1;
		}
		break;
	case 2:
		{
			temp.dir = ++temp.dir % 2;
		}
		break;
	case 3:
		{
			temp.dir = ++temp.dir % 4;
		}
		break;
	case 4:
		{
			temp.dir = ++temp.dir % 2;
		}
		break;
	}
	w = GetBlockWidth(&temp);
	h = GetBlockHeight(&temp);
	data = GetBlockData(&temp);
	if((temp.x + w <= g_background_width) && (temp.y + h <= g_background_height))
	{
		// 检查是否和背景重叠
		int i, j;
		for(j = 0; j < h; j++)
		{
			for(i = 0; i < w; i++)
			{
				if (data[j * w + i] == 1 && g_background[temp.x + i + (temp.y + j) * g_background_width] == 1)
				{
					return;
				}
			}
		}
		b->dir = temp.dir;
	}
}

int GetBlockWidth(block_t*b)	
{
	if(b->type==0)
	{
		if(b->dir==0)
		{
			return 2;
		}
		else if(b->dir==1)
		{
			return 3;
		}
		else if(b->dir==2)
		{
			return 2;
		}
		else if(b->dir==3)
		{
			return 3;
		}		
	}

	if(b->type==1)
	{
		return 2;
	}
																																																																											
	if(b->type==2) 
	{
		if(b->dir==0)
		{
			return 1;
		}
		else if(b->dir==1)
		{
			return 4;
		}
	}


	if (b->type==3)
	{
		if (b->dir==0)
		{
			return 3;
		}
		else if (b->dir==1)
		{
			return 2;
		}
		else if (b->dir==2)
		{
			return 3;
        }
		else if (b->dir==3)
		{
			return 2;
        }		
	}

	if (b->type==4)
	{
		if (b->dir==0)
		{
			return 2;
		}
		else if (b->dir==1)
		{
			return 3;
		}
	}
	return 0;
}

int GetBlockHeight(block_t*b)
{
	if (b->type==0)
	{
		if (b->dir==0)
		{
			return 3;
		}
		else if (b->dir==1)
		{
			return 2; 
		}
		else if (b->dir==2)
		{
			return 3;
		}
		else if (b->dir==3)
		{
			return 2;
		}
	}

	if (b->type==1)
	{
		if (b->dir==0)
		{
			return 2;
		}
	}

	if (b->type==2)
	{
		if (b->dir==0)
		{
			return 4;
		}
		else if (b->dir==1)
		{
			return 1;
		}
	}

	if (b->type==3)
	{
		if (b->dir==0)
		{
			return 2;
		}
		else if (b->dir==1)
		{
			return 3;
		}
		else if (b->dir==2)
		{
			return 2;
		}
		else if (b->dir==3)
		{			
			return 3;
		}
	}

	if (b->type==4)
	{
		if (b->dir==0)
		{
			return 3;
		}
		else if (b->dir==1)
		{
			return 2;
		}
	}
	return 0;
}

int* GetBlockData(block_t*b)
{
	static int a0[2*3]={1,0,1,0,1,1};
	static int a1[3*2]={1,1,1,1,0,0};
	static int a2[2*3]={1,1,0,1,0,1};
	static int a3[3*2]={0,0,1,1,1,1};
	static int b0[2*2]={1,1,1,1};
	static int c0[1*4]={1,1,1,1};
	static int c1[4*1]={1,1,1,1};
	static int d0[3*2]={1,1,1,0,1,0};
	static int d1[2*3]={0,1,1,1,0,1};
	static int d2[3*2]={0,1,0,1,1,1};
	static int d3[2*3]={1,0,1,1,1,0};
	static int e0[2*3]={1,0,1,1,0,1};
	static int e1[3*2]={0,1,1,1,1,0};

	switch (b->type)
	{
	case 0:
		{
			switch (b->dir)
			{
			case 0:
				{
					return a0;
				}
				break;
			case 1:
				{
					return a1;
				}
				break;
			case 2:
				{
					return a2;
				}
				break;
			case 3:
				{
					return a3;
				}
				break;
			}
		}
		break;
	case 1:
		{
			return b0;
		}
		break;
	case 2:
		{
			switch (b->dir)
			{
			case 0:
				{
					return c0;
				}
				break;
			case 1:
				{
					return c1;
				}
				break;
			}
		}
		break;
	case 3:
		{
			switch (b->dir)
			{
			case 0:
				{
					return d0;
				}
				break;
			case 1:
				{
					return d1;
				}
				break;
			case 2:
				{
					return d2;
				}
				break;
			case 3:
				{
					return d3;
				}
				break;
			}
			
		}
		break;
	case 4:
		{
			switch (b->dir)
			{
			case 0:
				{
					return e0;
				}
				break;
			case 1:
				{
					return e1;
				}
				break;
			}
		}
		break;
	}
	return NULL;
}

int MoveBlock(block_t*b, int dir)
{
	block_t temp;
	int w, h;
	int* data = NULL;
	memcpy(&temp, b, sizeof(block_t));
	switch(dir)
	{
	case MOVE_LEFT:
		{
			temp.x--;
		}
		break;
	case MOVE_RIGHT:
		{
			temp.x++;
		}
		break;
	case MOVE_DOWN:
		{
			temp.y++;
		}
		break;
	}
	w = GetBlockWidth(&temp);
	h = GetBlockHeight(&temp);
	data = GetBlockData(&temp);
	if(temp.x >= 0 && (temp.x + w <= g_background_width) && temp.y >= 0 && (temp.y + h <= g_background_height))
	{
		// 检查是否和背景重叠
		int i, j;
		for(j = 0; j < h; j++)
		{
			for(i = 0; i < w; i++)
			{
				if (data[j * w + i] == 1 && g_background[temp.x + i + (temp.y + j) * g_background_width] == 1)
				{
					return 0;
				}
			}
		}
		b->x = temp.x;
		b->y = temp.y;
		return 1;
	}	
	return 0;
}

void MergeBlockTobackground(block_t*b)
{
	int i, j;
	int w = GetBlockWidth(b);
	int h = GetBlockHeight(b);
	int*data=GetBlockData(b);
	for(j = 0; j < h; j++)
	{
		for(i = 0; i < w; i++)
		{
			if(data[j*w+i] == 1)
			{
				g_background[(b->y + j) * g_background_width + (b->x + i)] = data[j*w+i];
 			}
		}
	}
	// 清除整行
	CleanWholeRows();
}

void CleanWholeRows()
{
	int i, j, k;
	for(j = g_background_height - 1; j >= 0; j--)
	{
		int whole_row = 1;
		for(i = 0; i < g_background_width; i++)
		{
			if(g_background[j * g_background_width +i] == 0)
			{
				whole_row = 0;
				break;
			}
		}
		if(whole_row == 1)
		{	
			for(k = j - 1; k >= 0; k--)
			{
				int* src = g_background + k * g_background_width;
				int* dst = g_background + (k + 1) * g_background_width;
				memcpy(dst, src, g_background_width*sizeof(int));
			}
			CleanWholeRows();
		}
	}
}

void DrawBlock(block_t* b)
{
	int i, j;
	int w = GetBlockWidth(b);
	int h = GetBlockHeight(b);
	int*data=GetBlockData(b);
	for(j = 0; j < h; j++)
	{
		for(i = 0; i < w; i++)
		{
			if (data[j*w+i]== 1)
			{
				blt(wnd_pixels, WND_WIDTH, WND_HEIGHT, block_bmp, (b->x + i) * block_bmp->width, (b->y + j) * block_bmp->height);
			}
		}
	}
}

void DrawBackground()
{
	int i, j;
	for(j = 0; j < g_background_height; j++)
	{
		for(i = 0; i < g_background_width; i++)
		{
			if (g_background[j*g_background_width+i]== 1)
			{
				blt(wnd_pixels, WND_WIDTH, WND_HEIGHT, block_bmp, i * block_bmp->width, j * block_bmp->height);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
	
void blt(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y)
{
	int src_left, src_right, src_top, src_bottom;
	int dst_left, dst_right, dst_top, dst_bottom;
	int clip_left, clip_right, clip_top, clip_bottom;
	int src_start_x, src_start_y, dst_start_x, dst_start_y;
	int width, height;
	int i, j;
	BYTE *src = NULL;
	WORD *dst = NULL;

	src_left = x;
	src_right = x + bitmap->width;
	src_top = y;
	src_bottom = y + bitmap->height;

	dst_left = 0;
	dst_right = wnd_w;
	dst_top = 0;
	dst_bottom = wnd_h;

	clip_left = (src_left > dst_left) ? src_left : dst_left;
	clip_right = (src_right < dst_right ) ? src_right : dst_right;
	clip_top = (src_top > dst_top ) ? src_top : dst_top;
	clip_bottom = (src_bottom < dst_bottom ) ? src_bottom : dst_bottom;

	src_start_x = clip_left - src_left;
	src_start_y = clip_top - src_top;
	dst_start_x = clip_left - dst_left;
	dst_start_y = clip_top - dst_top;
	width = clip_right - clip_left;
	height = clip_bottom - clip_top;
	if(width <= 0 || height <= 0) return;

	src = bitmap->pixels + (src_start_x + src_start_y * bitmap->width) * 4;     
	dst = wnd_pixels + (dst_start_x + dst_start_y * wnd_w);
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			unsigned char b = src[j * 4 + 0 + i * bitmap->width * 4];
			unsigned char g = src[j * 4 + 1 + i * bitmap->width * 4];
			unsigned char r = src[j * 4 + 2 + i * bitmap->width * 4];
			unsigned char a = src[j * 4 + 3 + i * bitmap->width * 4];		 
			dst[j + i * wnd_w] = ((r>>3)<<11) | ((g>>3)<<6) | (b>>3);
		}
	}
}

void blt_alpha(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y)
{
	int src_left, src_right, src_top, src_bottom;
	int dst_left, dst_right, dst_top, dst_bottom;
	int clip_left, clip_right, clip_top, clip_bottom;
	int src_start_x, src_start_y, dst_start_x, dst_start_y;
	int width, height;
	int i, j;
	BYTE *src = NULL;
	WORD *dst = NULL;

	src_left = x;
	src_right = x + bitmap->width;
	src_top = y;
	src_bottom = y + bitmap->height;

	dst_left = 0;
	dst_right = wnd_w;
	dst_top = 0;
	dst_bottom = wnd_h;

	clip_left = (src_left > dst_left) ? src_left : dst_left;
	clip_right = (src_right < dst_right ) ? src_right : dst_right;
	clip_top = (src_top > dst_top ) ? src_top : dst_top;
	clip_bottom = (src_bottom < dst_bottom ) ? src_bottom : dst_bottom;

	src_start_x = clip_left - src_left;
	src_start_y = clip_top - src_top;
	dst_start_x = clip_left - dst_left;
	dst_start_y = clip_top - dst_top;
	width = clip_right - clip_left;
	height = clip_bottom - clip_top;
	if(width <= 0 || height <= 0) return;

	src = bitmap->pixels + (src_start_x + src_start_y * bitmap->width) * 4;     
	dst = wnd_pixels + (dst_start_x + dst_start_y * wnd_w);	

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			BYTE a = src[j * 4 + 3 + i * bitmap->width * 4];
			if(a != 0 )
			{
				WORD color;
				BYTE rs, gs, bs, rd, gd, bd, rc, gc, bc;
				rs = src[j * 4 + 2 + i * bitmap->width * 4] >> 3;
				gs = src[j * 4 + 1 + i * bitmap->width * 4] >> 3;
				bs = src[j * 4 + 0 + i * bitmap->width * 4] >> 3;
				color = dst[j + i * wnd_w];
				rd = (color >> 11) & 0x1f;
				gd = (color >> 6) & 0x1f;
				bd = color & 0x1f;
				rc = (rs - rd) * a / 255 + rd;
				gc = (gs - gd) * a / 255 + gd;
				bc = (bs - bd) * a / 255 + bd;
				dst[j + i * wnd_w] = (rc<<11) | (gc<<6) | bc;
			}
		}
	}
}

void blt_mask(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y, unsigned short mask)
{
	int src_left, src_right, src_top, src_bottom;
	int dst_left, dst_right, dst_top, dst_bottom;
	int clip_left, clip_right, clip_top, clip_bottom;
	int src_start_x, src_start_y, dst_start_x, dst_start_y;
	int width, height;
	int i, j;
	BYTE *src = NULL;
	WORD *dst = NULL;

	src_left = x;
	src_right = x + bitmap->width;
	src_top = y;
	src_bottom = y + bitmap->height;

	dst_left = 0;
	dst_right = wnd_w;
	dst_top = 0;
	dst_bottom = wnd_h;

	clip_left = (src_left > dst_left) ? src_left : dst_left;
	clip_right = (src_right < dst_right ) ? src_right : dst_right;
	clip_top = (src_top > dst_top ) ? src_top : dst_top;
	clip_bottom = (src_bottom < dst_bottom ) ? src_bottom : dst_bottom;

	src_start_x = clip_left - src_left;
	src_start_y = clip_top - src_top;
	dst_start_x = clip_left - dst_left;
	dst_start_y = clip_top - dst_top;
	width = clip_right - clip_left;
	height = clip_bottom - clip_top;
	if(width <= 0 || height <= 0) return;

	src = bitmap->pixels + (src_start_x + src_start_y * bitmap->width) * 4;     
	dst = wnd_pixels + (dst_start_x + dst_start_y * wnd_w);
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			unsigned char b = src[j * 4 + 0 + i * bitmap->width * 4];
			unsigned char g = src[j * 4 + 1 + i * bitmap->width * 4];
			unsigned char r = src[j * 4 + 2 + i * bitmap->width * 4];
			unsigned char a = src[j * 4 + 3 + i * bitmap->width * 4];
			WORD color = ((r>>3)<<11) | ((g>>3)<<6) | (b>>3);
			if(color == mask) continue;
			dst[j + i * wnd_w] = color;
		}
	}
}

void blt_trans(WORD*wnd_pixels, int wnd_w, int wnd_h, bitmap_t* bitmap, int x, int y, unsigned char alpha)
{
	int src_left, src_right, src_top, src_bottom;
	int dst_left, dst_right, dst_top, dst_bottom;
	int clip_left, clip_right, clip_top, clip_bottom;
	int src_start_x, src_start_y, dst_start_x, dst_start_y;
	int width, height;
	int i, j;
	BYTE *src = NULL;
	WORD *dst = NULL;

	src_left = x;
	src_right = x + bitmap->width;
	src_top = y;
	src_bottom = y + bitmap->height;

	dst_left = 0;
	dst_right = wnd_w;
	dst_top = 0;
	dst_bottom = wnd_h;

	clip_left = (src_left > dst_left) ? src_left : dst_left;
	clip_right = (src_right < dst_right ) ? src_right : dst_right;
	clip_top = (src_top > dst_top ) ? src_top : dst_top;
	clip_bottom = (src_bottom < dst_bottom ) ? src_bottom : dst_bottom;

	src_start_x = clip_left - src_left;
	src_start_y = clip_top - src_top;
	dst_start_x = clip_left - dst_left;
	dst_start_y = clip_top - dst_top;
	width = clip_right - clip_left;
	height = clip_bottom - clip_top;
	if(width <= 0 || height <= 0) return;

	src = bitmap->pixels + (src_start_x + src_start_y * bitmap->width) * 4;     
	dst = wnd_pixels + (dst_start_x + dst_start_y * wnd_w);	

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			WORD color;
			BYTE rs, gs, bs, rd, gd, bd, rc, gc, bc;
			rs = src[j * 4 + 2 + i * bitmap->width * 4] >> 3;
			gs = src[j * 4 + 1 + i * bitmap->width * 4] >> 3;
			bs = src[j * 4 + 0 + i * bitmap->width * 4] >> 3;
			color = dst[j + i * wnd_w];
			rd = (color >> 11) & 0x1f;
			gd = (color >> 6) & 0x1f;
			bd = color & 0x1f;
			rc = (rs - rd) * alpha / 255 + rd;
			gc = (gs - gd) * alpha / 255 + gd;
			bc = (bs - bd) * alpha / 255 + bd;
			dst[j + i * wnd_w] = (rc<<11) | (gc<<6) | bc;
		}
	}
}

//////////////////////////////////////////////////////////////////////////