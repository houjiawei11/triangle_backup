#ifdef WIN32
#include <windows.h>
#endif
#include <glut.h>
#include "teddy.h"
#include "matrix.h"
#include "OpenGLProjector.h"
using namespace std;

#define REAL double
#define ANSI_DECLARATORS
#include "triangle.h"
#include <hjw_function_triangle.h>

triangulateio in, out, vorout;

// the way the mesh is rendered
enum EnumDisplayMode 
{ 
	WIREFRAME, 
	HIDDENLINE, 
	FLATSHADED, 
	SMOOTHSHADED, 
	COLORSMOOTHSHADED 
};

// variables
int displayMode = SMOOTHSHADED;					// current display mode
int mainMenu, displayMenu;						// glut menu handlers
int winWidth, winHeight;						// window width and height
double winAspect;								// winWidth / winHeight;
int lastX, lastY;								// last mouse motion position
bool leftDown, middleDown, shiftDown,leftUp;	// mouse down and shift down flags
double sphi = 90.0, stheta = 45.0, sdepth = 10;	// for simple trackball
double xpan = 0.0, ypan = 0.0;					// for simple trackball
double zNear = 1.0, zFar = 100.0;				// clipping
double g_fov = 45.0;
Vector3d g_center;
double g_sdepth;
Mesh mesh;	// our mesh, this is for demonstration purpose, remove when you code
#define sample_segment 21

vector<Vector2f> strokePoints;					// 2d stroke points
vector<Vector2f> strokepts_sampled;
vector<Vector2f> strokepts_sam_saved;
Teddy teddy;
bool stroke_flag = 0;
bool sample_flag = 0;
bool intersect_flag = false;
bool triangulate_flag = false;



// editing mode
enum Mode 
{ 
	Viewing, 
	Sketching, 
	Moving 
};
Mode currentMode = Sketching;
int downX, downY;								// mouse down position
int selectedHandleIndex = -1;					// the index of the handle

// functions
void SetBoundaryBox(const Vector3d & bmin, const Vector3d & bmax);
void InitGL();
void InitMenu();
void InitGeometry();

// window related 
void MenuCallback(int value);
void ReshapeFunc(int width, int height);

// rendering functions
void DisplayFunc();
void DrawWireframe();
void DrawHiddenLine();
void DrawFlatShaded();
void DrawSmoothShaded();
void DrawColorSmoothShaded();
void DrawStrokePoints();
void DrawStrokePoints_Sample();
void DrawSampledPoints();
void StrokePoints_Sample();
void drawtriangle(vector<Vector2f>, triangulateio &);

// input related glut functions
void KeyboardFunc(unsigned char ch, int x, int y);
void MouseFunc(int button, int state, int x, int y);
void MotionFunc(int x, int y);

void SelectVertexByRect();						// select ROI for editing
int  StartMove();								// a single editing operation


void SetBoundaryBox(const Vector3d & bmin, const Vector3d & bmax) 
{
	double PI = 3.14159265358979323846;
	double radius = bmax.Distance(bmin);
	g_center = 0.5 * (bmin+bmax);
	zNear    = 0.2 * radius / sin(0.5 * g_fov * PI / 180.0);
	zFar     = zNear + 2.0 * radius;
	g_sdepth = zNear + radius;
	zNear *= 0.1;
	zFar *= 10;
	sdepth = g_sdepth;
}

