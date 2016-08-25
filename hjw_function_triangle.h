#ifdef WIN32
#include <windows.h>
#endif
#include <glut.h>
#include "teddy.h"
#include "matrix.h"
#include "OpenGLProjector.h"
using namespace std;
float distanceofstrokePoints(int , Vector2f , Vector2f );
bool segment_intersection(Vector2f , Vector2f, Vector2f, Vector2f);