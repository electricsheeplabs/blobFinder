/*
Blob Finder
Programmer: Nick Crescimanno
Date created: 1/6/2017

This program uses openCV to identify white blobs in the center of a 1080p image. 
It's original purpose is to identify the signal from ionized liquid Helium nanodroplets 
whose ions (or electrons) are accelerated into an MCP (multi-channel plate) which amplifies 
the electrical signal by multiple orders of magnitude. The signal is then converted to light 
via a phosphor screen and a camera literally records the light from the phosphor screen. 

There is some hard coding for our specific setup, such as the region of interest which is 
fixed towards the center of the image. Also the image conditioning parameters (blur, threshold, morphing) 
is hardcoded to get best results for our signal. So, parameters likely will not be optimal for a different setup. 

The use of the program is simple in that one just enters the directory of the location of the images 
he/she wishes to analyze and the script does the rest. A folder "analyzed" is created with the data file, 
and other stuff if you want (one can toggle, for example, the processed image output). Currently the data columns 
in "data.txt" are diameter, x coord, and y coord of the blob. 

TO DO:
. Add in brightness of blob analysis
. Utilize SimpleBlobDetector more efficiently, or use different object detection all together (hugh circle??)
. add Windows use instructions (see below)
. crop blob before intensity measurement!!!

Instructions for use:
Linux 
. download and compile openCV
. install cmake
. clone da repo to blobFinder directory
. in terminal execute "cmake .", "make" to make the program
. run with "./blobFinder"

Windows
dude idk right now just cry http://docs.opencv.org/2.4/doc/tutorials/introduction/windows_install/windows_install.html
*/

#include "opencv2/opencv.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST

using namespace cv;
using namespace std;

//methods
void printPic(Mat m, string extension, int i);
Mat getRoi(Mat m);
SimpleBlobDetector::Params getParams();
void printData(string out, int numDatapoints, int dia[], int coords[][2], double intensity[]);
bool isDirExist(const std::string& path);
bool makePath(const std::string& path);
double getIntensity(Mat, int rad);
Mat blobOnly(Mat m, int x, int y, int rad);

//things user may want to change in the future
int const threshold_min = 10;
int const threshold_max = 25;
//box of allowed event centers
int const targetXmin = 180;
int const targetXmax = 220;
int const targetYmin = 180;
int const targetYmax = 220;

//constants from morph.cpp
int const blurVal = 25;
int morph_size = 3;
int morph_elem = 0;
int threshold_value = 10; 
int const max_BINARY_value = 255;
int threshold_type = 0;
int const max_value = 255;
string window_name = "out";

//other globals
int flag = 1;
int totBlobs = 0;
int convergedBlobs = 0;

Mat im, im2, imBlurred;

int main(int argc, char** argv)
{
    //get info from user
    string extension;
    cout << "enter path to image data: " << endl;
    cin >> extension;
    string dataPath = extension+"/analyzed";
    if(!isDirExist(dataPath)) makePath(dataPath);

	vector<String> filenames; // notice here that we are using the Opencv's embedded "String" class
    glob(extension, filenames); // convenient opencv function that gets all the files! :D
	const int numImages = filenames.size();
	int diameters[numImages] = {0};
    int radius = 0;
	int coords[numImages][2] = {0};
    double intensity[numImages] = {0.0};

    //loop over all images in the folder
    for(size_t i = 0; i < filenames.size(); ++i)
    {
	    //shitty progress bar
	    if(i%100==0&&i!=0) cout << "images processed: " << i << endl;
        //read in file and throw msg if problems, continue is needed to skip to next iteration of for loop!
        im = imread(filenames[i], IMREAD_GRAYSCALE);
        if(!im.data){
            cout << "Problem loading image!!!" << endl;
	    continue;
	    }

        //crop and blur
        im = getRoi(im);
        blur(im, imBlurred, Size(blurVal,blurVal) );

        //do structure elem thing for morph later on...
        Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );

        // Set up detector with params and a keypoint vector which will, well, store keypoints...
        Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(getParams());
        vector<KeyPoint> keypoints;


            //loop until blob is found or thresh_max is reached
            flag = 1;
            while(flag){
                //threshold, morph, and invert. For morph, operation 2 is "opening", element is the kernel size set by morph_size
                threshold( imBlurred, im2, threshold_value, max_BINARY_value,threshold_type );
                morphologyEx( im2, im2, 2, element );
                bitwise_not ( im2, im2);

                // Detect blob method
                detector->detect( im2, keypoints);

                if(!keypoints.empty()) {
                    //we loop through identified blobs and only accept one if it is within the target zone...
                    for(std::vector<cv::KeyPoint>::iterator blobIterator = keypoints.begin(); blobIterator != keypoints.end(); blobIterator++){
                        if((blobIterator->pt.x > targetXmin)&&(blobIterator->pt.x < targetXmax)&&(blobIterator->pt.y > targetYmin)&&(blobIterator->pt.y < targetYmax)){
                            //store diameter and coords of blob in array
                            diameters[i] = (int)blobIterator->size;
                            coords[i][0] = (int)blobIterator->pt.x;
                            coords[i][1] = (int)blobIterator->pt.y;

                            //clean up and jump out of loop to move to next image
                            convergedBlobs++;
                            flag = 0;
                            threshold_value = threshold_min;
                            
                            //calculate intensity
                            radius = (int)(blobIterator->size/2.0);
                            Mat blob = blobOnly(im, coords[i][0], coords[i][1], radius);
                            intensity[i] = getIntensity(blob, radius);

                            //print processed image with circle around blob
                            circle(im2, Point(coords[i][0], coords[i][1]), radius, (0,255,0), 5, 8, 0);
                            printPic(im2, dataPath+"/", i);
                            //print blobOnly if you want
                            printPic(blob, dataPath+"/blobOnly", i);
                        }
                        else threshold_value++;
                        //keep track of total number of blobs identified
                        totBlobs++;
                    } 
                }
                //if nothing detected, exit loop...
                else if(threshold_value==threshold_max){
                    threshold_value = threshold_min;
                    flag = 0;
                    }
                //if neither blob found nor thresh max reached, increment threshold...
                else threshold_value++;
                //clear keypoints each time through
                keypoints.clear();
            }
    }
    
    //print to file
    printData(dataPath+"/data.txt", numImages, diameters, coords, intensity);

	//print info
	cout << "number of images: " << numImages << endl;
	cout << "blobs detected: " << totBlobs << endl;
	cout << "identified events: " << convergedBlobs << endl;
    cout << "data (and optional images) stored in " << dataPath+"/data.txt" << endl;
	
