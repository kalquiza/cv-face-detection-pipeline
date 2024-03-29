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

    char input_filename[1280], conninfo[1280];
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
    char db_statement[5000];
    PGconn   *db_connection;
    PGresult *db_result;
    FILE *f = fopen("../database/conninfo", "r");
    if (fgets(conninfo, 1280, f)==NULL) {
	printf("Missing database credentials\n");
    }
    db_connection = PQconnectdb(conninfo);
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

    /**
        Process the directory of extracted images
    */
    for (int i = 1; i <= num_frames; i++) {
        // copy image location
        char img_dir_format[] = "../../Output/video_id_%d/img_extract/video_id_%d_%d.png";
        char img_dir[1280];
        sprintf(img_dir, img_dir_format, video_id, video_id, i);
        strcpy (&input_filename[0], img_dir);

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
        cout << "Processing haar_cascades video_id_" << video_id << " [" << bar << "] " << (static_cast<int>(100 * i/num_frames)) << "% " << "(" << i << "/" << num_frames << ")";
        flush(cout); // Required

        // Find bounding boxes using Haar cascades
        if (face_cascade.load ("../haar_cascades/haarcascade_frontalface_alt2.xml") && left_eye_cascade.load ("../haar_cascades/ojoI.xml") && right_eye_cascade.load ("../haar_cascades/ojoD.xml") && nose_cascade.load ("../haar_cascades/Nariz.xml") && mouth_cascade.load ("../haar_cascades/Mouth.xml")) // loaded additional cascades for mouth and nose
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

        /**
            Insert bounding box data into table
        */
        int i_binary = htonl(i);
        int video_id_binary = htonl(video_id);
        int bounding_box_face_width_binary = htonl(bounding_box_face_width);
        int bounding_box_face_height_binary = htonl(bounding_box_face_height);
        int bounding_box_left_eye_width_binary = htonl(bounding_box_left_eye_width);
        int bounding_box_left_eye_height_binary = htonl(bounding_box_left_eye_height);
        int bounding_box_right_eye_width_binary = htonl(bounding_box_right_eye_width);
        int bounding_box_right_eye_height_binary = htonl(bounding_box_right_eye_height);
        int bounding_box_nose_width_binary = htonl(bounding_box_nose_width);
        int bounding_box_nose_height_binary = htonl(bounding_box_nose_height);
        int bounding_box_mouth_width_binary = htonl(bounding_box_mouth_width);
        int bounding_box_mouth_height_binary = htonl(bounding_box_mouth_height);
        int height_binary = htonl(height);

        const char* values[17];
        values[0] = (char *) &i_binary;
        values[1] = (char *) &video_id_binary;

        char point_format[] = "%d,%d";
        char face_point[25];
        char left_eye_point[25];
        char right_eye_point[25];
        char nose_point[25];
        char mouse_point[25];

        sprintf(face_point, point_format, bounding_box_face_x, bounding_box_face_y);
        values[2] = face_point;
        values[3] = (char *) &bounding_box_face_width_binary;
        values[4] = (char *) &bounding_box_face_height_binary;

        sprintf(left_eye_point, point_format, bounding_box_left_eye_x, bounding_box_left_eye_y);
        values[5] = left_eye_point;
        values[6] = (char *) &bounding_box_left_eye_width_binary;
        values[7] = (char *) &bounding_box_left_eye_height_binary;

        sprintf(right_eye_point, point_format, bounding_box_right_eye_x, bounding_box_right_eye_y);
        values[8] = right_eye_point;
        values[9] = (char *) &bounding_box_right_eye_width_binary;
        values[10] = (char *) &bounding_box_right_eye_height_binary;

        sprintf(nose_point, point_format, bounding_box_nose_x, bounding_box_nose_y);
        values[11] = nose_point;
        values[12] = (char *) &bounding_box_nose_width_binary;
        values[13] = (char *) &bounding_box_nose_height_binary;

        sprintf(mouse_point, point_format, bounding_box_mouth_x, bounding_box_mouth_y);
        values[14] = mouse_point;
        values[15] = (char *) &bounding_box_mouth_width_binary;
        values[16] = (char *) &bounding_box_mouth_height_binary;

        int lengths[17] = {sizeof(i_binary), sizeof(video_id_binary), sizeof(values[2]) ,sizeof(bounding_box_face_width_binary), sizeof(bounding_box_face_height_binary), sizeof(values[5]), sizeof(bounding_box_left_eye_width_binary), sizeof(bounding_box_left_eye_height_binary), sizeof(values[8]) ,sizeof(bounding_box_right_eye_width_binary), sizeof(bounding_box_right_eye_height_binary), sizeof(values[11]) ,sizeof(bounding_box_nose_width_binary), sizeof(bounding_box_nose_height_binary), sizeof(values[14]) ,sizeof(bounding_box_mouth_width_binary), sizeof(bounding_box_mouth_height_binary)};
        int binary[17] = {1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1};

        strncpy (&db_statement[0], "INSERT INTO bounding_box_data (frame_id, video_id, face_upper_left, face_width, face_height, left_eye_upper_left, left_eye_width, left_eye_height, right_eye_upper_left, right_eye_width, right_eye_height, nose_upper_left, nose_width, nose_height, mouth_upper_left, mouth_width, mouth_height) VALUES ($1::int4, $2::int4, $3::point, $4::int4, $5::int4, $6::point, $7::int4, $8::int4, $9::point, $10::int4, $11::int4, $12::point, $13::int4, $14::int4, $15::point, $16::int4, $17::int4)", 5000);
        db_result = PQexecParams (db_connection, db_statement, 17, NULL, values, lengths, binary, 0);
    }

	cout << "\nCompleted.\n";
}
