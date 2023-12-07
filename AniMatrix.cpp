// AniMatrix.cpp : Defines the entry point for the application.
//Author : VC

#include "framework.h"
#include "AniMatrix.h"

using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")


#define MAX_LOADSTRING      100
#define CELL_SIZE           100
#define TIMER_INTERVAL      50
#define ROTATE_SPEED        50 // angle_/s
#define COLOR_SPEED         1
#define RENEW_INTERVAL      1 // s
#define FLASH_SIZE          50


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
int                 RNG(int minN, int maxN);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ANIMATRIX, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ANIMATRIX));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
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
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ANIMATRIX));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ANIMATRIX);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
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
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
// CAniMatrix class declaration
//

using ColorType = COLORREF;
static const ColorType baseColors[] = {
    0x000000FF, 0x000FF000, 0x00FF0000, 0x0000FFFF, 0x00FF00FF, 0x00FFFF00
};

class CAniMatrix
{
public:
    CAniMatrix(int cellSize, int matrixRows, int matrixCols);

    void Draw(HWND hWnd, HDC hDC, int centerX, int centerY) const;
    void UpdateTime(float timeS);
    void Resize(int matrixRows, int matrixCols);

private:
    class Cell
    {
    public:
        Cell();

        void Draw(Graphics& g, int x, int y) const;
        void UpdateTime(float dtS);
        void Renew();

    private:
        int digit_;
        float angle_;
        int rotateDir_;
        ColorType color_;
        float flashState_;
    };

    int cellSize_;
    int matrixRows_;
    int matrixCols_;
    float aniTime_;
    float renewTime_;

    std::vector<Cell> cells_;
};

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static CAniMatrix aniM(CELL_SIZE, 1, 1);
    static FILETIME startTime = {0};

    switch (message)
    {
    case WM_CREATE:
        GetSystemTimeAsFileTime(&startTime);
        SetTimer(hWnd, 1, TIMER_INTERVAL, NULL);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
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
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT r = {0};
            GetClientRect(hWnd, &r);
            int cx = r.right - r.left;
            int cy = r.bottom - r.top;
            HDC hdc = BeginPaint(hWnd, &ps);
            HDC hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hBM = CreateCompatibleBitmap(hdc, cx, cy);
            HGDIOBJ hOldBM = SelectObject(hMemDC, hBM);
            SetBkMode(hMemDC, TRANSPARENT);
            FillRect(hMemDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
            aniM.Draw(hWnd, hMemDC, (r.left + r.right) / 2, (r.top + r.bottom) / 2);
            BitBlt(hdc, r.left, r.top, cx, cy, hMemDC, 0, 0, SRCCOPY);
            SelectObject(hMemDC, hOldBM);
            DeleteObject(hBM);
            DeleteDC(hMemDC);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_SIZE:
        {
            RECT r = { 0 };
            GetClientRect(hWnd, &r);
            aniM.Resize(
                ((r.top + r.bottom) + CELL_SIZE - 1) / CELL_SIZE,
                ((r.left + r.right) + CELL_SIZE - 1) / CELL_SIZE
            );
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_TIMER:
        if (wParam == 1)
        {
            FILETIME currTime;
            GetSystemTimeAsFileTime(&currTime);
            auto dt = (*(PULONGLONG)&currTime - *(PULONGLONG)&startTime) / 10000000.0f;
            aniM.UpdateTime(dt);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
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

//
// CAniMatrix class implementation
//

CAniMatrix::CAniMatrix(int cellSize, int matrixRows, int matrixCols)
    : cellSize_(cellSize), aniTime_(0.0f), renewTime_(0.0f)
{
    Resize(matrixRows, matrixCols);
}

void CAniMatrix::Draw(HWND hWnd, HDC hDC, int centerX, int centerY) const
{
    Graphics g(hDC);
    int y = centerY - matrixRows_ * cellSize_ / 2;
    for (int i = 0; i < matrixRows_; ++i, y += CELL_SIZE)
    {
        int x = centerX - matrixCols_ * cellSize_ / 2;
        for (int j = 0; j < matrixCols_; ++j, x += CELL_SIZE)
        {
            const auto& c = cells_[i * matrixCols_ + j];
            c.Draw(g, x, y);
        }
    }
}

void CAniMatrix::UpdateTime(float timeS)
{
    for (auto& c : cells_)
    {
        c.UpdateTime(timeS - aniTime_);
    }
    aniTime_ = timeS;

    if (timeS - renewTime_ > RENEW_INTERVAL)
    {
        int i = RNG(0, matrixRows_ - 1);
        int j = RNG(0, matrixCols_ - 1);
        cells_[i * matrixCols_ + j].Renew();
        renewTime_ = timeS;
    }
}

void CAniMatrix::Resize(int matrixRows, int matrixCols)
{
    matrixRows_ = matrixRows;
    matrixCols_ = matrixCols;
    cells_.resize(matrixRows * matrixCols);
}

//
// CAniMatrix Cell class implementation
//

CAniMatrix::Cell::Cell() :
    digit_(RNG(0, 9)),
    angle_((float)RNG(0, 360)),
    rotateDir_(RNG(0, 1) ? 1 : -1),
    color_(baseColors[RNG(0, sizeof(baseColors) / sizeof(baseColors[0]) - 1)]),
    flashState_(0.0f)
{
}

void CAniMatrix::Cell::Draw(Graphics& g, int x, int y) const
{
    static FontFamily fontFamily(L"Times New Roman");
    auto size = CELL_SIZE + flashState_ * FLASH_SIZE;
    RectF layoutRect(0.0f, 0.0f, size, size);
    Font font(&fontFamily, size, FontStyleRegular, UnitPixel);

    static auto between = [](BYTE color_, float state) -> auto {
        return (BYTE)(0xFF * state + color_ * (1 - state));
    };

    auto s = std::to_wstring(digit_);
    SolidBrush brush(Color(255, 
        between(color_ & 0xFF, flashState_), 
        between((color_ >> 8) & 0xFF, flashState_), 
        between((color_ >> 16) & 0xFF, flashState_))
    );
    RectF boundRect;
    StringFormat format;
    PointF origin(0, 0);

    g.TranslateTransform((float)(x + CELL_SIZE / 2), float(y + CELL_SIZE / 2)); // Set rotation point
    g.RotateTransform(angle_); // Rotate text
    g.MeasureString(s.c_str(), s.length(), &font, layoutRect, &format, &boundRect); // Get size of rotated text (bounding box)
    g.DrawString(s.c_str(), s.length(), &font, PointF(-boundRect.Width / 2, -boundRect.Height / 2), &brush); // Draw string centered in x, y
    g.ResetTransform(); // Only needed if you reuse the Graphics object for multiple calls to DrawString
}

void CAniMatrix::Cell::UpdateTime(float dtS)
{
    angle_ += dtS * rotateDir_ * ROTATE_SPEED;
    flashState_ -= dtS * COLOR_SPEED;
    if (flashState_ < 0.0f)
    {
        flashState_ = 0.0f;
    }
}

void CAniMatrix::Cell::Renew()
{
    flashState_ = 1.0f;
    digit_ = (digit_ + 1) % 10;
}

//
// Pseudo-random number generator which returns a number between minN and maxN (inclusive)
//
int RNG(int minN, int maxN)
{
    static std::random_device rd;   // non-deterministic generator
    static std::mt19937 gen(rd());  // to seed mersenne twister.
    std::uniform_int_distribution<> dist(minN, maxN); // distribute results between minN and maxN inclusive.
    return dist(gen);
}
