#include <stdio.h>
#include <stdlib.h>

#include "ml6.h"
#include "display.h"
#include "draw.h"
#include "matrix.h"
#include "math.h"
#include "gmath.h"

int setInRange(int input)
{
  if(input > 255) return 255;
  if(input < 0) return 0;
  return input;
}

void free2DArray(double ** a, int len)
{
  int i;
  for(i = 0; i < len; i++) free(a[i]);
  free(a);
}

void swap(double ** a, int i, int j)
{
  double * temp = a[i];
  a[i] = a[j];
  a[j] = temp;
}

//sorts points by their y coordinate
void sortPointsY(double ** vertices, int len)
{
  int i, j;
  for(i = 0; i < len; i++)
  {
    int swapCount = 0;
    for(j = 0; j < len - 1; j++)
    {
      if(vertices[j][1] >= vertices[j + 1][1]) 
      {
        swapCount++;
        swap(vertices, j, j + 1);
      }
    }
    if(swapCount == 0) break;
  }
}

/////////////////////////////////////////////Scanline implementations with different shading algorithms/////////////////////////////////////////////
void scanline_convert( struct matrix *points, int i, screen s, zbuffer zb, color c) 
{
  double ** vertices = (double **) calloc(3, sizeof(double *));

  //initialize list of points
  int j, k;
  for(j = 0; j < 3; j++)
  {
    vertices[j] = (double *) calloc(3, sizeof(double));
    for(k = 0; k < 3; k++)
    {
      vertices[j][k] = points->m[k][i + j];
    }
  }

  //get points in order
  sortPointsY(vertices, 3);
  double * B = vertices[0]; double * M = vertices[1]; double * T = vertices[2];

  ////////////////////////////Draw////////////////////////////
  double * left = (double *) calloc(3, sizeof(double)); //left will travel from B to T
  double * right = (double *) calloc(3, sizeof(double)); //right will travel from B to M, then M to T

  left[0] = right[0] = B[0]; left[1] = right[1] = B[1]; left[2] = right[2] = B[2];
  double yCurrent = B[1];

  while(yCurrent <= T[1]) //Note that B[1] is never equal to T[1], unless the triangle is degenerate.
  {
    //left
    left[0] = B[0] + (T[0] - B[0]) * (yCurrent - B[1]) / (T[1] - B[1]);
    left[1] = yCurrent;
    left[2] = B[2] + (T[2] - B[2]) * (yCurrent - B[1]) / (T[1] - B[1]);

    if(yCurrent < M[1]) //M[1] > B[1]
    {
      //right
      right[0] = B[0] + (M[0] - B[0]) * (yCurrent - B[1]) / (M[1] - B[1]);
      right[1] = yCurrent;
      right[2] = B[2] + (M[2] - B[2]) * (yCurrent - B[1]) / (M[1] - B[1]);
    }
    else if(yCurrent > M[1])
    {
      //right
      right[0] = M[0] + (T[0] - M[0]) * (yCurrent - M[1]) / (T[1] - M[1]);
      right[1] = yCurrent;
      right[2] = M[2] + (T[2] - M[2]) * (yCurrent - M[1]) / (T[1] - M[1]);
    }
    else
    {
      //right
      right[0] = M[0];
      right[1] = yCurrent;
      right[2] = M[2];
    }
    draw_line(left[0], left[1], left[2], right[0], right[1], right[2], s, zb, c);
    //printf("left: (%f, %f, %f) || right: (%f, %f, %f)\n", left[0], left[1], left[2], right[0], right[1], right[2]);
    yCurrent++;
  }

  ///
  free2DArray(vertices, 3);
  free(left); free(right);
  ///
}

