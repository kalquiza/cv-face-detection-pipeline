/**
    img_extract.c
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
    char input_video_filename[1280], conninfo[1280], str[100], video_id[25]; int num, den, num_frames, height, width; float frame_rate;

    /**
        Connect to postgres database
    */
    PGconn   *db_connection;
    PGresult *db_result;
    FILE *f = fopen("../database/conninfo", "r");
    fgets(conninfo, 1280, f);
    db_connection = PQconnectdb(conninfo);
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

   /**
        Insert metadata into table
    */
    char db_statement[5000];
    const char* values[4];

    int frame_rate_binary = htonl(*(unsigned long *)&frame_rate);
    int num_frames_binary = htonl(num_frames);
    int width_binary = htonl(width);
    int height_binary = htonl(height);
    values[0] = (char *) &frame_rate_binary;
    values[1] = (char *) &num_frames_binary;
    values[2] = (char *) &width_binary;
    values[3] = (char *) &height_binary;
    int lengths[4] = {sizeof(frame_rate_binary), sizeof(num_frames_binary), sizeof(width_binary), sizeof(height_binary)};
    int binary[4] = {1, 1, 1, 1};

    strncpy (&db_statement[0], "INSERT INTO input_video_metadata (frame_rate, num_frames, height, width) VALUES ($1::float4, $2::int4, $3::int4, $4::int4) RETURNING video_id", 5000);
    db_result = PQexecParams (db_connection, db_statement, 4, NULL, values, lengths, binary, 0);
    if (PQresultStatus(db_result) != PGRES_TUPLES_OK)
    {
        printf ("Postgres INSERT error: %s\n", PQerrorMessage(db_connection));
    }
    else
    {
        if (PQntuples(db_result) == 1)
        {
            if (strlen(PQgetvalue(db_result, 0, 0)) < 25)
            {
                strncpy (&video_id[0], PQgetvalue(db_result, 0, 0), 25);
            }
        }
        PQclear(db_result);
    }

    /**
        Extract images into directory
    */
    char dir[25] = "video_id_";
    mkdir (strcat(dir,(char *)video_id), 0755);
    char img_extract_command_format[] = "ffmpeg -i %s -vf fps=%f ./video_id_%s/video_id_%s_%%d.png";
    char img_extract_command[1280];
    sprintf(img_extract_command, img_extract_command_format, input_video_filename, frame_rate, &video_id[0], &video_id[0]);
    FILE *pipe_fp;
    pipe_fp = popen(img_extract_command, "r");
    pclose(pipe_fp);

    //Print operation results
    /**
    printf ("filename: %s\n", input_video_filename);
    printf ("video_id: %s\n", &video_id[0]);
    printf("frame_rate: %f\n", frame_rate);
    printf("num_frames: %d\n", num_frames);
    printf("width: %d\n", width);
    printf("height: %d\n", height);
    */

    printf("Completed.\n");
}
