/*
*  tetgen-based raytracing library in a single header
*  Copyright (C) 2016  Christian Lehmann
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*/

#ifndef TETGEN_MESHTRAVERSAL_H
#define TETGEN_MESHTRAVERSAL_H

#include <string>
#include <random>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <ctime>

#define epsilon 1e-8
#define inf 1e20

typedef int int32_t;
typedef unsigned int uint32_t;

struct float4
{
  float x,y,z,w;
  inline  float4 operator-=(float4 &a, const float4 b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; a.w -= b.w;	return make_float4(0, 0, 0, 0); }
  inline  float4 operator+(const float4 &a, const float4 &b) { return make_float4(a.x + b.x, a.y + b.y, a.z + b.z, 0); }
  inline  float4 operator-(const float4 &a, const float4 &b) { return make_float4(a.x - b.x, a.y - b.y, a.z - b.z, 0); }
  inline  float4 operator*(float4 &a, float4 &b) { return make_float4(a.x * b.x, a.y * b.y, a.z * b.z, 0); }
  inline  float4 operator*(const float4 &a, const float &b) { return make_float4(a.x*b, a.y*b, a.z*b, 0); }
  inline  float4 operator*(const float &b, const float4 &a) { return make_float4(a.x * b, a.y * b, a.z * b, 0); }
  inline  void operator*=(float4 &a, float4 b) { a.x *= b.x; a.y *= b.y; a.z *= b.z; }
  inline  void operator*=(float4 &a, float &b) { a.x *= b; a.y *= b; a.z *= b; }
  inline  float4 operator/(const float4 &a, const float &b) { return make_float4(a.x / b, a.y / b, a.z / b, 0); }
  inline  float4 operator+=(float4 &a, const float4 b) { a.x += b.x; a.y += b.y; a.z += b.z; return make_float4(0, 0, 0, 0); }
}


float4 normalize(float4 &a)
{
	float f = 1/sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
	return make_float4(a.x*f, a.y*f, a.z*f, 0);
}

float Dot(const float4 &a, const float4 &b)
{
	return  a.x * b.x + a.y * b.y + a.z * b.z;
}

float4 reflect(const float4 &i,const float4 &n)
{
	return i - 2.0f * n * Dot(n, i);
}

float4 Cross(const float4 &a, const float4 &b)
{
	return make_float4( a.y * b.z - a.z * b.y,
						a.z * b.x - a.x * b.z,
						a.x * b.y - a.y * b.x, 0);
}

float ScTP(const float4 &a, const float4 &b, const float4 &c)
{
	// computes scalar triple product
	return Dot(a, Cross(b, c));
}

int signf(float f)
{
	if (f > 0.0) return 1;
	if (f < 0.0) return -1;
	return 0;
}

bool SameSide(const float4 &v1, const float4 &v2, const float4 &v3, const float4 &v4, const float4 &p)
{
	float4 normal = Cross(v2 - v1, v3 - v1);
	float dotV4 = Dot(normal, v4 - v1);
	float dotP = Dot(normal, p - v1);
	return signf(dotV4) == signf(dotP);
}

struct rayhit
{
	float4 pos;
	int32_t tet = 0;
	int32_t face = 0;
	int depth = 0;
	bool wall = false;
	bool constrained = false;
	bool dark = false; // if hitpoint is too far away
};

struct BBox
{
	float4 min, max;
};

struct node
{
	uint32_t index;
	float x, y, z;
	float4 f_node(){ return make_float4(x, y, z, 0); }
};

struct edge
{
	uint32_t index;
	uint32_t node1, node2;
};

struct face
{
	uint32_t index;
	uint32_t node_a, node_b, node_c;
	bool face_is_constrained = false;
	bool face_is_wall = false;
};


struct tetrahedra
{
	uint32_t number;
	int32_t findex1, findex2, findex3, findex4;
	int32_t nindex1, nindex2, nindex3, nindex4;
	int32_t adjtet1, adjtet2, adjtet3, adjtet4;
};

