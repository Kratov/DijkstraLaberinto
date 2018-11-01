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

char constexpr NODE = 64;
char constexpr WALL = 219;

enum TipoPixel
{
	Camino,
	Pared
};

TipoPixel roadOrWall(const Color & color)
{
	if ((color.GetRed() + color.GetGreen() + color.GetBlue()) > 0)
		return TipoPixel::Camino;
	else
		return TipoPixel::Pared;
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

struct Maze
{

	struct Node {
		COORD pos = {0};
		Node * neighbor[4] = { 0 };
		Node() {}
		Node(COORD pos) 
		{
			this->pos = pos;
		}
	};

	typedef pair<int, Node*> tPair;

	vector<Node*> resultPath;
	Node * start = NULL;
	Node * end = NULL;
	int width = 0;
	int height = 0;
	int count = 0;

	Maze(const wchar_t * filename)
	{
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
		TipoPixel tipoDePixel;

		//Buffer nodos fila superior

		Node ** topNodes = new Node*[image->GetWidth()];
		

		int count = 0;

		//===== Define el nodo de inicio =====
		for (size_t i = 0; i < image->GetWidth(); i++)
		{
			bmpImage->GetPixel(i, 0, &color);
			tipoDePixel = roadOrWall(color);
			if (tipoDePixel == Camino)
			{
				COORD cord;
				cord.X = (short)i;
				cord.Y = (short)0;
				start = new Node(cord);
				topNodes[i] = start;
				count += 1;
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
		for (unsigned int i = 1; i < image->GetHeight() - 1; i++)
		{
			bool next, curr, prev;
			next = curr = prev = false;
			Node * leftNode = NULL;
			//Recorre columnas pixeles
			for (unsigned int j = 0; j < image->GetWidth(); j++)
			{

				//Primer iteracion -- Anterior pared
				prev = curr;
				//El actual pasa a ser el siguiente
				curr = next;
				//Verifica que el siguiente pixel sea camino
				bmpImage->GetPixel(j + 1, i, &color);
				next = roadOrWall(color) == Camino;

				Node * n = NULL;

				//Verifica que el actual no sea pared para no agregar nodo ahi
				if (!curr)
				{
					#ifdef _DEBUG
					gotoxy(j, i); cout << char(WALL); 
					#endif // _DEBUG
					continue;
				}
					

				if (prev)
				{
					if (next)
					{
						// CAMINO(PREV), CAMINO(CURR), CAMINO(NEXT) 
						// Agrega Nodo si existe otro encima o abajo
						bmpImage->GetPixel(j, i - 1, &color);
						bool caminoArriba = roadOrWall(color) == Camino;
						bmpImage->GetPixel(j, i + 1, &color);
						bool caminoAbajo = roadOrWall(color) == Camino;
						if (caminoArriba or caminoAbajo)
						{
							COORD pos;
							pos.X = j;
							pos.Y = i;
							n = new Node(pos);
							leftNode->neighbor[1] = n;
							n->neighbor[3] = leftNode;
							leftNode = n;
						}
					}
					else 
					{
						//CAMINO, CAMINO, PARED
						//Crea nodo al final del corredor.
						COORD pos;
						pos.X = j;
						pos.Y = i;
						n = new Node(pos);
						//Une al Nodo de la izquierda
						leftNode->neighbor[1] = n;
						n->neighbor[3] = leftNode;
						leftNode = NULL;
					}
				}
				else
				{
					if (next)
					{
						//PADED, CAMINO, CAMINO
						COORD pos;
						pos.X = j;
						pos.Y = i;
						n = new Node(pos);
						leftNode = n;
					}
					else
					{
						//PARED, CAMINO, PARED
						//Se crea solo si esta en camino sin salida
						bmpImage->GetPixel(j, i - 1, &color);
						bool caminoArriba = roadOrWall(color) == Camino;
						bmpImage->GetPixel(j, i + 1, &color);
						bool caminoAbajo = roadOrWall(color) == Camino;
						if (!caminoArriba or !caminoAbajo)
						{
							COORD pos;
							pos.X = j;
							pos.Y = i;
							n = new Node(pos);
						}
					}
				}

				if (n)
				{
					#ifdef _DEBUG
					gotoxy(j, i); cout << char(NODE);
					#endif // _DEBUG
					bmpImage->GetPixel(j, i - 1, &color);
					bool caminoArriba = roadOrWall(color) == Camino;
					if (caminoArriba)
					{
						Node * t = topNodes[j];
						t->neighbor[2] = n;
						n->neighbor[0] = t;
					}

					bmpImage->GetPixel(j, i + 1, &color);
					bool caminoAbajo = roadOrWall(color) == Camino;
					if (caminoAbajo) 
						topNodes[j] = n;
					else
						topNodes[j] = NULL;

					count += 1;

				}
			}
		}

		#ifdef _DEBUG
		cout << endl;
		#endif // _DEBUG

		//===== Define el nodo de fin =====
		for (size_t i = 0; i < image->GetWidth(); i++)
		{
			bmpImage->GetPixel(i, image->GetHeight() - 1, &color);
			tipoDePixel = roadOrWall(color);
			if (tipoDePixel == Camino)
			{
				COORD cord;
				cord.X = (SHORT)i;
				cord.Y = (SHORT)image->GetHeight() - 1;
				end = new Node(cord);
				Node * t = topNodes[i];
				t->neighbor[2] = end;
				end->neighbor[0] = t;
				count += 1;
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

		this->count = count;
		this->width = image->GetWidth();
		this->height = image->GetHeight();

		//====================================
		//===== FIN crear nodos en laberinto =====
		//====================================

		//===== Liberar memoria de imagen =====
		delete image; image = 0;

		//===== Terminar GDI+ =====
		GdiplusShutdown(gdiplusToken);
	}

	void solve()
	{
		int width = this->width;
		int total = this->width * this->height;

		Node * start = this->start;
		COORD startPos = start->pos;
		Node * end = this->end;
		COORD endPos = end->pos;

		bool * visited = new bool[total];
		for (int i = 0; i < total; i++)
			visited[i] = false;

		Node ** prev = new Node *[total];
		for (int i = 0; i < total; i++)
			prev[i] = NULL;

		int infinity = INT_MAX;

		int * distances = new int[total];
		for (int i = 0; i < total; i++)
			distances[i] = infinity;

		priority_queue<tPair, vector<tPair>, greater<tPair>> unvisited;
		
		tPair * nodeIndex = new tPair[total];

		distances[start->pos.X * width + start->pos.Y] = 0;
		unvisited.push(make_pair(0, start));

		int count = 0;
		int completed = 0;

		while (unvisited.size() > 0)
		{
			count += 1;

			tPair n = unvisited.top();
			unvisited.pop();

			Node * u = n.second;

			COORD uPos = u->pos;
			int uPosIndex = uPos.X * width + uPos.Y;
			
			if (distances[uPosIndex] == infinity)			
				break;

			if ((uPos.X == endPos.X) && (uPos.Y == endPos.Y))
			{
				completed = true;
				break;
			}

			for (int i = 0; i < 4; i++)
			{
				Node * v = u->neighbor[i];
				if (v)
				{
					COORD vPos = v->pos;
					int vPosIndex = vPos.X * width + vPos.Y;
					if (!visited[vPosIndex])
					{
						int d = abs(vPos.X - uPos.X) + abs(vPos.Y - uPos.Y);
						int newDistance = distances[uPosIndex] + d;

						if (newDistance	< distances[vPosIndex])
						{
							tPair vNode = nodeIndex[vPosIndex];
							if (!vNode.second)
							{
								tPair vNode = make_pair(newDistance, v);
								unvisited.push(vNode);
								nodeIndex[vPosIndex] = vNode;
								distances[vPosIndex] = newDistance;
								prev[vPosIndex] = u;
							}
							else
							{
								unvisited.push(make_pair(newDistance, vNode.second));
								distances[vPosIndex] = newDistance;
								prev[vPosIndex] = u;
							}
						}
					}
				}
				visited[uPosIndex] = true;
			}
		}

		Node * curr = end;
		while (curr)
		{
			resultPath.push_back(curr);
			curr = prev[curr->pos.X * width + curr->pos.Y];
		}
		reverse(resultPath.begin(), resultPath.end());
	}

	void solveImage(const wchar_t * filename) {

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

			Color color = { 100,0,0 };

			if (a->pos.X == b->pos.X)
				for (int j = min(a->pos.Y, b->pos.Y); j < max(a->pos.Y, b->pos.Y) + 1; j++)
					bmpImage->SetPixel(a->pos.X, j, color);
			else if (a->pos.Y == b->pos.Y)
				for (int j = min(a->pos.X, b->pos.X); j < max(a->pos.X, b->pos.X); j++)
					bmpImage->SetPixel(j, a->pos.Y, color);
		}

		CLSID pngClsid;
		CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid);
		image->Save(L"Teminado.png", &pngClsid, NULL);

		//===== Liberar memoria de imagen =====
		delete image; image = 0;

		//===== Terminar GDI+ =====
		GdiplusShutdown(gdiplusToken);
	}
};

int main()
{
	Maze laberinto(L"C:\\ProgramacionEstructuras\\ImagenLaberintos\\perfect15k.png");
	cout << endl << endl << "\nNumero de nodos generados: " << laberinto.count;
	laberinto.solve();
	for (int i = 0; i < laberinto.resultPath.size(); i++)
	{
		cout << "\nResultado path: ("<< laberinto.resultPath[i]->pos.X<<","<< laberinto.resultPath[i]->pos.Y<<")";
	}

	laberinto.solveImage(L"C:\\ProgramacionEstructuras\\ImagenLaberintos\\perfect15k.png");
	getchar();
	return 0;
}
