/**
    draw_bounding_boxes.cpp
    Purpose: Draw bounding box data on a copy of each still image frame and create a video

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
#include "/usr/local/include/opencv/cxcore.h"
#include "/usr/include/postgresql/libpq-fe.h"

using namespace cv;
using namespace std;

int main (int argc, char *argv[])
{
    /**
        Connect to postgres database
    */
    char db_statement[5000];
    PGconn   *db_connection;
    PGresult *db_result;
    db_connection = PQconnectdb("host = 'localhost' dbname = 'cv_face_detection_pipeline' user = 'postgres' password = 'opencv'");
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
    int frame_rate;
    char res[25];
    char metadata_query[1280] = "SELECT num_frames, frame_rate, width, height FROM input_video_metadata WHERE video_id = ";
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
            sscanf(res, "%d", &frame_rate);
            strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
            sscanf(res, "%d", &width);
            strncpy (&res[0], PQgetvalue(db_result, 0, 3), 25);
            sscanf(res, "%d", &height);
        }
        PQclear(db_result);
    }

    /**
        Create a new directory to store images and video
    */
    char output_filename[1280];
    char bounding_box_directory[50];
    snprintf(&bounding_box_directory[0], sizeof(bounding_box_directory) - 1, "bounding_box_video_id_%d", video_id);
    mkdir (bounding_box_directory, 0755);

    /**
        For each frame draw bounding boxes for facial features
    */
    for (int i = 1; i <= num_frames; i++)
    {
        // Load source image
        cv::Mat source_image;
        char input_filename[1280];
        snprintf(&input_filename[0], sizeof(input_filename) - 1, "./video_id_%d/video_id_%d_%d.png", video_id, video_id, i);
        source_image = imread (&input_filename[0], 1);

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
        cout << "Drawing bounding_boxes video_id_" << video_id << " [" << bar << "] " << (static_cast<int>(100 * i/num_frames)) << "% " << "(" << i << "/" << num_frames << ")";
        flush(cout); // Required

        if (!source_image.empty()) {
            // Query bounding box data
            char bounding_box_query_format[1280] = "SELECT %s_upper_left, %s_width, %s_height FROM bounding_box_data WHERE video_id = %d AND frame_id = %d";
            char bounding_box_query[1280];
            char bounding_box_pt[50];
            int bounding_box_x = 0;
            int bounding_box_y = 0;
            int bounding_box_width = 0;
            int bounding_box_height = 0;
            CvPoint point_P1, point_P2;


            // Draw bounding box for face (Blue)
            char face[] = "face";
            sprintf(bounding_box_query, bounding_box_query_format, face, face, face, video_id, i);
            strncpy (&db_statement[0], bounding_box_query, 5000);
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
                    sscanf(res, "%s", bounding_box_pt);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
                    sscanf(res, "%d", &bounding_box_width);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
                    sscanf(res, "%d", &bounding_box_height);
                }
                PQclear(db_result);
            }

            sscanf(bounding_box_pt, "(%i,%i)", &bounding_box_x, &bounding_box_y);
            point_P1 = cvPoint (bounding_box_x, bounding_box_y);
            point_P2 = cvPoint (bounding_box_x + bounding_box_width, bounding_box_y + bounding_box_height);

            if ((bounding_box_x > 0) && (bounding_box_y > 0)) {
                rectangle (source_image, point_P1, point_P2, CV_RGB (0, 0, 255), 2, 8, 0);
            }
            bounding_box_x = 0;
            bounding_box_y = 0;
            bounding_box_width = 0;
            bounding_box_height = 0;

            // Draw bounding box for left eye (Green)
            char l_eye[] = "left_eye";
            sprintf(bounding_box_query, bounding_box_query_format, l_eye, l_eye, l_eye, video_id, i);
            strncpy (&db_statement[0], bounding_box_query, 5000);
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
                    sscanf(res, "%s", bounding_box_pt);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
                    sscanf(res, "%d", &bounding_box_width);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
                    sscanf(res, "%d", &bounding_box_height);
                }
                PQclear(db_result);
            }

            sscanf(bounding_box_pt, "(%i,%i)", &bounding_box_x, &bounding_box_y);
            point_P1 = cvPoint (bounding_box_x, bounding_box_y);
            point_P2 = cvPoint (bounding_box_x + bounding_box_width, bounding_box_y + bounding_box_height);

            if ((bounding_box_x > 0) && (bounding_box_y > 0)) {
                rectangle (source_image, point_P1, point_P2, CV_RGB (0, 255, 0), 2, 8, 0);
            }
            bounding_box_x = 0;
            bounding_box_y = 0;
            bounding_box_width = 0;
            bounding_box_height = 0;

            // Draw bounding box for right eye (Red)
            char r_eye[] = "right_eye";
            sprintf(bounding_box_query, bounding_box_query_format, r_eye, r_eye, r_eye, video_id, i);
            strncpy (&db_statement[0], bounding_box_query, 5000);
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
                    sscanf(res, "%s", bounding_box_pt);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
                    sscanf(res, "%d", &bounding_box_width);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
                    sscanf(res, "%d", &bounding_box_height);
                }
                PQclear(db_result);
            }

            sscanf(bounding_box_pt, "(%i,%i)", &bounding_box_x, &bounding_box_y);
            point_P1 = cvPoint (bounding_box_x, bounding_box_y);
            point_P2 = cvPoint (bounding_box_x + bounding_box_width, bounding_box_y + bounding_box_height);

            if ((bounding_box_x > 0) && (bounding_box_y > 0)) {
                rectangle (source_image, point_P1, point_P2, CV_RGB (255, 0, 0), 2, 8, 0);
            }
            bounding_box_x = 0;
            bounding_box_y = 0;
            bounding_box_width = 0;
            bounding_box_height = 0;

            // Draw bounding box for nose (Yellow)
            char nose[] = "nose";
            sprintf(bounding_box_query, bounding_box_query_format, nose, nose, nose, video_id, i);
            strncpy (&db_statement[0], bounding_box_query, 5000);
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
                    sscanf(res, "%s", bounding_box_pt);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
                    sscanf(res, "%d", &bounding_box_width);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
                    sscanf(res, "%d", &bounding_box_height);
                }
                PQclear(db_result);
            }

            sscanf(bounding_box_pt, "(%i,%i)", &bounding_box_x, &bounding_box_y);
            point_P1 = cvPoint (bounding_box_x, bounding_box_y);
            point_P2 = cvPoint (bounding_box_x + bounding_box_width, bounding_box_y + bounding_box_height);

            if ((bounding_box_x > 0) && (bounding_box_y > 0)) {
                rectangle (source_image, point_P1, point_P2, CV_RGB (255, 255, 0), 2, 8, 0);
            }
            bounding_box_x = 0;
            bounding_box_y = 0;
            bounding_box_width = 0;
            bounding_box_height = 0;

            // Bounding box for mouth (Magenta)
            char mouth[] = "mouth";
            sprintf(bounding_box_query, bounding_box_query_format, mouth, mouth, mouth, video_id, i);
            strncpy (&db_statement[0], bounding_box_query, 5000);
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
                    sscanf(res, "%s", bounding_box_pt);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 1), 25);
                    sscanf(res, "%d", &bounding_box_width);
                    strncpy (&res[0], PQgetvalue(db_result, 0, 2), 25);
                    sscanf(res, "%d", &bounding_box_height);
                }
                PQclear(db_result);
            }

            sscanf(bounding_box_pt, "(%i,%i)", &bounding_box_x, &bounding_box_y);
            point_P1 = cvPoint (bounding_box_x, bounding_box_y);
            point_P2 = cvPoint (bounding_box_x + bounding_box_width, bounding_box_y + bounding_box_height);

            if ((bounding_box_x > 0) && (bounding_box_y > 0)) {
                rectangle (source_image, point_P1, point_P2, CV_RGB (255, 0, 255), 2, 8, 0);
            }
            bounding_box_x = 0;
            bounding_box_y = 0;
            bounding_box_width = 0;
            bounding_box_height = 0;

            // Save image to directory ./bounding_box_video_id_*
            snprintf (&output_filename[0], sizeof(output_filename) - 1,  "./%s/%s_%d.png", bounding_box_directory, bounding_box_directory, i);
            imwrite (output_filename, source_image);
        }
    }

    /**
        Create MP4 bounding_box_movie_video_id_* in directory ./bounding_box_VIDEO_ID_*
    */

    snprintf (&output_filename[0], sizeof(output_filename) - 1,  "./%s/%s.mp4", bounding_box_directory, bounding_box_directory);
    char video_export_command[1280];
    snprintf (&video_export_command[0], sizeof(video_export_command) - 1,  "ffmpeg -r %d -start_number 1 -f image2 -i ./%s/%s_%%d.png -c:v libx264 ./%s/%s.mp4", frame_rate, bounding_box_directory, bounding_box_directory, bounding_box_directory, bounding_box_directory);
    printf("%s", video_export_command);
    FILE *pipe_fp;
    pipe_fp = popen(video_export_command, "r");
    pclose(pipe_fp);

    cout << "\nCompleted.\n";
}