void scanline_convert_flat(struct matrix * points, int i, screen s, zbuffer zb, double ** lightSources, int lSlength, color c_Ambient, struct * constants consts)
{

  double ** vertices = (double **) calloc(3, sizeof(double *));

  //initialize list of points
  int j, k;
  for(j = 0; j < 3; j++)
  {
    vertices[j] = (double *) calloc(3, sizeof(double));
    for(k = 0; k < 3; k++)
    {
      vertices[j][k] = points->m[k][i + j];
    }
  }

  //get points in order
  sortPointsY(vertices, 3);
  double * B = vertices[0]; double * M = vertices[1]; double * T = vertices[2];

  ////////////////////////////Decide Color////////////////////////////
  color c_Polygon; c_Polygon.red = 0; c_Polygon.green = 0; c_Polygon.blue = 0;
  double * normal = calculate_normal(points, i);
  normalize(normal);

  //Handle Ambient Light
  double KAr = consts->r[0]; double KAg = consts->g[0]; double KAb = consts->b[0];
  c_Polygon.red += (int) c_Ambient.red * KAr; c_Polygon.green = (int) c_Ambient.green * KAg; c_Polygon.blue = (int) c_Ambient.blue * KAb;

  //Handle Diffuse and Specular Reflection
  double xAvg = (B[0] + M[0] + T[0]) / 3;
  double yAvg = (B[1] + M[1] + T[1]) / 3;
  double zAvg = (B[2] + M[2] + T[2]) / 3;

  double KDr = consts->r[1]; double KDg = consts->g[1]; double KDb = consts->b[1];

  int currentlS;
  for(currentlS = 0; currentlS < lSlength; currentlS++)
  {
    double * fields = lightSources[currentlS];
    int Lr, Lg, Lb; double Lx, Ly, Lz;
    Lr = (int) fields[0]; Lg = (int) fields[1]; Lb = (int) fields[2];
    Lx = fields[3]; Ly = fields[4]; Lz = fields[5];

    //DIFFUSE
    double dx = xAvg - Lx; double dy = yAvg - Ly; double dz = zAvg - Lz;
    double mag = sqrt(dx * dx + dy * dy + dz * dz);
    dx /= mag; dy /= mag; dz /= mag;
    double cos = abs(dx * normal[0] + dy * normal[1] + dz * normal[2]);

    c_Polygon.red += (int) (Lr * KDr * cos); c_Polygon.green += (int) (Lg * KDg * cos); c_Polygon.blue += (int) (Lb * KDb * cos);

    //SPECULAR

  }

  //Put color in ranges
  c_Polygon.red = setInRange(c.red); c_Polygon.green = setInRange(c.green); c_Polygon.blue = setInRange(c.blue);

  ////////////////////////////Draw////////////////////////////
  double * left = (double *) calloc(3, sizeof(double)); //left will travel from B to T
  double * right = (double *) calloc(3, sizeof(double)); //right will travel from B to M, then M to T

  left[0] = right[0] = B[0]; left[1] = right[1] = B[1]; left[2] = right[2] = B[2];
  double yCurrent = B[1];

  while(yCurrent <= T[1]) //Note that B[1] is never equal to T[1], unless the triangle is degenerate.
  {
    //left
    left[0] = B[0] + (T[0] - B[0]) * (yCurrent - B[1]) / (T[1] - B[1]);
    left[1] = yCurrent;
    left[2] = B[2] + (T[2] - B[2]) * (yCurrent - B[1]) / (T[1] - B[1]);

    if(yCurrent < M[1]) //M[1] > B[1]
    {
      //right
      right[0] = B[0] + (M[0] - B[0]) * (yCurrent - B[1]) / (M[1] - B[1]);
      right[1] = yCurrent;
      right[2] = B[2] + (M[2] - B[2]) * (yCurrent - B[1]) / (M[1] - B[1]);
    }
    else if(yCurrent > M[1])
    {
      //right
      right[0] = M[0] + (T[0] - M[0]) * (yCurrent - M[1]) / (T[1] - M[1]);
      right[1] = yCurrent;
      right[2] = M[2] + (T[2] - M[2]) * (yCurrent - M[1]) / (T[1] - M[1]);
    }
    else
    {
      //right
      right[0] = M[0];
      right[1] = yCurrent;
      right[2] = M[2];
    }
    draw_line(left[0], left[1], left[2], right[0], right[1], right[2], s, zb, c_Polygon);
    //printf("left: (%f, %f, %f) || right: (%f, %f, %f)\n", left[0], left[1], left[2], right[0], right[1], right[2]);
    yCurrent++;
  }

  ///
  free2DArray(vertices, 3);
  free(left); free(right);
  free(normal);
  ///  
}

