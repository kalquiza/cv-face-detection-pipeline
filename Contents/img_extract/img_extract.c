/**
    img_extract.cpp
    Purpose: Extract still images from video

    @author Kristoffer Alquiza
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "/usr/include/postgresql/libpq-fe.h"

int main (int argc, char *argv[])
{
    /**
        Connect to postgres database
    */
    char input_video_filename[1280], str[100], video_id[25]; int num, den, num_frames, height, width; float frame_rate;

    PGconn   *db_connection;
    PGresult *db_result;
    db_connection = PQconnectdb("host = 'localhost' dbname = 'cv_face_detection_pipeline' user = 'postgres' password = 'cvface'");
    if (PQstatus(db_connection) != CONNECTION_OK)
    {
        printf ("Connection to database failed: %s", PQerrorMessage(db_connection));
        PQfinish (db_connection);
        exit (EXIT_FAILURE);
    }

    // TODO: Extract video metadata into database and still images into a new directory
}
