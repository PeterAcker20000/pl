// Win32Project1.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ToolPath.h"
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <Windows.h>
#include <iostream>
#include "OpenFileDialog.h"
#include <vector>
using namespace std;

#define PI 3.1415926535897932 
static bool g_ClockwiseTopology;
static double g_xmin, g_ymin, g_xmax, g_ymax;
static double g_scale, g_offsetx, g_offsety;
static Shape* g_part;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
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

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WIN32PROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32PROJECT1));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WIN32PROJECT1);
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
   g_part = NULL;

   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      0, 0, 640, 640, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

d2D::d2D() {}

d2D::d2D(const d2D& other)
{
	x = other.x;
	y = other.y;
}

Line::Line()
{
	type = 1;
}

Line::Line(const Line& other)
{
	end0 = other.end0;
	end1 = other.end1;
}

CircularArc::CircularArc()
{
	type = 2;
}

CircularArc::CircularArc(const CircularArc& other)
{
	end0 = other.end0;
	end1 = other.end1;
	center.x = other.center.x;
	center.y = other.center.y;
	clockFrom = other.clockFrom;
}

Shape::~Shape()
{
	for (int i = 0; i < edgeCount; i++)
		delete edges[i];

	edges.clear();
	verts.clear();
}


Vertex::Vertex() {}

Vertex::Vertex(const Vertex& other)
{
	index = other.index;
	dp.x = other.dp.x;
	dp.y = other.dp.y;
}

bool FindXYFromIndex(int index, d2D &p)
{
	bool found = false;
	int i = 0;
	while (!found && i < g_part->vertCount)
	{
		if (index == g_part->verts[i].index)
		{
			found = true;
			p = g_part->verts[i].dp;
		}
		i++;
	}
	if (!found)
		MessageBox(0, NULL, _T("Invalid Index"),
			MB_OK | MB_OKCANCEL);

	return found;
}


double turn(const d2D &p0, const d2D &p1, const d2D &p2, const d2D &p3)
{
	double ang, ang1, ang2;

	d2D del1, del2;
	del1.x = p1.x - p0.x;
	del1.y = p1.y - p0.y;
	del2.x = p3.x - p2.x;
	del2.y = p3.y - p2.y;

	ang1 = atan2(del1.y, del1.x);
	ang2 = atan2(del2.y, del2.x);
	ang = ang2 - ang1;
	ang = ang / PI * 180.0;
	if (ang > 180)
		ang = ang - 360;
	else if (ang < -180)
		ang = ang + 360;
	return ang;
}


double ArcLengthJ(const d2D &c, const d2D & p0, const d2D & p1)
{
	double ang, ang1, ang2;
	d2D q0, q1;
	q0.x = p0.x - c.x;
	q0.y = p0.y - c.y;
	q1.x = p1.x - c.x;
	q1.y = p1.y - c.y;


	ang1 = atan2(q0.y, q0.x);
	ang2 = atan2(q1.y, q1.x);
	ang = ang2 - ang1;
	if (ang < 0)
		ang = -1.0*ang;
	double radius = sqrt(q0.x*q0.x + q0.y*q0.y);
	return ang * radius;

}

double d2DDist(const d2D &p0, const d2D &p1)
{
	return sqrt((p1.x - p0.x)*(p1.x - p0.x) + (p1.y - p0.y)*(p1.y - p0.y));
}
double EdgeShape::GetLength()
{
	d2D p0, p1;
	FindXYFromIndex(end0, p0);
	FindXYFromIndex(end1, p1);
	return d2DDist(p0, p1);

}
double CircularArc::GetLength()
{
	d2D p0, p1;
	FindXYFromIndex(end0, p0);
	FindXYFromIndex(end1, p1);
	return ArcLengthJ(center, p0, p1);
}



void DrawLine(HDC hdc, d2D &p0, const d2D &p1)
{
	POINT pnt0, pnt1;
	pnt0.x = (int)floor(g_scale * p0.x + g_offsetx);
	pnt0.y = (int)floor(g_scale * p0.y + g_offsety);
	pnt1.x = (int)floor(g_scale * p1.x + g_offsetx);
	pnt1.y = (int)floor(g_scale * p1.y + g_offsety);

	MoveToEx(hdc, pnt0.x, pnt0.y, NULL);
	LineTo(hdc, pnt1.x, pnt1.y);
}

