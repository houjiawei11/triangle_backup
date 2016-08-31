#ifndef __HJW_FUNCTION__
#define __HJW_FUNCTION__


#ifdef WIN32
#include <windows.h>
#endif
#include <glut.h>
#include "teddy.h"
#include "matrix.h"
#include "OpenGLProjector.h"
#include <triangle.h>



using namespace std;
float distanceofstrokePoints(int , Vector2f , Vector2f );
bool segment_intersection(Vector2f , Vector2f, Vector2f, Vector2f);
void triangulateio_init( vector<Vector2f> ,  triangulateio &,  triangulateio &);

void triangulateio_free_in(triangulateio &);
void triangulateio_free_out(triangulateio &);
#endif