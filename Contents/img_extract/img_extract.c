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
    char input_video_filename[1280], str[100], video_id[25]; int num, den, num_frames, height, width; float frame_rate;

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
        Read command line argument for video filename
    */
    printf ("Enter input video name: ");
    fgets(input_video_filename,1280,stdin);
    strtok(input_video_filename, "\n");
    if( access( input_video_filename, F_OK ) == -1 ) {
        printf("File not found.\n");
        exit (EXIT_FAILURE);
    }

    /**
        Extract video metadata
    */

    // Extract frame rate
    char frame_rate_command[1280] = "ffprobe -v error -select_streams v:0 -show_entries stream=avg_frame_rate -of default=noprint_wrappers=1:nokey=1 ";
    FILE *fps = popen(strcat(frame_rate_command, input_video_filename), "r");
    while(fgets(str,100,fps) != NULL ){
        char *tokenstring = str;
        sscanf(tokenstring, "%d/%d", &num,&den);
    }
    fclose(fps);
    frame_rate = num/den;

    // Extract total number of frames
    char nb_read_frames_command[1280] = "ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames -of default=nokey=1:noprint_wrappers=1 ";
    FILE *nb_read_frames = popen(strcat(nb_read_frames_command, input_video_filename), "r");
    while(fgets(str,100,fps) != NULL ){
        char *tokenstring = str;
        sscanf(tokenstring, "%d", &num_frames);
    }
    fclose(fps);

    // Extract resolution
    char res_command[1280] = "ffprobe -v error -of flat=s=_ -select_streams v:0 -show_entries stream=height,width ";
    FILE *res = popen(strcat(res_command, input_video_filename), "r");
    while(fgets(str,100,fps) != NULL ){
        char *tokenstring = str;
        sscanf(tokenstring, "streams_stream_0_width=%d", &width);
        sscanf(tokenstring, "streams_stream_0_height=%d", &height);
    }
    fclose(fps);

    // TODO: Enter metadata into database and still images into a new directory
}
