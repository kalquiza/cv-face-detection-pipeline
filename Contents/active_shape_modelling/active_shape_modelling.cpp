/**
    active_shape_modelling.cpp
    Purpose: Implement the active shape modelling algorithm proposed by T. F. Cootes, D. Cooper, C. J.
    Tyler, and J. Graham in their article "Active Shape Models - Their Training and Application" using
    the open-source program STASM written by Stephen Milborrow (http://www.milbo.org/stasm-files/5/stasm4.1.0.tar.gz).

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
#include "../stasm/stasm_lib.h"

using namespace cv;
using namespace std;

int main (int argc, char *argv[])
{

    /**
        Connect to postgres database
    */
    char db_statement[10000];
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
        For each frame send to STASM for analysis
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
        cout << "Processing active_shape_modelling video_id_" << video_id << " [" << bar << "] " << (static_cast<int>(100 * i/num_frames)) << "% " << "(" << i << "/" << num_frames << ")";
        flush(cout); // Required

        source_image = imread (&input_filename[0], 1);
        if (!source_image.empty()) {

                // minimal.cpp
                static const char* path = input_filename;

                cv::Mat_<unsigned char> img(cv::imread(path, CV_LOAD_IMAGE_GRAYSCALE));

                if (!img.data)
                {
                printf("Cannot load %s\n", path);
                exit(1);
                }

                int foundface;
                float landmarks[2 * stasm_NLANDMARKS]; // x,y coords

                if (!stasm_search_single(&foundface, landmarks,
                             (char*)img.data, img.cols, img.rows, path, "../data"))
                {
                printf("Error in stasm_search_single: %s\n", stasm_lasterr());
                exit(1);
                }

                /**
                    Insert STASM data into table
                */
                if (foundface)
                {
                char point_format[] = "%d,%d";
                char stasm_point[77][25];
                int i_binary = htonl(i);
                int video_id_binary = htonl(video_id);
                const char* values[79];
                values[0] = (char *) &i_binary;
                values[1] = (char *) &video_id_binary;
                // For each of 77 STASM data points
                for (int j = 0; j < stasm_NLANDMARKS; j++) {
                    sprintf(stasm_point[j], point_format, (int)landmarks[j*2+1], (int)landmarks[j*2]);
                    values[j+2] = stasm_point[j];
                }

                int lengths[79] = {sizeof(i_binary), sizeof(video_id_binary), sizeof(values[2]), sizeof(values[3]), sizeof(values[4]), sizeof(values[5]), sizeof(values[6]), sizeof(values[7]), sizeof(values[8]), sizeof(values[9]), sizeof(values[10]), sizeof(values[11]), sizeof(values[12]), sizeof(values[13]), sizeof(values[14]), sizeof(values[15]), sizeof(values[16]), sizeof(values[17]), sizeof(values[18]), sizeof(values[19]), sizeof(values[20]), sizeof(values[21]), sizeof(values[22]), sizeof(values[23]), sizeof(values[24]), sizeof(values[25]), sizeof(values[26]), sizeof(values[27]), sizeof(values[28]), sizeof(values[29]), sizeof(values[30]), sizeof(values[31]), sizeof(values[32]), sizeof(values[33]), sizeof(values[34]), sizeof(values[35]), sizeof(values[36]), sizeof(values[37]), sizeof(values[38]), sizeof(values[39]), sizeof(values[40]), sizeof(values[41]), sizeof(values[42]), sizeof(values[43]), sizeof(values[44]), sizeof(values[45]), sizeof(values[46]), sizeof(values[47]), sizeof(values[48]), sizeof(values[49]), sizeof(values[50]), sizeof(values[51]), sizeof(values[52]), sizeof(values[53]), sizeof(values[54]), sizeof(values[55]), sizeof(values[56]), sizeof(values[57]), sizeof(values[58]), sizeof(values[59]), sizeof(values[60]), sizeof(values[61]), sizeof(values[62]), sizeof(values[63]), sizeof(values[64]), sizeof(values[65]), sizeof(values[66]), sizeof(values[67]), sizeof(values[68]), sizeof(values[69]), sizeof(values[70]), sizeof(values[71]), sizeof(values[72]), sizeof(values[73]), sizeof(values[74]), sizeof(values[75]), sizeof(values[76]), sizeof(values[77]), sizeof(values[78])};

                int binary[79] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                strncpy (&db_statement[0], "INSERT INTO stasm_data (frame_id, video_id, data_point_0, data_point_1, data_point_2, data_point_3, data_point_4,  data_point_5, data_point_6, data_point_7, data_point_8, data_point_9, data_point_10, data_point_11, data_point_12, data_point_13, data_point_14, data_point_15, data_point_16, data_point_17, data_point_18, data_point_19, data_point_20, data_point_21, data_point_22, data_point_23, data_point_24, data_point_25, data_point_26, data_point_27, data_point_28, data_point_29, data_point_30, data_point_31, data_point_32, data_point_33, data_point_34, data_point_35, data_point_36, data_point_37, data_point_38, data_point_39, data_point_40, data_point_41, data_point_42, data_point_43, data_point_44, data_point_45, data_point_46, data_point_47, data_point_48, data_point_49, data_point_50, data_point_51, data_point_52, data_point_53, data_point_54, data_point_55, data_point_56, data_point_57, data_point_58, data_point_59, data_point_60, data_point_61, data_point_62, data_point_63, data_point_64, data_point_65, data_point_66, data_point_67, data_point_68, data_point_69, data_point_70, data_point_71, data_point_72, data_point_73, data_point_74, data_point_75, data_point_76) VALUES ($1::int4, $2::int4, $3::point, $4::point, $5::point, $6::point, $7::point, $8::point, $9::point, $10::point, $11::point, $12::point, $13::point, $14::point, $15::point, $16::point, $17::point, $18::point, $19::point, $20::point,$21::point, $22::point, $23::point, $24::point, $25::point, $26::point, $27::point, $28::point, $29::point, $30::point, $31::point, $32::point, $33::point, $34::point, $35::point, $36::point, $37::point, $38::point, $39::point, $40::point, $41::point, $42::point, $43::point, $44::point, $45::point, $46::point, $47::point, $48::point, $49::point, $50::point, $51::point, $52::point, $53::point, $54::point, $55::point, $56::point, $57::point, $58::point, $59::point, $60::point, $61::point, $62::point, $63::point, $64::point, $65::point, $66::point, $67::point, $68::point, $69::point, $70::point, $71::point, $72::point, $73::point, $74::point, $75::point, $76::point, $77::point, $78::point, $79::point)", 10000);
                db_result = PQexecParams (db_connection, db_statement, 79, NULL, values, lengths, binary, 0);
            }
        }
    }
    cout << "\nCompleted.\n";
}
