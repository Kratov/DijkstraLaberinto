#include<Windows.h>
#include<iostream>
#include<climits>
#include<queue>
#include<algorithm>
#include<vector>
#include<map>

#pragma comment( lib, "gdiplus.lib" ) 
#include <gdiplus.h> 

using namespace Gdiplus;
using namespace std;

char constexpr NODE = 79;
char constexpr WALL = 219;
BYTE constexpr R = 0;
BYTE constexpr G = 0;
BYTE constexpr B = 100;

BYTE constexpr TOP = 0;
BYTE constexpr RIGHT = 1;
BYTE constexpr LEFT = 3;
BYTE constexpr BOTTOM = 2;

BYTE constexpr N_CARDINALS = 4;

struct Pos {
	int x;
	int y;
};

struct Node {
	Pos pos;
	Node * neighbor[N_CARDINALS] = { 0 };
};

typedef pair<int, Node*> TDistanceNodePair;

//====================================
//===== Datos del laberinto =====
//====================================
vector<Node*> resultPath;
const wchar_t * filename;
Node * mazeStart = NULL;
Node * mazeEnd = NULL;
int mazeWidth = 0;
int mazeHeight = 0;
int nNodesInMaze = 0;

//====================================
//===== FIN nodos en laberinto =====
//====================================

enum TPixel
{
	Road,
	Wall
};

TPixel roadOrWall(const Color & color)
{
	if ((color.GetRed() + color.GetGreen() + color.GetBlue()) > 0)
		return TPixel::Road;
	else
		return TPixel::Wall;
}

void gotoxy(int column, int line)
{
	COORD coord;
	coord.X = column;
	coord.Y = line;
	SetConsoleCursorPosition(
		GetStdHandle(STD_OUTPUT_HANDLE),
		coord
	);
}

