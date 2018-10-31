#include<Windows.h>
#include <iostream>

#pragma comment( lib, "gdiplus.lib" ) 
#include <gdiplus.h> 

using namespace Gdiplus;
using namespace std;

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

	Node * start = NULL;
	Node * end = NULL;

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

		//===== Define el nodo de inicio =====
		for (size_t i = 0; i < image->GetWidth(); i++)
		{
			bmpImage->GetPixel(i, 0, &color);
			tipoDePixel = roadOrWall(color);
			if (tipoDePixel == Camino)
			{
				COORD cord;
				cord.X = i;
				cord.Y = 0;
				start = new Node(cord);
				topNodes[i] = start;
				break;
			}
		}


		//===== Nodos entre laberinto =====
		//Recorre filas pixeles
		for (unsigned int i = 1; i < image->GetHeight(); i++)
		{
			bool next, curr, prev;
			next = curr = prev = false;

			//Recorre columnas pixeles
			for (unsigned int j = 0; j < image->GetWidth(); j++)
			{
				Node * leftNode = NULL;

				//Verifica que el siguiente pixel sea camino
				bmpImage->GetPixel(j + 1, i, &color);
				next = roadOrWall(color) == Camino;
				//Primer iteracion -- Anterior pared
				prev = curr;

				//El actual pasa a ser el siguiente
				curr = next;

				Node * n = NULL;

				//Verifica que el actual no sea pared para no agregar nodo ahi
				if (!curr)
					continue;

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
							pos.X = i;
							pos.Y = j;
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
						pos.X = i;
						pos.Y = j;
						n = new Node(pos);
						//Une al Nodo de la izquierda
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
						pos.X = i;
						pos.Y = j;
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
							pos.X = i;
							pos.Y = j;
							n = new Node(pos);
						}
					}
				}

				if (n)
				{
					if (true)
					{
						Node * t = topNodes[j];
						t->neighbor[2] = n;
						n->neighbor[0] = t;
					}

					bmpImage->GetPixel(j, i + 1, &color);
					bool caminoAbajo = roadOrWall(color) == Camino;
					if (caminoAbajo > 0) 
						topNodes[j] = n;
					else
						topNodes[j] = NULL;

				}
			}
		}

		//===== Define el nodo de inicio =====
		for (size_t i = 0; i < image->GetWidth(); i++)
		{
			bmpImage->GetPixel(i, image->GetHeight() - 1, &color);
			tipoDePixel = roadOrWall(color);
			if (tipoDePixel == Camino)
			{
				COORD cord;
				cord.X = i;
				cord.Y = 0;
				end = new Node(cord);
				break;
			}
		}

		//====================================
		//===== FIN crear nodos en laberinto =====
		//====================================

		//====================================
		//===== Imprimir laberinto		 =====
		//====================================;


		//===== Imprimir fila start =====
		for (size_t i = 0; i < image->GetWidth(); i++)
		{
			bmpImage->GetPixel(i, 0, &color);
			tipoDePixel = roadOrWall(color);

			if (i == start->pos.X)			
				cout << char(64);
			else
			if (tipoDePixel == Camino)
				cout << " ";
			else
				cout << char(219);
		}

		cout << endl;

		//===== Imprimir fila Between =====
		for (unsigned int i = 1; i < image->GetHeight() - 1; i++)
		{
			if(i != 1) cout << endl;
			for (unsigned int j = 0; j < image->GetWidth(); j++)
			{
				bmpImage->GetPixel(j, i, &color);
				tipoDePixel = roadOrWall(color);
				if (tipoDePixel == Camino)
					cout << " ";
				else
					cout << char(219);
			}
		}

		cout << endl;

		//===== Imprimir fila end =====
		for (size_t i = 0; i < image->GetWidth(); i++)
		{
			bmpImage->GetPixel(i, image->GetHeight() - 1, &color);
			tipoDePixel = roadOrWall(color);
			if ( end && i == end->pos.X)
				cout << char(64);
			else
			if (tipoDePixel == Camino)
				cout << " ";
			else
				cout << char(219);
		}

		//====================================
		//===== FIN Imprimir laberinto		 =====
		//====================================

		//===== Liberar memoria de imagen =====
		delete image; image = 0;

		//===== Terminar GDI+ =====
		GdiplusShutdown(gdiplusToken);
	}
};

int main()
{
	Maze laberinto(L"C:\\ProgramacionEstructuras\\ImagenLaberintos\\tiny.png");
	getchar();
	return 0;
}