void DrawCircularArc(HDC hdc, const d2D &p0, const d2D &p1, const d2D &center)
{
	d2D p, pLast;
	double rad = d2DDist(center, p0);
	double spanAng, ang1, ang2;

	d2D del1, del2;
	del1.x = p0.x - center.x;
	del1.y = p0.y - center.y;
	del2.x = p1.x - center.x;
	del2.y = p1.y - center.y;

	ang1 = atan2(del1.y, del1.x);
	ang2 = atan2(del2.y, del2.x);
	spanAng = ang2 - ang1;

	double startAng = ang1;
	double incr = spanAng / 18.0;
	if (incr > 0)
		incr *= -1;

	pLast.x = cos(startAng)*rad + center.x;
	pLast.y = sin(startAng)*rad + center.y;

	for (int j = 0; j < 19; j++)
	{
		double ang = startAng + j * incr;
		p.x = cos(ang)*rad + center.x;
		p.y = sin(ang)*rad + center.y;
		DrawLine(hdc, pLast, p);
		pLast = p;
	}

}
double ComputeCuttingCost()
{
	double time = 0.0;
	d2D p0, p1, c;
	for (int i = 0; i < g_part->edgeCount; i++)
	{
		if (g_part->edges[i]->type == 1)
		{
			FindXYFromIndex(g_part->edges[i]->end0, p0);
			FindXYFromIndex(g_part->edges[i]->end1, p1);
			time = time + g_part->edges[i]->GetLength() / 0.5;
		}
		else if (g_part->edges[i]->type == 2)
		{
			CircularArc* arc = (CircularArc*)g_part->edges[i];
			c = arc->center;
			FindXYFromIndex(arc->end0, p0);
			double radius = d2DDist(p0, c);
			double al = arc->GetLength();
			double speed = exp(-1 / radius) * 0.5;
			time = time + al / speed;
		}
	}
	return time;

}
double wind()
{
	int j;
	double accum = 0;
	d2D p0, p1, p2, p3;
	for (int i = 0; i < g_part->edgeCount; i++)
	{
		j = i + 1;
		if (j >= g_part->edgeCount)
			j = 0;

		FindXYFromIndex(g_part->edges[i]->end0, p0);
		FindXYFromIndex(g_part->edges[i]->end1, p1);
		FindXYFromIndex(g_part->edges[j]->end0, p2);
		if ((p1.x != p2.x) || (p1.y != p2.y))
			MessageBox(0, NULL, _T("Invalid ClockwiseFrom Index"),
				MB_OK | MB_OKCANCEL);
		FindXYFromIndex(g_part->edges[j]->end1, p3);
		accum = accum + turn(p0, p1, p2, p3);
	}
	return accum;
}
void ComputeBounds(double & xmin, double &xmax, double &ymin, double &ymax)
{
	double windDir = wind();
	if (windDir < 0)
		g_ClockwiseTopology = true;
	else
		g_ClockwiseTopology = false;

	xmin = 1E20;
	xmax = -1E20;
	ymin = 1E20;
	ymax = -1E20;
	d2D p0, p1, c;
	for (int i = 0; i < g_part->edgeCount; i++)
	{
		if (g_part->edges[i]->type == 1)
		{
			FindXYFromIndex(g_part->edges[i]->end0, p0);
			FindXYFromIndex(g_part->edges[i]->end1, p1);
			if (p0.x < xmin)
				xmin = p0.x;
			if (p0.x > xmax)
				xmax = p0.x;
			if (p0.y < ymin)
				ymin = p0.y;
			if (p0.y > ymax)
				ymax = p0.y;

			if (p1.x < xmin)
				xmin = p1.x;
			if (p1.x > xmax)
				xmax = p1.x;
			if (p1.y < ymin)
				ymin = p1.y;
			if (p1.y > ymax)
				ymax = p1.y;

		}
		else if (g_part->edges[i]->type == 2)
		{
			CircularArc* arc = (CircularArc*)g_part->edges[i];
			int clockFrom = arc->clockFrom;
			if (g_ClockwiseTopology)
			{
				if (clockFrom != arc->end0)
					continue;
			}
			else
			{
				if (clockFrom != arc->end1)
					continue;

			}
			c = arc->center;

			FindXYFromIndex(arc->end0, p0);
			double radius = d2DDist(p0, c);


			if (c.x + radius < xmin)
				xmin = c.x + radius;
			if (c.x + radius > xmax)
				xmax = c.x + radius;
			if (c.y + radius < ymin)
				ymin = c.y + radius;
			if (c.y + radius > ymax)
				ymax = c.y + radius;

			if (c.x - radius < xmin)
				xmin = c.x - radius;
			if (c.x - radius > xmax)
				xmax = c.x - radius;
			if (c.y - radius < ymin)
				ymin = c.y - radius;
			if (c.y - radius > ymax)
				ymax = c.y - radius;
		}
	}

}
void DrawEdges(HDC hdc)
{
	d2D p0, p1, c;
	for (int i = 0; i < g_part->edgeCount; i++)
	{
		if (g_part->edges[i]->type == 1)
		{
			FindXYFromIndex(g_part->edges[i]->end0, p0);
			FindXYFromIndex(g_part->edges[i]->end1, p1);
			DrawLine(hdc, p0, p1);
		}
		else if (g_part->edges[i]->type == 2)
		{
			CircularArc* arc = (CircularArc*)g_part->edges[i];
			c = arc->center;
			FindXYFromIndex(arc->end0, p0);
			FindXYFromIndex(arc->end1, p1);


			if (arc->clockFrom == g_part->edges[i]->end0)
				DrawCircularArc(hdc, p0, p1, c);
			else if (arc->clockFrom == g_part->edges[i]->end1)
				DrawCircularArc(hdc, p1, p0, c);
		}
	}
}


