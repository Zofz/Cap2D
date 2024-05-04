// Resist2D.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Cap2D.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
char szTitle[MAX_LOADSTRING];					// The title bar text
char szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND hwndA, hwndStat;
const int StatHeight = 28;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ScaleDlg(HWND, UINT, WPARAM, LPARAM);


#define MAINWINDOWSTYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

CImage img;
CImage res;

volatile char ProgressStage;
volatile float ProgressDone;

int sqr(int x) { return x*x; }
double sqr(double x) { return x*x; }
float sqr(float x) { return x*x; }

struct idximageinfo {
	int sizx, sizy, imgsiz, nvar;
	int vidxt[];
};

struct varrectype {
	int x,y,l;
	double q;
};


varrectype * VarTab;
idximageinfo * ImgIdx;
float * VImage;
int nVar;
double LayerDist=2;       // in pixels
double PixelSize=0.1e+3; // in micro-meters
double TotalQ[2];
double Vtop;
const double Vbtm = -1.0;

double Escale, rScaleX, rScaleY, rScaleZ, rOffsetX, rOffsetY;

bool Antialiasing;
bool AllowVertMag;
bool FixedPotential;
POINT Efield[4];
#define PI 3.1415926535897932384626433832795
#define ε0 (8.854187817e-12)


idximageinfo * IndexVariables(void);
unsigned __stdcall Calculate2DCap(void *);
unsigned __stdcall CalculateResultV(void * VoidCutPt);
void DrawResultImage(char mode);
void PointInfo(HWND hwnd, int x, int y);
void CalcIDensityGraph(char AllTopBtm);
void ProgressInfo(HWND hwnd);
HANDLE ExtractVariablesAndStartSolver(bool ReloadOnly, POINT CutPt[2]);

bool LoadImageToTwoBuffers(HWND hwnd)
{
	char str[MAX_PATH];
	str[0]=0;
    OPENFILENAME   ofn;
    ZeroMemory( &ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = hInst;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Portable Network Graphics (*.PNG)\0*.png\0Graphics Interchange Format (*.GIF)\0*.gif\0 bitmap image (*.BMP)\0*.bmp\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = str;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open 2D Resistance Picture";
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    if(GetOpenFileName( &ofn )==0) return false;
	CImage src;
	src.Load(str);
	if(src.IsNull()) return false;
	img.Destroy();
	res.Destroy();
	img.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(img.GetDC(), 0,0);
	img.ReleaseDC();
	res.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(res.GetDC(), 0,0);
	res.ReleaseDC();
	return true;
}

bool LoadResourceImageToTwoBuffers(HINSTANCE hInstance, UINT resID)
{
	CImage src;
	src.LoadFromResource(hInstance, resID);
	if(src.IsNull()) return false;
	img.Destroy();
	res.Destroy();
	img.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(img.GetDC(), 0,0);
	img.ReleaseDC();
	res.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(res.GetDC(), 0,0);
	res.ReleaseDC();
	return true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CAP2D, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CAP2D));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CAP2D));
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CAP2D);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

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
	InitCommonControls();
   

	hInst = hInstance; // Store instance handle in our global variable

	hwndA = CreateWindow(szWindowClass, szTitle, MAINWINDOWSTYLE,
		CW_USEDEFAULT, 0, 320, 150, NULL, NULL, hInstance, NULL);

	if(!hwndA)
	{
		return FALSE;
	}
	hwndStat = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | ES_READONLY, 0, 10, 320, 40, hwndA, NULL, hInstance, NULL);

	ShowWindow(hwndA, nCmdShow);
	UpdateWindow(hwndA);
	SetTimer(hwndA, 1, 500, NULL);
#ifndef _DEBUG
	if(LoadResourceImageToTwoBuffers(hInstance, IDB_EXAMPLE1)) PostMessage(hwndA, WM_APP+3, 0, 0);
#endif
	return TRUE;
}

void MemoryErrorMsg(HWND hWnd, unsigned nvar)
{
	char str[256];
	sprintf_s(str, "Not enough memory for %ux%u matrix. (%.2fGB)\r\n"
		"Please reduce conductor surface..", nvar, nvar, (1.0/1024/1024/1024)*nvar*nvar*sizeof(double));
	MessageBox(hWnd, str, "ERROR", MB_ICONERROR);
}

void NewAllTopBtm(char AllTopBtm)
{
	if(VarTab) {
		CalcIDensityGraph(AllTopBtm);
		RECT rect = {0, 0, img.GetWidth(), img.GetHeight()};
		RedrawWindow(hwndA, &rect , NULL, RDW_NOERASE|RDW_INVALIDATE);
	}
	CheckMenuRadioItem(GetMenu(hwndA), IDM_VIEW_BOTHLAYERS, IDM_VIEW_ONLYBTM, IDM_VIEW_BOTHLAYERS+AllTopBtm, MF_BYCOMMAND);
}