void createGraph() {
	//===== Inicializar GDI+ ===== 
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	//===== Cargar imagen del laberinto =====
	Image* image = Image::FromFile(filename);
	Bitmap * bmpImage = (Bitmap*)image;

	//====================================
	//===== Crear nodos en laberinto =====
	//====================================

	Color color;
	TPixel pixelType;
	mazeHeight = image->GetHeight();
	mazeWidth = image->GetWidth();

	//Buffer nodos fila superior

	Node ** aboveNodes = new Node*[mazeWidth];

	//===== Define el nodo de inicio =====
	for (int i = 0; i < mazeWidth; i++)
	{
		bmpImage->GetPixel(i, 0, &color);
		pixelType = roadOrWall(color);
		if (pixelType == Road)
		{
			mazeStart = new Node();
			mazeStart->pos.x = i;
			mazeStart->pos.y = 0;
			aboveNodes[i] = mazeStart;
			nNodesInMaze += 1;
#ifdef _DEBUG
			cout << char(NODE);
#endif // _DEBUG
#ifndef _DEBUG
			break;
#endif // _DEBUG
		}
#ifdef _DEBUG
		else
			cout << char(WALL);
#endif // DEBUG

	}

#ifdef _DEBUG
	cout << endl;
#endif // _DEBUG

	//===== Nodos entre laberinto =====
	//Recorre filas pixeles
	for (int i = 1; i < mazeHeight - 1; i++)
	{
		bool next, currentNode, terminatedStack;
		next = currentNode = terminatedStack = false;
		Node * leftNode = NULL;
		//Recorre columnas pixeles
		for (int j = 0; j < mazeWidth; j++)
		{

			//Primer iteracion -- Anterior Wall
			terminatedStack = currentNode;
			//El actual pasa a ser el siguiente
			currentNode = next;
			//Verifica que el siguiente pixel sea Road
			bmpImage->GetPixel(j + 1, i, &color);
			next = roadOrWall(color) == Road;

			Node * newNode = NULL;

			//Verifica que el actual no sea Wall para no agregar nodo ahi
			if (!currentNode)
			{
#ifdef _DEBUG
				gotoxy(j, i); cout << char(WALL);
#endif // _DEBUG
				continue;
			}


			if (terminatedStack)
			{
				if (next)
				{
					// Camino(terminatedStack), Camino(currentNode), Camino(NEXT) 
					// Agrega Nodo si existe otro encima o abajo
					bmpImage->GetPixel(j, i - 1, &color);
					bool RoadAbove = roadOrWall(color) == Road;
					bmpImage->GetPixel(j, i + 1, &color);
					bool RoadBelow = roadOrWall(color) == Road;
					if (RoadAbove or RoadBelow)
					{
						newNode = new Node();
						newNode->pos.x = j;
						newNode->pos.y = i;
						leftNode->neighbor[RIGHT] = newNode;
						newNode->neighbor[LEFT] = leftNode;
						leftNode = newNode;
					}
				}
				else
				{
					//Camino, Camino, Pared
					//Crea nodo al final del corredor.
					newNode = new Node();
					newNode->pos.x = j;
					newNode->pos.y = i;
					//Une al Nodo de la izquierda
					leftNode->neighbor[RIGHT] = newNode;
					newNode->neighbor[LEFT] = leftNode;
					leftNode = NULL;
				}
			}
			else
			{
				if (next)
				{
					//Pared, Camino, Camino
					newNode = new Node();
					newNode->pos.x = j;
					newNode->pos.y = i;
					leftNode = newNode;
				}
				else
				{
					//Pared, Camino, Pared
					//Se crea solo si esta en Road sin salida
					bmpImage->GetPixel(j, i - 1, &color);
					bool RoadAbove = roadOrWall(color) == Road;
					bmpImage->GetPixel(j, i + 1, &color);
					bool RoadBelow = roadOrWall(color) == Road;
					if (!RoadAbove or !RoadBelow)
					{
						newNode = new Node();
						newNode->pos.x = j;
						newNode->pos.y = i;
					}
				}
			}

			if (newNode)
			{
#ifdef _DEBUG
				gotoxy(j, i); cout << char(NODE);
#endif // _DEBUG
				bmpImage->GetPixel(j, i - 1, &color);
				bool RoadAbove = roadOrWall(color) == Road;
				if (RoadAbove)
				{
					Node * topNode = aboveNodes[j];
					topNode->neighbor[BOTTOM] = newNode;
					newNode->neighbor[TOP] = topNode;
				}

				bmpImage->GetPixel(j, i + 1, &color);
				bool RoadBelow = roadOrWall(color) == Road;
				if (RoadBelow)
					aboveNodes[j] = newNode;
				else
					aboveNodes[j] = NULL;

				nNodesInMaze += 1;

			}
		}
	}

#ifdef _DEBUG
	cout << endl;
#endif // _DEBUG

	//===== Define el nodo de fin =====
	for (int i = 0; i < mazeWidth; i++)
	{
		bmpImage->GetPixel(i, mazeHeight - 1, &color);
		pixelType = roadOrWall(color);
		if (pixelType == Road)
		{
			mazeEnd = new Node();
			mazeEnd->pos.x = i;
			mazeEnd->pos.y = mazeHeight - 1;
			Node * topNode = aboveNodes[i];
			topNode->neighbor[BOTTOM] = mazeEnd;
			mazeEnd->neighbor[TOP] = topNode;
			nNodesInMaze += 1;
#ifdef _DEBUG
			cout << char(NODE);
#endif // _DEBUG
#ifndef _DEBUG
			break;
#endif // _DEBUG
		}
#ifdef _DEBUG
		else
			cout << char(WALL);
#endif // _DEBUG
	}

	//====================================
	//===== FIN crear nodos en laberinto =====
	//====================================

	//Liberar memoria
	delete[] aboveNodes;

	//===== Liberar memoria de imagen =====
	delete image; image = 0;
	

	//===== Terminar GDI+ =====
	GdiplusShutdown(gdiplusToken);
}

