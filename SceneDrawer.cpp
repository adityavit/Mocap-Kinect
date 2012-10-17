

/*******************************************************************************
*                                                                              *
*   PrimeSense NITE 1.3 - Players Sample                                       *
*   Copyright (C) 2010 PrimeSense Ltd.                                         *
*                                                                              *
*******************************************************************************/

#include "SceneDrawer.h"
#include <math.h>;
#include <sstream>
#include <string>
#ifdef USE_GLUT
#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
        #include <GLUT/glut.h>
#else
        #include <GL/glut.h>
#endif
#else
#include "opengles.h"
#endif

extern xn::UserGenerator g_UserGenerator;
extern xn::DepthGenerator g_DepthGenerator;

#define MAX_DEPTH 10000
float g_pDepthHist[MAX_DEPTH];
int g_ball1Radius = 50;
int g_ball2Radius = 50;
int g_ball1CenterX = 200;
int g_ball1CenterY = 100;
int g_ball2CenterX = 500;
int g_ball2CenterY = 300;
int g_windowHeight = 480;
int g_windowWidth = 640;
int g_ballSpeed = 5;
int g_ballRadiusRateIncrease = 5;
int g_maxRadius = 100;
int g_initialRadius = 50;
bool isBall1Hidden = false;
bool isBall2Hidden = false;
int g_globalScore = 0;
unsigned int g_ball1DepthThreshold = MAX_DEPTH;
unsigned int g_ball2DepthThreshold = MAX_DEPTH;
unsigned int getClosestPowerOfTwo(unsigned int n)
{
	unsigned int m = 2;
	while(m < n) m<<=1;

	return m;
}
GLuint initTexture(void** buf, int& width, int& height)
{
	GLuint texID = 0;
	glGenTextures(1,&texID);

	width = getClosestPowerOfTwo(width);
	height = getClosestPowerOfTwo(height); 
	*buf = new unsigned char[width*height*4];
	glBindTexture(GL_TEXTURE_2D,texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texID;
}

GLfloat texcoords[8];
void DrawRectangle(float topLeftX, float topLeftY, float bottomRightX, float bottomRightY)
{
	GLfloat verts[8] = {	topLeftX, topLeftY,
		topLeftX, bottomRightY,
		bottomRightX, bottomRightY,
		bottomRightX, topLeftY
	};
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	//TODO: Maybe glFinish needed here instead - if there's some bad graphics crap
	glFlush();
}
void DrawTexture(float topLeftX, float topLeftY, float bottomRightX, float bottomRightY)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	DrawRectangle(topLeftX, topLeftY, bottomRightX, bottomRightY);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

XnFloat Colors[][3] =
{
	{0,1,1},
	{0,0,1},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,.5,0},
	{.5,1,0},
	{0,.5,1},
	{.5,0,1},
	{1,1,.5},
	{1,1,1}
};
XnUInt32 nColors = 10;

void glPrintString(void *font,const char *str)
{
	size_t i,l = strlen(str);
  glRasterPos2f(20,20);
	for(i=0; i<l; i++)
	{
		glutBitmapCharacter(font,*str++);
	}
}
  
void DrawLimb(XnUserID player, XnSkeletonJoint eJoint1, XnSkeletonJoint eJoint2)
{
	if (!g_UserGenerator.GetSkeletonCap().IsCalibrated(player))
	{
		printf("not calibrated!\n");
		return;
	}
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		printf("not tracked!\n");
		return;
	}

	XnSkeletonJointPosition joint1, joint2;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint1, joint1);
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint2, joint2);

	if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5)
	{
		return;
	}

	XnPoint3D pt[2];
	pt[0] = joint1.position;
	pt[1] = joint2.position;

	g_DepthGenerator.ConvertRealWorldToProjective(2, pt, pt);
	glVertex3i(pt[0].X, pt[0].Y, 0);
	glVertex3i(pt[1].X, pt[1].Y, 0);
}

const float DEG2RAD = 3.14159/180;
const float PI = 3.14159;

//Function to draw circle using openGL
void drawCircle(int x, int y, float radius)
{
	glBegin(GL_LINE_LOOP);
	for (int i=0; i < 360; i++)
	{
		float degInRad = i*DEG2RAD;
		glVertex2f(x+cos(degInRad)*radius, y+sin(degInRad)*radius);
	}
	glEnd();
}

void drawCompleteCircle(int x,int y,float radius){
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(x,y);
  for(int i=0;i < 360; i++){
    float degInRad  = i*DEG2RAD;
    glVertex2f(x+cos(degInRad)*radius,y+sin(degInRad)*radius);
  }
  glEnd();
}

void DrawDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd, XnUserID player)
{
	static bool bInitialized = false;	
	static GLuint depthTexID;
	static unsigned char* pDepthTexBuf;
	static int texWidth, texHeight;

	 float topLeftX;
	 float topLeftY;
	 float bottomRightY;
	 float bottomRightX;
	float texXpos;
	float texYpos;

	if(!bInitialized)
	{

		texWidth =  getClosestPowerOfTwo(dmd.XRes());
		texHeight = getClosestPowerOfTwo(dmd.YRes());

//		printf("Initializing depth texture: width = %d, height = %d\n", texWidth, texHeight);
		depthTexID = initTexture((void**)&pDepthTexBuf,texWidth, texHeight) ;

//		printf("Initialized depth texture: width = %d, height = %d\n", texWidth, texHeight);
		bInitialized = true;

		topLeftX = dmd.XRes();
		topLeftY = 0;
		bottomRightY = dmd.YRes();
		bottomRightX = 0;
		texXpos =(float)dmd.XRes()/texWidth;
		texYpos  =(float)dmd.YRes()/texHeight;

		memset(texcoords, 0, 8*sizeof(float));
		texcoords[0] = texXpos, texcoords[1] = texYpos, texcoords[2] = texXpos, texcoords[7] = texYpos;

	}
	unsigned int nValue = 0;
	unsigned int nHistValue = 0;
	unsigned int nIndex = 0;
	unsigned int nX = 0;
	unsigned int nY = 0;
	unsigned int nNumberOfPoints = 0;
	XnUInt16 g_nXRes = dmd.XRes();
	XnUInt16 g_nYRes = dmd.YRes();
  unsigned int pDepthSum = 0;
  unsigned int pMeanDepth = 0;
	unsigned char* pDestImage = pDepthTexBuf;

	const XnDepthPixel* pDepth = dmd.Data();
	const XnLabel* pLabels = smd.Data();

	int ind = 0;
	XnDepthPixel arrToSort[9];
	
	//Kinect return a 16bit value in which first 13 bits give you depth in milimeters
	int minDepth = MAX_DEPTH;
	
	// Calculate the accumulative histogram
	memset(g_pDepthHist, 0, MAX_DEPTH*sizeof(float));
	for (nY=0; nY<g_nYRes; nY++)
	{
		for (nX=0; nX<g_nXRes; nX++)
		{
			nValue = *pDepth;

			if (nValue != 0)
			{
				//computing min depth value
				if (nValue < minDepth) {
					minDepth = nValue;
				}
				//computing histogram
				g_pDepthHist[nValue]++;
				nNumberOfPoints++;
			}

			pDepth++;
		}
	}
    
	
	float g_pDepthHist_orig[MAX_DEPTH];
	memcpy(g_pDepthHist_orig,g_pDepthHist,MAX_DEPTH);
	//Computing cumulative histogram
	for (nIndex=1; nIndex<MAX_DEPTH; nIndex++)
	{
		g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
	}
	
	//converting original depth value to a number between 0-256
	if (nNumberOfPoints)
	{
		for (nIndex=1; nIndex<MAX_DEPTH; nIndex++)
		{
			g_pDepthHist[nIndex] = (unsigned int)(256 * (1.0f - (g_pDepthHist[nIndex] / nNumberOfPoints)));
		}
	}

	//midX and midY are the x and y coordinates of the center of the closest object
	int midX = 0;
	int midY = 0;
	int numPixelsOfClosestObject = 0;
	pDepth = dmd.Data();
	{
		XnUInt32 nIndex = 0;
		// Prepare the texture map
		for (nY=0; nY<g_nYRes; nY++)
		{
			for (nX=0; nX < g_nXRes; nX++, nIndex++)
			{
				nValue = *pDepth;
				XnLabel label = *pLabels;
				XnUInt32 nColorID = label % nColors;
				if (label == 0)
				{
					nColorID = nColors;
				}
				
				int depthRange = 30;
				int minObjectSize = 10;
				//set color of the current pixel in the image to g_pDepthHist[nValue](which is between 0-256) only if
				//a-> nValue is between [minDepth - (minDepth+depthRange)],
				//    You can use different values than "depthRange" to increase or decrease the range of the depth of the pixels you want to display
				//b-> Consider this pixel only if number of pixels at this depth are more than minObjectSize(you can again play around with this number), 
				//    This is used to eliminate the noise pixels
				//NOTE: We want you to play around with these values and decide appropriate working values(and understand how does it work)
				//so that's why we have not provided the "good" values, I suggest to play with 2 these values one at a time
				if (nValue != 0 && nValue >=minDepth && nValue < (minDepth + depthRange) && g_pDepthHist_orig[nValue] > minObjectSize)
				{
					nHistValue = g_pDepthHist[nValue];

					pDestImage[0] = nHistValue;
					pDestImage[1] = nHistValue;
					pDestImage[2] = nHistValue;
					
					midX += nX;
					midY += nY;
					numPixelsOfClosestObject++;
				}
				else
				{
					pDestImage[0] = 0;
					pDestImage[1] = 0;
					pDestImage[2] = 0;
				}

				pDepth++;
				pLabels++;
				pDestImage+=3;
			}
      pDepthSum += nValue;
			pDestImage += (texWidth - g_nXRes) *3;
		}
	}

	//compute middle pixel
	midX/=numPixelsOfClosestObject;
	midY/=numPixelsOfClosestObject;
	pMeanDepth = pDepthSum/numPixelsOfClosestObject;
	glBindTexture(GL_TEXTURE_2D, depthTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pDepthTexBuf);
	glColor4f(0.75,0.75,0.75,1);
	glEnable(GL_TEXTURE_2D);
	DrawTexture(dmd.XRes(),dmd.YRes(),0,0);	
	glDisable(GL_TEXTURE_2D);
	
	//Set the color for the circle
	glColor3f(1.0f,0.0f,0.0f);
	
	//draw circle with center=(midX,midY) and radius = count/100
	//You can use different centre points and radius here
	//Here I have used numPixelsOfClosestObject/100 just to make radius of circle proportional to number of pixels of "closest object"	
	if(numPixelsOfClosestObject > 0)
		drawCircle(midX,midY,numPixelsOfClosestObject/100);
  g_ball1CenterY += g_ballSpeed;
  g_ball2CenterY += g_ballSpeed;
  glColor3f(0.0f,1.0f,0.0f);
  if(!isBall1Hidden){
     drawCompleteCircle(g_ball1CenterX,g_ball1CenterY,g_ball1Radius);
  }
  glColor3f(0.0f,0.0f,1.0f);
  if(!isBall2Hidden){
    drawCompleteCircle(g_ball2CenterX,g_ball2CenterY,g_ball2Radius);
  }
  if(g_ball1CenterY == g_windowHeight + g_ball1Radius){
    g_ball1CenterY = 0;
    g_ball1Radius = g_initialRadius;
    isBall1Hidden = false;
    g_ball1DepthThreshold = MAX_DEPTH;
  }
  if(g_ball2CenterY == g_windowHeight + g_ball2Radius){
    g_ball2CenterY = 0;
    g_ball2Radius = g_initialRadius;
    isBall2Hidden = false;
    g_ball2DepthThreshold = MAX_DEPTH;
  }
  if(midX >= g_ball1CenterX - g_ball1Radius && midX <= g_ball1CenterX + g_ball1Radius && midY >= g_ball1CenterY - g_ball1Radius && midY <= g_ball1CenterY + g_ball1Radius && g_ball1Radius <= g_maxRadius && pMeanDepth < g_ball1DepthThreshold){
    g_ball1Radius += g_ballRadiusRateIncrease; 
    g_ball1DepthThreshold = pMeanDepth;
  }
  if(midX >= g_ball2CenterX - g_ball2Radius && midX <= g_ball2CenterX + g_ball2Radius && midY >= g_ball2CenterY - g_ball2Radius && midY <= g_ball2CenterY + g_ball2Radius && g_ball2Radius <= g_maxRadius && pMeanDepth < g_ball2DepthThreshold){
    g_ball2Radius += g_ballRadiusRateIncrease; 
    g_ball2DepthThreshold = pMeanDepth;
  }
  if(g_ball1Radius  == g_maxRadius){
    isBall1Hidden = true;
    g_globalScore++;
    g_ball1DepthThreshold = MAX_DEPTH;
  }
  if(g_ball2Radius == g_maxRadius){
    isBall2Hidden = true;
    g_globalScore++;
    g_ball2DepthThreshold = MAX_DEPTH;
  }
  std::stringstream out;
  out << g_globalScore;
  std::string scoreString = "Score: " + out.str();
//  printf("%s\n",scoreString.c_str());
  glColor3f(1.0f,0.0f,0.0f);
  glPrintString(GLUT_BITMAP_TIMES_ROMAN_24,scoreString.c_str());
  
}