class tetrahedra_mesh
{
public:
	uint32_t tetnum, nodenum, facenum, edgenum;
	std::deque<tetrahedra>tetrahedras;
	std::deque<node>nodes;
	std::deque<face>faces;
	std::deque<edge>edges;
	uint32_t max = 1000000000;

	void load_tet_neigh(std::string filename);
	void load_tet_ele(std::string filename);
	void load_tet_node(std::string filename);
	void load_tet_face(std::string filename);
	void load_tet_t2f(std::string filename);
	void load_tet_edge(std::string filename);
};

void tetrahedra_mesh::load_tet_ele(std::string filename)
{
	uint32_t num = 0;
	tetrahedra tet1;
	std::string line;
	std::ifstream myfile(filename);
	if (myfile.is_open())
	{
		while (std::getline(myfile, line) && num<max) // Nur die ersten tausend Zeilen einlesen
		{
			std::stringstream in(line);
			std::vector<int32_t> ints;
			copy(std::istream_iterator<int32_t, char>(in), std::istream_iterator<int32_t, char>(), back_inserter(ints));
			if (num == 0) //Erste Zeile
			{
				tetnum = ints.at(0); //In erster Zeile der .ele-Datei ist Anzahl der Tetraheder abgelegt
				tetrahedras.resize(tetnum, tet1); //Tetrahedra-Deque füllen
			}
			else if (ints.size() != NULL) // restliche Zeilen
			{
				tetrahedras.at(ints.at(0)).number = ints.at(0); //nummer von aktuellem tetrahedra
				tetrahedras.at(ints.at(0)).nindex1 = ints.at(1);
				tetrahedras.at(ints.at(0)).nindex2 = ints.at(2);
				tetrahedras.at(ints.at(0)).nindex3 = ints.at(3);
				tetrahedras.at(ints.at(0)).nindex4 = ints.at(4);
			}
			num++;
		}
		myfile.close();
	}
	else std::cout << "Unable to open .ele file";
	fprintf_s(stderr, "Total number of tetrahedra in .ele-file: %u \n", num);
}


void tetrahedra_mesh::load_tet_neigh(std::string filename)
{
	uint32_t num = 0;
	std::string line;
	std::ifstream myfile(filename);
	if (myfile.is_open())
	{
		while (std::getline(myfile, line) && num<max) // Nur die ersten tausend Zeilen einlesen
		{
			std::stringstream in(line);
			std::vector<int32_t> ints;
			copy(std::istream_iterator<int32_t, char>(in), std::istream_iterator<int32_t, char>(), back_inserter(ints));
			if (num != 0 && ints.size() != NULL)
			{
				tetrahedras.at(ints.at(0)).adjtet1 = ints.at(1);
				tetrahedras.at(ints.at(0)).adjtet2 = ints.at(2);
				tetrahedras.at(ints.at(0)).adjtet3 = ints.at(3);
				tetrahedras.at(ints.at(0)).adjtet4 = ints.at(4);
			}
			num++;
		}
		myfile.close();
	}
	else std::cout << "Unable to open .neigh file";
	fprintf_s(stderr, "Total number of tetrahedra in .neigh-file: %u \n", num);
}



