/**
    pupil_tracking.cpp
    Purpose: Implement the pupil tracking algorithm proposed by Fabian Timm and Erhardt Barth in their
    article "Accurate eye centre localisation by means of gradients" using a modified open-source
    implementation of the algorithm by Tristan Hume (https://github.com/trishume/eyeLike).

    @author Kristoffer Alquiza
*/

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <queue>
#include <math.h>

#include "/usr/local/include/opencv/cv.h"
#include "/usr/local/include/opencv/cvaux.h"
#include "/usr/local/include/opencv/highgui.h"
#include "/usr/local/include/opencv/cxcore.h"
#include "/usr/include/postgresql/libpq-fe.h"
#include "constants.h"
#include "findEyeCenter.h"

using namespace cv;
using namespace std;

std::string main_window_name = "Capture - Face detection";
std::string face_window_name = "Capture - Face";
cv::RNG rng(12345);
cv::Mat debugImage;
cv::Mat skinCrCbHist = cv::Mat::zeros(cv::Size(256, 256), CV_8UC1);

/**
* @function main
*/
int main (int argc, char *argv[]) {

    /**
        Connect to postgres database
    */
    char db_statement[5000];
    PGconn   *db_connection;
    PGresult *db_result;
    db_connection = PQconnectdb("host = 'localhost' dbname = 'cv_face_detection_pipeline' user = 'postgres' password = 'opencv'");
    if (PQstatus(db_connection) != CONNECTION_OK) {
        printf ("Connection to database failed: %s", PQerrorMessage(db_connection));
        PQfinish (db_connection);
        exit (EXIT_FAILURE);
    }

    /**
        Get video id from argc and argv
    */
    int video_id;
    if(argc != 2) {
        printf("Provide a valid argument\n");
        exit(0);
    }
    else if (sscanf (argv[1], "%i", &video_id)!=1) {
        printf ("Provide a valid video id\n");
        exit(0);
    }

    /**
        Query the database for video metadata
    */
    int num_frames, width, height, frame_rate;
    char res[25];
    char output_filename[1280];
    char metadata_query[1280] = "SELECT num_frames, frame_rate, width, height FROM input_video_metadata WHERE video_id = ";

    strncpy (&db_statement[0], strcat(metadata_query, argv[1]), 5000);
    db_result = PQexecParams (db_connection, db_statement, 0, NULL, 0, 0, 0, 0);
    if (PQresultStatus(db_result) != PGRES_TUPLES_OK) {
        printf ("Postgres INSERT error: %s\n", PQerrorMessage(db_connection));
    }
    else {
        if (PQntuples(db_result) == 1) {
            strncpy (&res[0], PQgetvalue(db_result, 0, 0), 25);
            sscanf(res, "%d", &num_frames);
            strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
            sscanf(res, "%d", &frame_rate);
            strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
            sscanf(res, "%d", &width);
            strncpy (&res[0], PQgetvalue(db_result, 0, 3), 25);
            sscanf(res, "%d", &height);
        }
            PQclear(db_result);
    }

    /**
        For each frame call Hume's eye center tracking function and store the results to the database
    */
    for (int i = 1; i <= num_frames; i++) {
        // load source image
        cv::Mat source_image;
        char input_filename[1280];
        snprintf(&input_filename[0], sizeof(input_filename) - 1, "./video_id_%d/video_id_%d_%d.png", video_id, video_id, i);

	// print progress bar
  	cout << "\x1B[2K"; // Erase the entire current line.
  	cout << "\x1B[0E"; // Move to the beginning of the current line.
        string bar;
        for (int b = 0; b < 50; b++) {
           if (b < static_cast<int>(50 * i/num_frames)) {
              bar += "=";
           } else {
              bar += " ";
           }
        }
        cout << "Processing pupil_tracking video_id_" << video_id << " [" << bar << "] " << (static_cast<int>(100 * i/num_frames)) << "% " << "(" << i << "/" << num_frames << ")";
        flush(cout); // Required

        source_image = imread (&input_filename[0], 1);
        // detectAndDisplay
        std::vector<cv::Mat> rgbChannels(3);
        cv::split(source_image, rgbChannels);
        cv::Mat frame_gray = rgbChannels[2];

        if (!source_image.empty()) {
            // Query for eye bounding boxes
            char bounding_box_query_format[1280] = "SELECT %s_upper_left, %s_width, %s_height FROM bounding_box_data WHERE video_id = %d AND frame_id = %d";
            char bounding_box_query[1280], bounding_box_pt[50];
            int bounding_box_x, bounding_box_y, bounding_box_width, bounding_box_height;
            cv::Rect face;

            char face_bb[] = "face";
            sprintf(bounding_box_query, bounding_box_query_format, face_bb, face_bb, face_bb, video_id, i);
            strncpy (&db_statement[0], bounding_box_query, 5000);
            db_result = PQexecParams (db_connection, db_statement, 0, NULL, 0, 0, 0, 0);
            if (PQresultStatus(db_result) != PGRES_TUPLES_OK) printf ("Postgres INSERT error: %s\n", PQerrorMessage(db_connection));
            else {
                if (PQntuples(db_result) == 1) {
                    strncpy (&res[0], PQgetvalue(db_result, 0, 0), 25);
                    sscanf(res, "%s", bounding_box_pt);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
                    sscanf(res, "%d", &bounding_box_width);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
                    sscanf(res, "%d", &bounding_box_height);
                }
                PQclear(db_result);
            }
            sscanf(bounding_box_pt, "(%i,%i)", &bounding_box_x, &bounding_box_y);
            face = cvRect (bounding_box_x, bounding_box_y, bounding_box_width, bounding_box_height);

            if (face.width > 0 && face.height > 0) {

                cv::Mat faceROI = frame_gray(face);
                cv::Mat debugFace = faceROI;
                if (kSmoothFaceImage) {
                    double sigma = kSmoothFaceFactor * face.width;
                    GaussianBlur( faceROI, faceROI, cv::Size( 0, 0 ), sigma);
                }
                //-- Find eye regions and draw them
                int eye_region_width = face.width * (kEyePercentWidth/100.0);
                int eye_region_height = face.width * (kEyePercentHeight/100.0);
                int eye_region_top = face.height * (kEyePercentTop/100.0);


                // If left eye bounding box data exists, call the eye tracking function with the left eye bounding box data
                char l_eye[] = "left_eye";
                cv::Point leftPupil;
                sprintf(bounding_box_query, bounding_box_query_format, l_eye, l_eye, l_eye, video_id, i);
                strncpy (&db_statement[0], bounding_box_query, 5000);
                db_result = PQexecParams (db_connection, db_statement, 0, NULL, 0, 0, 0, 0);
                if (PQresultStatus(db_result) != PGRES_TUPLES_OK) printf ("Postgres SELECT error: %s\n", PQerrorMessage(db_connection));
                else {
                    if (PQntuples(db_result) == 1) {
                        //-- Find Eye Center
                        cv::Rect leftEyeRegion(face.width*(kEyePercentSide/100.0), eye_region_top,eye_region_width,eye_region_height);
                        leftPupil = findEyeCenter(faceROI,leftEyeRegion,"Left Eye");

                        // get corner regions
                        cv::Rect leftRightCornerRegion(leftEyeRegion);
                        leftRightCornerRegion.width -= leftPupil.x;
                        leftRightCornerRegion.x += leftPupil.x;
                        leftRightCornerRegion.height /= 2;
                        leftRightCornerRegion.y += leftRightCornerRegion.height / 2;
                        cv::Rect leftLeftCornerRegion(leftEyeRegion);
                        leftLeftCornerRegion.width = leftPupil.x;
                        leftLeftCornerRegion.height /= 2;
                        leftLeftCornerRegion.y += leftLeftCornerRegion.height / 2;

                        // change eye centers to face coordinates
                        leftPupil.x += leftEyeRegion.x;
                        leftPupil.y += leftEyeRegion.y;

                        // change eye centers to source image coordinates
                        leftPupil.x += bounding_box_x;
                        leftPupil.y += bounding_box_y;

                    }
                    PQclear(db_result);
                }

                // If right eye bounding box data exists, call the eye tracking function with the right eye bounding box data.
                char r_eye[] = "right_eye";
                cv::Point rightPupil;
                sprintf(bounding_box_query, bounding_box_query_format, r_eye, r_eye, r_eye, video_id, i);
                strncpy (&db_statement[0], bounding_box_query, 5000);
                db_result = PQexecParams (db_connection, db_statement, 0, NULL, 0, 0, 0, 0);
                if (PQresultStatus(db_result) != PGRES_TUPLES_OK) printf ("Postgres SELECT error: %s\n", PQerrorMessage(db_connection));
                else {
                    if (PQntuples(db_result) == 1) {
                        //-- Find Eye Center
                        cv::Rect rightEyeRegion(face.width - eye_region_width - face.width*(kEyePercentSide/100.0), eye_region_top,eye_region_width,eye_region_height);
                        rightPupil = findEyeCenter(faceROI,rightEyeRegion,"Right Eye");

                        // get corner regions
                        cv::Rect rightLeftCornerRegion(rightEyeRegion);
                        rightLeftCornerRegion.width = rightPupil.x;
                        rightLeftCornerRegion.height /= 2;
                        rightLeftCornerRegion.y += rightLeftCornerRegion.height / 2;
                        cv::Rect rightRightCornerRegion(rightEyeRegion);
                        rightRightCornerRegion.width -= rightPupil.x;
                        rightRightCornerRegion.x += rightPupil.x;
                        rightRightCornerRegion.height /= 2;
                        rightRightCornerRegion.y += rightRightCornerRegion.height / 2;

                        // change eye centers to face coordinates
                        rightPupil.x += rightEyeRegion.x;
                        rightPupil.y += rightEyeRegion.y;

                        // change eye centers to source image coordinates
                        rightPupil.x += bounding_box_x;
                        rightPupil.y += bounding_box_y;

                    }
                    PQclear(db_result);
                }

                /**
                    Insert pupil data into table
                */
                int i_binary = htonl(i);
                int video_id_binary = htonl(video_id);
                const char* values[4];
                values[0] = (char *) &i_binary;
                values[1] = (char *) &video_id_binary;
                char point_format[] = "%d,%d";
                char left_pupil_point[25];
                char right_pupil_point[25];
                sprintf(left_pupil_point, point_format, leftPupil.x, leftPupil.y);
                values[2] = left_pupil_point;
                sprintf(right_pupil_point, point_format, rightPupil.x, rightPupil.y);
                values[3] = right_pupil_point;
                int lengths[4] = {sizeof(i_binary), sizeof(video_id_binary), sizeof(values[2]), sizeof(values[3])};
                int binary[4] = {1, 1, 0, 0};

                strncpy (&db_statement[0], "INSERT INTO pupil_data (frame_id, video_id, left_pupil, right_pupil) VALUES ($1::int4, $2::int4, $3::point, $4::point)", 5000);
                db_result = PQexecParams (db_connection, db_statement, 4, NULL, values, lengths, binary, 0);

            }
        }
    }
    cout << "\nCompleted.\n";
}
