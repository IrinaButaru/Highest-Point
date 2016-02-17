#include "mpi.h"
#include <stdio.h>
#include <math.h>
#include <ctime> 
#include <fstream>
#pragma comment (lib, "msmpi.lib")

#define M_PI   3.14159265358979323846
#define TOTAL_POINT_NUM 100000
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
	int  proc_num, id, rc;
	double global_max_lon,global_max_lat,global_max = 0.0,proc_global_max_lon,proc_global_max_lat,proc_global_max = 0.0,
		   proc_local_max_lon,proc_local_max_lat,proc_local_max,x,y;
	//Point global_max_point;
	Point * P = new Point[TOTAL_POINT_NUM];
	Point Candidates[100];
	double Heights[100];

	double start,stop;

	rc = MPI_Init(&argc,&argv);
   if (rc != MPI_SUCCESS) 
   {
     printf ("Error starting MPI program. Terminating.\n");
     MPI_Abort(MPI_COMM_WORLD, rc);
     }

   MPI_Comm_size(MPI_COMM_WORLD,&proc_num);
   MPI_Comm_rank(MPI_COMM_WORLD,&id);

    int blen[2] = {1, 1};
    MPI_Aint disp[2];
	MPI_Datatype type[2] = {MPI_DOUBLE, MPI_DOUBLE};
	MPI_Datatype MPI_Point;

   disp[0] = 0;// &P[0].lon - &P[0]; 
   disp[1] = &P[0].lat - &P[0].lon;
   MPI_Type_struct(2,blen,disp,type,&MPI_Point);
   MPI_Type_commit(&MPI_Point);

   start = clock();

   if(id == 0)
   {
	   for(int i = 0; i < TOTAL_POINT_NUM; i++)
		   P[i] = chooseRandomPoint();
   }


   MPI_Bcast(P,TOTAL_POINT_NUM,MPI_Point,0,MPI_COMM_WORLD);

   int proc_point_num = TOTAL_POINT_NUM / proc_num;
   int l = id * proc_point_num;
   int r = l + proc_point_num;
   for( int i = l; i < r; i++)
   {
	   Point p = P[i];
	   x = p.lon;
	   y = p.lat;
	   Point q = Point(p.lon + 0.01, p.lat + 0.01);
	  while(height(q) > height(p))
	  {
		  x = q.lon;
		  y = q.lat;
		  p = q;
		  q = Point(p.lon + 0.01, p.lat + 0.01);
	  }
	  proc_local_max = height(p);
	  proc_local_max_lon = x;
	  proc_local_max_lat = y;
	  if(proc_local_max > proc_global_max)
	  {
		  proc_global_max = proc_local_max;
		  proc_global_max_lon = proc_local_max_lon;
		  proc_global_max_lat = proc_local_max_lat;
	  }
   }
   Point m = Point(proc_global_max_lon,proc_global_max_lat);

    MPI_Gather(&proc_global_max,1,MPI_DOUBLE,&Heights,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
    MPI_Gather(&m,1,MPI_Point,&Candidates,1,MPI_Point,0,MPI_COMM_WORLD);

	if( id == 0 )
	{
		global_max = Heights[0];
		global_max_lon = Candidates[0].lon;
		global_max_lat = Candidates[0].lat;
		for(int i = 1; i < proc_num; i++)
			if(Heights[i] > global_max)
			{
				global_max = Heights[i];
				global_max_lon = Candidates[i].lon;
				global_max_lat = Candidates[i].lat;
			}
			
	}

	stop = clock();

   printf ("Procesorul cu numarul %d: \n",id);
   for(int i = 0; i < proc_num;i++)
		if(id == i)
			printf("(%f, %f) --> %f\n",proc_global_max_lon,proc_global_max_lat,proc_global_max);
   if(id == 0)
   {
	     printf("Cel mai inalt punct de pe planetoid are coordonatele (%f, %f) \nsi inaltimea %f \n",global_max_lon,global_max_lat,global_max);
		 printf("Rezultatul a fost obtinut in %f",(stop-start)/CLOCKS_PER_SEC);
   }
	 

   MPI_Finalize();
   delete[] P;
   return 0;
}