void tetrahedra_mesh::load_tet_node(std::string filename)
{
	uint32_t num = 0;
	node nd1;
	std::string line;
	std::ifstream myfile(filename);
	if (myfile.is_open())
	{
		while (std::getline(myfile, line) && num<max) // Nur die ersten tausend Zeilen einlesen
		{
			std::stringstream in(line);
			std::vector<float> ints;
			copy(std::istream_iterator<float, char>(in), std::istream_iterator<float, char>(), back_inserter(ints));
			if (num == 0) //Erste Zeile
			{
				nodenum = int(ints.at(0)); //In erster Zeile der .ele-Datei ist Anzahl der Tetraheder abgelegt
				nodes.resize(nodenum, nd1); //Tetrahedra-Deque füllen
			}
			else if (ints.size() != NULL) // restliche Zeilen
			{
				nodes.at((int)ints.at(0)).index = ints.at(0);
				nodes.at((int)ints.at(0)).x = ints.at(1);
				nodes.at((int)ints.at(0)).y = ints.at(2);
				nodes.at((int)ints.at(0)).z = ints.at(3);
			}
			num++;
		}
		myfile.close();
	}
	else std::cout << "Unable to open .node file";
	fprintf_s(stderr, "Total number of Nodes in .node-file: %u \n", num);
}


void tetrahedra_mesh::load_tet_face(std::string filename)
{
	uint32_t num = 0;
	face fc1;
	std::string line;
	std::ifstream myfile(filename);
	if (myfile.is_open())
	{
		while (std::getline(myfile, line) && num<max) // Nur die ersten tausend Zeilen einlesen
		{
			std::stringstream in(line);
			std::vector<int32_t> ints;
			copy(std::istream_iterator<int32_t, char>(in), std::istream_iterator<int32_t, char>(), back_inserter(ints));
			if (num == 0) //Erste Zeile
			{
				facenum = int(ints.at(0)); //In erster Zeile der .ele-Datei ist Anzahl der Tetraheder abgelegt
				faces.resize(facenum, fc1); //Tetrahedra-Deque füllen
			}
			else if (ints.size() != NULL) // restliche Zeilen
			{
				faces.at(ints.at(0)).index = ints.at(0);
				faces.at(ints.at(0)).node_a = ints.at(1);
				faces.at(ints.at(0)).node_b = ints.at(2);
				faces.at(ints.at(0)).node_c = ints.at(3);



				if (ints.at(5) == -1 || ints.at(6) == -1) { faces.at(ints.at(0)).face_is_wall = true; }
				else if (ints.at(4) == -1) faces.at(ints.at(0)).face_is_constrained = true;
			}
			num++;
		}
		myfile.close();
	}
	else std::cout << "Unable to open .face file";
	fprintf_s(stderr, "Total number of Faces in .face-file: %u \n", num);
}



void tetrahedra_mesh::load_tet_edge(std::string filename)
{
	uint32_t num = 0;
	edge ed1;
	std::string line;
	std::ifstream myfile(filename);
	if (myfile.is_open())
	{
		while (std::getline(myfile, line) && num<max) // Nur die ersten tausend Zeilen einlesen
		{
			std::stringstream in(line);
			std::vector<int32_t> ints;
			copy(std::istream_iterator<int32_t, char>(in), std::istream_iterator<int32_t, char>(), back_inserter(ints));
			if (num == 0) //Erste Zeile
			{
				edgenum = int(ints.at(0)); //In erster Zeile der .ele-Datei ist Anzahl der Tetraheder abgelegt
				edges.resize(edgenum, ed1); //Tetrahedra-Deque füllen
			}
			else if (ints.size() != NULL) // restliche Zeilen
			{
				edges.at(ints.at(0)).index = ints.at(0);
				edges.at(ints.at(0)).node1 = ints.at(1);
				edges.at(ints.at(0)).node1 = ints.at(2);
			}
			num++;
		}
		myfile.close();
	}
	else std::cout << "Unable to open .edge file";
	fprintf_s(stderr, "Total number of Edges in .edge-file: %u \n", num);
}


