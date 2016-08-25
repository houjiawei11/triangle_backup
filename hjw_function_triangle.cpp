#include <hjw_function_triangle.h>
float distanceofstrokePoints(int i,Vector2f& vi, Vector2f& v1)
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
	if (c<0.1)
		return 1;
	else
		return 0;
}
bool segment_intersection(Vector2f& v11, Vector2f& v12, Vector2f& v21, Vector2f& v22)
{
	if (v12.X()>v11.X())
	{
		Vector2f& vtemp = v12;
		v12 = v11;
		v11 = vtemp;
	}
	if (v22.X()>v21.X())
	{
		Vector2f& vtemp = v22;
		v22 = v21;
		v21 = vtemp;
	}
	float a1, b1, a2, b2;
	a1 = (v11.Y() - v12.Y()) / (v11.X() - v12.X());
	b1 = v12.Y() - v12.X()*a1;
	a2 = (v21.Y() - v22.Y()) / (v21.X() - v22.X());
	b2 = v22.Y() - v22.X()*a2;

	//if (a1 == a2)
	if(near_equal(a1,a2))
	{
		//if (b1 == b2)
		if (near_equal(b1, b2))
		{
			if ((v11.X() <= v22.X() && v11.X() >= v21.X())||(v21.X() <= v12.X() && v21.X() >= v11.X()))
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		float x_r, y_r1, y_r2;
		x_r = (b2 - b1) / (a1 - a2);
		y_r1 = a1*x_r + b1;
		y_r2 = a2*x_r + b2;
		if (near_equal(y_r1, y_r2))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	
}