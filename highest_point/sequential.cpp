#include "mpi.h"
#include <stdio.h>
#include <math.h>
#include <ctime> 
#pragma comment (lib, "msmpi.lib")
#include <fstream>
using namespace std;

# define M_PI   3.14159265358979323846
#define POINT_NUM 100000
#define MIN_RAND_LAT 0.0
#define MAX_RAND_LAT 360.0
#define MIN_RAND_LON 0.0
#define MAX_RAND_LON 180.0



 class Point
{ public:
  double lon;
  double lat;

  Point() {lon =0; lat = 0;}
  Point(double x, double y)
  {
	  lon = x;
	  lat = y;
  }

  Point& Point::operator = (Point q)
  {
	  lon = q.lon;
	  lat = q.lat;
	  return *this;
  }
};



double height(Point p)
{
	return  35000.0 * sin(3.0 * p.lon) * sin(2.0 * p.lat) + 9700.0 * cos(10.0 * p.lon) * cos(20.0 * p.lat) -
		    800.0 * sin(25.0 * p.lon + 0.03 * M_PI ) + 550.0 * cos(p.lat + 0.2 * M_PI);	
}

Point chooseRandomPoint()
{
	Point p;
	double f = (double)rand() / RAND_MAX;
	p.lon = MIN_RAND_LON + f * (MAX_RAND_LON - MIN_RAND_LON);
	f = (double)rand() / RAND_MAX;
	p.lat = MIN_RAND_LAT + f * (MAX_RAND_LAT - MIN_RAND_LAT);
	return p;

}

int main(int argc, char* argv[])
{
	srand(time(NULL));
	double local_max,local_max_lon,local_max_lat, global_max = 0.0,global_max_lon,global_max_lat,x,y;

	Point p = Point();
	Point q = Point();

	double start,stop;

	start = clock();

   for(int i = 0; i < POINT_NUM; i++)
   {
	  p = chooseRandomPoint();
	  x = p.lon;
	  y = p.lat;
	  q = Point(p.lon + 0.01, p.lat + 0.01);
	  while(height(q) > height(p))
	  {
		  x = q.lon;
		  y = q.lat;
		  p = q;
		  q = Point(p.lon + 0.01, p.lat + 0.01);
	  }
	  local_max = height(p);
	  local_max_lon = x;
	  local_max_lat = y;
	  if(local_max > global_max)
	  {
		  global_max = local_max;
		  global_max_lon = local_max_lon;
		  global_max_lat = local_max_lat;
	  }
   }

   stop = clock();

   printf("Cel mai inalt punct de pe planetoid are coordonatele (%f, %f) \nsi inaltimea %f \n",global_max_lon,global_max_lat,global_max);
   printf("Rezultatul a fost obtinut in %f",(stop-start)/CLOCKS_PER_SEC);
   getchar();
	return 0;
}