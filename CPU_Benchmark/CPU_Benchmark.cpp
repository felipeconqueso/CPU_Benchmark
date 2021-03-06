#include "stdafx.h"
#include "CPU_Benchmark.h"
#include <stdio.h>
#include <strsafe.h>
#include <string>
#include <atlstr.h>
#include "Benchmark.h"
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <bitset>
#include <windows.h>

#define MAX_LOADSTRING 100
#define BTN_START 101
#define MB 1048576
#define mem_x 25
#define mem_y 75
#define cpu_x 350
#define cpu_y 75

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING]; 
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hWnd, start_btn, mem, processor_info, processor_status;
WCHAR mem_avail[16];
Benchmark* b;
MEMORYSTATUSEX mem_status;
SYSTEM_INFO sys_info;
int num_threads, affinity = 1;
float cpu_percent[100];
int mem_percent[100];

void update_status();
void get_cpu_info();
void get_cpu_status();
float GetCPULoad();
unsigned long long FileTimeToInt64(const FILETIME & ft);
float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks);
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	mem_status.dwLength = sizeof(mem_status);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CPUBENCHMARK, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CPUBENCHMARK));

	mem = CreateWindow(TEXT("STATIC"), LPCWSTR(" "), WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT, mem_x, mem_y, 300, 100, hWnd, NULL, NULL, NULL);
	processor_info = CreateWindow(TEXT("STATIC"), LPCWSTR(" "), WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT, cpu_x, cpu_y, 300, 100, hWnd, NULL, NULL, NULL);
	processor_status = CreateWindow(TEXT("STATIC"), LPCWSTR(" "), WS_CHILD | WS_VISIBLE | WS_EX_TRANSPARENT, cpu_x, cpu_y + 66, 300, 100, hWnd, NULL, NULL, NULL);
	get_cpu_info();
	std::thread mem_thread(update_status);
	mem_thread.detach();
	std::thread cpu_status_thread(get_cpu_status);
	cpu_status_thread.detach();
	b = new Benchmark(hWnd, &affinity);
    MSG msg;

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

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CPUBENCHMARK));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CPUBENCHMARK);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   start_btn = CreateWindow(TEXT("button"), TEXT("Run Benchmark"), WS_CHILD | WS_VISIBLE, 85, 250, 120, 30, hWnd, (HMENU)BTN_START, NULL, NULL);
   

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdcStatic;
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
			case BTN_START:
				b->start_benchmark();
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
	case WM_CTLCOLORSTATIC:
		hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LRESULT)GetStockObject(NULL_BRUSH);
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
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

void update_status() {
	CString str;
	int x = 0;
	while (1) {
		GlobalMemoryStatusEx(&mem_status);
		str.Format(L"System Memory Status:\n\n"
					L"Memory Usage %%:                %d %%\n"
					L"Memory Usage MB:             %d MB\n"
					L"Total System Memory:         %d MB\n"
					L"Total Available Memory:      %d MB",
					mem_status.dwMemoryLoad, ((mem_status.ullTotalPhys - mem_status.ullAvailPhys) / MB),
					(mem_status.ullTotalPhys / MB), (mem_status.ullAvailPhys / MB));
		SetWindowText(mem, str);
		ShowWindow(mem, SW_HIDE);
		ShowWindow(mem, SW_SHOW);
		mem_percent[x] = (mem_status.ullTotalPhys - mem_status.ullAvailPhys) / MB;
		x++;
		if (x = 100) x = 0;
		std::this_thread::sleep_for(std::chrono::seconds(1));	
	}
}

void get_cpu_info() {

	CString sMHz;
	DWORD BufSize = _MAX_PATH;
	DWORD dwMHz = _MAX_PATH;
	HKEY hKey;
	CString str, temp;
	std::vector<std::array<int, 4>> data_;
	std::array<int, 4> cpu_info;
	std::string brand_;
	int nEx;
	std::bitset<32> f_81_ECX_;
	std::bitset<32> f_81_EDX_;
	std::vector<std::array<int, 4>> extdata_;
	__cpuid(cpu_info.data(), 0x80000000);
	nEx = cpu_info[0];

	char brand[0x40];
	memset(brand, 0, sizeof(brand));

	for (int i = 0x80000000; i <= nEx; ++i)
	{
		__cpuidex(cpu_info.data(), i, 0);
		extdata_.push_back(cpu_info);
	}
	if (nEx >= 0x80000001)
	{
		f_81_ECX_ = extdata_[1][2];
		f_81_EDX_ = extdata_[1][3];
	}
	if (nEx >= 0x80000004)
	{
		memcpy(brand, extdata_[2].data(), sizeof(cpu_info));
		memcpy(brand + 16, extdata_[3].data(), sizeof(cpu_info));
		memcpy(brand + 32, extdata_[4].data(), sizeof(cpu_info));
		brand_ = brand;
	}
	str = brand_.c_str();
	str = str + L"\n";
	long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0,
		KEY_READ,
		&hKey);
	RegQueryValueEx(hKey, L"~MHz", NULL, NULL, (LPBYTE)&dwMHz, &BufSize);
	sMHz.Format(L"%d", dwMHz);
	str = str + L"Clock Speed:                  " + sMHz + L" MHz\n";
	GetSystemInfo(&sys_info);
	str.Format(str + L"Logical Processors:       %d", sys_info.dwNumberOfProcessors);
	num_threads = sys_info.dwNumberOfProcessors;
	SetWindowText(processor_info, str);
}

void get_cpu_status() {
	SetThreadAffinityMask(GetCurrentThread(), affinity);
	affinity = affinity * 2;
	CString str;
	float cpu_usage;
	int x = 0;
	while (1) {
		cpu_usage = GetCPULoad();
		str.Format(L"CPU Usage:                    %.2f %%", cpu_usage * 100);
		SetWindowText(processor_status, str);
		ShowWindow(processor_status, SW_HIDE);
		ShowWindow(processor_status, SW_SHOW);
		cpu_percent[x] = cpu_usage;
		x++;
		if (x = 100) x = 0;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks) {
	static unsigned long long _previousTotalTicks = 0;
	static unsigned long long _previousIdleTicks = 0;

	unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
	unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

	float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

	_previousTotalTicks = totalTicks;
	_previousIdleTicks = idleTicks;
	return ret;
}

unsigned long long FileTimeToInt64(const FILETIME & ft) { return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime); }

float GetCPULoad()
{
	FILETIME idleTime, kernelTime, userTime;
	return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}