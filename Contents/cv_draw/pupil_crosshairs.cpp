/**
    draw_pupil_crosshairs.cpp
    Purpose: Draw pupil data as crosshairs on a copy of each still image frame and create a video

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
    char db_statement[5000], conninfo[1280];
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
    char pupil_crosshairs_directory[100];
    char pupil_crosshairs_img_directory[100];
    snprintf(&pupil_crosshairs_directory[0], sizeof(pupil_crosshairs_directory) - 1, "../../Output/video_id_%d/pupil_crosshairs", video_id);
    snprintf(&pupil_crosshairs_img_directory[0], sizeof(pupil_crosshairs_img_directory) - 1, "%s/pupil_crosshairs_img", pupil_crosshairs_directory);
    mkdir (pupil_crosshairs_directory, 0755);
    mkdir (pupil_crosshairs_img_directory, 0755);

    /**
        For each frame draw pupil crosshairs
    */
    for (int i = 1; i <= num_frames; i++) {

        // load source image
        cv::Mat source_image;
        char input_filename[1280];
        snprintf(&input_filename[0], sizeof(input_filename) - 1, "../../Output/video_id_%d/img_extract/video_id_%d_%d.png", video_id, video_id, i);

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
        cout << "Drawing pupil_crosshairs video_id_" << video_id << " [" << bar << "] " << (static_cast<int>(100 * i/num_frames)) << "% " << "(" << i << "/" << num_frames << ")";
        flush(cout); // Required

        source_image = imread (&input_filename[0], 1);
        if (!source_image.empty()) {

            // Query eye pupil data
            char eye_pupil_tracking_query_format[1280] = "SELECT %s_pupil FROM pupil_data WHERE video_id = %d AND frame_id = %d";
            char eye_pupil_tracking_query[1280], left_eye_pupil_tracking_pt[50], right_eye_pupil_tracking_pt[50];
            int left_eye_pupil_tracking_x, left_eye_pupil_tracking_y, right_eye_pupil_tracking_x, right_eye_pupil_tracking_y;
            CvPoint left_eye_point, right_eye_point;

            // Left eye pupil
            char left[] = "left";
            sprintf(eye_pupil_tracking_query, eye_pupil_tracking_query_format, left, video_id, i);
            strncpy (&db_statement[0], eye_pupil_tracking_query, 5000);
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
                    sscanf(res, "%s", left_eye_pupil_tracking_pt);
                }
                PQclear(db_result);
            }

            sscanf(left_eye_pupil_tracking_pt, "(%i,%i)", &left_eye_pupil_tracking_x, &left_eye_pupil_tracking_y);
            left_eye_point = cvPoint (left_eye_pupil_tracking_x,left_eye_pupil_tracking_y);

            // Draw left pupil crosshair
            if ((left_eye_pupil_tracking_x > 0) && (left_eye_pupil_tracking_y > 0)) {

                line (source_image, left_eye_point, cvPoint(left_eye_point.x, left_eye_point.y + 10), CV_RGB (255, 255, 0), 1, 8, 0);
                line (source_image, left_eye_point, cvPoint(left_eye_point.x, left_eye_point.y - 10), CV_RGB (255, 255, 0), 1, 8, 0);
                line (source_image, left_eye_point, cvPoint(left_eye_point.x + 10, left_eye_point.y), CV_RGB (255, 255, 0), 1, 8, 0);
                line (source_image, left_eye_point, cvPoint(left_eye_point.x - 10, left_eye_point.y), CV_RGB (255, 255, 0), 1, 8, 0);
            }

            // Right eye pupil
            char right[] = "right";
            sprintf(eye_pupil_tracking_query, eye_pupil_tracking_query_format, right, video_id, i);
            strncpy (&db_statement[0], eye_pupil_tracking_query, 5000);
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
                    sscanf(res, "%s", right_eye_pupil_tracking_pt);
                }
                PQclear(db_result);
            }

            sscanf(right_eye_pupil_tracking_pt, "(%i,%i)", &right_eye_pupil_tracking_x, &right_eye_pupil_tracking_y);
            right_eye_point = cvPoint (right_eye_pupil_tracking_x,right_eye_pupil_tracking_y);

            // Draw right pupil crosshair
            if ((right_eye_pupil_tracking_x > 0) && (right_eye_pupil_tracking_y > 0)) {
                line (source_image, right_eye_point, cvPoint(right_eye_point.x, right_eye_point.y + 10), CV_RGB (255, 255, 0), 1, 8, 0);
                line (source_image, right_eye_point, cvPoint(right_eye_point.x, right_eye_point.y - 10), CV_RGB (255, 255, 0), 1, 8, 0);
                line (source_image, right_eye_point, cvPoint(right_eye_point.x + 10, right_eye_point.y), CV_RGB (255, 255, 0), 1, 8, 0);
                line (source_image, right_eye_point, cvPoint(right_eye_point.x - 10, right_eye_point.y), CV_RGB (255, 255, 0), 1, 8, 0);
            }

            // Save image to directory pupil_crosshairs/pupil_crosshairs_img
            snprintf (&output_filename[0], sizeof(output_filename) - 1, "%s/pupil_crosshairs_video_id_%d_%d.png", pupil_crosshairs_img_directory, video_id, i);
            imwrite (output_filename, source_image);
        }
    }

        /**
            Create MP4 eye_pupil_crosshairs_video_id_*.mp4
         */

        snprintf (&output_filename[0], sizeof(output_filename) - 1,  "%s/pupil_crosshairs_video_id_%d.mp4", pupil_crosshairs_directory, video_id);
        char video_export_command[1280];
        snprintf (&video_export_command[0], sizeof(video_export_command) - 1,  "ffmpeg -r %d -start_number 1 -f image2 -i %s/pupil_crosshairs_video_id_%d_%%d.png -c:v libx264 %s/pupil_crosshairs_video_id_%d.mp4", frame_rate, pupil_crosshairs_img_directory, video_id, pupil_crosshairs_directory, video_id);
        printf("%s", video_export_command);
    	FILE *pipe_fp;
    	pipe_fp = popen(video_export_command, "r");
    	pclose(pipe_fp);

    	cout << "\nCompleted.\n";
}