/*======== void add_polygon() ==========
Inputs:   struct matrix *surfaces
         double x0
         double y0
         double z0
         double x1
         double y1
         double z1
         double x2
         double y2
         double z2  
Returns: 
Adds the vertices (x0, y0, z0), (x1, y1, z1)
and (x2, y2, z2) to the polygon matrix. They
define a single triangle surface.
====================*/
void add_polygon( struct matrix *polygons, double x0, double y0, double z0, double x1, double y1, double z1, double x2, double y2, double z2 ) {

  add_point(polygons, x0, y0, z0);
  add_point(polygons, x1, y1, z1);
  add_point(polygons, x2, y2, z2);
}

/*======== void draw_polygons() ==========
Inputs:   struct matrix *polygons
          screen s
          color c  
Returns: 
Goes through polygons 3 points at a time, drawing 
lines connecting each points to create bounding
triangles
====================*/
void draw_polygons( struct matrix *polygons, screen s, zbuffer zb, color c ) {
  if ( polygons->lastcol < 3 ) {
    printf("Need at least 3 points to draw a polygon!\n");
    return;
  }
 
  int point;
  double *normal;
  
  for (point=0; point < polygons->lastcol-2; point+=3) {

    normal = calculate_normal(polygons, point);
    
    if ( normal[2] > 0 ) {
      
      //printf("polygon %d\n", point);
      scanline_convert( polygons, point, s, zb, c);
  }
  }
}

void draw_polygons_flat(struct matrix * polygons, screen s, zbuffer zb, double ** lightSources, int lSlength, color c_Ambient, struct * constants consts)
{
  if(polygons->lastcol < 3)
  {
    printf("Need at least 3 points to draw a polygon!\n");
    return;
  }

  int point;
  double * normal;

  for(point = 0; point < polygons->lastcol - 2; point += 3) {

    normal = calculate_normal(polygons, point);

    if(normal[2] > 0)
    {
      scanline_convert_flat(polygons, point, s, zb, lightSources, lSlength, c_Ambient, consts);
    }

  }
}

/*======== void add_box() ==========
  Inputs:   struct matrix * edges
            double x
	    double y
	    double z
	    double width
	    double height
	    double depth
  Returns: 

  add the points for a rectagular prism whose 
  upper-left corner is (x, y, z) with width, 
  height and depth dimensions.
  ====================*/
void add_box( struct matrix * polygons, double x, double y, double z, double width, double height, double depth ) {

  double x1, y1, z1;
  x1 = x+width;
  y1 = y-height;
  z1 = z-depth;

  //front
  add_polygon(polygons, x, y, z, x1, y1, z, x1, y, z);
  add_polygon(polygons, x, y, z, x, y1, z, x1, y1, z);
  
  //back
  add_polygon(polygons, x1, y, z1, x, y1, z1, x, y, z1);
  add_polygon(polygons, x1, y, z1, x1, y1, z1, x, y1, z1);
  
  //right side
  add_polygon(polygons, x1, y, z, x1, y1, z1, x1, y, z1);
  add_polygon(polygons, x1, y, z, x1, y1, z, x1, y1, z1);
  //left side
  add_polygon(polygons, x, y, z1, x, y1, z, x, y, z);
  add_polygon(polygons, x, y, z1, x, y1, z1, x, y1, z);
  
  //top
  add_polygon(polygons, x, y, z1, x1, y, z, x1, y, z1);
  add_polygon(polygons, x, y, z1, x, y, z, x1, y, z);
  //bottom
  add_polygon(polygons, x, y1, z, x1, y1, z1, x1, y1, z);
  add_polygon(polygons, x, y1, z, x, y1, z1, x1, y1, z1);
}//end add_box

/*======== void add_sphere() ==========
  Inputs:   struct matrix * points
            double cx
	    double cy
	    double cz
	    double r
	    double step  
  Returns: 

  adds all the points for a sphere with center 
  (cx, cy, cz) and radius r.

  should call generate_sphere to create the
  necessary points
  ====================*/
