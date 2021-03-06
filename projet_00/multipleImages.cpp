#include "stdafx.h"
  
  #include <stdio.h>
  #include <stdarg.h>
#include "multipleImages.hpp"
#define DIST_X 180 //-1 for no border at all
#define DIST_I 180 //distance between the two images
#define DIST_Y 200 //distance between border & first image
#define MAX_SIZE 550

// TODO: optimise cvShowManyImages !!!

using namespace cv;

int wWidth = 1200; //oculus: 2160
int wHeigth = 900;
  /*Function///////////////////////////////////////////////////////////////
  
  Name:       cvShowManyImages
 
 Purpose:    This is a function illustrating how to display more than one 
               image in a single window using Intel OpenCV
 
 Parameters: char *title: Title of the window to be displayed
             int nArgs:   Number of images to be displayed
             ...:         IplImage*, which contains the images
 
 Language:   C++
 
 The method used is to set the ROIs of a Single Big image and then resizing 
 and copying the input images on to the Single Big Image.
 
 This function does not stretch the image... 
 It resizes the image without modifying the width/height ratio..
 
 This function can be called like this:
 
 cvShowManyImages("Images", 2, img1, img2);
 or
 cvShowManyImages("Images", 5, img2, img2, img3, img4, img5);
 
 This function can display upto 12 images in a single window.
 It does not check whether the arguments are of type IplImage* or not.
 The maximum window size is 700 by 660 pixels.
 Does not display anything if the number of arguments is less than
     one or greater than 12.
 
 If you pass a pointer that is not IplImage*, Error will occur.
 Take care of the number of arguments you pass, and the type of arguments, 
 which should be of type IplImage* ONLY.
 
 Idea was from [[BettySanchi]] of OpenCV Yahoo! Groups.
 
 If you have trouble compiling and/or executing
 this code, I would like to hear about it.
 
 You could try posting on the OpenCV Yahoo! Groups
 [url]http://groups.yahoo.com/group/OpenCV/messages/ [/url]
 
 Parameswaran, 
 Chennai, India.
 
 cegparamesh[at]gmail[dot]com            
 
 modified to the purpose of this project by Cyrill Baumann
 ///////////////////////////////////////////////////////////////////////*/
 


 void cvShowManyImages(char* title, int nArgs, ...) {
 
     // img - Used for getting the arguments 
     IplImage *img;
 
     // [[DispImage]] - the image in which input images are to be copied
     IplImage *DispImage;
 
     int size;
     int i;
     int m, n;
     int x, y;
 
     // w - Maximum number of images in a row 
     // h - Maximum number of images in a column 
     int w, h;
 
     // scale - How much we have to resize the image
    float scale;
     int max;
 
     // If the number of arguments is lesser than 0 or greater than 12
     // return without displaying 
     if(nArgs <= 0) {
         printf("Number of arguments too small....\n");
         return;
     }
     else if(nArgs > 12) {
         printf("Number of arguments too large....\n");
         return;
     }
     // Determine the size of the image, 
     // and the number of rows/cols 
     // from number of arguments 
    else if (nArgs == 1) {
         w = h = 1;
        size = wWidth;
     }
	//the case of our utilisation
    else if (nArgs == 2) {
         w = 1; h = 2;
        size = (wHeigth-DIST_Y)/2;
		size = MAX_SIZE;
		std::cout << "Wheight: " << wHeigth << "\n";
     }
  
    else {
       w = 4; h = 3;
        size = 150;
    }

   // Create a new 3 channel image
	int margin_x = DIST_X > 0 ? DIST_X : 0;
   // DispImage  = cvCreateImage( cvSize(2*wWidth+margin_x,  DIST_I+DIST_Y+500+ size*h), 8, 3 );
	DispImage = cvCreateImage(cvSize(2600, 2600), 8, 3);
    // Used to get the arguments passesd
    va_list args;
   va_start(args, nArgs);

   // Loop for nArgs number of arguments
    for (i = 0, m = DIST_X, n = DIST_Y; i < nArgs; i++, m += (size)) {

       // Get the Pointer to the IplImage
        img = va_arg(args, IplImage*);

        // Check whether it is NULL or not
       // If it is NULL, release the image, and return
        if(img == 0) {
            printf("Invalid arguments");
            cvReleaseImage(&DispImage);
            return;
        }

        // Find the width and height of the image
        x = img->width;
        y = img->height;

        // Find whether height or width is greater in order to resize the image
        max = (x > y)? x: y;

        // Find the scaling factor to resize the image
		scale = (float) ( (float)y / size);

		//check for too big scale
		scale = (y / scale > MAX_SIZE) ? (y/MAX_SIZE) : scale;

        // Used to Align the images
       // if( i % w == 0 && m!= 10) {
       //  m = 10;
       //   n+= 10 + size;
      //  }
		if (i == 1) {
			n = DIST_Y;
			m = DIST_X + DIST_I + x/scale;
		}

        // Set the image ROI to display the current image
        cvSetImageROI(DispImage, cvRect(m, n, (int)( x/scale ), (int)( y/scale )));

        // Resize the input image and copy the it to the Single Big Image
        cvResize(img, DispImage);

        // Reset the ROI in order to display the next image
        cvResetImageROI(DispImage);
    }

    // show the Single Big Image
    cvShowImage( title, DispImage);


    // End the number of arguments
    va_end(args);

    // Release the Image Memory
    cvReleaseImage(&DispImage);
}

void setWindowSize(int width, int height)
{
	wWidth = width;
	wHeigth = height;
	std::cout << "New width: " << wWidth << "\t height: " << wHeigth << "\n";
}

