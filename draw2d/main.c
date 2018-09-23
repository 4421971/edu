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

bitmap_t* mouse_bmp = NULL;

int block_y = 0;
bitmap_t* block_bmp = NULL;

#define BG_BITMAP_NUM 4
bitmap_t* bg_bmps[BG_BITMAP_NUM];
int bg_cur = 0;
BYTE bg_alpha = 0;

#define WK_BITMAP_NUM 22
bitmap_t* wk_bmps[WK_BITMAP_NUM];
int wk_frame_cur = 0;

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
	wcex.lpszClassName	= "Draw2DWindow";
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
			Sleep(10);
		}
	}

	appExit();

	// unregister the window class
	UnregisterClass("Draw2DWindow", GetModuleHandle(NULL));

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
	hwnd = CreateWindowEx(ex_style, "Draw2DWindow", "Draw2DWindow",style, x,y, wnd_width,wnd_height,NULL,NULL,GetModuleHandle(NULL), NULL);
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
	int i;

	// 创建窗口，获得可以对窗口绘图的dc
	gWnd = createDraw2DWindow();
	if (gWnd == NULL) return;
	gDC = GetDC(gWnd);
	if (gDC == NULL) return;
	// 创建窗口的像素缓存，所有图片先绘制到这个缓存上，最后再贴在窗口上
	wnd_pixels =(WORD*)malloc(WND_WIDTH*WND_HEIGHT*sizeof(unsigned short));
	memset(wnd_pixels, 0xff, WND_WIDTH*WND_HEIGHT*sizeof(unsigned short));  // 16位	

	// 导入动画图片
	for(i = 0; i < BG_BITMAP_NUM; i++)
	{
		char bg_path[256];
		sprintf(bg_path, "bg%d.bmp", i);
		bg_bmps[i] = load_bmp(bg_path);
	}

	mouse_bmp = load_bmp("mm16.bmp");
	block_bmp = load_bmp("block.bmp");

	// 导入背景图片
	for(i = 0; i < WK_BITMAP_NUM; i++)
	{
		char wk_path[256];
		sprintf(wk_path, "wk/wk_%d.bmp", i);
		wk_bmps[i] = load_bmp(wk_path);
	}
}

/*
====================
appRun
====================
*/
static void appRun()
{
	BITMAPV4HEADER bmp_header;
	POINT pt;

	// 画背景	
	bg_alpha++;
	if(bg_alpha % 255 == 0) 
	{
		bg_cur++;
		bg_alpha = 0;
	}
	blt(wnd_pixels, WND_WIDTH, WND_HEIGHT, bg_bmps[bg_cur%BG_BITMAP_NUM], 0, 0);
	blt_trans(wnd_pixels, WND_WIDTH, WND_HEIGHT, bg_bmps[(bg_cur+1)%BG_BITMAP_NUM], 0, 0, bg_alpha);

	// 画砖块
	block_y += 32;
	blt(wnd_pixels, WND_WIDTH, WND_HEIGHT, block_bmp, 0, block_y % WND_HEIGHT);

	// 画动画
	blt_alpha(wnd_pixels, WND_WIDTH, WND_HEIGHT, wk_bmps[((wk_frame_cur++)/3)%WK_BITMAP_NUM], 0, 0);

	GetCursorPos(&pt);
	ScreenToClient(gWnd, &pt);
	// blt(wnd_pixels, wnd_w, wnd_h, mouse_bmp, 0, 0);
	// blt_alpha(wnd_pixels, wnd_w, wnd_h, mouse_bmp, pt.x - mouse_bmp->width/2, pt.y - mouse_bmp->height);
	blt_mask(wnd_pixels, WND_WIDTH, WND_HEIGHT, mouse_bmp, pt.x - mouse_bmp->width/2, pt.y - mouse_bmp->height, 0x1f);

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
	int i;
	for(i = 0; i < WK_BITMAP_NUM; i++)
	{
		if (wk_bmps[i])
		{
			if (wk_bmps[i]->pixels)
			{
				free(wk_bmps[i]->pixels);
			}
			free(wk_bmps[i]);
			wk_bmps[i] = NULL;
		}
	}

	for(i = 0; i < BG_BITMAP_NUM; i++)
	{
		if (bg_bmps[i])
		{
			if (bg_bmps[i]->pixels)
			{
				free(bg_bmps[i]->pixels);
			}
			free(bg_bmps[i]);
			bg_bmps[i] = NULL;
		}
	}

	if (block_bmp)
	{
		if (block_bmp->pixels)
		{
			free(block_bmp->pixels);
		}
		free(block_bmp);
		block_bmp = NULL;
	}

	if (mouse_bmp)
	{
		if (mouse_bmp->pixels)
		{
			free(mouse_bmp->pixels);
		}
		free(mouse_bmp);
		mouse_bmp = NULL;
	}

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