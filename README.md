Blob Finder
Programmer: Nick Crescimanno
Date created: 1/6/2017

This program uses openCV to identify white blobs in the center of a 1080p image. It's original purpose is to identify the signal from ionized liquid Helium nanodroplets whose ions (or electrons) are accelerated into an MCP (multi-channel plate) which amplifies the electrical signal by multiple orders of magnitude. The signal is then converted to light via a phosphor screen and a camera literally records the light from the phosphor screen.

There is some hard coding for our specific setup, such as the region of interest which is fixed towards the center of the image. Also the image conditioning parameters (blur, threshold, morphing) is hardcoded to get best results for our signal. So, parameters likely will not be optimal for a different setup. 

The use of the program is simple in that one just enters the directory of the location of the images he/she wishes to analyze and the script does the rest. A folder "analyzed" is created with the data file, and other stuff if you want (one can toggle, for example, the processed image output). Currently the data columns in "data.txt" are diameter, x coord, and y coord of the blob.

See "blobFinderSupplemental.pdf" for a visual example of the code's workings...

TO DO:
. Add in brightness of blob analysis
. Utilize SimpleBlobDetector more efficiently, or use different object detection all together (hugh circle??)
. add Windows use instructions (see below)

Instructions for use:
Linux 
. download and compile openCV
. install cmake
. clone da repo to blobFinder directory
. in terminal execute "cmake .", "make" to make the program
. run with "./blobFinder"

Windows
dude idk right now just cry http://docs.opencv.org/2.4/doc/tutorials/introduction/windows_install/windows_install.html

make blob video:
ffmpeg -f image2 -framerate 25 -pattern_type glob -r 10 -i '*.tiff' -pix_fmt yuv420p -s 720x480 fast.avi