void DrawScene(HWND hwnd, HDC hdc)
{
	if (!g_part)
		return;
	RECT rect;

	GetClientRect(hwnd, &rect);
	double w = (g_xmax - g_xmin)*1.1;
	double h = (g_ymax - g_ymin)*1.1;
	int rw = rect.right - rect.left;
	int rh = rect.bottom - rect.top;
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	double xScale = rw / w;
	double yScale = rh / h;

	if (xScale > yScale)
		g_scale = yScale;
	else
		g_scale = xScale;

	double drawingCenterx = g_scale*(g_xmax + g_xmin) / 2.0;
	double drawingCentery = g_scale*(g_ymax + g_ymin) / 2.0;
	int rectCenterx = (rect.right + rect.left) / 2;
	int rectCentery = (rect.bottom - rect.top) / 2;


	g_offsetx = rectCenterx - drawingCenterx + 5;
	g_offsety = rectCentery - drawingCentery;

	DrawEdges(hdc);

}
bool ParenBalance(string& line, int &paren, int parenSave, bool &going)
{
	if (strstr(line.c_str(), "{"))	paren++;
	if (strstr(line.c_str(), "}"))	paren--;
	if (paren == parenSave)
	{
		going = false;
		return true;
	}
	return false;
}

int ExtractIndex(string line)
{
	size_t off0, off1;
	string number;
	int index;
	off0 = line.find('"');
	off1 = line.find('"', off0 + 1);
	number = line.substr(off0 + 1, off1 - off0 - 1);
	index = atoi(number.c_str());
	return index;
}
int ExtractIndex1(string line)
{
	size_t off0, off1;
	int index;
	string number;
	off0 = line.find_last_of('\t');
	off1 = line.find(',', off0 + 1);
	number = line.substr(off0 + 1, off1 - off0 - 1);
	index = atoi(number.c_str());
	return index;
}
int ExtractIndex2(string line)
{
	size_t off0, off1;
	string number;
	size_t len = line.length();
	off0 = line.find(':');
	off1 = line.find('"', off0 + 1);
	number = line.substr(off0 + 1, len);
	int index = atoi(number.c_str());
	return index;
}

double ExtractFloat(string line)
{
	size_t off0, off1;
	string number;
	off0 = line.find(':');
	off1 = line.find(',', off0 + 1);
	number = line.substr(off0 + 1, off1 - off0 - 1);
	double d = atof(number.c_str());
	return d;
}


