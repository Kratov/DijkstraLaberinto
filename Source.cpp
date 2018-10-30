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
		COORD pos;
		Node * neighbor[4] = { 0 };
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
				break;
			}
		}


		//===== Nodos entre laberinto =====
		for (unsigned int i = 1; i < image->GetHeight(); i++)
		{
			for (unsigned int j = 0; j < image->GetWidth(); j++)
			{
				
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
		//====================================


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
	Maze laberinto(L"C:\\ProyectoGrafos\\Laberinto.png");
	getchar();
	return 0;
}