void add_sphere( struct matrix * edges, double cx, double cy, double cz, double r, double step ) {

  struct matrix *points = generate_sphere(cx, cy, cz, r, step);
  int num_steps = (int)(1/step +0.1);
  int p0, p1, p2, p3, lat, longt;
  int latStop, longStop, latStart, longStart;
  latStart = 0;
  latStop = num_steps;
  longStart = 0;
  longStop = num_steps;

  num_steps++;
  for ( lat = latStart; lat < latStop; lat++ ) {
    for ( longt = longStart; longt < longStop; longt++ ) {

      p0 = lat * (num_steps) + longt;
      p1 = p0+1;
      p2 = (p1+num_steps) % (num_steps * (num_steps-1));
      p3 = (p0+num_steps) % (num_steps * (num_steps-1));

      if ( longt < longStop-1 ) 
	add_polygon( edges, points->m[0][p0],
		     points->m[1][p0],
		     points->m[2][p0],
		     points->m[0][p1],
		     points->m[1][p1],
		     points->m[2][p1],
		     points->m[0][p2],
		     points->m[1][p2],
		     points->m[2][p2]);
	if ( longt >  0 )
	  add_polygon( edges, points->m[0][p0],
		       points->m[1][p0],
		       points->m[2][p0],
		       points->m[0][p2],
		       points->m[1][p2],
		       points->m[2][p2],
		       points->m[0][p3],
		       points->m[1][p3],
		       points->m[2][p3]);
	//}//end non edge latitude
    }
  }  
  free_matrix(points);
}

/*======== void generate_sphere() ==========
  Inputs:   double cx
	    double cy
	    double cz
	    double r
	    double step  
  Returns: Generates all the points along the surface 
           of a sphere with center (cx, cy, cz) and
	   radius r.
	   Returns a matrix of those points
  ====================*/
struct matrix * generate_sphere(double cx, double cy, double cz, double r, double step ) {

  int num_steps = (int)(1/step +0.1);
  
  struct matrix *points = new_matrix(4, num_steps * num_steps);
  int circle, rotation, rot_start, rot_stop, circ_start, circ_stop;
  double x, y, z, rot, circ;
  
  rot_start = 0;
  rot_stop = num_steps;
  circ_start = 0;
  circ_stop = num_steps;
  
  for (rotation = rot_start; rotation < rot_stop; rotation++) {
    rot = (double)rotation / num_steps;
    
    for(circle = circ_start; circle <= circ_stop; circle++){
      circ = (double)circle / num_steps;

      x = r * cos(M_PI * circ) + cx;
      y = r * sin(M_PI * circ) *
	cos(2*M_PI * rot) + cy;
      z = r * sin(M_PI * circ) *
	sin(2*M_PI * rot) + cz;
      
      /* printf("rotation: %d\tcircle: %d\n", rotation, circle); */
      /* printf("rot: %lf\tcirc: %lf\n", rot, circ); */
      /* printf("sphere point: (%0.2f, %0.2f, %0.2f)\n\n", x, y, z); */
      add_point(points, x, y, z);
    }
  }
  
  return points;
}

/*======== void add_torus() ==========
  Inputs:   struct matrix * points
            double cx
	    double cy
	    double cz
	    double r1
	    double r2
	    double step  
  Returns: 

  adds all the points required to make a torus
  with center (cx, cy, cz) and radii r1 and r2.

  should call generate_torus to create the
  necessary points
  ====================*/
void add_torus( struct matrix * edges, double cx, double cy, double cz, double r1, double r2, double step ) {
  
  struct matrix *points = generate_torus(cx, cy, cz, r1, r2, step);
  int num_steps = (int)(1/step +0.1);
  int p0, p1, p2, p3, lat, longt;
  int latStop, longStop, latStart, longStart;
  latStart = 0;
  latStop = num_steps;
  longStart = 0;
  longStop = num_steps;
  
  for ( lat = latStart; lat < latStop; lat++ ) {
    for ( longt = longStart; longt < longStop; longt++ ) {
      
      p0 = lat * (num_steps) + longt;
      if (longt == num_steps - 1)
	p1 = p0 - longt;
      else
	p1 = p0 + 1;
      p2 = (p0 + num_steps) % (num_steps * num_steps);
      p3 = (p1 + num_steps) % (num_steps * num_steps);
      
      //printf("p0: %d\tp1: %d\tp2: %d\tp3: %d\n", p0, p1, p2, p3);
      
      add_polygon( edges, points->m[0][p0],
		   points->m[1][p0],
		   points->m[2][p0],
		   points->m[0][p2],
		   points->m[1][p2],
		   points->m[2][p2],
		   points->m[0][p3],
		   points->m[1][p3],
		   points->m[2][p3]);
      add_polygon( edges, points->m[0][p0],
		   points->m[1][p0],
		   points->m[2][p0],
		   points->m[0][p3],
		   points->m[1][p3],
		   points->m[2][p3],
		   points->m[0][p1],
		   points->m[1][p1],
		   points->m[2][p1]);
      
      
    }
  }  
  free_matrix(points);
}