// init openGL environment
void InitGL() 
{
	GLfloat light0Position[] = { 0, 1, 0, 1.0 }; 

	// initialize GLUT stuffs
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(500, 500);
	glutCreateWindow("icg: mesh viewer");

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glPolygonOffset(1.0, 1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glLightfv (GL_LIGHT0, GL_POSITION, light0Position);
	glEnable(GL_LIGHT0);

	glutReshapeFunc(ReshapeFunc);
	glutDisplayFunc(DisplayFunc);
	glutKeyboardFunc(KeyboardFunc);
	glutMouseFunc(MouseFunc);
	glutMotionFunc(MotionFunc);
}

// init right-click menu
void InitMenu() 
{
	displayMenu = glutCreateMenu(MenuCallback);
	glutAddMenuEntry("Wireframe", WIREFRAME);
	glutAddMenuEntry("Hidden Line", HIDDENLINE);
	glutAddMenuEntry("Flat Shaded", FLATSHADED);
	glutAddMenuEntry("Smooth Shaded", SMOOTHSHADED);
	glutAddMenuEntry("Color Smooth Shaded", COLORSMOOTHSHADED);

	mainMenu = glutCreateMenu(MenuCallback);
	glutAddSubMenu("Display", displayMenu);
	glutAddMenuEntry("Exit", 99);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// init geometry (if no input argument is provided)
void InitGeometry() 
{
	const int VSIZE = 4;
	const int HESIZE = 12;
	const int FSIZE = 4;
	int i;
	Vertex *v[VSIZE];
	HEdge *he[HESIZE];
	Face *f[FSIZE];

	for (i=0; i<VSIZE; i++) {
		v[i] = new Vertex();
		mesh.vList.push_back(v[i]);
	}
	v[0]->SetPosition(Vector3d(0.0, 0.0, 0.0));
	v[1]->SetPosition(Vector3d(10.0, 0.0, 0.0));
	v[2]->SetPosition(Vector3d(0.0, 10.0, 0.0));
	v[3]->SetPosition(Vector3d(0.0, 0.0, 10.0));

	v[0]->SetNormal(Vector3d(-0.577, -0.577, -0.577));
	v[1]->SetNormal(Vector3d(0.0, -0.7, -0.7));
	v[2]->SetNormal(Vector3d(-0.7, 0.0, -0.7));
	v[3]->SetNormal(Vector3d(-0.7, -0.7, 0.0));

	for (i=0; i<FSIZE; i++) {
		f[i] = new Face();
		mesh.fList.push_back(f[i]);
	}

	for (i=0; i<HESIZE; i++) {
		he[i] = new HEdge();
		mesh.heList.push_back(he[i]);
	}
	for (i=0; i<FSIZE; i++) {
		int base = i*3;
		SetPrevNext(he[base], he[base+1]);
		SetPrevNext(he[base+1], he[base+2]);
		SetPrevNext(he[base+2], he[base]);
		SetFace(f[i], he[base]);
	}
	SetTwin(he[0], he[4]);
	SetTwin(he[1], he[7]);
	SetTwin(he[2], he[10]);
	SetTwin(he[3], he[8]);
	SetTwin(he[5], he[9]);
	SetTwin(he[6], he[11]);
	he[0]->SetStart(v[1]); he[1]->SetStart(v[2]); he[2]->SetStart(v[3]);
	he[3]->SetStart(v[0]); he[4]->SetStart(v[2]); he[5]->SetStart(v[1]);
	he[6]->SetStart(v[0]); he[7]->SetStart(v[3]); he[8]->SetStart(v[2]);
	he[9]->SetStart(v[0]); he[10]->SetStart(v[1]); he[11]->SetStart(v[3]);
	v[0]->SetHalfEdge(he[3]);
	v[1]->SetHalfEdge(he[0]);
	v[2]->SetHalfEdge(he[1]);
	v[3]->SetHalfEdge(he[2]);
}


// GLUT menu callback function
void MenuCallback(int value) 
{
	switch (value) 
	{
	case 99: 
		exit(0); 
		break;
	case 101: 
		break;
	case 107:
		break;
	default: 
		displayMode = value;
		glutPostRedisplay();
		break;
	}
}


// GLUT reshape callback function
void ReshapeFunc(int width, int height) 
{
	winWidth = width;
	winHeight = height;
	winAspect = (double)width/(double)height;
	glViewport(0, 0, width, height);
	glutPostRedisplay();
}


// GLUT display callback function
void DisplayFunc() 
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(g_fov, winAspect, zNear, zFar);
	//glOrtho(-2.0, 2.0, -2.0, 2.0, zNear, zFar);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); 
	glTranslatef(xpan, ypan, -sdepth);
	glRotatef(stheta, 1.0, 0.0, 0.0);
	glRotatef(-sphi, 0.0, 1.0, 0.0);
	glTranslatef(-g_center[0], -g_center[1], -g_center[2]);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	switch (displayMode) 
	{
		case WIREFRAME: 
			DrawWireframe(); 
			break;
		case HIDDENLINE: 
			DrawHiddenLine(); 
			break;
		case FLATSHADED: 
			DrawFlatShaded(); 
			break;
		case SMOOTHSHADED: 
			DrawSmoothShaded(); 
			break;
		case COLORSMOOTHSHADED: 
			DrawColorSmoothShaded(); 
			break;
	}

	if (!stroke_flag)
	{
		DrawStrokePoints();

	}
	else
	{
		DrawSampledPoints();
	}
	//DrawStrokePoints_Sample();
	if (leftUp&&stroke_flag&&triangulate_flag)
	{

		drawtriangle(strokepts_sam_saved, out);

	}

	glutSwapBuffers();
}

// Display functions
void DrawWireframe() 
{
	HEdgeList heList = mesh.Edges();
	HEdgeList bheList = mesh.BoundaryEdges();
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	size_t i;
	for (i=0; i<heList.size(); i++) 
	{
		glVertex3dv(heList[i]->Start()->Position().ToArray());
		glVertex3dv(heList[i]->End()->Position().ToArray());
	}
	glColor3f(1.0f, 0.0f, 0.0f);
	for (i=0; i<bheList.size(); i++) 
	{
		glVertex3dv(bheList[i]->Start()->Position().ToArray());
		glVertex3dv(bheList[i]->End()->Position().ToArray());
	}
	glEnd();
}

void DrawHiddenLine() 
{
	FaceList fList = mesh.Faces();
	glShadeModel(GL_FLAT); 
	glEnable(GL_POLYGON_OFFSET_FILL);
	glColor3f(0, 0, 0);
	glBegin(GL_TRIANGLES);
	for (size_t i=0; i<fList.size(); i++) {
		Face *f = fList[i];
		const Vector3d & pos1 = f->HalfEdge()->Start()->Position();
		const Vector3d & pos2 = f->HalfEdge()->End()->Position();
		const Vector3d & pos3 = f->HalfEdge()->Next()->End()->Position();
		glVertex3dv(pos1.ToArray());
		glVertex3dv(pos2.ToArray());
		glVertex3dv(pos3.ToArray());
	}
	glEnd();
	glDisable(GL_POLYGON_OFFSET_FILL);

	DrawWireframe();
}

void DrawFlatShaded() 
{
	FaceList fList = mesh.Faces();
	glShadeModel(GL_FLAT); 
	glEnable(GL_LIGHTING);
	glColor3f(0.4f, 0.4f, 1.0f);
	glBegin(GL_TRIANGLES) ;
	for (size_t i=0; i<fList.size(); i++) {
		Face *f = fList[i];
		const Vector3d & pos1 = f->HalfEdge()->Start()->Position();
		const Vector3d & pos2 = f->HalfEdge()->End()->Position();
		const Vector3d & pos3 = f->HalfEdge()->Next()->End()->Position();
		Vector3d normal = (pos2-pos1).Cross(pos3-pos1);
		normal /= normal.L2Norm();
		glNormal3dv(normal.ToArray());
		glVertex3dv(pos1.ToArray());
		glVertex3dv(pos2.ToArray());
		glVertex3dv(pos3.ToArray());
	}
	glEnd();
	glDisable(GL_LIGHTING);
}

void DrawSmoothShaded() 
{ 
	FaceList fList = mesh.Faces();
	glShadeModel(GL_SMOOTH); 
	glEnable(GL_LIGHTING);
	glColor3f(0.4f, 0.4f, 1.0f);
	glBegin(GL_TRIANGLES) ;
	for (size_t i=0; i<fList.size(); i++) 
	{
		Face *f = fList[i];
		Vertex * v1 = f->HalfEdge()->Start();
		Vertex * v2 = f->HalfEdge()->End();
		Vertex * v3 = f->HalfEdge()->Next()->End();
		glNormal3dv(v1->Normal().ToArray());
		glVertex3dv(v1->Position().ToArray());
		glNormal3dv(v2->Normal().ToArray());
		glVertex3dv(v2->Position().ToArray());
		glNormal3dv(v3->Normal().ToArray());
		glVertex3dv(v3->Position().ToArray());
	}
	glEnd();
	glDisable(GL_LIGHTING);
}

void DrawColorSmoothShaded() 
{
	FaceList fList = mesh.Faces();
	glShadeModel(GL_SMOOTH); 
	glEnable(GL_LIGHTING);
	glColor3f(0.4f, 0.4f, 1.0f);
	glBegin(GL_TRIANGLES) ;
	for (size_t i=0; i<fList.size(); i++) 
	{
		Face *f = fList[i];
		Vertex * v1 = f->HalfEdge()->Start();
		Vertex * v2 = f->HalfEdge()->End();
		Vertex * v3 = f->HalfEdge()->Next()->End();
		glNormal3dv(v1->Normal().ToArray());
		glColor3dv(v1->Color().ToArray());
		glVertex3dv(v1->Position().ToArray());
		glNormal3dv(v2->Normal().ToArray());
		glColor3dv(v2->Color().ToArray());
		glVertex3dv(v2->Position().ToArray());
		glNormal3dv(v3->Normal().ToArray());
		glColor3dv(v3->Color().ToArray());
		glVertex3dv(v3->Position().ToArray());
	}
	glEnd();
	glDisable(GL_LIGHTING);
}

void DrawStrokePoints()
{
	if (strokePoints.size() < 2) return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, winWidth, 0, winHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glLineWidth(3.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0.7f, 0.8f, 0.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < strokePoints.size()-1; ++i)
	{
		Vector2f& v1 = strokePoints[i];
		Vector2f& v2 = strokePoints[i+1];
		glVertex2f(v1.X(), v1.Y());
		glVertex2f(v2.X(), v2.Y());
	}
	glEnd();
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void DrawStrokePoints_Sample()
{
	if (strokePoints.size() < 2) return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, winWidth, 0, winHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glLineWidth(3.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0.2f, 0.8f, 0.0f);
	Vector2f v1 = strokePoints[0];
	int j = 0;
	
	if (strokepts_sampled.size() == 0)
	{
		strokepts_sampled.push_back(v1);
	}
	for (int i = 1; i < strokePoints.size() ; ++i)
	{
		if (intersect_flag)break;
		//printf("\n strokePoints[i]:%f,%f", strokePoints[i].X(), strokePoints[i].Y());
		if (distanceofstrokePoints(i, strokePoints[i], v1) >= sample_segment)
		{
			glBegin(GL_LINES);
			Vector2f& v2 = strokePoints[i];
			glVertex2f(v1.X(), v1.Y());
			glVertex2f(v2.X(), v2.Y());
			glEnd();
			
			//cout << "v1= " << v1 << ", v2= " << v2 << endl;

			v1 = strokePoints[i];
			//把采样的点存进strokepts_sampled
			if (strokepts_sampled.back().X() != v1.X() && strokepts_sampled.back().Y() != v1.Y() && strokepts_sampled.size()-1 <= j)
			{
				strokepts_sampled.push_back(v1);
			}

			j++;
			//检查相交
			for (int k = 1; k + 2 < strokepts_sampled.size() ; k++)
			{
				//cout << "k=" << k << "  strokepts_sampled.size()=" << strokepts_sampled.size() << endl;
				if (segment_intersection(strokepts_sampled[strokepts_sampled.size()-2], strokepts_sampled[strokepts_sampled.size()-1], strokepts_sampled[k], strokepts_sampled[k - 1]))
				{
					intersect_flag = true;
					cout << "find Intersecting segments!!! draw again! (segment["<< strokepts_sampled.size() - 2<<", "<< strokepts_sampled.size() - 1<<"] and segment[" << k << ", " << k - 1 << "])" << endl;
					printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", strokepts_sampled.size() - 2, strokepts_sampled[strokepts_sampled.size() - 2].X(), strokepts_sampled[strokepts_sampled.size() - 2].Y(), strokepts_sampled.size() - 1, strokepts_sampled[strokepts_sampled.size() - 1].X(), strokepts_sampled[strokepts_sampled.size() - 1].Y());
					printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", k, strokepts_sampled[k].X(), strokepts_sampled[k].Y(), k - 1, strokepts_sampled[k - 1].X(), strokepts_sampled[k - 1].Y());
					break;
				}

			}
		}
		
	}
	//for (int i = 0; i < strokepts_sampled.size(); ++i)
	//{
	//	printf("strokepts_sampled[%d]:%f,%f \n ", i, strokepts_sampled[i].X(), strokepts_sampled[i].Y());
	//}
	if (leftUp)
	{
		float d= distanceofstrokePoints(strokepts_sampled.size() - 1, strokePoints[strokePoints.size() - 1], strokepts_sampled[strokepts_sampled.size()-1]);
		if (d >= 10.0)
		{
			strokepts_sampled.push_back(strokePoints[strokePoints.size() - 1]);
			glBegin(GL_LINES);
			glVertex2f(v1.X(), v1.Y());
			glVertex2f(strokepts_sampled.back().X(), strokepts_sampled.back().Y());
			glEnd();
			for (int k = 1; k + 2 < strokepts_sampled.size(); k++)
			{
				//cout << "k=" << k << "  strokepts_sampled.size()=" << strokepts_sampled.size() << endl;
				if (segment_intersection(strokepts_sampled[strokepts_sampled.size() - 2], strokepts_sampled[strokepts_sampled.size() - 1], strokepts_sampled[k], strokepts_sampled[k - 1]))
				{
					intersect_flag = true;
					cout << "find Intersecting segments!!! draw again! (segment[" << strokepts_sampled.size() - 2 << ", " << strokepts_sampled.size() - 1 << "] and segment[" << k << ", " << k - 1 << "])" << endl;
					printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", strokepts_sampled.size() - 2, strokepts_sampled[strokepts_sampled.size() - 2].X(), strokepts_sampled[strokepts_sampled.size() - 2].Y(), strokepts_sampled.size() - 1, strokepts_sampled[strokepts_sampled.size() - 1].X(), strokepts_sampled[strokepts_sampled.size() - 1].Y());
					printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", k, strokepts_sampled[k].X(), strokepts_sampled[k].Y(), k - 1, strokepts_sampled[k - 1].X(), strokepts_sampled[k - 1].Y());
					break;
				}

			}
		}
		d= distanceofstrokePoints(strokepts_sampled.size() - 1, strokepts_sampled[strokepts_sampled.size() - 1], strokepts_sampled[0]);
		if (d <= sample_segment)
		{

			glBegin(GL_LINES);
			glVertex2f(strokePoints[strokePoints.size() - 1].X(), strokePoints[strokePoints.size() - 1].Y());
			glVertex2f(strokePoints[0].X(), strokePoints[0].Y());
			glEnd();
			//第一次画圈或是上次画圈不合格，即strokepts_sam_saved未使用：把strokepts_sampled赋给strokepts_sam_saved
			if (!stroke_flag && !intersect_flag)
			{
				strokepts_sam_saved.clear();
				strokepts_sam_saved = strokepts_sampled;
				stroke_flag = 1;

			}
			//printf "strokepts_sampled" and "strokepts_sam_saved"
			/*
			for (int i = 0; i < strokepts_sampled.size(); ++i)
			{
				printf("\n strokepts_sampled[%d]:%f,%f", i, strokepts_sampled[i].X(), strokepts_sampled[i].Y());
			}
			for (int i = 0; i < strokepts_sam_saved.size(); ++i)
			{
				printf(" strokepts_sam_saved[%d]:%f,%f \n", i, strokepts_sam_saved[i].X(), strokepts_sam_saved[i].Y());
			}*/
			
		}
		else
		{
			printf("\n error!!!\n Please draw again to make the distance between the last point and the first point closer (less than 20px)!\n");
		}
		
	}
	

	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void drawtriangle(vector<Vector2f> strokepts, triangulateio &out)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, winWidth, 0, winHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glLineWidth(3.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0.4f, 0.2f, 0.6f);
	//for (int i = 0; i < out.numberoftriangles*3; i++)
	//{
	//	cout<<"out.trianglelist["<<i<<"]:" << out.trianglelist[i] << endl;
	//}

	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < out.numberofedges; i++)
	{
		cout << "out.edgelist[" << i << "]: " << out.edgelist[i] << endl;
		Vector2f& v1 = strokepts[out.edgelist[i * 2]];
		Vector2f& v2 = strokepts[out.edgelist[i * 2 + 1]];
		cout << "v1= " << v1 << ", v2= " << v2 << endl;
		glVertex2f(v1.X(), v1.Y());
		glVertex2f(v2.X(), v2.Y());
	}

	glEnd();

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void StrokePoints_Sample()
{
	Vector2f v1 = strokePoints[0];
	int j = 0;

	if (strokepts_sampled.size() == 0)
	{
		strokepts_sampled.push_back(v1);
		intersect_flag = false;
	}
	for (int i = 1; i < strokePoints.size(); ++i)
	{
		if (intersect_flag)break;
		//printf("\n strokePoints[i]:%f,%f", strokePoints[i].X(), strokePoints[i].Y());
		if (distanceofstrokePoints(i, strokePoints[i], v1) >= sample_segment)
		{
			v1 = strokePoints[i];
			//把采样的点存进strokepts_sampled
			//if (strokepts_sampled.back().X() != v1.X() && strokepts_sampled.back().Y() != v1.Y() && strokepts_sampled.size() - 1 <= j)
			{
				strokepts_sampled.push_back(v1);
			}

			j++;
			//检查相交
			for (int k = 1; k + 2 < strokepts_sampled.size(); k++)
			{
				//cout << "k=" << k << "  strokepts_sampled.size()=" << strokepts_sampled.size() << endl;
				if (segment_intersection(strokepts_sampled[strokepts_sampled.size() - 2], strokepts_sampled[strokepts_sampled.size() - 1], strokepts_sampled[k], strokepts_sampled[k - 1]))
				{
					intersect_flag = true;
					cout << "find Intersecting segments!!! draw again! (segment[" << strokepts_sampled.size() - 2 << ", " << strokepts_sampled.size() - 1 << "] and segment[" << k << ", " << k - 1 << "])" << endl;
					printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", strokepts_sampled.size() - 2, strokepts_sampled[strokepts_sampled.size() - 2].X(), strokepts_sampled[strokepts_sampled.size() - 2].Y(), strokepts_sampled.size() - 1, strokepts_sampled[strokepts_sampled.size() - 1].X(), strokepts_sampled[strokepts_sampled.size() - 1].Y());
					printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", k, strokepts_sampled[k].X(), strokepts_sampled[k].Y(), k - 1, strokepts_sampled[k - 1].X(), strokepts_sampled[k - 1].Y());
					break;
				}

			}
		}

	}
	
	
	float d = distanceofstrokePoints(strokepts_sampled.size() - 1, strokePoints[strokePoints.size() - 1], strokepts_sampled[strokepts_sampled.size() - 1]);
	if (d >= 10.0)
	{
		strokepts_sampled.push_back(strokePoints[strokePoints.size() - 1]);
			
		for (int k = 1; k + 2 < strokepts_sampled.size(); k++)
		{
			//cout << "k=" << k << "  strokepts_sampled.size()=" << strokepts_sampled.size() << endl;
			if (segment_intersection(strokepts_sampled[strokepts_sampled.size() - 2], strokepts_sampled[strokepts_sampled.size() - 1], strokepts_sampled[k], strokepts_sampled[k - 1]))
			{
				intersect_flag = true;
				cout << "find Intersecting segments!!! draw again! (segment[" << strokepts_sampled.size() - 2 << ", " << strokepts_sampled.size() - 1 << "] and segment[" << k << ", " << k - 1 << "])" << endl;
				printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", strokepts_sampled.size() - 2, strokepts_sampled[strokepts_sampled.size() - 2].X(), strokepts_sampled[strokepts_sampled.size() - 2].Y(), strokepts_sampled.size() - 1, strokepts_sampled[strokepts_sampled.size() - 1].X(), strokepts_sampled[strokepts_sampled.size() - 1].Y());
				printf("\n strokepts_sampled[%d]:%f,%f, strokepts_sampled[%d]:%f,%f", k, strokepts_sampled[k].X(), strokepts_sampled[k].Y(), k - 1, strokepts_sampled[k - 1].X(), strokepts_sampled[k - 1].Y());
				break;
			}

		}
	}
	d = distanceofstrokePoints(strokepts_sampled.size() - 1, strokepts_sampled[strokepts_sampled.size() - 1], strokepts_sampled[0]);
	if (d <= sample_segment)
	{

			
		//第一次画圈或是上次画圈不合格，即strokepts_sam_saved未使用：把strokepts_sampled赋给strokepts_sam_saved
		if (!stroke_flag && !intersect_flag)
		{
			strokepts_sam_saved.clear();
			strokepts_sam_saved = strokepts_sampled;
			stroke_flag = 1;

		}

	}
	else
	{
		printf("\n error!!!\n Please draw again to make the distance between the last point and the first point closer (less than 20px)!\n");
	}

	//for (int i = 0; i < strokepts_sampled.size(); ++i)
	//{
	//	printf("strokepts_sampled[%d]:%f,%f \n ", i, strokepts_sampled[i].X(), strokepts_sampled[i].Y());
	//}

}
void DrawSampledPoints()
{
	if (strokepts_sam_saved.size() < 3) return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, winWidth, 0, winHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glLineWidth(3.0f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glColor3f(0.1f, 0.8f, 0.0f);
	glBegin(GL_LINES);
	for (int i = 0; i < strokepts_sam_saved.size() - 1; ++i)
	{
		Vector2f& v1 = strokepts_sam_saved[i];
		Vector2f& v2 = strokepts_sam_saved[i + 1];
		glVertex2f(v1.X(), v1.Y());
		glVertex2f(v2.X(), v2.Y());
	}
	glEnd();
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

// GLUT keyboard callback function
void KeyboardFunc(unsigned char ch, int x, int y) {
	switch (ch) {
	case 'u':
	case 'U':
		break;
	case 's':
	case 'S':
		break;
	case '1':	// key '1'
		currentMode = Viewing;
		cout << "Viewing mode" << endl;
		break;
	case '2':	// key '2'
		currentMode = Sketching;
		cout << "Sketching mode" << endl;
		break;
	case '3':	// key '3'
		currentMode = Moving;
		cout << "Moving mode" << endl;
		break;
	case 27:	// Esc char
		exit(0);
		break;
	}
	glutPostRedisplay();
}


// GLUT mouse callback function
void MouseFunc(int button, int state, int x, int y) 
{
	if (state == GLUT_DOWN)
	{
		downX = x;
		downY = y;
	}
	lastX = x;
	lastY = y;
	leftDown = (button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN);
	leftUp = (button == GLUT_LEFT_BUTTON) && (state == GLUT_UP);
	middleDown = (button == GLUT_MIDDLE_BUTTON) && (state == GLUT_DOWN);
	shiftDown = (glutGetModifiers() & GLUT_ACTIVE_SHIFT);

	if (state == GLUT_DOWN && currentMode == Sketching)
	{
		strokePoints.clear();
		strokepts_sampled.clear();
		//intersect_flag = false;
	}
	else if (state == GLUT_UP && currentMode == Sketching)
	{
		if (stroke_flag)
		{
			cout << "stroke_flag=true" << endl;
			if (!triangulate_flag)
			{
				triangulateio_init(strokepts_sam_saved, in, out);
				triangulate("pszen", &in, &out, &vorout);
				triangulateio_free_in(in);
				triangulate_flag = 1;
			}
			
		}
		else
		{
			StrokePoints_Sample();
			sample_flag = true;

		}

	}



	glutPostRedisplay();
}


// GLUT mouse motion callback function
void MotionFunc(int x, int y) 
{
	switch (currentMode)
	{
	case Viewing:
		if (leftDown)
		{
			if(!shiftDown) 
			{	// rotate
				sphi += (double)(x - lastX) / 4.0;
				stheta += (double)(lastY - y) / 4.0;
			} 
			else 
			{	// pan
				xpan += (double)(x - lastX)*sdepth/zNear/winWidth;
				ypan += (double)(lastY - y)*sdepth/zNear/winHeight;
			}
		}
		// scale
		if (middleDown) 
			sdepth += (double)(lastY - y) / 10.0;
		break;

	case Sketching:
		if (leftDown)
		{
			float dx = abs(x-lastX), dy = abs(y-lastY);
			if (dx > 1 || dy > 1)
			{
				strokePoints.push_back(Vector2f(x, winHeight - y));
			}
		}
		break;

	case Moving:
		
		break;
	}

	lastX = x;
	lastY = y;
	glutPostRedisplay();
}


// main function
void main(int argc, char **argv) 
{
	glutInit(&argc, argv);
	InitGL();
	InitMenu();
	//if (argc>=2) 
	//	mesh.LoadObjFile(argv[1]);
	//else 
	////	mesh.LoadObjFile("dinosaur-14k-remesh.obj");
	//	InitGeometry();
	//SetBoundaryBox(mesh.MinCoord(), mesh.MaxCoord());

	//mesh.ComputeVertexNormals();
	//mesh.DisplayMeshInfo();
	//mesh.ComputeVertexCurvatures();


	glutMainLoop();

	
	triangulateio_free_out(out);
	triangulate_flag = 0;
}