void tetrahedra_mesh::load_tet_t2f(std::string filename)
{
	uint32_t num = 0;
	std::string line;
	std::ifstream myfile(filename);
	if (myfile.is_open())
	{
		while (std::getline(myfile, line) && num<max) // Nur die ersten tausend Zeilen einlesen
		{
			std::stringstream in(line);
			std::vector<int32_t> ints;
			copy(std::istream_iterator<int32_t, char>(in), std::istream_iterator<int32_t, char>(), back_inserter(ints));

			if (ints.size() != NULL) // alle Zeilen
			{
				tetrahedras.at(ints.at(0) - 1).findex1 = ints.at(1);
				tetrahedras.at(ints.at(0) - 1).findex2 = ints.at(2);
				tetrahedras.at(ints.at(0) - 1).findex3 = ints.at(3);
				tetrahedras.at(ints.at(0) - 1).findex4 = ints.at(4);
			}
			num++;
		}
		myfile.close();
	}
	else std::cout << "Unable to open .t2f file";
	fprintf_s(stderr, "Total number of Tetrahedra in .t2f-file: %u \n", num);
}

//--------------------------------------------------------------------------------------------------------------------------------------


bool IsPointInTetrahedron(float4 v1, float4 v2, float4 v3, float4 v4, float4 p)
{
		// https://stackoverflow.com/questions/25179693/how-to-check-whether-the-point-is-in-the-tetrahedron-or-not/25180158#25180158
		return SameSide(v1, v2, v3, v4, p) &&
		SameSide(v2, v3, v4, v1, p) &&
		SameSide(v3, v4, v1, v2, p) &&
		SameSide(v4, v1, v2, v3, p);
}

bool IsPointInThisTet(mesh2* mesh, float4 v, int32_t tet)
{
	float4 nodes[4] = {
		make_float4(mesh->n_x[mesh->t_nindex1[tet]], mesh->n_y[mesh->t_nindex1[tet]], mesh->n_z[mesh->t_nindex1[tet]], 0),
		make_float4(mesh->n_x[mesh->t_nindex2[tet]], mesh->n_y[mesh->t_nindex2[tet]], mesh->n_z[mesh->t_nindex2[tet]], 0),
		make_float4(mesh->n_x[mesh->t_nindex3[tet]], mesh->n_y[mesh->t_nindex3[tet]], mesh->n_z[mesh->t_nindex3[tet]], 0),
		make_float4(mesh->n_x[mesh->t_nindex4[tet]], mesh->n_y[mesh->t_nindex4[tet]], mesh->n_z[mesh->t_nindex4[tet]], 0) };
	if (IsPointInTetrahedron(nodes[0], nodes[1], nodes[2], nodes[3], v) == true) return true;
	else return false;
}

int32_t GetTetrahedraFromPoint(mesh2* mesh, float4 p)
{
		for (int i=0;i<mesh->tetnum;i++)
    {
			float4 v1 = make_float4(mesh->n_x[mesh->t_nindex1[i]], mesh->n_y[mesh->t_nindex1[i]], mesh->n_z[mesh->t_nindex1[i]], 0);
			float4 v2 = make_float4(mesh->n_x[mesh->t_nindex2[i]], mesh->n_y[mesh->t_nindex2[i]], mesh->n_z[mesh->t_nindex2[i]], 0);
			float4 v3 = make_float4(mesh->n_x[mesh->t_nindex3[i]], mesh->n_y[mesh->t_nindex3[i]], mesh->n_z[mesh->t_nindex3[i]], 0);
			float4 v4 = make_float4(mesh->n_x[mesh->t_nindex4[i]], mesh->n_y[mesh->t_nindex4[i]], mesh->n_z[mesh->t_nindex4[i]], 0);
			if (IsPointInTetrahedron(v1, v2, v3, v4, p) == true) return i;
		}
}