/*======== void generate_torus() ==========
  Inputs:   struct matrix * points
            double cx
	    double cy
	    double cz
	    double r
	    double step  
  Returns: Generates all the points along the surface 
           of a torus with center (cx, cy, cz) and
	   radii r1 and r2.
	   Returns a matrix of those points
  ====================*/
struct matrix * generate_torus( double cx, double cy, double cz, double r1, double r2, double step ) {
  int num_steps = (int)(1/step +0.1);
  
  struct matrix *points = new_matrix(4, num_steps * num_steps);
  int circle, rotation, rot_start, rot_stop, circ_start, circ_stop;
  double x, y, z, rot, circ;

  rot_start = 0;
  rot_stop = num_steps;
  circ_start = 0;
  circ_stop = num_steps;
  
  for (rotation = rot_start; rotation < rot_stop; rotation++) {
    rot = (double)rotation / num_steps;
    
    for(circle = circ_start; circle < circ_stop; circle++){
      circ = (double)circle / num_steps;

      x = cos(2*M_PI * rot) *
	(r1 * cos(2*M_PI * circ) + r2) + cx;
      y = r1 * sin(2*M_PI * circ) + cy;
      z = -1*sin(2*M_PI * rot) *
	(r1 * cos(2*M_PI * circ) + r2) + cz;
      
      //printf("rotation: %d\tcircle: %d\n", rotation, circle);
      //printf("torus point: (%0.2f, %0.2f, %0.2f)\n", x, y, z);
      add_point(points, x, y, z);
    }
  }
  
  return points;
}

/*======== void add_circle() ==========
  Inputs:   struct matrix * points
            double cx
	    double cy
	    double r
	    double step  
  Returns: 

  Adds the circle at (cx, cy) with radius r to edges
  ====================*/
void add_circle( struct matrix * edges, double cx, double cy, double cz, double r, double step ) {
  
  double x0, y0, x1, y1, t;

  x0 = r + cx;
  y0 = cy;
  for (t=step; t <= 1.00001; t+= step) {
    
    x1 = r * cos(2 * M_PI * t) + cx;
    y1 = r * sin(2 * M_PI * t) + cy;

    add_edge(edges, x0, y0, cz, x1, y1, cz);
    x0 = x1;
    y0 = y1;
  }
}

/*======== void add_curve() ==========
Inputs:   struct matrix *points
         double x0
         double y0
         double x1
         double y1
         double x2
         double y2
         double x3
         double y3
         double step
         int type  
Returns: 

Adds the curve bounded by the 4 points passsed as parameters
of type specified in type (see matrix.h for curve type constants)
to the matrix points
====================*/
void add_curve( struct matrix *edges, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double step, int type ) {

  double t, x, y; 
  struct matrix *xcoefs;
  struct matrix *ycoefs;

  xcoefs = generate_curve_coefs(x0, x1, x2, x3, type);
  ycoefs = generate_curve_coefs(y0, y1, y2, y3, type);
  
  /* print_matrix(xcoefs); */
  /* printf("\n"); */
  /* print_matrix(ycoefs); */
  
  for (t=step; t <= 1.000001; t+= step) {

    x = xcoefs->m[0][0] *t*t*t + xcoefs->m[1][0] *t*t+
      xcoefs->m[2][0] *t + xcoefs->m[3][0];
    y = ycoefs->m[0][0] *t*t*t + ycoefs->m[1][0] *t*t+
      ycoefs->m[2][0] *t + ycoefs->m[3][0];
    
    add_edge(edges, x0, y0, 0, x, y, 0);
    x0 = x;
    y0 = y;
  }
  
  free_matrix(xcoefs);
  free_matrix(ycoefs);
}


