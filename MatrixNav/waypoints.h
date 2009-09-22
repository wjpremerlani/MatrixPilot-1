
// you have the option of using either cross tracking,
// in which navigation is based on the distance of the plane
// to the line between the waypoints,
// or you can use navigation directly toward the goal point.
// if you want to use cross tracking, define CROSSTRACKING,
// otherwise, to use navigation directly toward the goal,
// comment out the following #define:

#define CROSSTRACKING

// waypoint definitions

// define the course as
// const struct relative3D waypoints[] = {
//						waypoint1 ,
//						waypoint2 ,
//
//						etc.
//							}

// a waypoint is defined as { X , Y , Z } 
// where X, Y, and Z are the three coordinates of the waypoint in meters, 
// relative to the initialization location of the board.
// X is positive as you move east
// Y is positive as you move north
// Z is the height
//
// you do not need to specify how many points you have, the compiler will count them for you.
// you can use the facilities of the compiler to do some simple calculations in defining the course.

// As an example, here is the T3 contest course

// initialize the board at the center of the course

// CORNER is the absolute value of the X or Y coordinate
 
#define CORNER 100

// CLEARANCE is an allowance for obstacles.

#define CLEARANCE 25

									
// here is the T3 course definition

const struct relative3D waypoints[] = {

						{    CORNER  ,    CORNER  , CLEARANCE + 100 } ,
						{    CORNER  ,  - CORNER  , CLEARANCE +  75 } ,
						{  - CORNER  ,    CORNER  , CLEARANCE +  50 } ,
						{  - CORNER  ,  - CORNER  , CLEARANCE +  25 } ,
						{    CORNER  ,    CORNER  , CLEARANCE +  50 } ,
						{    CORNER  ,  - CORNER  , CLEARANCE +  75 } ,
						{  - CORNER  ,    CORNER  , CLEARANCE + 100 } ,
						{  - CORNER  ,  - CORNER  , CLEARANCE +  75 } ,
						{    CORNER  ,    CORNER  , CLEARANCE +  50 } ,
						{    CORNER  ,  - CORNER  , CLEARANCE +  25 } ,
						{  - CORNER  ,    CORNER  , CLEARANCE +  50 } ,
						{  - CORNER  ,  - CORNER  , CLEARANCE +  75 } ,

										} ;	