void solveByDijkstra()
{
	int total = mazeWidth * mazeHeight;

	Pos startPos = mazeStart->pos;
	Pos endPos = mazeEnd->pos;

	bool * visitedStack = new bool[total];
	for (int i = 0; i < total; i++)
		visitedStack[i] = false;

	Node ** terminatedStack = new Node *[total];
	for (int i = 0; i < total; i++)
		terminatedStack[i] = NULL;

	int infinity = INT_MAX;

	int * distancesStack = new int[total];
	for (int i = 0; i < total; i++)
		distancesStack[i] = infinity;

	priority_queue<TDistanceNodePair, vector<TDistanceNodePair>, greater<TDistanceNodePair>> unvisitedStack;

	TDistanceNodePair * nodeIndex = new TDistanceNodePair[total];

	distancesStack[mazeStart->pos.x * mazeWidth + mazeStart->pos.y] = 0;
	unvisitedStack.push(make_pair(0, mazeStart));

	int completed = 0;

	while (unvisitedStack.size() > 0)
	{

		TDistanceNodePair newNode = unvisitedStack.top();
		unvisitedStack.pop();

		Node * currentAnalizeNode = newNode.second;
		Pos currentAnalizeNodePos = currentAnalizeNode->pos;
		int currentAnalizeNodePosIndex = currentAnalizeNodePos.x * mazeWidth + currentAnalizeNodePos.y;

		if (distancesStack[currentAnalizeNodePosIndex] == infinity)
			break;

		if ((currentAnalizeNodePos.x == endPos.x) && (currentAnalizeNodePos.y == endPos.y))
		{
			completed = true;
			break;
		}

		for (int i = 0; i < N_CARDINALS; i++)
		{
			Node * neighborNode = currentAnalizeNode->neighbor[i];
			if (neighborNode)
			{
				Pos neighborNodePos = neighborNode->pos;
				int neighborNodePosIndex = neighborNodePos.x * mazeWidth + neighborNodePos.y;
				if (!visitedStack[neighborNodePosIndex])
				{
					int d = abs(neighborNodePos.x - currentAnalizeNodePos.x) + abs(neighborNodePos.y - currentAnalizeNodePos.y);
					int newDistance = distancesStack[currentAnalizeNodePosIndex] + d;

					if (newDistance < distancesStack[neighborNodePosIndex])
					{
						TDistanceNodePair vNode = nodeIndex[neighborNodePosIndex];
						if (!vNode.second)
						{
							TDistanceNodePair vNode = make_pair(newDistance, neighborNode);
							unvisitedStack.push(vNode);
							nodeIndex[neighborNodePosIndex] = vNode;
							distancesStack[neighborNodePosIndex] = newDistance;
							terminatedStack[neighborNodePosIndex] = currentAnalizeNode;
						}
						else
						{
							unvisitedStack.push(make_pair(newDistance, vNode.second));
							distancesStack[neighborNodePosIndex] = newDistance;
							terminatedStack[neighborNodePosIndex] = currentAnalizeNode;
						}
					}
				}
			}
			visitedStack[currentAnalizeNodePosIndex] = true;
		}
	}

	Node * currentNode = mazeEnd;
	while (currentNode)
	{
		resultPath.push_back(currentNode);
		currentNode = terminatedStack[currentNode->pos.x * mazeWidth + currentNode->pos.y];
	}
	reverse(resultPath.begin(), resultPath.end());

	//Liberar memoria
	delete[] visitedStack;
	delete[] distancesStack;
	delete[] terminatedStack;
}

void saveSolveInImage() {

	//===== Inicializar GDI+ ===== 
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	//===== Cargar imagen del laberinto =====
	Image* image = Image::FromFile(filename);
	Bitmap * bmpImage = (Bitmap*)image;
	for (int i = 0; i < resultPath.size() - 1; i++)
	{
		Node * a = resultPath[i];
		Node * b = resultPath[i + 1];

		Color color = { R,G,B };

		if (a->pos.x == b->pos.x)
			for (int j = min(a->pos.y, b->pos.y); j < max(a->pos.y, b->pos.y) + 1; j++)
				bmpImage->SetPixel(a->pos.x, j, color);
		else if (a->pos.y == b->pos.y)
			for (int j = min(a->pos.x, b->pos.x); j < max(a->pos.x, b->pos.x); j++)
				bmpImage->SetPixel(j, a->pos.y, color);
	}

	CLSID pngClsid;
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid);
	image->Save(L"Teminado.png", &pngClsid, NULL);

	//===== Liberar memoria de imagen =====
	delete image; image = 0;

	//===== Terminar GDI+ =====
	GdiplusShutdown(gdiplusToken);
}

int main()
{
	filename = L"C:\\ProgramacionEstructuras\\ImagenLaberintos\\6.png";
	createGraph();
	solveByDijkstra();
	for (int i = 0; i < resultPath.size(); i++)
		cout << "\nResultado path: ("<< resultPath[i]->pos.x<<","<< resultPath[i]->pos.y<<")";
	cout << endl << endl << "\nNumero de nodos generados: " << nNodesInMaze;
	saveSolveInImage();
	getchar();
	return 0;
}
