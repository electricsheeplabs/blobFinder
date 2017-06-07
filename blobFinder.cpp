
//NOTES
////////////////////////////////loop until blob is found or no convergence made////////////////////////////////////////
	//Specifically: the loop increases threshold_value from threshold_min to threshold_max and checks if blob finder sees 
	//anything at each threshold value...simple blob finder actually basically does this exact thing but i for some reason like 
    //this way, have more control...at least for now it seems to work, should try to use just simple blob finder (or a different algorithm) in the future...
//poop
#include "opencv2/opencv.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST

using namespace cv;
using namespace std;

//methods
void printPic(Mat m, string extension, int i, double x, double y, double dia);
Mat getRoi(Mat m);
SimpleBlobDetector::Params getParams();
void printData(string out, int numDatapoints, double dia[], double coords[][2]);
bool isDirExist(const std::string& path);
bool makePath(const std::string& path);

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
	double diameters[numImages] = {0};
	double coords[numImages][2] = {0};

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
                            diameters[i] = blobIterator->size;
                            coords[i][0] = blobIterator->pt.x;
                            coords[i][1] = blobIterator->pt.y;

                            //clean up and jump out of loop to move to next image
                            convergedBlobs++;
                            flag = 0;
                            threshold_value = threshold_min;
                            printPic(im2, dataPath+"/", i, coords[i][0], coords[i][1], diameters[i]);
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
    printData(dataPath+"/data.txt", numImages, diameters, coords);

	//print info
	cout << "number of images: " << numImages << endl;
	cout << "blobs detected: " << totBlobs << endl;
	cout << "identified events: " << convergedBlobs << endl;
	
return 0;
}



void printPic(Mat m, string extension, int i, double x, double y, double dia){
    //print pic of resulting convergence
	Mat im_with_keypoints;
    stringstream ss;
	ss << extension << i+1 << ".jpg";
	string out = ss.str();
	int rad = (int)(dia/2.0);
	//drawKeypoints( im2, keypoints, im_with_keypoints, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
	circle(m, Point(x,y),rad, (0,255,0), 5, 8, 0);
	imwrite( out, im2);
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

void printData(string out, int numDatapoints, double dia[], double coords[][2]){
    //print diameter data...
    ofstream outFile;
    string diaOut = out;
    outFile.open(diaOut.c_str());
	for(int i = 0; i < numDatapoints; ++i) outFile << dia[i] << " " << coords[i][0] << " " << coords[i][1] << endl;
	outFile.close();
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
