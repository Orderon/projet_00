// projet_00.cpp : Defines the entry point for the console application.
//
// inspired from http://r3dux.org/
// Library dependencies: OpenCV core, video and highgui libs, GL, glew, glfw
// Kinect with OpenCV usage guide: http://opencv.itseez.com/doc/user_guide/ug_highgui.html
#include "stdafx.h"
#include "multipleImages.hpp"
#include <math.h>
//#include "glew.h"
//#include <GLFW/glfw3.h>
//#include <GLFW\glfw3native.h>
//#include <gl\GL.h>
//#include <gl\GLU.h>
#define WAIT 30 //wait in ms after each frame
#define EYE_DISTANCE 0//50 //in px
#define EYE_ANGLE 0//2 //degrees
#define INPLANE_ROT 0 //90 degrees rotation for oculus!
#define LIMIT_FPS 500 //max frames per second
#define FULLSCREEN_W  2060
#define FULLSCREEN_H 2060
#define INIT_X 900
#define INIT_Y 600
#define DIST_BETW_PICTS 30
#define DIST_Y 100
#define DIST_X 50

bool shouldExit = false; // set true to terminate.

using namespace cv;
using namespace std;
//using namespace gl;

//title of the window
char * title = "3d Drone view";


// Frame counting and limiting
int    frameCount = 0;
double frameStartTime, frameEndTime, frameRate;
//GLFWwindow* window = NULL;

//inspired from http://stackoverflow.com/questions/35410744/fisheye-distortion-correction as well as
// from http://stackoverflow.com/questions/28130618/what-ist-the-correct-oculus-rift-barrel-distortion-radius-function
/*
returns the img wiht a barrel distortion adapted for the oculus rift.
*/
Mat_<Vec3b> barrel_dist(Mat_<Vec3b> img)
{
	static Mat_<Vec3b> dstImage(img.rows,img.cols);
	static Mat_<Vec3f> transf(img.rows,img.cols);
	static int should_init = 1;
	int sourceX;
	int sourceY;
	//everything is initialized, just calculate as fast as possible..
	if(!should_init){
		for (int i = 0; i < dstImage.rows; ++i)
		{
			for (int j = 0; j < dstImage.cols; j++)
			{
				sourceX = transf(i, j)[0];
				sourceY = transf(i, j)[1];
				
				//if sourceX < 0 then pixel doesn't have to be redrawn (outside of transformation area)
				if (transf(i,j)[2]<1)
				{
					//put pixels
					dstImage(i, j)[0] = img(sourceX, sourceY)[0];
					dstImage(i, j)[1] = img(sourceX, sourceY)[1];
					dstImage(i, j)[2] = img(sourceX, sourceY)[2];
				} //else do nothing --> dstImage stays the way it is: black
				else {
				}
			}
		}
		return dstImage;
	}
	//if should init:
		cout << "init transformation\n";
		should_init = 0;
		int halfWidth = (img).rows / 2;
		int halfHeight = (img).cols / 2;
		double strength = 1;
		double zoom = 0.9;
		double correctionRadius = sqrt(pow((img).rows, 2) + pow((img).cols, 2)) / strength;
		int newX, newY;
		double distance;
		double theta;
		double r;

		for (int i = 0; i < dstImage.rows; ++i)
		{
			for (int j = 0; j < dstImage.cols; j++)
			{
				newX = i - halfWidth;
				newY = j - halfHeight;
				distance = sqrt(pow(newX, 2) + pow(newY, 2));
				r = distance / correctionRadius;

				double scale = zoom*(0.24*pow(r, 2) + 0.22*r + 1.0);

				sourceX = round(halfWidth + scale*newX);
				sourceY = round(halfHeight + scale*newY);

				//if outside of transformation area
				if (sourceX < 0 || sourceX >2 * halfWidth || sourceY < 0 || sourceY > 2 * halfHeight)
				{
					//set color to Blue/Black
					dstImage(i, j)[0] = 250;
					dstImage(i, j)[1] = 250;
					dstImage(i, j)[2] = 0;
				
					//set transf[2] to >1 in order to detect that this pixel doesn't have to be redrawn
					transf(i, j)[0] = 0;
					transf(i, j)[1] = 0;
					transf(i, j)[2] = 10;
				}
				//inside the transformation area
				else {

					//store destination of pixels in transf matrix
					transf(i, j)[0] = sourceX;
					transf(i, j)[1] = sourceY;
					transf(i, j)[2] = 0;

					//calculate very first image frame
					dstImage(i, j)[0] = img(sourceX, sourceY)[0];
					dstImage(i, j)[1] = img(sourceX, sourceY)[1];
					dstImage(i, j)[2] = img(sourceX, sourceY)[2];
				}
			}
		}

	return dstImage;
}



