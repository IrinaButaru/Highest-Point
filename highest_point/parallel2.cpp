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

enum CommunicationTag
{
	COMM_TAG_MASTER_SEND_TASK,
	COMM_TAG_MASTER_SEND_TERMINATE,
	COMM_TAG_SLAVE_SEND_RESULT,
};

int main(int argc, char* argv[])
{
	srand(time(NULL));
	int  proc_num, id, rc;
	double global_max_lon,global_max_lat,global_max = 0.0,
		   proc_local_max_lon,proc_local_max_lat,proc_local_max,x,y;
	//Point global_max_point;
	 Point * P = new Point[TOTAL_POINT_NUM];

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

   //master

   start = clock();

      if(id == 0)
   {
	  
	
	   // choose random start points
	   for(int i = 0; i < TOTAL_POINT_NUM; i++)
		   P[i] = chooseRandomPoint();
	  

	   const int chunk_size = TOTAL_POINT_NUM / 100;
	   int next_index_to_send = 0;

	   // send first tasks to procesors
	   
	   for(int i = 1; i < proc_num; i++)
	   {
		   //MPI_Send(&P[next_index_to_send],(chunk_size * sizeof(Point)),MPI_CHAR,i,COMM_TAG_MASTER_SEND_TASK, MPI_COMM_WORLD);
		   MPI_Send(&P[next_index_to_send],chunk_size,MPI_Point,i,COMM_TAG_MASTER_SEND_TASK, MPI_COMM_WORLD);
		   next_index_to_send += chunk_size;
	   }
	 

	   // waiting to receive data and sending more tasks

	   while(true)
	   {
		   MPI_Status stat;
		   double proc_global_max = 0.0;
		   Point m;
		   MPI_Recv(&proc_global_max,1,MPI_DOUBLE,MPI_ANY_SOURCE,COMM_TAG_SLAVE_SEND_RESULT, MPI_COMM_WORLD, &stat);
		   MPI_Recv(&m,1,MPI_Point,MPI_ANY_SOURCE,COMM_TAG_SLAVE_SEND_RESULT, MPI_COMM_WORLD, &stat);
		   if(proc_global_max > global_max)
		   {
			   global_max = proc_global_max;
			   global_max_lon = m.lon;
			   global_max_lat = m.lat;
		   }

		   if (next_index_to_send >= TOTAL_POINT_NUM)
					break;
		

		  // MPI_Send(&P[next_index_to_send],(chunk_size * sizeof(Point)),MPI_CHAR,stat.MPI_SOURCE,COMM_TAG_MASTER_SEND_TASK, MPI_COMM_WORLD);
		    MPI_Send(&P[next_index_to_send],chunk_size,MPI_Point,stat.MPI_SOURCE,COMM_TAG_MASTER_SEND_TASK, MPI_COMM_WORLD);
		    next_index_to_send += chunk_size;
	   }

	   //send termination 

	   for (int i = 1; i < proc_num; i++)
		{
			char dummy;
			MPI_Send(&dummy, 1, MPI_CHAR, i, COMM_TAG_MASTER_SEND_TERMINATE, MPI_COMM_WORLD);
		}
	    
	      stop = clock();


	      printf ("Procesorul cu numarul %d: \n",id);
	      printf("Cel mai inalt punct de pe planetoid are coordonatele (%f, %f) \nsi inaltimea %f \n",global_max_lon,global_max_lat,global_max);
		  printf("Rezultatul a fost obtinut in %f",(stop-start)/CLOCKS_PER_SEC);

   }
	
	  // slaves

	else
	{
	  MPI_Status stat;
	  double proc_global_max_lon,proc_global_max_lat,proc_global_max = 0.0;
	  int recv_num = 0;
	 	  while(true)
	  {
		  MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&stat);
		//  MPI_Get_count(&stat,MPI_CHAR,&recv_num);
		  MPI_Get_count(&stat,MPI_Point,&recv_num);

	      if (stat.MPI_TAG == COMM_TAG_MASTER_SEND_TERMINATE)
			  break;
		
		//  const int chunk_size = recv_num / sizeof(Point);
		//  Point *P = new Point[chunk_size];

		  Point * P = new Point[recv_num];

		//  MPI_Recv(P,recv_num,MPI_CHAR,0,COMM_TAG_MASTER_SEND_TASK, MPI_COMM_WORLD, &stat);
		  MPI_Recv(P,recv_num,MPI_Point,0,COMM_TAG_MASTER_SEND_TASK, MPI_COMM_WORLD, &stat);

		//  for(int i = 0; i < chunk_size; i++)
		  for(int i = 0; i < recv_num; i++)
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

		  MPI_Send(&proc_global_max,1,MPI_DOUBLE,0,COMM_TAG_SLAVE_SEND_RESULT,MPI_COMM_WORLD);
		  MPI_Send(&m,1,MPI_Point,0,COMM_TAG_SLAVE_SEND_RESULT,MPI_COMM_WORLD);

		  delete[] P;
	  }

	  printf ("Procesorul cu numarul %d: \n",id);
	  printf("(%f, %f) --> %f\n",proc_global_max_lon,proc_global_max_lat,proc_global_max);
}

	MPI_Finalize();
    return 0;
}