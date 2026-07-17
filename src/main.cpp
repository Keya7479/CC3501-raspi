#include <opencv2/opencv.hpp>
#include <sys/time.h>

int main()
{
    // Create matrixes to store frames, these will be rewritten for each frame within the for loop below
    cv::Mat frame;
    cv::Mat bgr_frame;
    cv::Mat hsv_frame;

    cv::Mat thresh_frame;

    // Open the video camera.
    std::string pipeline = "libcamerasrc"
                           " ! video/x-raw, width=800, height=600" // camera needs to capture at a higher resolution
                           " ! videoconvert"
                           " ! videoscale"
                           " ! video/x-raw, width=400, height=300" // can downsample the image after capturing
                           " ! videoflip method=rotate-180"        // remove this line if the image is upside-down
                           " ! appsink drop=true max_buffers=2";
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    if (!cap.isOpened())
    {
        printf("Could not open camera.\n");
        return 1;
    }

    // Create a control window
    cv::namedWindow("Control", cv::WINDOW_AUTOSIZE);
    int iLowH = 0;
    int iHighH = 179;

    int iLowS = 0;
    int iHighS = 255;

    int iLowV = 0;
    int iHighV = 255;

    int open_radius = 2;
    int close_radius = 2;

    // Create trackbars in "Control" window
    cv::createTrackbar("LowH", "Control", &iLowH, 179); // Hue (0 - 179)
    cv::createTrackbar("HighH", "Control", &iHighH, 179);

    cv::createTrackbar("LowS", "Control", &iLowS, 255); // Saturation (0 - 255)
    cv::createTrackbar("HighS", "Control", &iHighS, 255);

    cv::createTrackbar("LowV", "Control", &iLowV, 255); // Value (0 - 255)
    cv::createTrackbar("HighV", "Control", &iHighV, 255);

    cv::createTrackbar("Open Radius", "Control", &open_radius, 10);
    cv::createTrackbar("Close Radius", "Control", &close_radius, 10);

    // Create a threshold window
    cv::namedWindow("Thresholded", cv::WINDOW_AUTOSIZE);

    // Create a camera window
    cv::namedWindow("Camera", cv::WINDOW_AUTOSIZE);

    // Measure the frame rate - initialise variables
    int frame_id = 0;
    timeval start, end;
    gettimeofday(&start, NULL);

    for (;;)
    {
        // for each frame:
        // 1. CAMERA: capture, show original frame, measure frame rate
        // 2. THRESHOLDING: Convert to HSV, threshold, show thresholded frame
        // 3. MORPHOLGY: create structuring element, perform open and close ops, show cleaned thresholded frame

        cv::Mat display_frame = bgr_frame.clone(); // deep copy because we will modify it below

        // capture frame
        if (!cap.read(display_frame))
        {
            printf("Could not read a frame.\n");
            break;
        }

        // show frame
        cv::imshow("Camera", display_frame);
        cv::waitKey(1);

        // Measure the frame rate
        frame_id++;
        if (frame_id >= 30)
        {
            gettimeofday(&end, NULL);
            double diff = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) / 1000000.0;
            printf("30 frames in %f seconds = %f FPS\n", diff, 30 / diff);
            frame_id = 0;
            gettimeofday(&start, NULL);
        }

        // Convert to HSV colour space
        cv::cvtColor(display_frame, hsv_frame, cv::COLOR_BGR2HSV);

        // Threshold the frame
        cv::inRange(hsv_frame, cv::Scalar(iLowH, iLowS, iLowV), cv::Scalar(iHighH, iHighS, iHighV), thresh_frame);

        // Get structuring element
        int open_kernel_size = 2 * open_radius + 1;
        int close_kernel_size = 2 * close_radius + 1;
        cv::Mat open_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(open_kernel_size, open_kernel_size));
        cv::Mat close_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(close_kernel_size, close_kernel_size));

        // Open morphology operation (remove 'salt' noise)
        morphologyEx(thresh_frame, thresh_frame, cv::MORPH_OPEN, open_kernel);

        // Close morphology operation (remove 'pepper' noise)
        morphologyEx(thresh_frame, thresh_frame, cv::MORPH_CLOSE, close_kernel);

        // Show the cleaned thresholded frame
        cv::imshow("Thresholded", thresh_frame);
    }

    // Free the camera
    cap.release();
    return 0;
}
