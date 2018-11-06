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

//Relaciona una distancia con un nodo. (Usado en cola de prioridad)
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


//Road -> Camino, Wall -> Pared
enum TPixel
{
	Road,
	Wall
};

//Road(Pixel blanco), Wall(Pixel negro)
TPixel roadOrWall(const Color & color)
{
	if ((color.GetRed() + color.GetGreen() + color.GetBlue()) > 0)
		return TPixel::Road;
	else
		return TPixel::Wall;
}

//Funcion para posicion de cursor en consola.
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
		//Obtiene pixel (X,Y)
		bmpImage->GetPixel(i, 0, &color);
		pixelType = roadOrWall(color);

		if (pixelType == Road)
		{
			//Define Nodo inicio (SOLO UNO)
			mazeStart = new Node();
			mazeStart->pos.x = i;
			mazeStart->pos.y = 0;
			//Guarda el nodo el Buffer de nodos superiores
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
		bool next, currentNode, previus;
		next = currentNode = previus = false;
		//Guarda el nodo a la izquierda del currentNode para luego ligarlo
		Node * leftNode = NULL;
		//Recorre columnas pixeles
		for (int j = 0; j < mazeWidth; j++)
		{
			//Primer iteracion -- Anterior Wall
			previus = currentNode;
			//El actual pasa a ser el siguiente
			currentNode = next;
			//Verifica que el siguiente pixel sea Road
			bmpImage->GetPixel(j + 1, i, &color);
			next = roadOrWall(color) == Road;

			//Guarda el nodo si cumple con los criterios
			Node * newNode = NULL;

			//Verifica que el actual no sea Wall para no agregar nodo ahi
			if (!currentNode)
			{
#ifdef _DEBUG
				//Inprimimos si estamos en DEBUG una pared del laberinto
				gotoxy(j, i); cout << char(WALL);
#endif // _DEBUG
				continue;
			}


			if (previus)
			{
				if (next)
				{
					// Camino(previus), Camino(currentNode), Camino(NEXT) 
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

			//Si existe un nuevo nodo (AQUI UNIMOS LOS NODOS SUPERIORES)
			if (newNode)
			{
#ifdef _DEBUG
				gotoxy(j, i); cout << char(NODE);
#endif // _DEBUG
				bmpImage->GetPixel(j, i - 1, &color);
				bool RoadAbove = roadOrWall(color) == Road;
				// Si existe un camino arriba
				if (RoadAbove)
				{
					//Extrae nodo superior en el mismo X.
					Node * topNode = aboveNodes[j];
					//Los unimos
					topNode->neighbor[BOTTOM] = newNode;
					newNode->neighbor[TOP] = topNode;
				}

				bmpImage->GetPixel(j, i + 1, &color);
				bool RoadBelow = roadOrWall(color) == Road;
				//Si existe un camino inferior
				if (RoadBelow)
					//El nodo nuevo entra a ser el superior
					aboveNodes[j] = newNode;
				else
					//No existen nodos ineriores al que unir se limpia.
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
	//Total de pixeles que existen en el laberinto
	int total = mazeWidth * mazeHeight;

	//Posicion de inicio del laberinto y final
	Pos startPos = mazeStart->pos;
	Pos endPos = mazeEnd->pos;

	//Crea un stack de Nodos ya visitados en su INDEX. Inicializa a FALSE
	bool * visitedStack = new bool[total];
	for (int i = 0; i < total; i++)
		visitedStack[i] = false;

	//Stack de nodos ya evaluados
	Node ** terminatedStack = new Node *[total];
	for (int i = 0; i < total; i++)
		terminatedStack[i] = NULL;

	//Determina la distancia INFINITA. Para el stack de distancias.
	int infinity = INT_MAX;

	//Stack de distancias guarda distancia segun INDEX del NODO.
	int * distancesStack = new int[total];
	for (int i = 0; i < total; i++)
		distancesStack[i] = infinity;

	//Crea cola de prioridad - Menor a mayor
	priority_queue<TDistanceNodePair, vector<TDistanceNodePair>, greater<TDistanceNodePair>> unvisitedStack;

	//Crea un stack de INDEX para los nodos
	TDistanceNodePair * nodeIndex = new TDistanceNodePair[total];

	//Asigna la distancia 0 al nodo de inicio 
	distancesStack[mazeStart->pos.x * mazeWidth + mazeStart->pos.y] = 0;

	//Ingresa Nodo inicio a cola de prioridad
	unvisitedStack.push(make_pair(0, mazeStart));

	int completed = 0;

	//Mientras existan Nodos por explorar en la Cola
	while (unvisitedStack.size() > 0)
	{

		//Extrae el nodo de la Cola
		TDistanceNodePair newNode = unvisitedStack.top();
		//Lo elimina de la cola
		unvisitedStack.pop();

		//Toma el nodo de la pareja TDistanceNodePair
		Node * currentAnalizeNode = newNode.second;

		//Extrae la posicion del NODO analizado
		Pos currentAnalizeNodePos = currentAnalizeNode->pos;

		//Determina el INDEX del nodo analizado
		int currentAnalizeNodePosIndex = currentAnalizeNodePos.x * mazeWidth + currentAnalizeNodePos.y;

		//Si la distancia del nodo analizado es INFINITO
		if (distancesStack[currentAnalizeNodePosIndex] == infinity)
			break;

		//Si las posiciones del nodo actual es igual a la del final entonces llegamos al nodo META
		if ((currentAnalizeNodePos.x == endPos.x) && (currentAnalizeNodePos.y == endPos.y))
		{
			completed = true;
			break;
		}

		//For cara cada uno de los nodos adyacentes
		for (int i = 0; i < N_CARDINALS; i++)
		{
			//Extrae el nodo adyacente
			Node * neighborNode = currentAnalizeNode->neighbor[i];

			//Si existe un vecino
			if (neighborNode)
			{
				//Extraemos la posicion del NODO vecino.
				Pos neighborNodePos = neighborNode->pos;

				//Extraemos el INDEX
				int neighborNodePosIndex = neighborNodePos.x * mazeWidth + neighborNodePos.y;

				//Si no fue visitado
				if (!visitedStack[neighborNodePosIndex])
				{
					//Sacamos la distancia del NODO actual al NODO vecino
					int d = abs(neighborNodePos.x - currentAnalizeNodePos.x) + abs(neighborNodePos.y - currentAnalizeNodePos.y);

					//Suma la distancia del nodo actual 
					int newDistance = distancesStack[currentAnalizeNodePosIndex] + d;

					//Si la nueva distancia es menor al nodo vecino
					if (newDistance < distancesStack[neighborNodePosIndex])
					{
						//Obtiene el NODO con mayor distancia (VECINO) y se la remplaza con la menor
						TDistanceNodePair vNode = nodeIndex[neighborNodePosIndex];

						//Si no existe el nodo en la cola de prioridad
						if (!vNode.second)
						{
							TDistanceNodePair vNode = make_pair(newDistance, neighborNode);
							unvisitedStack.push(vNode);
							//Remplaza el nodo con la nueva distancia.
							nodeIndex[neighborNodePosIndex] = vNode;
						}
						else
							//Agrega el nodo si no existia en la cola
							unvisitedStack.push(make_pair(newDistance, vNode.second));
						
						distancesStack[neighborNodePosIndex] = newDistance;
						terminatedStack[neighborNodePosIndex] = currentAnalizeNode;
					}
				}
			}
			//Nodo queda analizado
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
	filename = L"C:\\ProgramacionEstructuras\\ImagenLaberintos\\8.png";
	createGraph();
	solveByDijkstra();
	for (int i = 0; i < resultPath.size(); i++)
		cout << "\nResultado path: ("<< resultPath[i]->pos.x<<","<< resultPath[i]->pos.y<<")";
	cout << endl << endl << "\nNumero de nodos generados: " << nNodesInMaze;
	saveSolveInImage();
	getchar();
	return 0;
}