return 0;
}



void printPic(Mat m, string extension, int i){
    //print pic of resulting convergence
	//Mat im_with_keypoints;
    stringstream ss;
	ss << extension << i+1 << ".jpg";
	string out = ss.str();
	//drawKeypoints( im2, keypoints, im_with_keypoints, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
	imwrite( out, m);
}

Mat getRoi(Mat m){
    //Set Region of Interest, NOTE IT IS HARDCODED FOR CURRENT EXPERIMENTAL SETUP
	int offset_x = 820;//hard code it :D
	int offset_y = 450;
	cv::Rect roi;
	roi.x = offset_x;
	roi.y = offset_y;

	roi.width = 400;//more hard coding :D
	roi.height = 400;
	// Crop the original image to the defined ROI 
    m = m(roi);
    return m;
}

SimpleBlobDetector::Params getParams(){
    ///////////////////////Setup SimpleBlobDetector and set parameters//////////////////////////////////
	SimpleBlobDetector::Params params;

	//The threshold thing...yeah the blob finder does almost exactly what i am doing in this program with thresholding in the loop
	//played around 170605 and these values needed for the algorithm...more investigation needed to see if this can be used better...
	params.minThreshold = 8;
	params.maxThreshold = 20;
	params.thresholdStep = 10;

	// Filter by Area.
	params.filterByArea = true;
	params.minArea = 314; //about 20px dia
    params.maxArea = 120000;//about 390px dia

	// Filter by Circularity
	params.filterByCircularity = true;
	params.minCircularity = 0.1;

	// Filter by Convexity
	params.filterByConvexity = true;
	params.minConvexity = .87;

	// Filter by Inertia
	params.filterByInertia = true;
	params.minInertiaRatio = 0.3;

    return params;
}

void printData(string out, int numDatapoints, int dia[], int coords[][2], double intensity[]){
    //print diameter data...
    ofstream outFile;
    string diaOut = out;
    outFile.open(diaOut.c_str());
    outFile <<setw(6)<<left<<"#dia"<<setw(6)<<left<<"x"<<setw(6)<<left<<"y"<<setw(6)<<left<<"intensity" << endl;
	for(int i = 0; i < numDatapoints; ++i) outFile <<setw(6)<<left<< dia[i] <<setw(6)<<left<< coords[i][0] <<setw(6)<<left<< coords[i][1] <<setw(6)<<left<<setprecision(4)<< intensity[i] << endl;
	outFile.close();
}

Mat blobOnly(Mat m, int x, int y, int rad){
    //create black mask
    Mat mask = cv::Mat::zeros(m.size(), CV_8UC1);
    //draw white circle on mask where blob is
    circle(mask, Point(x, y), rad, 255, -1);
    //create another black image that only the blob will copy in to
    Mat blobOnly = cv::Mat::zeros(m.size(), m.type());
    //mask is a white circle with black background, and its non-zero elements tell "copyTo" which pixels of "m" to put in the black "blobOnly"
    m.copyTo(blobOnly, mask);
    return blobOnly;
}

double getIntensity(Mat blob, int rad){
    //sum the blob and all the black pixels in the background, no its not efficient but its one line so... :D
    double intensity = sum(blob)[0];
    //so intensity now has the average intensity of the blob
    intensity /= (3.14159*rad*rad);
    return intensity;
}

//https://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
bool isDirExist(const std::string& path)
{
#if defined(_WIN32)
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else 
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

bool makePath(const std::string& path)
{
#if defined(_WIN32)
    int ret = _mkdir(path.c_str());
#else
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);
#endif
    if (ret == 0)
        return true;

    switch (errno)
    {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos)
#if defined(_WIN32)
                pos = path.find_last_of('\\');
            if (pos == std::string::npos)
#endif
                return false;
            if (!makePath( path.substr(0, pos) ))
                return false;
        }
        // now, try to create again
#if defined(_WIN32)
        return 0 == _mkdir(path.c_str());
#else 
        return 0 == mkdir(path.c_str(), mode);
#endif

    case EEXIST:
        // done!
        return isDirExist(path);

    default:
        return false;
    }
}