BBox init_BBox(mesh2* mesh)
{
	BBox boundingbox;
	boundingbox.min = make_float4(-inf, -inf, -inf, 0);
	boundingbox.max = make_float4(inf, inf, inf, 0);
	for (uint32_t i = 0; i < mesh->nodenum; i++)
	{
		if (boundingbox.min.x < mesh->n_x[i])  boundingbox.min.x = mesh->n_x[i];
		if (boundingbox.max.x > mesh->n_x[i])  boundingbox.max.x = mesh->n_x[i];
		if (boundingbox.min.y < mesh->n_y[i])  boundingbox.min.y = mesh->n_y[i];
		if (boundingbox.max.y > mesh->n_y[i])  boundingbox.max.y = mesh->n_y[i];
		if (boundingbox.min.z < mesh->n_z[i])  boundingbox.min.z = mesh->n_z[i];
		if (boundingbox.max.z > mesh->n_z[i])  boundingbox.max.z = mesh->n_z[i];
	}
	return boundingbox;
}

void ConstrainToBBox(BBox* boundingbox, float4 &p)
{
	if (boundingbox->min.x + 0.2 < p.x)  p.x = boundingbox->min.x;
	if (boundingbox->max.x - 0.2 > p.x)  p.x = boundingbox->max.x;
	if (boundingbox->min.y + 0.2 < p.y)  p.y = boundingbox->min.y;
	if (boundingbox->max.y - 0.2 > p.y)  p.y = boundingbox->max.y;
	if (boundingbox->min.z + 0.2 < p.z)  p.z = boundingbox->min.z;
	if (boundingbox->max.z - 0.2 > p.z)  p.z = boundingbox->max.z;
}

void GetExitTet(float4 ray_o, float4 ray_d, float4* nodes, int32_t findex[4], int32_t adjtet[4], int32_t lface, int32_t &face, int32_t &tet)
{
	face = 0;
	tet = 0;

	// http://realtimecollisiondetection.net/blog/?p=13
	// and https://github.com/JKolios/RayTetra/blob/master/RayTetra/RayTetraSTP0.cl

	// translate Ray to origin and vertices same as ray
	float4 q = ray_d;

	float4 v0 = make_float4(nodes[0].x, nodes[0].y, nodes[0].z, 0); // A
	float4 v1 = make_float4(nodes[1].x, nodes[1].y, nodes[1].z, 0); // B
	float4 v2 = make_float4(nodes[2].x, nodes[2].y, nodes[2].z, 0); // C
	float4 v3 = make_float4(nodes[3].x, nodes[3].y, nodes[3].z, 0); // D

	float4 p0 = v0 - ray_o;
	float4 p1 = v1 - ray_o;
	float4 p2 = v2 - ray_o;
	float4 p3 = v3 - ray_o;

	double QAB = ScTP(q, p0, p1); // A B
	double QBC = ScTP(q, p1, p2); // B C
	double QAC = ScTP(q, p0, p2); // A C
	double QAD = ScTP(q, p0, p3); // A D
	double QBD = ScTP(q, p1, p3); // B D
	double QCD = ScTP(q, p2, p3); // C D

	double sQAB = signf(QAB); // A B
	double sQBC = signf(QBC); // B C
	double sQAC = signf(QAC); // A C
	double sQAD = signf(QAD); // A D
	double sQBD = signf(QBD); // B D
	double sQCD = signf(QCD); // C D

	// ABC
	if (sQAB != 0 && sQAC !=0 && sQBC != 0)
	{
		if (sQAB < 0 && sQAC > 0 && sQBC < 0) { face = findex[3]; tet = adjtet[3]; } // exit face
	}
	// BAD
	if (sQAB != 0 && sQAD != 0 && sQBD != 0)
	{
		if (sQAB > 0 && sQAD < 0 && sQBD > 0) { face = findex[2]; tet = adjtet[2]; } // exit face
	}
	// CDA
	if (sQAD != 0 && sQAC != 0 && sQCD != 0)
	{
		if (sQAD > 0 && sQAC < 0 && sQCD < 0) { face = findex[1]; tet = adjtet[1]; } // exit face
	}
	// DCB
	if (sQBC != 0 && sQBD != 0 && sQCD != 0)
	{
		if (sQBC > 0 && sQBD < 0 && sQCD > 0) { face = findex[0]; tet = adjtet[0]; } // exit face
	}
	// No face hit
	// if (face == 0 && tet == 0) { printf("Error! No exit tet found. \n"); }
}