void NewResDispMode(char ResDispMode)
{
	if(!img.IsNull()) {
		DrawResultImage(ResDispMode);
		RECT rect = {img.GetWidth(), 0, img.GetWidth()*2, img.GetHeight()};
		RedrawWindow(hwndA, &rect , NULL, RDW_NOERASE|RDW_INVALIDATE);
	}
	CheckMenuRadioItem(GetMenu(hwndA), IDM_VIEW_BW, IDM_VIEW_14STEPS, IDM_VIEW_BW + (
		(ResDispMode&1)? ((ResDispMode+1)>>1) : 0), MF_BYCOMMAND);
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
	static int DragIdx=-1;
	static POINT dpo;
	static POINT CutPt[2];
	static char AllTopBtm=0;
	static bool ActualizeCutPt;
	static char ResDispMode;
	static HANDLE thread;

	POINT dp;
	int wmId, wmEvent,i, mindist;
	float lcdi;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;


	switch (message)
	{
	case WM_CREATE:
		CheckMenuRadioItem(GetMenu(hWnd), IDM_VIEW_BOTHLAYERS, IDM_VIEW_ONLYBTM, IDM_VIEW_BOTHLAYERS, MF_BYCOMMAND);
		CheckMenuRadioItem(GetMenu(hWnd), IDM_VIEW_BW, IDM_VIEW_14STEPS, IDM_VIEW_BW, MF_BYCOMMAND);
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch(wmId)
		{
		case IDM_FILE_LOAD:
			if(thread) {
				MessageBox(hWnd, "Not Ready, try again later.", szWindowClass, MB_ICONHAND);
				break;
			}
			if(LoadImageToTwoBuffers(hWnd)) thread = ExtractVariablesAndStartSolver(false, CutPt);
			break;

		case IDM_PARAMS:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PARAMS), hWnd, ScaleDlg);
			break;

		case IDM_VIEW_BOTHLAYERS:
		case IDM_VIEW_ONLYTOP:
		case IDM_VIEW_ONLYBTM:
			NewAllTopBtm(AllTopBtm = wmId-IDM_VIEW_BOTHLAYERS);
			break; 

		case IDM_VIEW_BW:
			NewResDispMode(ResDispMode = ResDispMode & ~1);
			break;
		case IDM_VIEW_2STEPS:
		case IDM_VIEW_6STEPS:
		case IDM_VIEW_10STEPS:
		case IDM_VIEW_14STEPS:
			NewResDispMode(ResDispMode = ((wmId-IDM_VIEW_2STEPS)<<1)|1);
			break; 

		case IDM_VIEW_ALLOWVERTICALMAG:
			AllowVertMag = !AllowVertMag;
			CheckMenuItem(GetMenu(hWnd), IDM_VIEW_ALLOWVERTICALMAG, AllowVertMag? MF_CHECKED : MF_UNCHECKED);
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
		break;

	case WM_MOUSEMOVE:
		dp.x = short(LOWORD(lParam));
		dp.y = short(HIWORD(lParam));
		PointInfo(hwndStat, dp.x, dp.y);
		if(DragIdx<0) break;
		if(DragIdx<2) {
			if(dp.x<0 || dp.y<0) break;
			if(dp.x>=img.GetWidth() || dp.y>=img.GetHeight()) break;
			CutPt[DragIdx] = dp;
		} else {
			dp.x -= dpo.x;
			dp.y -= dpo.y;
			for(i=0; i<2; i++) {
				if(CutPt[i].x+dp.x<0 || CutPt[i].y+dp.y<0 || CutPt[i].x+dp.x>=img.GetWidth() || CutPt[i].y+dp.y>=img.GetHeight()) break;
			}
			if(i<2) break;
			for(i=0; i<2; i++) {
				CutPt[i].x += dp.x;
				CutPt[i].y += dp.y;
			}
			dpo.x += dp.x;
			dpo.y += dp.y;
		}
		rect.left=rect.top = 0;
		rect.bottom=img.GetHeight();
		rect.right=img.GetWidth();
		RedrawWindow(hWnd, &rect , NULL, RDW_NOERASE|RDW_INVALIDATE);
		if(res.IsNull()) break;
		if(thread) ActualizeCutPt = true; else 
		{
			ActualizeCutPt = false;
			thread = (HANDLE)_beginthreadex(NULL, 0x10000, CalculateResultV, &CutPt, 0, NULL);
		}
		break;

	case WM_APP+5:
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread=NULL;
		free(VImage);
		VImage = (float*)lParam;
		NewResDispMode(ResDispMode=0);
		InvalidateRect(hWnd, NULL, false);
		if(ActualizeCutPt)
		{
			ActualizeCutPt = false;
			if(!res.IsNull()) thread = (HANDLE)_beginthreadex(NULL, 0x10000, CalculateResultV, &CutPt, 0, NULL);
		}
		break;

	case WM_LBUTTONDOWN:
		dp.x = short(LOWORD(lParam));
		dp.y = short(HIWORD(lParam));
		mindist=100;
		DragIdx=-1;
		lcdi=0;
		for(i=0; i<2; i++)
		{
			int d = sqr(CutPt[i].x-dp.x) + sqr(CutPt[i].y-dp.y);
			if(d<mindist) { mindist=d; DragIdx=i; }
			lcdi += sqrt((float)d);
		}
		if(DragIdx<0) {
			if(abs(lcdi-sqrt(float(sqr(CutPt[0].x-CutPt[1].x) + sqr(CutPt[0].y-CutPt[1].y))))<2) DragIdx = 2;
		}
		if(DragIdx<0) break;
		dpo = dp;
		SetCapture(hWnd);
		break;

	case WM_LBUTTONUP:
		if(DragIdx<0) break;
		ReleaseCapture();
		DragIdx=-1;
		break;

	case WM_RBUTTONUP:
		if(GET_X_LPARAM(lParam)<img.GetWidth())
		{
			NewAllTopBtm(AllTopBtm = (AllTopBtm+1)%3);
		} else {
			NewResDispMode(ResDispMode = (ResDispMode+1)&7);
		}
		break;

	case WM_APP+3:
		if(thread) return false;
		thread = ExtractVariablesAndStartSolver(wParam&1, CutPt);
		return true;

	case WM_APP+2:
		MemoryErrorMsg(hWnd, (unsigned)wParam);
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread=NULL;
		break;

	case WM_APP+1:
		nVar = (int)wParam;
		VarTab = (varrectype*)lParam;
		AllTopBtm=0;
		CalcIDensityGraph(AllTopBtm);
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
		thread=NULL;
		PointInfo(hwndStat, -1, -1);
		ActualizeCutPt = false;
		thread = (HANDLE)_beginthreadex(NULL, 0x10000, CalculateResultV, &CutPt, 0, NULL);
		rect.left=rect.top = 0;
		rect.bottom=img.GetHeight();
		rect.right=img.GetWidth();
		RedrawWindow(hWnd, &rect , NULL, RDW_NOERASE|RDW_INVALIDATE);
		break;
		
	case WM_TIMER:
		if(VarTab==0) ProgressInfo(hwndStat);
		break;

	case WM_SETCURSOR: 
		if(LOWORD(lParam)!=HTCLIENT) return DefWindowProc(hWnd, message, wParam, lParam);
		if(HWND(wParam)==hWnd)
		{
			SetCursor(LoadCursor(NULL, IDC_CROSS));
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.dwHoverTime = 0;
			tme.hwndTrack = hWnd;
			_TrackMouseEvent(&tme);
			return 1;
		}
		return 0;

	case WM_MOUSELEAVE:
		if(Efield[0].x) { RedrawWindow(hwndA, NULL , NULL, RDW_NOERASE|RDW_INVALIDATE); Efield[0].x=0; }
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if(!img.IsNull()) img.Draw(hdc, 0, 0);
		SelectObject(hdc, GetStockObject(WHITE_PEN));
		MoveToEx(ps.hdc, CutPt[0].x, CutPt[0].y, NULL);
		LineTo(hdc, CutPt[1].x, CutPt[1].y);
		SelectObject(hdc, GetStockObject(GRAY_BRUSH));
		Ellipse(hdc, CutPt[0].x-3, CutPt[0].y-3, CutPt[0].x+4, CutPt[0].y+4);
		SelectObject(hdc, GetStockObject(LTGRAY_BRUSH));
		Ellipse(hdc, CutPt[1].x-3, CutPt[1].y-3, CutPt[1].x+4, CutPt[1].y+4);
		if(!res.IsNull()) res.Draw(hdc, img.GetWidth(), 0);
		if(Efield[0].x) {
			Polyline(hdc, Efield, 2);
			Polygon(hdc, Efield+1, 3);
		}
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		if(thread) {
			TerminateThread(thread, 0);
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
			thread=NULL;
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
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

// Message handler for parameters box.
INT_PTR CALLBACK ScaleDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	char str[256];
	double d1, d2;
	switch (message)
	{
	case WM_INITDIALOG:
		sprintf_s(str, "%g", PixelSize/1000);
		SetDlgItemText(hDlg, IDC_EDIT1, str);
		sprintf_s(str, "%g", PixelSize*LayerDist/1000);
		SetDlgItemText(hDlg, IDC_EDIT2, str);
		CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1+FixedPotential);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			GetDlgItemText(hDlg, IDC_EDIT1, str, sizeof(str));
			d1 = atof(str);
			if(d1<=0) { SetFocus(GetDlgItem(hDlg, IDC_EDIT1)); break; }
			GetDlgItemText(hDlg, IDC_EDIT2, str, sizeof(str));
			d2 = atof(str);
			if(d2<=0) { SetFocus(GetDlgItem(hDlg, IDC_EDIT2)); break; }
			PixelSize = d1*1000; // no recalculate is needed
			d2 = d2 / d1;
			if(abs((LayerDist-d2)/d2)>1.0e-6) {
				LayerDist = d2;
				if(ImgIdx==NULL || !SendMessage(hwndA, WM_APP+3, 1, 0)) {
					MessageBox(hDlg, "Not Ready, try again later.", szWindowClass, MB_ICONHAND);
					break;
				}
			}
			FixedPotential = BST_CHECKED==Button_GetCheck(GetDlgItem(hDlg, IDC_RADIO2));
			// no break
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// all coefficients do not contain 1/(4*pi*ε0) coefficient
// ε0 = 8.854 187 817... × 10−12 F/m (farads per meter).
// here we should also correct for meters/pixel

double TheSamePixelVcoef(void) 
{
	return 4*log(sqrt(2.0) + 1);
}

double CoplanarPixelVcoef(int nx, int ny) // one parameter must be nonzero
{
	double x0 = nx-0.5;
	double y0 = ny-0.5;
	double x1 = nx+0.5;
	double y1 = ny+0.5;
	double x02 = x0*x0;
	double y02 = y0*y0;
	double x12 = x1*x1;
	double y12 = y1*y1;
	double dist00 = sqrt(x02 + y02);
	double dist10 = sqrt(x12 + y02);
	double dist01 = sqrt(x02 + y12);
	double dist11 = sqrt(x12 + y12);
	return	  x0*log((dist00 + y0)*(dist01 - y1)/x02) 
			- y0*log((dist00 - x0)*(dist10 + x1)/y02) 
			- x1*log((dist10 + y0)*(dist11 - y1)/x12)
			+ y1*log((dist01 - x0)*(dist11 + x1)/y12);
}

inline double integral3d(double d, double x0, double y)
{
	double x1 = x0+1;
	double d2 = d*d;
	double y2 = y*y;
	double x02 = x0*x0;
	double x12 = x1*x1;
	double dist1 = sqrt(d2+x12+y2);
	double dist2 = sqrt(d2+x02+y2);
	return   y*log((dist1+x1)/(dist2+x0))
			-x0*log(dist2+y)
			+d*atan((x0*y)/(d*dist2))
			+x1*log(dist1+y)
			-d*atan((x1*y)/(d*dist1));
}

double ExtraplanarPixelVcoef(double d, double nx, double ny)
{
	return integral3d(d, nx-0.5, ny+0.5) - integral3d(d, nx-0.5, ny-0.5);
}


HANDLE ExtractVariablesAndStartSolver(bool ReloadOnly, POINT CutPt[2])
{
	ProgressStage = 0;
	nVar = 0;
	free(VarTab);
	free(VImage);
	VarTab = NULL;
	VImage = NULL;
	if(!ReloadOnly) {
		free(ImgIdx);
		ImgIdx = IndexVariables();
		RECT rect = {0, 0, 2*img.GetWidth(), img.GetHeight()+StatHeight};
		AdjustWindowRect(&rect, MAINWINDOWSTYLE, true);
		SetWindowPos(hwndA, NULL, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE | SWP_NOZORDER);
		MoveWindow(hwndStat, 0, img.GetHeight(), rect.right-rect.left, StatHeight, false);
		CutPt[0].y = CutPt[1].y = img.GetHeight()/2;
		CutPt[0].x = 2*img.GetWidth()/6;
		CutPt[1].x = 4*img.GetWidth()/6;
		RedrawWindow(hwndA, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
	}
	if(ImgIdx==NULL) return NULL;
	return (HANDLE)_beginthreadex(NULL, 0x20000, Calculate2DCap, hwndA, 0, NULL);
}



void ScalarMulAndSubst(double a, double * src, double * dst, int i, int nvar)
{
	for(i=i; i<=nvar; i++) dst[i] -= a*src[i];
}

const int Isolator      = -1;


int xprintf(const char *format,... )
{
	va_list argptr;
	va_start( argptr, format );
	char str[2048];
	int j = vsprintf_s(str, format, argptr);
	OutputDebugStringA(str);
	return j;
}


idximageinfo * IndexVariables(void)
{
	int x,y, sizx=img.GetWidth(), sizy=img.GetHeight();
	int imgsiz = sizx*sizy;
	idximageinfo * ii = (idximageinfo*)malloc(sizx*sizy*(2*sizeof(int)+sizeof(WORD))+sizeof(idximageinfo));
	int* itop = &ii->vidxt[0];
	int* ibtm = &ii->vidxt[imgsiz];
	WORD * lumtab = (WORD*)&ii->vidxt[2*imgsiz];
	int nvar=0, addr;
	for(addr=y=0; y<sizy; y++)
		for(x=0; x<sizx; x++, addr++)
		{
			COLORREF pix = img.GetPixel(x, y);
			BYTE r = GetRValue(pix);
			BYTE g = GetGValue(pix);
			BYTE b = GetBValue(pix);
			lumtab[addr] = (g<<8) | r;
			if(r>25) itop[addr] = nvar++;  // this point is RED = top layer
			else itop[addr] = Isolator;    // voltage in this point is not relevant, infinite resistance
			if(g>25) ibtm[addr] = nvar++;  // this point is BLUE = bottom layer
			else ibtm[addr] = Isolator;    // voltage in this point is not relevant, infinite resistance
		}
	// we have nvar unknown voltages (variables)
	ii->sizx = sizx;
	ii->sizy = sizy;
	ii->nvar = nvar;
	ii->imgsiz = imgsiz;
	if(nvar>0) return ii; 
	free(ii);
	return NULL;
}


unsigned __stdcall Calculate2DCap(void * par)
{
	bool fixedCharge = !FixedPotential;
	ProgressStage=1;
	ProgressDone=0;
	Vtop = 1.0;
	int addr, x,y,l,j, nQvar = ImgIdx->nvar, nvar = ImgIdx->nvar+fixedCharge;
	varrectype * vartab = (varrectype*)malloc(nvar*sizeof(varrectype));
	for(addr=l=0; l<2; l++)
	{
		for(y=0; y<ImgIdx->sizy; y++)
		{
			for(x=0; x<ImgIdx->sizx; x++, addr++)
			{
				j = ImgIdx->vidxt[addr];
				if(j!=Isolator)
				{
					vartab[j].x = x;
					vartab[j].y = y;
					vartab[j].l = l;
				}
			}
		}
	}
	// we build a matrix nvar lines, nvar+1 columns, representing the equation system to solve
	// every line represents a potential V on a single pixel = -0.5 (top) or +0.5 (bottom)
	// as the sum of potentials contributions generated by all other (and self) pixels.
	// there are 3 types of contributions: self, coplanar, extraplanar (the other plane)
	// if the pixel is more far than e.g. 10 then a simplified 1/r may be used.

	int i,k;
	double * row;
	double ** matrix = (double**)malloc(nvar*sizeof(double*));
	for(j=0; j<nvar; j++)
	{
		row = (double*)malloc((nvar+1)*sizeof(double));
		if(row==NULL) {
			while(j>0) free(matrix[--j]);
			free(matrix);
			free(vartab);
			PostMessage((HWND)par, WM_APP+2, nvar, 0);
			_heapmin();
			return 0;
		}
		matrix[j] = row;
	}
	for(j=0; j<nQvar; j++)
	{
		ProgressDone = float(j)/nvar;
		row = matrix[j];
		for(i=0; i<nQvar; i++)
			if(vartab[i].l!=vartab[j].l) 
				row[i] = ExtraplanarPixelVcoef(LayerDist, vartab[i].x-vartab[j].x, vartab[i].y-vartab[j].y);
			else if(i!=j)
				row[i] = CoplanarPixelVcoef(vartab[i].x-vartab[j].x, vartab[i].y-vartab[j].y);
			else 
				row[i] = TheSamePixelVcoef();
		if(fixedCharge) {
			row[nQvar] = vartab[j].l-1;
			row[nvar ] = 0-vartab[j].l;
		} else
			row[nvar] = 1.0-2*vartab[j].l;
	}
	if(fixedCharge) // eqation for: Sum of all Q is zero 
	{
		row = matrix[nQvar];
		for(i=0; i<nQvar; i++) row[i] = 1;
		row[nQvar] = 0;
		row[nvar ] = 0;
	}

	// now we need to solve the eq-system, a sparse system, at the moment reasonably pivoted
	// we do no tricks or fast algorithms
	// Gaussian Elimination
	ProgressStage=2;
	double * tmp;
	for(l=0; l<nvar; l++)
	{
		ProgressDone = float(l)/nvar;
		double d = abs(matrix[l][l]);
		for(j=l+1; j<nvar; j++)
			if(abs(matrix[j][l]) > d) 
			{
				tmp = matrix[j];
				matrix[j] = matrix[l];
				matrix[l] = tmp;
				d = abs(matrix[l][l]);
			}
		
		for(j=l+1; j<nvar; j++)
		{
			if(matrix[j][l]) 
			{
				ScalarMulAndSubst(
					matrix[j][l] / matrix[l][l], 
					matrix[l], 
					matrix[j], 
					l+1, nvar);
				matrix[j][l] = 0;
			} 
		}	
	}
	// backward substitution
	ProgressStage=3;
	k=l=nvar-1;

	while(l>=0 && k>=0)
	{
		ProgressDone = float(nvar-l)/nvar;
		double d;
		if(matrix[l][k]!=0)
		{
			d = matrix[l][nvar] / matrix[l][k];
			for(j=l-1; j>=0; j--) 
			{
				matrix[j][nvar] -= matrix[j][k]*d;
				matrix[j][k] = 0;
			}
			l--;
		} else {
			d = 0; // unknown
			for(j=l; j>=0; j--) 
			{
				matrix[j][nvar] -= matrix[j][k]*0;
				matrix[j][k] = 0;
			}
		}
		if(k<nQvar)	vartab[k].q = d; else Vtop = d;
		k--;
	}
	if(k==nQvar) { Vtop = 0; k--; }
	while(k>=0) vartab[k--].q = 0; // unknown
	for(j=0; j<nvar; j++) {
		free(matrix[j]);
	}
	free(matrix);
	PostMessage((HWND)par, WM_APP+1, nQvar, LPARAM(vartab));
	_heapmin();

	// total charge on top and bottom layer
	double q[2] = {0,0};
	for(i=0; i<nQvar; i++) 
	{
		q[vartab[i].l] += vartab[i].q;
	}
	TotalQ[0] = q[0];
	TotalQ[1] = q[1];
	ProgressStage=4;
	return 1;
}

inline bool GetTopLayer(int x, int y)
{
	return ImgIdx->vidxt[x + y*ImgIdx->sizy] != Isolator;
}

inline bool GetBtmLayer(int x, int y)
{
	return ImgIdx->vidxt[x + y*ImgIdx->sizy + ImgIdx->imgsiz] != Isolator;
}


unsigned __stdcall CalculateResultV(void * VoidCutPt)
{
	POINT * CutPt = (POINT*) VoidCutPt;
	double CutSizX = CutPt[1].x-CutPt[0].x;
	double CutSizY = CutPt[1].y-CutPt[0].y;
	double CutOfsX = (CutPt[1].x+CutPt[0].x)*0.5;
	double CutOfsY = (CutPt[1].y+CutPt[0].y)*0.5;

	double CutPtDist = sqrt(sqr(CutSizX) + sqr(CutSizY));
	double Mag = CutPtDist / (ImgIdx->sizx*0.95);
	if( Mag*(ImgIdx->sizy*0.95) < LayerDist ) Mag = LayerDist / (ImgIdx->sizy*0.95);

	int hsizy = ImgIdx->sizy/2;
	int hdd = int(floor(LayerDist/Mag*0.5+0.5));
	int topy = hsizy + hdd;
	int btmy = hsizy - hdd;
	int yofs = ImgIdx->sizx-1;
	double magd = Mag / CutPtDist;
	double magX = CutSizX*magd;
	double magY = CutSizY*magd;

	rScaleX = magX;
	rScaleY = magY;
	rScaleZ = Mag;
	rOffsetX = CutOfsX;
	rOffsetY = CutOfsY;

	const float almost_0=1.0e-38f, almost_1=0.99999995f;

	float * Vimg = (float*)malloc(ImgIdx->imgsiz*sizeof(float));

	int x,y,i;
	for(x=0; x<ImgIdx->sizx; x++)
	{
		double t = x - ImgIdx->sizx*0.5;
		double sx = CutOfsX + magX*t;
		double sy = CutOfsY + magY*t;

		int sx0    = int(floor(sx));
		double sxr = sx-sx0;
		int sy0    = int(floor(sy));
		double syr = sy-sy0;

		double topa = GetTopLayer(sx0, sy0)*(1-sxr)*(1-syr) + GetTopLayer(sx0+1, sy0)*sxr*(1-syr) + GetTopLayer(sx0, sy0+1)*(1-sxr)*syr + GetTopLayer(sx0+1, sy0+1)*sxr*syr;
		double btma = GetBtmLayer(sx0, sy0)*(1-sxr)*(1-syr) + GetBtmLayer(sx0+1, sy0)*sxr*(1-syr) + GetBtmLayer(sx0, sy0+1)*(1-sxr)*syr + GetBtmLayer(sx0+1, sy0+1)*sxr*syr;
		for(y=0; y<ImgIdx->sizy; y++)
		{
			if(y==topy && topa>0.5) 
				Vimg[x + (yofs-y)*ImgIdx->sizy]=1.0f;
			else if(y==btmy && btma>0.5) 
				Vimg[x + (yofs-y)*ImgIdx->sizy]=0.0f;
			else {
				double sz = (y-hsizy)*Mag;
				double V=0;
				for(i=0; i<nVar; i++)
				{
					double dz = (0.5-VarTab[i].l)*LayerDist - sz;
					double dx = VarTab[i].x-sx;
					double dy = VarTab[i].y-sy;
					double r2 = sqr(dx)+sqr(dy)+sqr(dz);
					if(r2>20*20) V += VarTab[i].q / sqrt(r2); else V += VarTab[i].q * ExtraplanarPixelVcoef(dz, dx, dy);
				}
				V = (V-Vbtm)/(Vtop-Vbtm);
				float v = float(V);
				if(v>=1.0f) 
					v=almost_1;
				else if(v<=0) 
					v=almost_0;
				Vimg[x + (yofs-y)*ImgIdx->sizy] = v;
			}
		}
	}
	Escale = 1/(PixelSize*1.0e-6*Mag);
	PostMessage(hwndA, WM_APP+5, 0, (LPARAM)Vimg);
	return 0;
}


void DrawResultImage(char mode)
{
	if(res.IsNull() || VImage==NULL) return;
	int x,y,a;
	for(a=y=0; y<ImgIdx->sizy; y++)
	{
		for(x=0; x<ImgIdx->sizx; x++, a++)
		{
			DWORD col;
			float V = VImage[a];
			if(V==1.0f) col = 0x0000FF; 
			else if(V==0) col = 0x00FF00;
			else {
				if((mode&1)==0) {
					col = (BYTE(floor(V*255))<<16) | (BYTE(floor(V*255))<<8) | BYTE(floor(V*255));
				} else {
					float u;
					float sql = (float)modf(V*(mode&7), &u);
					sql = sql<0.125f ? (sql*16-1) : sql<0.5f ? 1 : sql<0.625f ? 1-(sql-0.5f)*16 : -1;
					float b = 127+63*(float)sin(V*17)+50*sql;
					float r = 127+53*(float)sin(V*9)+60*sql;
					float g = 127+63*(float)sin(V*11)+40*sql;
					col = (BYTE(floor(b))<<16) | (BYTE(floor(g))<<8) | BYTE(floor(r));
				}
			}
			res.SetPixel(x, y, col);
		}
	}
}



struct printf_eng {
	int dp;
	int prefix;
	double val;
};


void EngDisp(double val, printf_eng &ret) // for %.*f%c
{
	ret.val = val;
	val = abs(val);
	if(val>=1.0e+12)  { ret.val /= 1.0e+12; ret.prefix='T';  } else 
	if(val>=1.0e+9 )  { ret.val /= 1.0e+9;  ret.prefix='G';  } else 
	if(val>=1.0e+6 )  { ret.val /= 1.0e+6;  ret.prefix='M';  } else
	if(val>=1.0e+3 )  { ret.val /= 1.0e+3;  ret.prefix='k';  } else
	if(val>=1.0    )  {                     ret.prefix=' ';  } else
	if(val>=1.0e-3 )  { ret.val /= 1.0e-3;  ret.prefix='m';  } else
	if(val>=1.0e-6 )  { ret.val /= 1.0e-6;  ret.prefix=0xB5; } else
	if(val>=1.0e-9 )  { ret.val /= 1.0e-9;  ret.prefix='n';  } else
	if(val>=1.0e-12)  { ret.val /= 1.0e-12; ret.prefix='p';  } else
	if(val>=1.0e-15)  { ret.val /= 1.0e-15; ret.prefix='f';  } else
	if(val>0       )  { ret.val /= 1.0e-18; ret.prefix='a';  } else
	                  {                     ret.prefix=' ';  } 
	val=abs(ret.val); // if might be more clear than log10
	if(val>=100  ) ret.dp = 0; else 
	if(val>=10   ) ret.dp = 1; else  
	if(val>=1    ) ret.dp = 2; else 
	if(val>=0.1  ) ret.dp = 3; else 
	if(val>=0.01 ) ret.dp = 4; else 
	if(val>=0.001) ret.dp = 5; else 
	if(val>0     ) ret.dp = 6; else 
				   ret.dp = 0;
}



void PointInfo(HWND hwnd, int x, int y)
{
	printf_eng fmd;
	int j=0;
	char str[256];
	str[0]=0;
	if(VarTab==NULL || ImgIdx==NULL) return;
	if(ImgIdx && x>=0 && x<2*ImgIdx->sizx && y>=0 && y<ImgIdx->sizy)
	{
		if(x >= ImgIdx->sizx) {
			x -= ImgIdx->sizx;
			if(VImage)
			{
				float V = VImage[x + y*ImgIdx->sizy];
				j = sprintf_s(str, "V: %.3fVolt", (V-0.5)*2);

				if(x<1) x=1; else if(x>ImgIdx->sizx-2) x=ImgIdx->sizx-2;
				if(y<1) y=1; else if(y>ImgIdx->sizy-2) y=ImgIdx->sizy-2;

				if(V!=0.0f && V!=1.0f)
				{
					double dx = VImage[x-1 + y*ImgIdx->sizy] - VImage[x+1 + y*ImgIdx->sizy];
					double dy = VImage[x + (y-1)*ImgIdx->sizy] - VImage[x + (y+1)*ImgIdx->sizy];
					double el = sqrt(sqr(dx)+sqr(dy));
					double E = el*Escale;
					double dBe = log(E);
					dx *= ((30.0f+dBe)/el);
					dy *= ((30.0f+dBe)/el);
					Efield[0].x = ImgIdx->sizx+x;
					Efield[0].y = y;
					Efield[1].x = Efield[0].x + int(floor(0.5 + dx));
					Efield[1].y = Efield[0].y + int(floor(0.5 + dy));
					dx*=-0.2f;
					dy*=-0.2f;
					Efield[2].x = Efield[1].x + int(floor(0.5 + 0.8660254037*dx - 0.5*dy));
					Efield[2].y = Efield[1].y + int(floor(0.5 + 0.5*dx + 0.8660254037*dy));
					Efield[3].x = Efield[1].x + int(floor(0.5 + 0.8660254037*dx + 0.5*dy)); 
					Efield[3].y = Efield[1].y + int(floor(0.5 + 0.8660254033*dy - 0.5*dx));
					EngDisp(E, fmd);
					j += sprintf_s(str+j, sizeof(str)-j, " E: %.*f%cV/m", fmd.dp, fmd.val, fmd.prefix);
				} else 
					Efield[0].x=0;
				
				RECT rect = {ImgIdx->sizx, 0, 2*ImgIdx->sizx, ImgIdx->sizy};
				RedrawWindow(hwndA, &rect , NULL, RDW_NOERASE|RDW_INVALIDATE);
				j += sprintf_s(str+j, sizeof(str)-j, " at (%.1f,%.1f,%.1f)", rOffsetX+rScaleX*(x-ImgIdx->sizx*0.5), rOffsetY+rScaleY*(x-ImgIdx->sizx*0.5), rScaleZ*(y-ImgIdx->sizy*0.5));
			}
		} else {
			if(Efield[0].x) { RedrawWindow(hwndA, NULL , NULL, RDW_NOERASE|RDW_INVALIDATE); Efield[0].x=0; }
			int i0 = ImgIdx->vidxt[x + y*ImgIdx->sizy];
			int i1 = ImgIdx->vidxt[x + y*ImgIdx->sizy + ImgIdx->imgsiz];
			if(i0!=Isolator || i1!=Isolator)
			{
				if(i0!=Isolator) j = sprintf_s(str, "Qtop=%.4f ", VarTab[i0].q);
				if(i1!=Isolator) j += sprintf_s(str+j, sizeof(str)-j, "Qbtm=%.4f ", VarTab[i1].q);
				j += sprintf_s(str+j, sizeof(str)-j, "at (%i,%i)", x, y);
			}
		}
	} 
	// coefficients in matrix do not contain 1/(4*pi*ε0) coefficient
	// ε0 = 8.854187817... × 10−12 F/m (farads per meter).

	double coe = (4*PI*ε0/1.0e6)*PixelSize;

	double Q = (TotalQ[0]-TotalQ[1])*0.5; 
	double C = Q*coe / (Vtop-Vbtm);

	EngDisp(C, fmd);
	if(str[0]==0) sprintf_s(str, "Capacitance: %.*f%cF", fmd.dp, fmd.val, fmd.prefix); 
	SetWindowText(hwnd, str);
}


void CalcIDensityGraph(char AllTopBtm)
{
	double maxq[2] = { 0, 0 };
	int i, i0,i1;
	for(i=0; i<nVar; i++)
		if(VarTab[i].l)
		{
			if(VarTab[i].q < maxq[1]) maxq[1] = VarTab[i].q;
		} else {
			if(VarTab[i].q > maxq[0]) maxq[0] = VarTab[i].q;
		}
	int x,y,a0,a1;
	a0 = 0;
	a1 = ImgIdx->imgsiz;
	WORD * lumtab = (WORD*)&ImgIdx->vidxt[2*a1];
	switch(AllTopBtm) 
	{
	case 1:
		for(y=0; y<ImgIdx->sizy; y++)
			for(x=0; x<ImgIdx->sizx; x++, a0++)
			{
				i0 = ImgIdx->vidxt[a0];
				if(i0==Isolator) img.SetPixel(x, y, 0); else
				{
					float q = sqrt(float(VarTab[i0].q/maxq[0]));
					float g = q<0.5? 0 : (q-0.5f)*2;
					float r = sin(q*(3.141592f/2));
					float b = sin(q*(3.141592f*2))/2 + q*q;
					float lum = Antialiasing? (float)LOBYTE(lumtab[a0]) : 255;
					img.SetPixel(x, y, (BYTE(floor(0.2*lum))<<16) | (BYTE(floor(g*lum))<<8) | BYTE(floor(r*lum)));
				}
			}
		break;

	case 2:
		for(y=0; y<ImgIdx->sizy; y++)
			for(x=0; x<ImgIdx->sizx; x++, a0++, a1++)
			{
				i1 = ImgIdx->vidxt[a1];
				if(i1==Isolator) img.SetPixel(x, y, 0); else
				{
					float q = sqrt(float(VarTab[i1].q/maxq[1]));
					float g = q<0.5? 0 : (q-0.5f)*2;
					float r = sin(q*(3.141592f/2));
					float b = sin(q*(3.141592f*2))/2 + q*q;
					float lum = Antialiasing? (float)HIBYTE(lumtab[a0]) : 255;
					img.SetPixel(x, y, (BYTE(floor(0.2*lum))<<16) | (BYTE(floor(g*lum))<<8) | BYTE(floor(r*lum)));
				}
			}
		break;

	default:
		for(y=0; y<ImgIdx->sizy; y++)
			for(x=0; x<ImgIdx->sizx; x++, a0++, a1++)
			{
				i0 = ImgIdx->vidxt[a0];
				i1 = ImgIdx->vidxt[a1];
				if(i0==Isolator && i1==Isolator) img.SetPixel(x, y, 0); else
				{
					float r = i0==Isolator? 0 : sqrt(float(VarTab[i0].q/maxq[0]));
					float g = i1==Isolator? 0 : sqrt(float(VarTab[i1].q/maxq[1]));
					float lumr = Antialiasing? (float)LOBYTE(lumtab[a0]) : 255;
					float lumg = Antialiasing? (float)HIBYTE(lumtab[a0]) : 255;
					img.SetPixel(x, y, (30<<16) | (BYTE(floor(g*lumg))<<8) | BYTE(floor(r*lumr)));
				}
			}
		break;
	}
}

void ProgressInfo(HWND hwnd)
{
	char str[128];
	switch(ProgressStage) {
	case 0: str[0] = 0; break;
	case 1: sprintf_s(str, "Create Matrix, Progress %.1f%% of rows", ProgressDone*100); break;
	case 2: sprintf_s(str, "Gaussian elimination, Progress %.1f%% of rows", ProgressDone*100); break;
	case 3: sprintf_s(str, "backward substitution, Progress %.1f%% of rows", ProgressDone*100); break;
	default: sprintf_s(str, "Done"); break;
	}
	SetWindowText(hwnd, str);
}