Mat_<Vec3b> merge2Images(Mat_<Vec3b> left, Mat_<Vec3b> right) {
		//statics to improve calculation speed
		static int width = left.cols + right.cols+ DIST_BETW_PICTS+ DIST_X*2;
		static int height = max(left.rows, right.rows)+ DIST_Y*2;
		static Mat_<Vec3b> merged(height, width);

		left.copyTo(merged(Rect(DIST_X, DIST_Y, left.cols, left.rows)));
		right.copyTo(merged(Rect(left.cols+ DIST_BETW_PICTS + DIST_X, DIST_Y, right.cols, right.rows)));

	return merged;
}


/* Rotates an Image around:
alpha = x
beta = y
gamma = z
axis.
dx,dy,dz are shifts in the corresponding axis.
f is the focal lenghts (should be equal to dz for whole picture to be visible)
*/
void rotateImage(const Mat &input, Mat &output, double alpha, double beta, double gamma, double dx, double dy, double dz, double f)
{
	alpha = (alpha - 90.)*CV_PI / 180.;
	beta = (beta - 90.)*CV_PI / 180.;
	gamma = (gamma - 90.)*CV_PI / 180.;
	// get width and height for ease of use in matrices
	double w = (double)input.cols;
	double h = (double)input.rows;
	// Projection 2D -> 3D matrix
	Mat A1 = (Mat_<double>(4, 3) <<
		1, 0, -w / 2,
		0, 1, -h / 2,
		0, 0, 0,
		0, 0, 1);
	// Rotation matrices around the X, Y, and Z axis
	Mat RX = (Mat_<double>(4, 4) <<
		1, 0, 0, 0,
		0, cos(alpha), -sin(alpha), 0,
		0, sin(alpha), cos(alpha), 0,
		0, 0, 0, 1);
	Mat RY = (Mat_<double>(4, 4) <<
		cos(beta), 0, -sin(beta), 0,
		0, 1, 0, 0,
		sin(beta), 0, cos(beta), 0,
		0, 0, 0, 1);
	Mat RZ = (Mat_<double>(4, 4) <<
		cos(gamma), -sin(gamma), 0, 0,
		sin(gamma), cos(gamma), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
	// Composed rotation matrix with (RX, RY, RZ)
	Mat R = RX * RY * RZ;
	// Translation matrix
	Mat T = (Mat_<double>(4, 4) <<
		1, 0, 0, dx,
		0, 1, 0, dy,
		0, 0, 1, dz,
		0, 0, 0, 1);
	// 3D -> 2D matrix
	Mat A2 = (Mat_<double>(3, 4) <<
		f, 0, w / 2, 0,
		0, f, h / 2, 0,
		0, 0, 1, 0);
	// Final transformation matrix
	Mat trans = A2 * (T * (R * A1));
	// Apply matrix transformation
	warpPerspective(input, output, trans, input.size(), INTER_LANCZOS4);
}

/*
Draws the frame (given as Mat) as stereoscopic view into the named window "title"
TBD: Ajust parameters EYE_ANGLE and EYE_DISTANCE
*/
void draw(Mat_<Vec3b> frame, char* title) {
	Mat_<Vec3b> left, right; //create Mat to store viewpoints

	//IplImage* imLeft = cvCloneImage(&(IplImage)left);
	//IplImage* imRight = cvCloneImage(&(IplImage)right);
	//namedWindow("test");
//		//	imshow("test", frame);
	left = barrel_dist(frame);
	right = barrel_dist(frame);
	imshow(title, merge2Images(left, right));
	//imshow(title, merge2Images(frame, frame));
	//draw on splitscreen
	//cvShowManyImages(title, 2, imLeft, imRight);
	
}




void draw2(Mat frame, char* title) {
	Mat left, right; //create Mat to store viewpoints	
					 // calculate perspectives
	/*int left_b = 90;
	int left_a = (90 + EYE_ANGLE / 2);
	int left_c = 90 + INPLANE_ROT;
	int left_dx = -(EYE_DISTANCE / 2);
	int left_dy = 0;
	int dz = 200;
	int right_b = 90;
	int right_a = (90 - EYE_ANGLE / 2);
	int right_c = 90 + INPLANE_ROT;
	int right_dx = (EYE_DISTANCE / 2);
	int right_dy = 0;
	int f = 200;*/
	//rotateImage(frame, left, left_a, left_b, left_c, left_dx, left_dy, dz, f);
	//rotateImage(frame, right, right_a, right_b, right_c, right_dx, right_dy, dz, f);

	frame = barrel_dist(frame);
	double cx = 0.24;
	double cy = 0.22;
	double kx = 0.0005;
	double ky = 0.0005;
	//draw on splitscreen
	IplImage* imLeft = cvCloneImage(&(IplImage)frame);
	IplImage* imRight = cvCloneImage(&(IplImage)frame);
//	imRight = barrel_pincusion_dist(imRight, cx, cy, kx, ky);
	cvShowManyImages(title, 2, imLeft, imLeft);
	namedWindow("blabla");
	cvShowImage("blabla", imRight);
	cout << "test\n";
}


/*Adds additional (text) layers onto the image*/
void addInfoLayer(Mat frame) {
	Scalar color = Scalar(250, 250, 250);
	stringstream textstream;
	string text;
	//write text into stringstream
	textstream << "'E' for exit, 'M' for maximise";
	//convert to string
	text = textstream.str();
	//empty stream
	textstream.str(std::string());
	//draw text onto frame
	putText(frame, text, cvPoint(80, 25), cv::FONT_HERSHEY_PLAIN, 2, color);

	//write text into stringstream
	textstream << "fps: " << (int)frameRate;
	//convert to string
	text = textstream.str();
	//empty stream
	textstream.str(std::string());
	//draw text onto frame
	putText(frame, text, cvPoint(100, 400), cv::FONT_HERSHEY_PLAIN, 2, color);
}

/*Takes care of the inputs occured on the display window*/
void handleInputs(char key) {
	if (key == 'e') {
		cout << "Terminating program\n";
		shouldExit = true;
	}

	if (key == 'm'){
		resizeWindow(title, FULLSCREEN_W, FULLSCREEN_H);
		setWindowSize(FULLSCREEN_W, FULLSCREEN_H);
		//cv::setWindowProperty(title, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
		moveWindow(title, 0, 0);
		//todo: max with better solution... -------------------------------------------------------------------------
	}
}

/*fixes the framerate. Ajust LIMIT_FPS if too low*/
void fixFrameRate(double start) {
	char k;
	do {
		frameRate = getTickFrequency() / ((double)getTickCount() - start);

		k = waitKey(1);							 //delay WAIT ms ------------------------------ TODO_ diminuer a max: EVEN AT 1 too long?! --> threads?

		handleInputs(k);
	} while (frameRate > LIMIT_FPS);

	//cout << "frameRate: \t" << frameRate << "\n";
}

int main() {
	// Initialize openCV
	Mat frame;							  //Create Matrix to store image
	VideoCapture cap(0);				  //create videostream
	if (!cap.isOpened()) {			  // check if we succeeded
		cout << "Camera not detected. Will exit now... \n";
		return -1;
	}
	//create Window
	cvNamedWindow(title, 1);
	//cvNamedWindow("123");

	//resizeWindow(title, 900, 600);
	//setWindowSize(INIT_X, INIT_Y);
	//initGL();
	cout << "Initalized..\n";
	bool useOpt = useOptimized();
	cout << "Optimised opencv: " << useOpt << "\n"
		<< "Number of Threads: " << getNumThreads() << "\n"
		<< "Number of CPUs: " << getNumberOfCPUs() << "\n";


	Mat_<Vec3b> frame2;
	double t = 0;
	// Loop until the user closes the window 
	while (1)
	{
		if (shouldExit) {
			return EXIT_SUCCESS;
		}
		//copy webcam stream to matrix
		frameStartTime = getTickCount();
		//cap >> frame;
		cap >> frame2;
	//	t = (double)getTickCount();
		//imshow(title, frame2);

		addInfoLayer(frame2);
		//draw everything to window called title
		

		draw(frame2, title);


		//frameCount += 1;
		fixFrameRate(frameStartTime);
	//	cout << "time:\t" << ((double)getTickCount() - t) / getTickFrequency()*1000 << "\n";

	}

	return EXIT_FAILURE;
}