/* traverse the mesh until a 'wall' or 'constrained' triangle face is found */
/* the indices of the face and the tetrahedron which are hit are stored in the rayhit structure */
void traverse_ray(mesh2 *mesh, float4 rayo, float4 rayd, int32_t start, rayhit &d)
{
	int32_t current_tet = start;
	int32_t nexttet, nextface, lastface = 0;
	bool hitfound = false;

	for (d.depth = 0; d.depth < 80; d.depth++)
	{
		if (!hitfound)
		{
			int32_t findex[4] = { mesh->t_findex1[current_tet], mesh->t_findex2[current_tet], mesh->t_findex3[current_tet], mesh->t_findex4[current_tet] };
			int32_t adjtets[4] = { mesh->t_adjtet1[current_tet], mesh->t_adjtet2[current_tet], mesh->t_adjtet3[current_tet], mesh->t_adjtet4[current_tet] };
			float4 nodes[4] = {
				make_float4(mesh->n_x[mesh->t_nindex1[current_tet]], mesh->n_y[mesh->t_nindex1[current_tet]], mesh->n_z[mesh->t_nindex1[current_tet]], 0),
				make_float4(mesh->n_x[mesh->t_nindex2[current_tet]], mesh->n_y[mesh->t_nindex2[current_tet]], mesh->n_z[mesh->t_nindex2[current_tet]], 0),
				make_float4(mesh->n_x[mesh->t_nindex3[current_tet]], mesh->n_y[mesh->t_nindex3[current_tet]], mesh->n_z[mesh->t_nindex3[current_tet]], 0),
				make_float4(mesh->n_x[mesh->t_nindex4[current_tet]], mesh->n_y[mesh->t_nindex4[current_tet]], mesh->n_z[mesh->t_nindex4[current_tet]], 0) };

			GetExitTet(rayo, rayd, nodes, findex, adjtets, lastface, nextface, nexttet);

			if (mesh->face_is_constrained[nextface] == true) { d.constrained = true; d.face = nextface; d.tet = current_tet; hitfound = true; } // vorher tet = nexttet
			if (mesh->face_is_wall[nextface] == true) { d.wall = true; d.face = nextface; d.tet = current_tet; hitfound = true; } // vorher tet = nexttet
			if (nexttet == -1 || nextface == -1) { d.wall = true; d.face = nextface; d.tet = current_tet; hitfound = true; } // when adjacent tetrahedra is -1, ray stops
			lastface = nextface;
			current_tet = nexttet;
		}
	}

	// get nodes from nextface
	//float4 na = make_float4(mesh->n_x[mesh->f_node_a[nextface]], mesh->n_y[mesh->f_node_a[nextface]], mesh->n_z[mesh->f_node_a[nextface]], 0);
	//float4 nb = make_float4(mesh->n_x[mesh->f_node_b[nextface]], mesh->n_y[mesh->f_node_b[nextface]], mesh->n_z[mesh->f_node_b[nextface]], 0);
	//float4 nc = make_float4(mesh->n_x[mesh->f_node_c[nextface]], mesh->n_y[mesh->f_node_c[nextface]], mesh->n_z[mesh->f_node_c[nextface]], 0);

	if (!hitfound)
	{
		d.dark = true;
		d.face = nextface;
		d.tet = current_tet;
	}
}