/*======== void add_point() ==========
Inputs:   struct matrix * points
         int x
         int y
         int z 
Returns: 
adds point (x, y, z) to points and increment points.lastcol
if points is full, should call grow on points
====================*/
void add_point( struct matrix * points, double x, double y, double z) {

  if ( points->lastcol == points->cols )
    grow_matrix( points, points->lastcol + 100 );
  
  points->m[0][ points->lastcol ] = x;
  points->m[1][ points->lastcol ] = y;
  points->m[2][ points->lastcol ] = z;
  points->m[3][ points->lastcol ] = 1;
  points->lastcol++;
} //end add_point

/*======== void add_edge() ==========
Inputs:   struct matrix * points
          int x0, int y0, int z0, int x1, int y1, int z1
Returns: 
add the line connecting (x0, y0, z0) to (x1, y1, z1) to points
should use add_point
====================*/
void add_edge( struct matrix * points, double x0, double y0, double z0, double x1, double y1, double z1) {
  add_point( points, x0, y0, z0 );
  add_point( points, x1, y1, z1 );
}

/*======== void draw_lines() ==========
Inputs:   struct matrix * points
         screen s
         color c 
Returns: 
Go through points 2 at a time and call draw_line to add that line
to the screen
====================*/
void draw_lines( struct matrix * points, screen s, zbuffer zb, color c) {

 if ( points->lastcol < 2 ) {
   printf("Need at least 2 points to draw a line!\n");
   return;
 }
 
 int point;
 for (point=0; point < points->lastcol-1; point+=2)
   draw_line( points->m[0][point],
	      points->m[1][point],
	      points->m[2][point],
	      points->m[0][point+1],
	      points->m[1][point+1],
	      points->m[2][point + 1],
	      s, zb, c);	       
}// end draw_lines

void draw_line(int x0, int y0, double z0, int x1, int y1, double z1, screen s, zbuffer zb, color c) {
  
  int x, y, d, A, B;
  int dy_east, dy_northeast, dx_east, dx_northeast, d_east, d_northeast;
  int loop_start, loop_end;
  double distance;
  double z, dz;

  //swap points if going right -> left
  int xt, yt;
  if (x0 > x1) {
    xt = x0;
    yt = y0;
    z = z0;
    x0 = x1;
    y0 = y1;
    z0 = z1;
    x1 = xt;
    y1 = yt;
    z1 = z;
  }

  x = x0;
  y = y0;
  z = z0; dz = (z1 - z0) / (x1 - x0); //Now we can increment z by dz * (change in x) in each iteration.
  A = 2 * (y1 - y0);
  B = -2 * (x1 - x0);
  int wide = 0;
  int tall = 0;
  //octants 1 and 8
  if ( abs(x1 - x0) >= abs(y1 - y0) ) { //octant 1/8
    wide = 1;
    loop_start = x;
    loop_end = x1;
    dx_east = dx_northeast = 1;
    dy_east = 0;
    d_east = A;
    distance = x1 - x;
    if ( A > 0 ) { //octant 1
      d = A + B/2;
      dy_northeast = 1;
      d_northeast = A + B;
    }
    else { //octant 8
      d = A - B/2;
      dy_northeast = -1;
      d_northeast = A - B;
    }
  }//end octant 1/8
  else { //octant 2/7
    tall = 1;
    dx_east = 0;
    dx_northeast = 1;    
    distance = abs(y1 - y);
    if ( A > 0 ) {     //octant 2
      d = A/2 + B;
      dy_east = dy_northeast = 1;
      d_northeast = A + B;
      d_east = B;
      loop_start = y;
      loop_end = y1;
    }
    else {     //octant 7
      d = A/2 - B;
      dy_east = dy_northeast = -1;
      d_northeast = A - B;
      d_east = -1 * B;
      loop_start = y1;
      loop_end = y;
    }
  }

  while ( loop_start < loop_end ) {
    
    plot( s, zb, c, x, y, z );
    if ( (wide && ((A > 0 && d > 0) ||
		   (A < 0 && d < 0)))
	 ||
	 (tall && ((A > 0 && d < 0 ) ||
		   (A < 0 && d > 0) ))) {
      y+= dy_northeast;
      d+= d_northeast;
      x+= dx_northeast;
      z += dz * dx_northeast;
    }
    else {
      x+= dx_east;
      y+= dy_east;
      d+= d_east;
      z += dz * dx_east;
    }
    loop_start++;
  } //end drawing loop
  plot( s, zb, c, x1, y1, z );
} //end draw_line
