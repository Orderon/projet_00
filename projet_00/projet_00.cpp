// projet_00.cpp : Defines the entry point for the console application.
//
// inspired from http://r3dux.org/
// Library dependencies: OpenCV core, video and highgui libs, GL, glew, glfw
// Kinect with OpenCV usage guide: http://opencv.itseez.com/doc/user_guide/ug_highgui.html
#include "stdafx.h"
#include "multipleImages.hpp"
//#include "glew.h"
//#include <GLFW/glfw3.h>
//#include <GLFW\glfw3native.h>
//#include <gl\GL.h>
//#include <gl\GLU.h>
#define WAIT 30 //wait in ms after each frame
#define EYE_DISTANCE 0//50 //in px
#define EYE_ANGLE 5//2 //degrees
#define INPLANE_ROT -90 //90 degrees rotation for oculus!
#define LIMIT_FPS 20 //max frames per second
#define FULLSCREEN_W  2060
#define FULLSCREEN_H 2060
#define INIT_X 900
#define INIT_Y 600
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



// Function turn a cv::Mat into a texture, and return the texture ID as a GLuint for use
// Source:  http://r3dux.org/2012/01/how-to-convert-an-opencv-cvmat-to-an-opengl-texture/
/*GLuint matToTexture(cv::Mat &mat, GLenum minFilter, GLenum magFilter, GLenum wrapFilter)
{
	// Generate a number for our textureID's unique handle
	GLuint textureID;
	glGenTextures(1, &textureID);

	// Bind to our texture handle
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Catch silly-mistake texture interpolation method for magnification
	if (magFilter == GL_LINEAR_MIPMAP_LINEAR ||
		magFilter == GL_LINEAR_MIPMAP_NEAREST ||
		magFilter == GL_NEAREST_MIPMAP_LINEAR ||
		magFilter == GL_NEAREST_MIPMAP_NEAREST)
	{
		std::cout << "You can't use MIPMAPs for magnification - setting filter to GL_LINEAR\n";
		magFilter = GL_LINEAR;
	}

	// Set texture interpolation methods for minification and magnification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	// Set texture clamping method
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapFilter);

	// Set incoming texture format to:
	// GL_BGR       for CV_CAP_OPENNI_BGR_IMAGE,
	// GL_LUMINANCE for CV_CAP_OPENNI_DISPARITY_MAP,
	// Work out other mappings as required ( there's a list in comments in main() )
	GLenum inputColourFormat = GL_BGR;
	if (mat.channels() == 1)
	{
		inputColourFormat = GL_LUMINANCE;
	}

	// Create the texture
	glTexImage2D(GL_TEXTURE_2D,     // Type of texture
		0,                 // Pyramid level (for mip-mapping) - 0 is the top level
		GL_RGB,            // Internal colour format to convert to
		mat.cols,          // Image width  i.e. 640 for Kinect in standard mode
		mat.rows,          // Image height i.e. 480 for Kinect in standard mode
		0,                 // Border width in pixels (can either be 1 or 0)
		inputColourFormat, // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
		GL_UNSIGNED_BYTE,  // Image data type
		mat.ptr());        // The actual image data itself

						   // If we're using mipmaps then generate them. Note: This requires OpenGL 3.0 or higher
	if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
		minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		minFilter == GL_NEAREST_MIPMAP_LINEAR ||
		minFilter == GL_NEAREST_MIPMAP_NEAREST)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	return textureID;
}*/

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
void draw(Mat frame, char* title) {
	Mat left, right; //create Mat to store viewpoints

	// calculate perspectives
	int left_b = 90;
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
	int f = 200;
	rotateImage(frame, left, left_a, left_b, left_c, left_dx, left_dy, dz, f);
	rotateImage(frame, right, right_a, right_b, right_c, right_dx, right_dy, dz, f);
	
	//convert to IplImage for drawing
	IplImage* imLeft = cvCloneImage(&(IplImage)left);
	IplImage* imRight = cvCloneImage(&(IplImage)right);
	namedWindow("test");
	cvShowImage("test", imLeft);

	//draw on splitscreen
	cvShowManyImages(title, 2, imLeft, imRight);
	
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

		k = waitKey(20);							 //delay WAIT ms

		handleInputs(k);
	} while (frameRate > LIMIT_FPS);
}

int main() {

	// Initialize openCV
	Mat frame;							  //Create Matrix to store image
	VideoCapture cap(1);				  //create videostream
	if (!cap.isOpened())				  // check if we succeeded
		return -1;
	
	//create Window
	cvNamedWindow(title, 1);

	resizeWindow(title, 1200, 900);
	setWindowSize(INIT_X, INIT_Y);
	//initGL();

	cout << "Initalized..\n";
	// Loop until the user closes the window 
	while (1)
	{
		if (shouldExit) {
			return EXIT_SUCCESS;
		}
		//copy webcam stream to matrix
		frameStartTime = getTickCount();
		cap >> frame;
		addInfoLayer(frame);
		
		//draw everything to window called title
		draw(frame, title);


		frameCount += 1;
		fixFrameRate(frameStartTime);

	}

	return EXIT_FAILURE;

}



