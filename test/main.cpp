#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/features2d.hpp"
#include "test1.hpp"
#include "test2.hpp"

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

int main(int argc, const char * argv[]) {
    //TestOne();
    //TestTwo();
    
    const int MAX_FEATURES = 2000;//was 500
    const int MIN_FEATURES = 15;
    
    // Read tracker image
    //string trackerFileName("card.jpg");
    //string trackerFileName("6ft.PNG");
    string trackerFileName("dollar.jpg");
    cout << "Reading tracker image : " << trackerFileName << endl;
    Mat trackerGray = imread(trackerFileName, IMREAD_GRAYSCALE);

    //Create detector and matcher
    Ptr<ORB> detector = ORB::create(MAX_FEATURES);
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::BRUTEFORCE_HAMMING);
    
    //Detect ORB features and compute descriptors for tracker
    vector<KeyPoint> trackerKeyPoints;
    Mat trackerDescriptors;
    detector->detectAndCompute(trackerGray, Mat(), trackerKeyPoints, trackerDescriptors);
    
    //Capture stream from webcam.
    VideoCapture capture(0);
    
    //Check if we can get the webcam stream.
    if(!capture.isOpened()) {
        cout << "Could not open camera" << std::endl;
        return -1;
    }
    
    while (true) {
        //This variable will hold the image from the camera.
        Mat cameraFrame;
        
        //Read an image from the camera.
        capture.read(cameraFrame);
        
        // Detect ORB features and compute descriptors for camera
        Mat cameraDescriptors;
        vector<KeyPoint> cameraKeyPoints;
        detector->detectAndCompute(cameraFrame, Mat(), cameraKeyPoints, cameraDescriptors);
        
        // Find 2 nearest matches
        vector<vector<DMatch> > knn_matches;
        if (trackerDescriptors.cols == cameraDescriptors.cols){
            matcher->knnMatch( trackerDescriptors, cameraDescriptors, knn_matches, 2 );
        }
        
        //Filter matches using the Lowes ratio test
        const float ratio_thresh = 0.75f;
        vector<DMatch> good_matches;
        for (size_t i = 0; i < knn_matches.size(); i++){
            if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance){
                good_matches.push_back(knn_matches[i][0]);
            }
        }
        
        cout << good_matches.size() << endl;
        
        //dont draw matches if less than threshold
        if (good_matches.size() < MIN_FEATURES){
            good_matches.clear();
        }
        
        //Draw matches
        //Mat img_matches;
        //drawMatches( trackerGray, trackerKeyPoints, cameraFrame, cameraKeyPoints, good_matches, img_matches);
        
        //Localize the object
        vector<Point2f> objectMatchPoints;
        vector<Point2f> cameraMatchPoints;
        for( size_t i = 0; i < good_matches.size(); i++ ){
            //Get keypoints from the good matches
            objectMatchPoints.push_back( trackerKeyPoints[ good_matches[i].queryIdx ].pt );
            cameraMatchPoints.push_back( cameraKeyPoints[ good_matches[i].trainIdx ].pt );
        }
        //if we have matches find homography
        if (good_matches.size() > 1){
            Mat H = findHomography( objectMatchPoints, cameraMatchPoints, RANSAC );
            //Get corners from image we are detecting
            vector<Point2f> obj_corners(4);
            obj_corners[0] = Point2f(0, 0);
            obj_corners[1] = Point2f( (float)trackerGray.cols, 0 );
            obj_corners[2] = Point2f( (float)trackerGray.cols, (float)trackerGray.rows );
            obj_corners[3] = Point2f( 0, (float)trackerGray.rows );
            vector<Point2f> scene_corners(4);
            perspectiveTransform( obj_corners, scene_corners, H);
            //Draw lines between the corners of mapped object in scene
            line( cameraFrame, scene_corners[0],scene_corners[1], Scalar(0, 255, 0), 4 );
            line( cameraFrame, scene_corners[1],scene_corners[2], Scalar( 0, 255, 0), 4 );
            line( cameraFrame, scene_corners[2],scene_corners[3], Scalar( 0, 255, 0), 4 );
            line( cameraFrame, scene_corners[3],scene_corners[0], Scalar( 0, 255, 0), 4 );
        }
        
        //make window half the size
        resize(cameraFrame, cameraFrame, Size(cameraFrame.cols/2, cameraFrame.rows/2));
        namedWindow( "Camera", WINDOW_AUTOSIZE);
        imshow("Camera", cameraFrame);
        
        //Waits 50 miliseconds for key press, returns -1 if no key is pressed during that time
        if (waitKey(50) >= 0)
            break;
    }
    return 0;
}
