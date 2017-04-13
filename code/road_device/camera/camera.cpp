// camera.cpp
// Control various functions using camera.
// Executed by child process



// Get the frame from the camera.
// Create a mask that produces a movable part in this frame.
// The mask generated by this work gives an ROI on the areas of the road where the movement is made.
// After this, the frame is obtained only by passing the images through the mask, and the detector detects this area with each thread.



#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/bgsegm.hpp>
#include <iostream>
#include <thread>

#include "BackgroundMask.h"
#include "Detectors.h"
#include "Situation.h"
#include "CamDef.h"

using namespace cv; // openCV




// Detect objects by each detectors.
// calls detect () overridden on each detector.
// param - detector : This detector has learned the characteristics of the objects to be detected.
// param - fgimg :  (Forground images) Detection of objects in this image is detected, and the detection results are also shown in this image.
// ( The pedestrian is indicated by the green color, and the vehicle is red. )
void detectObjects(Detector& detector, UMat& fgimg) {
    detector.detect(fgimg);
}



//It is called from main().
// Performs overall operations using the camera.
int takeRoad(void)
{
    // Select the source of the video.

    /* // Connect camera
    VideoCapture vc(0);
    vc.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    vc.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    vc.set(CV_CAP_PROP_FPS, 12);
    */
    // Load test video
    VideoCapture vc( CamDef::sampleVideo );
    if (!vc.isOpened()) {
        std::cerr << "ERROR : Cannot open the camera" << std::endl;
        return false;
    }




    // Background recognition and removal
    BackgroundMask bgMask;
    bgMask.setRecognizeNumFrames(24);  // Default : 120 ( BackgroundSubtractorGMG's default value )
    bgMask.setNoiseRemovalNumFrames( vc.get(CV_CAP_PROP_FPS) ); // Default : 12
    bgMask.setAccumulateNumFrames(300); // Default : 600
    bgMask.setLearningRate(0.025); // Default : 0.025
    bgMask.printProperties();

    // Select the source of the mask.
    // UMat mask = bgMask.createBackgroundMask(vc);
    UMat mask = bgMask.loadBackgroundMask();
    imshow( CamDef::mask, mask );  // show background mask




    UMat img, fgimg; // using OpenCL ( UMat )
    PedestriansDetector pe_Detector;
    VehiclesDetector car_Detector;
    Situation situation( vc.get(CV_CAP_PROP_FRAME_HEIGHT), vc.get(CV_CAP_PROP_FRAME_WIDTH), vc.get(CV_CAP_PROP_FPS)*4 );
    situation.loadRoadImg();


    std::cout << "Start Detection ..." << std::endl;
    bool playVideo = true; char pressedKey;
    while (1) {

        if(playVideo) {
            // Put the captured image in img
            vc >> img;
            if (img.empty())  {
                std::cerr << "ERROR : Unable to load frame" << std::endl;
                break;
            }
            // show original image
            imshow( CamDef::originalVideo, img );


            // Exclude areas excluding road areas in the original image.
            bgMask.locateForeground(img, fgimg);

            // Detect pedestrians and vehicle
            // Detect pedestrians and vehicle
            std::thread t1(detectObjects, std::ref(pe_Detector), std::ref(fgimg));
            std::thread t2(detectObjects, std::ref(car_Detector), std::ref(fgimg));
            t1.join();
            t2.join();


            // Judge the situation of the road
            // situation.updateRoadImg( car_Detector.getFoundObjects() );
            situation.sendPredictedSituation( pe_Detector.getFoundObjects() );
            imshow( CamDef::roadImg, situation.getRoadImg() );


            // show image processing result
            imshow( CamDef::resultVideo, fgimg );
        }


        // press SPACE BAR -> pause video
        // press ESC -> close video
        if ( ( pressedKey = waitKey( CamDef::DELAY ) ) == CamDef::PAUSE ) // SPACE BAR
            playVideo = !playVideo;
        else if(  pressedKey == CamDef::CLOSE ) { // ESC
            std::cout << "Disconnecting from camera and returning resources ..." << std::endl;
            break;
        }
    }


    // Return resources.
    destroyAllWindows();
    img.release();
    fgimg.release();
    mask.release();
    vc.release();

    return true;
}