/* traverse the mesh until the tetrahedron which contains the specific point 'end' is found */
void traverse_until_point(mesh2 *mesh, float4 rayo, float4 rayd, int32_t start, float4 end, rayhit &d)
{
	int32_t current_tet = start;
	int32_t nexttet, nextface, lastface = 0;
	bool hitfound = false;

	for (d.depth = 0; d.depth < 80; d.depth++)
	{
		if (!hitfound)
		{
			int32_t findex[4] = { mesh->t_findex1[current_tet], mesh->t_findex2[current_tet], mesh->t_findex3[current_tet], mesh->t_findex4[current_tet] };
			int32_t adjtets[4] = { mesh->t_adjtet1[current_tet], mesh->t_adjtet2[current_tet], mesh->t_adjtet3[current_tet], mesh->t_adjtet4[current_tet] };
			float4 nodes[4] = {
				make_float4(mesh->n_x[mesh->t_nindex1[current_tet]], mesh->n_y[mesh->t_nindex1[current_tet]], mesh->n_z[mesh->t_nindex1[current_tet]], 0),
				make_float4(mesh->n_x[mesh->t_nindex2[current_tet]], mesh->n_y[mesh->t_nindex2[current_tet]], mesh->n_z[mesh->t_nindex2[current_tet]], 0),
				make_float4(mesh->n_x[mesh->t_nindex3[current_tet]], mesh->n_y[mesh->t_nindex3[current_tet]], mesh->n_z[mesh->t_nindex3[current_tet]], 0),
				make_float4(mesh->n_x[mesh->t_nindex4[current_tet]], mesh->n_y[mesh->t_nindex4[current_tet]], mesh->n_z[mesh->t_nindex4[current_tet]], 0) };

			GetExitTet(rayo, rayd, nodes, findex, adjtets, lastface, nextface, nexttet);

			if (IsPointInTetrahedron(nodes[0], nodes[1], nodes[2], nodes[3], end)) { hitfound = true;d.face = nextface; d.tet = current_tet;  }

			if (mesh->face_is_constrained[nextface] == true) { d.constrained = true; d.face = nextface; d.tet = current_tet; hitfound = true; } // vorher tet = nexttet
			if (mesh->face_is_wall[nextface] == true) { d.wall = true; d.face = nextface; d.tet = current_tet; hitfound = true; } // vorher tet = nexttet
			if (nexttet == -1 || nextface == -1) { d.wall = true; d.face = nextface; d.tet = current_tet; hitfound = true; } // when adjacent tetrahedra is -1, ray stops
			lastface = nextface;
			current_tet = nexttet;
		}
	}

	if (!hitfound)
	{
		d.dark = true;
		d.face = nextface;
		d.tet = current_tet;
	}
}



//----------------------- obj parser -----------------------------------------------------------------

int loadObj(std::string inputfile)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str());
	if (!err.empty()) {	std::cerr << err << std::endl; }
	if (!ret) {	exit(1); }

	std::cout << "# of shapes    : " << shapes.size() << std::endl;
	std::cout << "# of materials : " << materials.size() << std::endl;

	for (uint32_t i = 0; i < shapes.size(); i++) {
		printf("shape[%ld].name = %s\n", i, shapes[i].name.c_str());
		printf("Size of shape[%ld].indices: %ld\n", i, shapes[i].mesh.indices.size());
		printf("Size of shape[%ld].material_ids: %ld\n", i, shapes[i].mesh.material_ids.size());
		assert((shapes[i].mesh.indices.size() % 3) == 0);
		for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
			printf("  idx[%ld] = %d, %d, %d. mat_id = %d\n", f, shapes[i].mesh.indices[3 * f + 0], shapes[i].mesh.indices[3 * f + 1], shapes[i].mesh.indices[3 * f + 2], shapes[i].mesh.material_ids[f]);
		}

		printf("shape[%ld].vertices: %ld\n", i, shapes[i].mesh.positions.size());
		assert((shapes[i].mesh.positions.size() % 3) == 0);
		for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++) {
			printf("  v[%ld] = (%f, %f, %f)\n", v,
				shapes[i].mesh.positions[3 * v + 0],
				shapes[i].mesh.positions[3 * v + 1],
				shapes[i].mesh.positions[3 * v + 2]);
		}
	}



}


#endif
