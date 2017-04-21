/**
    haar_cascades.cpp
    Purpose: Use Haar cascades to determine bounding boxes for facial features

    @author Kristoffer Alquiza
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "/usr/local/include/opencv/cv.h"
#include "/usr/local/include/opencv/cvaux.h"
#include "/usr/local/include/opencv/highgui.h"
#include "/usr/include/postgresql/libpq-fe.h"

using namespace cv;
using namespace std;

int main (int argc, char *argv[])
{
    Mat source_image_gray, source_image_gray_face_region, source_image_gray_left_eye_region, source_image_gray_right_eye_region, source_image_gray_nose_region, source_image_gray_mouth_region;
    CascadeClassifier face_cascade, left_eye_cascade, right_eye_cascade, nose_cascade, mouth_cascade;
    std::vector<Rect> face, eyes, nose, mouth;

    char input_filename[1280];
    int number_of_left_eyes_detected, number_of_right_eyes_detected, number_of_noses_detected, number_of_mouths_detected, min_neighbors;
    int maximum_width_left_eye, maximum_height_left_eye, maximum_width_right_eye, maximum_height_right_eye;
    int minimum_width_left_eye, minimum_height_left_eye, minimum_width_right_eye, minimum_height_right_eye;
    int maximum_width_nose, maximum_height_nose, maximum_width_mouth, maximum_height_mouth;
    int minimum_width_nose, minimum_height_nose, minimum_width_mouth, minimum_height_mouth;
    float minimum_width_bounding_box, minimum_height_bounding_box, maximum_width_bounding_box, maximum_height_bounding_box;
    int bounding_box_face_x, bounding_box_face_y, bounding_box_face_width, bounding_box_face_height;
    int bounding_box_left_eye_x, bounding_box_left_eye_y, bounding_box_left_eye_width, bounding_box_left_eye_height;
    int bounding_box_right_eye_x, bounding_box_right_eye_y, bounding_box_right_eye_width, bounding_box_right_eye_height;
    int bounding_box_nose_x, bounding_box_nose_y, bounding_box_nose_width, bounding_box_nose_height;
    int bounding_box_mouth_x, bounding_box_mouth_y, bounding_box_mouth_width, bounding_box_mouth_height;
    double scale_factor;

    min_neighbors = 10;
    scale_factor  = 1.01;

    /**
        Connect to postgres database
    */
    PGconn   *db_connection;
    PGresult *db_result;
    db_connection = PQconnectdb("host = 'localhost' dbname = 'cv_face_detection_pipeline' user = 'postgres' password = 'cvface'");
    if (PQstatus(db_connection) != CONNECTION_OK)
    {
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
    int num_frames;
    int width;
    int height;
    char res[25];
    char metadata_query[1280] = "SELECT num_frames, width, height FROM input_video_metadata WHERE video_id = ";
    strncpy (&db_statement[0], strcat(metadata_query, argv[1]), 5000);
    db_result = PQexecParams (db_connection, db_statement, 0, NULL, 0, 0, 0, 0);
    if (PQresultStatus(db_result) != PGRES_TUPLES_OK)
    {
        printf ("Postgres INSERT error: %s\n", PQerrorMessage(db_connection));
    }
    else
    {
        if (PQntuples(db_result) == 1)
        {
            strncpy (&res[0], PQgetvalue(db_result, 0, 0), 25);
            sscanf(res, "%d", &num_frames);
            strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
            sscanf(res, "%d", &width);
            strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
            sscanf(res, "%d", &height);
        }
        PQclear(db_result);
    }

    printf("video_id: %d\n", video_id);
    printf("num_frames: %d\n", num_frames);
    printf("width: %d\n", width);
    printf("height %d\n", height);

    /**
        Process the directory of extracted images
    */
    for (int i = 1; i <= num_frames; i++) {
        // copy image location
        char img_dir_format[] = "./video_id_%d/video_id_%d_%d.png";
        char img_dir[1280];
        sprintf(img_dir, img_dir_format, video_id, video_id, i);
        strcpy (&input_filename[0], img_dir);
        printf("processing %s\n", img_dir);

        // Find bounding boxes using Haar cascades
        if (face_cascade.load ("./haarcascade_frontalface_alt2.xml") && left_eye_cascade.load ("./ojoI.xml") && right_eye_cascade.load ("./ojoD.xml") && nose_cascade.load ("./Nariz.xml") && mouth_cascade.load ("./Mouth.xml")) // loaded additional cascades for mouth and nose
        {
            bounding_box_face_x           = 0;
            bounding_box_face_y           = 0;
            bounding_box_face_width       = 0;
            bounding_box_face_height      = 0;
            bounding_box_left_eye_x       = 0;
            bounding_box_left_eye_y       = 0;
            bounding_box_left_eye_width   = 0;
            bounding_box_left_eye_height  = 0;
            bounding_box_right_eye_x      = 0;
            bounding_box_right_eye_y      = 0;
            bounding_box_right_eye_width  = 0;
            bounding_box_right_eye_height = 0;
            bounding_box_nose_x           = 0;
            bounding_box_nose_y           = 0;
            bounding_box_nose_width       = 0;
            bounding_box_nose_height      = 0;
            bounding_box_mouth_x          = 0;
            bounding_box_mouth_y          = 0;
            bounding_box_mouth_width      = 0;
            bounding_box_mouth_height     = 0;

            source_image_gray = imread (input_filename, CV_LOAD_IMAGE_GRAYSCALE);
            if (!source_image_gray.empty())
            {
                equalizeHist (source_image_gray, source_image_gray);

                // face
                face_cascade.detectMultiScale (source_image_gray, face, scale_factor, min_neighbors, CV_HAAR_FIND_BIGGEST_OBJECT);

                // set minimum and maximum values
                if (face.size() > 0)
                {
                    bounding_box_face_x      = face[0].x;
                    bounding_box_face_y      = face[0].y;
                    bounding_box_face_width  = face[0].width;
                    bounding_box_face_height = face[0].height;
                    minimum_width_left_eye =  0.18 * bounding_box_face_width;
                    minimum_height_left_eye = 0.14 * bounding_box_face_height;
                    minimum_width_right_eye =  minimum_width_left_eye;
                    minimum_height_right_eye = minimum_height_left_eye;
                    if ((minimum_width_left_eye < 18) || (minimum_width_right_eye < 18) || (minimum_height_left_eye < 12) || (minimum_height_right_eye < 12))
                    {
                        minimum_width_left_eye   = 18;
                        minimum_height_left_eye  = 12;
                        minimum_width_right_eye  = 18;
                        minimum_height_right_eye = 12;
                    }
                    maximum_width_left_eye   = minimum_width_left_eye * 2;
                    maximum_height_left_eye  = minimum_height_left_eye * 2;
                    maximum_width_right_eye  = minimum_width_right_eye * 2;
                    maximum_height_right_eye = minimum_height_right_eye * 2;

                    minimum_width_nose =  0.18 * bounding_box_face_width;
                    minimum_height_nose = 0.14 * bounding_box_face_height;
                    minimum_width_mouth = minimum_width_nose;
                    minimum_height_mouth = minimum_height_nose;
                    if ((minimum_width_nose < 25) || (minimum_width_mouth < 25) || (minimum_height_mouth < 15) || (minimum_height_mouth < 15))
                    {
                        minimum_width_nose   = 25;
                        minimum_height_nose  = 15;
                        minimum_width_mouth  = 25;
                        minimum_height_mouth = 15;
                    }
                    maximum_width_nose   = minimum_width_nose * 2;
                    maximum_height_nose  = minimum_height_nose * 2;
                    maximum_width_mouth  = minimum_width_mouth * 2;
                    maximum_height_mouth = minimum_height_mouth * 2;

                  // left_eye
                  source_image_gray_left_eye_region = source_image_gray (Rect (bounding_box_face_x + bounding_box_face_width / 2, bounding_box_face_y + bounding_box_face_height / 5.5, bounding_box_face_width / 2, bounding_box_face_height / 3));
                  left_eye_cascade.detectMultiScale (source_image_gray_left_eye_region, eyes, scale_factor, min_neighbors, CV_HAAR_SCALE_IMAGE, Size(minimum_width_left_eye, minimum_height_left_eye), Size(maximum_width_left_eye, maximum_height_left_eye));
                  number_of_left_eyes_detected = eyes.size();
                  if (eyes.size() > 0)
                  {
                      bounding_box_left_eye_x      = bounding_box_face_x + bounding_box_face_width / 2 + eyes[0].x;
                      bounding_box_left_eye_y      = bounding_box_face_y + bounding_box_face_height / 5.5 + eyes[0].y;
                      bounding_box_left_eye_width  = eyes[0].width;
                      bounding_box_left_eye_height = eyes[0].height;
                  }

                  // right_eye
                  source_image_gray_right_eye_region = source_image_gray (Rect (bounding_box_face_x, bounding_box_face_y + bounding_box_face_height / 5.5, bounding_box_face_width / 2, bounding_box_face_height / 3));
                  number_of_right_eyes_detected = 0;
                  right_eye_cascade.detectMultiScale (source_image_gray_right_eye_region, eyes, scale_factor, min_neighbors, CV_HAAR_SCALE_IMAGE, Size(minimum_width_right_eye, minimum_height_right_eye), Size(maximum_width_right_eye, maximum_height_right_eye));
                  number_of_right_eyes_detected = eyes.size();
                  if (eyes.size() > 0)
                  {
                      bounding_box_right_eye_x      = bounding_box_face_x + eyes[0].x;
                      bounding_box_right_eye_y      = bounding_box_face_y + bounding_box_face_height / 5.5 + eyes[0].y;
                      bounding_box_right_eye_width  = eyes[0].width;
                      bounding_box_right_eye_height = eyes[0].height;
                  }

                  // nose
                  int bounding_box_eyes_y = (bounding_box_right_eye_y + bounding_box_left_eye_y) / 2;
                  source_image_gray_nose_region = source_image_gray (Rect (bounding_box_face_x, bounding_box_eyes_y, bounding_box_face_width, bounding_box_face_height - (bounding_box_eyes_y - bounding_box_face_y)));
                  number_of_noses_detected = 0;
                  nose_cascade.detectMultiScale (source_image_gray_nose_region, nose, scale_factor, min_neighbors, CV_HAAR_SCALE_IMAGE, Size(minimum_width_nose, minimum_height_nose), Size(maximum_width_nose, maximum_height_nose));
                  number_of_noses_detected = nose.size();
                  if (nose.size() > 0)
                  {
                      bounding_box_nose_x      = bounding_box_face_x + nose[0].x;
                      bounding_box_nose_y      = bounding_box_eyes_y + nose[0].y;
                      bounding_box_nose_width  = nose[0].width;
                      bounding_box_nose_height = nose[0].height;
                  }

                  // mouth
                  source_image_gray_mouth_region = source_image_gray (Rect (bounding_box_face_x, bounding_box_nose_y, bounding_box_face_width, bounding_box_face_height - (bounding_box_nose_y - bounding_box_face_y)));
                  number_of_mouths_detected = 0;
                  mouth_cascade.detectMultiScale (source_image_gray_mouth_region, mouth, scale_factor, min_neighbors, CV_HAAR_SCALE_IMAGE, Size(minimum_width_mouth, minimum_height_mouth), Size(maximum_width_mouth, maximum_height_mouth));
                  number_of_mouths_detected = mouth.size();
                  if (mouth.size() > 0)
                  {
                      bounding_box_mouth_x      = bounding_box_face_x + mouth[0].x;
                      bounding_box_mouth_y      = bounding_box_nose_y + mouth[0].y;
                      bounding_box_mouth_width  = mouth[0].width;
                      bounding_box_mouth_height = mouth[0].height;
                  }

                }
            }
        }

        // TODO: Insert bounding box data into table
    }
}
