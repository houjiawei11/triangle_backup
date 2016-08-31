//#include <triangle.h>
#include <hjw_function_triangle.h>
float distanceofstrokePoints(int i,Vector2f vi, Vector2f v1)
{
	//printf("\n v1: %f, %f", v1.X(), v1.Y());
	if (i < 1)
	{
		printf("the first point/n");
		return 1.0;
	}
	float x_distance = vi.X() - v1.X();
	float y_distance = vi.Y() - v1.Y();
	float distance = x_distance*x_distance + y_distance*y_distance;
	distance = sqrt(distance);
	return distance;
}

bool near_equal(float a, float b)
{
	float c = fabs(a - b);
	if (c<0.001)
		return 1;
	else
		return 0;
}
bool segment_intersection(Vector2f v11, Vector2f v12, Vector2f v21, Vector2f v22)
{
	//cout << "two segment: " << v11.X() << " , " << v11.Y() << ", " << v12.X() << " , " << v12.Y() << "; " << endl << v21.X() << " , " << v21.Y() << ", " << v22.X() << " , " << v22.Y() << endl;
	
	float a1, b1, a2, b2;
	a1 = (v11.Y() - v12.Y()) / (v11.X() - v12.X());
	b1 = v12.Y() - v12.X()*a1;
	a2 = (v21.Y() - v22.Y()) / (v21.X() - v22.X());
	b2 = v22.Y() - v22.X()*a2;

	float x_r, y_r1, y_r2;
	x_r = (b2 - b1) / (a1 - a2);
	y_r1 = a1*x_r + b1;
	y_r2 = a2*x_r + b2;

	//if (near_equal(y_r1, y_r2))
	if(y_r1==y_r2)
	{
		bool r1 = (x_r <= v12.X() && x_r >= v11.X()) || (x_r <= v11.X() && x_r >= v12.X());
		bool r2 = (x_r <= v22.X() && x_r >= v21.X()) || (x_r <= v21.X() && x_r >= v22.X());
		if (r1&&r2)
		{
			cout << "a1= " << a1 << " b1= " << b1 << ", a2= " << a2 << " b2= " << b2 << endl;
			cout << "x_r= " << x_r << ", y_r1= " << y_r1 << ", y_r2= " << y_r2 << endl;
			cout << "orderd by x: two segment: " << v11.X() << " , " << v11.Y() << ", " << v12.X() << " , " << v12.Y() << "; " << endl << v21.X() << " , " << v21.Y() << ", " << v22.X() << " , " << v22.Y() << endl;
			
			return 1;
		}
		else
			return 0;
	}
	else
	{
		return 0;
	}
}

void triangulateio_init( vector<Vector2f> strokepts,  triangulateio& in,  triangulateio& out)
{
	int pts_num= strokepts.size();
	for (int i = 0; i < strokepts.size(); ++i)
	{
		printf("strokepts[%d]:%f,%f \n ", i, strokepts[i].X(), strokepts[i].Y());
	}
	//init in
	in.numberofpoints = pts_num;
	in.numberofpointattributes = 0;
	in.pointlist = (REAL *)malloc(in.numberofpoints * 2 * sizeof(REAL));
	for (int i = 0; i<pts_num; i++)
	{
		in.pointlist[i * 2] = strokepts[i].X();
		in.pointlist[i * 2 + 1] = strokepts[i].Y();
	}

	//in.pointattributelist = (REAL *)NULL;		// malloc(in.numberofpoints * in.numberofpointattributes * sizeof(REAL));
	in.pointmarkerlist = (int *)malloc(in.numberofpoints * sizeof(int));
	for (int i = 0; i<pts_num; i++)
	{
		in.pointmarkerlist[i] = 2;
	}

	in.numberofsegments = pts_num;
	in.segmentlist = (int *)malloc(in.numberofsegments * 2 * sizeof(int));
	for (int i = 0; i < in.numberofsegments-1; i++)
	{
		in.segmentlist[i * 2] = i;
		in.segmentlist[i * 2 + 1] = i+1;

	}
	in.segmentlist[(in.numberofsegments - 1) * 2] = in.numberofsegments - 1;
	in.segmentlist[(in.numberofsegments - 1) * 2 + 1] = 0;
	//in.regionlist = (REAL *)NULL;
	cout << "init in successd: pointlist, pointmarkerlist, segmentlist..." << endl;

	//init out
	out.pointlist = (REAL *)NULL;		//no N
	out.numberofpointattributes = 0;
	out.pointmarkerlist = (int *)NULL;
	out.trianglelist = (int *)NULL;		//no E
	out.numberoftriangleattributes = 0;
	out.neighborlist = (int *)NULL;		//n
	out.segmentlist = (int *)NULL;		//p or c, no P and B
	out.segmentmarkerlist = (int *)NULL;
	out.edgelist = (int *)NULL;			//e, no B
	out.edgemarkerlist = (int *)NULL;
	cout << "init out successed!" << endl;

}

void triangulateio_free_in(triangulateio &in)
{
	free(in.pointlist);
	free(in.pointmarkerlist);
	free(in.segmentlist);
}
void triangulateio_free_out(triangulateio &out)
{

	free(out.pointlist);
	free(out.pointmarkerlist);
	free(out.trianglelist);
	free(out.neighborlist);
	free(out.segmentlist);
	free(out.segmentmarkerlist);
	free(out.edgelist);
	free(out.edgemarkerlist);
}