int readFile(LPWSTR fName)
{
	ifstream myfile(fName);
	string line;
	bool findingEdges = false;
	bool findingVerts = false;
	bool going = true;
	int paren = 0;
	int parenSave = 0;
	g_part->vertCount = 0;
	g_part->edgeCount = 0;
	d2D p;

	if (myfile.is_open())
	{
		while (going)
		{
			getline(myfile, line);
			if (ParenBalance(line, paren, 0, going)) continue;

			if (strstr(line.c_str(), "Vertices"))
			{
				parenSave = paren - 1;
				findingVerts = true;
				while (findingVerts)
				{
					Vertex vert;
					getline(myfile, line);
					if (ParenBalance(line, paren, parenSave, findingVerts))	continue;
					int index = ExtractIndex(line);
					vert.SetIndex(index);
					getline(myfile, line);
					if (ParenBalance(line, paren, parenSave, findingVerts))	continue;
					getline(myfile, line);
					p.x = ExtractFloat(line);
					getline(myfile, line);
					p.y = ExtractFloat(line);
					vert.SetPoint(p);
					g_part->verts.push_back(vert);
					g_part->vertCount++;
					getline(myfile, line);
					if (ParenBalance(line, paren, parenSave, findingVerts))	continue;
					getline(myfile, line);
					if (ParenBalance(line, paren, parenSave, findingVerts))	continue;
				}
			}
			else if (strstr(line.c_str(), "Edges"))
			{
				parenSave = paren - 1;
				findingEdges = true;
				while (findingEdges)
				{
					getline(myfile, line);
					if (ParenBalance(line, paren, parenSave, findingEdges))	continue;
					int indexMain = ExtractIndex(line);
					getline(myfile, line);
					if (strstr(line.c_str(), "LineSegment"))
					{
						Line *l = new Line();
						l->SetIndex(indexMain);
						getline(myfile, line);
						getline(myfile, line);
						l->SetEnd0(ExtractIndex1(line));
						getline(myfile, line);
						l->SetEnd1(ExtractIndex1(line));
						g_part->edgeCount++;
						g_part->edges.push_back(l);
						getline(myfile, line);
						getline(myfile, line);
						if (ParenBalance(line, paren, parenSave, findingEdges))	continue;
					}
					else if (strstr(line.c_str(), "Arc"))
					{
						CircularArc *a = new CircularArc();
						getline(myfile, line);
						getline(myfile, line);

						a->SetIndex(indexMain);
						int index = ExtractIndex1(line);
						a->SetEnd0(index);
						getline(myfile, line);
						index = ExtractIndex1(line);
						a->SetEnd1(index);
						getline(myfile, line);
						getline(myfile, line);
						if (ParenBalance(line, paren, parenSave, findingEdges))	continue;
						getline(myfile, line);
						p.x = ExtractFloat(line);
						getline(myfile, line);
						p.y = ExtractFloat(line);
						a->SetCenter(p);
						getline(myfile, line);
						if (ParenBalance(line, paren, parenSave, findingEdges))	continue;
						getline(myfile, line);
						index = ExtractIndex2(line);
						a->SetClockFrom(index);
						g_part->edges.push_back(a);
						g_part->edgeCount++;
						getline(myfile, line);
						if (ParenBalance(line, paren, parenSave, findingEdges))	continue;
					}
				}
			}
		}
		myfile.close();
	}
	else cout << "Unable to open file";

	return 0;
}
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

void CostFunction()
{
	if (g_part)
		delete g_part;
	g_part = new Shape();

	OpenFileDialog* openFileDialog1 = new OpenFileDialog();
	openFileDialog1->FilterIndex = 1;
	openFileDialog1->Flags |= OFN_SHOWHELP;
	openFileDialog1->InitialDir = _T("C:\\JSON\\");
	openFileDialog1->Title = _T("Open JSON File");


	if (openFileDialog1->ShowDialog())
	{
		int a = 0;

	readFile(openFileDialog1->FileName);

	}
	ComputeBounds(g_xmin, g_xmax, g_ymin, g_ymax);
	g_xmax = g_xmax + 0.1;
	g_ymax = g_ymax + 0.1;
	double time = ComputeCuttingCost();
	double area = (g_xmax - g_xmin)*(g_ymax - g_ymin);
	double materialCost = 0.75 * area;
	double cuttingCost = time * 0.07;
	double cost = materialCost + cuttingCost;
	cost = cost*100.0;
	int icost = (int)floor(round(cost) + 0.5);
	char ccost[100];
	sprintf_s(ccost, "%d", icost);
	size_t len = strlen(ccost);
	char dollars[100];
	char cents[100];
	char outStr[100];
	int i = 0;
	int j = 0;
	while (i < len - 2)
		dollars[j++] = ccost[i++];
	dollars[j] = NULL;
	j = 0;
	while (ccost[i] != NULL)
		cents[j++] = ccost[i++];
	cents[j] = NULL;
	strstr("b", "p");
	sprintf_s(outStr, "Cost= $%s.%s", dollars, cents);
	std::wstring stemp = s2ws(outStr);
	LPCWSTR result = stemp.c_str();

	MessageBox(0, result, _T("COST = Material Cost + Cutting Cost"),
		MB_OK | MB_OKCANCEL);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
static bool ok = false;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
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
			case IDM_OPEN:
				CostFunction();
				InvalidateRect(hWnd, NULL, TRUE);
			
				break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
			if (g_part)
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);
				HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 0, 255));
				SelectObject(hdc, pen);
				DrawScene(hWnd, hdc);
				EndPaint(hWnd, &ps);

			}
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
