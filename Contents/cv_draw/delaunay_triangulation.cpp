/**
    delaunay_triangulation.cpp
    Purpose: Implement OpenCV's Delaunay triangles algorithm to draw a face mesh on each still frame using
    our active shape modelling data points and stitch these images to create a video.

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
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "/usr/local/include/opencv/cv.h"
#include "/usr/local/include/opencv/cvaux.h"
#include "/usr/local/include/opencv/highgui.h"
#include "/usr/local/include/opencv/cxcore.h"
#include "/usr/include/postgresql/libpq-fe.h"

using namespace cv;
using namespace std;

typedef struct face_landmark_node
{
  int frame;
  int indice;
  float x;
  float y;
  struct face_landmark_node *next;
} face_landmark_node;

static void draw_point (Mat &img, Point2f fp, Scalar color)
{
  circle (img, fp, 3, color, CV_FILLED, CV_AA, 0);
}

static void draw_delaunay (Mat &img, Subdiv2D &subdiv, Scalar delaunay_color)
{
  vector<Vec6f> triangleList;
  subdiv.getTriangleList(triangleList);
  vector<Point> pt(3);
  Size size = img.size();
  Rect rect(0,0, size.width, size.height);

  for (size_t i = 0; i < triangleList.size(); i++)
  {
    Vec6f t = triangleList[i];
    pt[0] = Point(cvRound(t[0]), cvRound(t[1]));
    pt[1] = Point(cvRound(t[2]), cvRound(t[3]));
    pt[2] = Point(cvRound(t[4]), cvRound(t[5]));

    // Draw rectangles completely inside the image.
    if (rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
    {
      line (img, pt[0], pt[1], delaunay_color, 1, CV_AA, 0);
      line (img, pt[1], pt[2], delaunay_color, 1, CV_AA, 0);
      line (img, pt[2], pt[0], delaunay_color, 1, CV_AA, 0);
    }
  }
}

static Mat run (face_landmark_node *face_landmark_list_head, Mat source_image)
{
  face_landmark_node *face_landmark_element;
  Scalar delaunay_color(255,0,0), points_color(0, 0, 255); // Note: delaunay_color and points_color are in BGR (BLUE, GREEN, RED) format
  Size source_image_resolution;

  source_image_resolution = source_image.size();
  Rect rect(0, 0, source_image_resolution.width, source_image_resolution.height);
  Subdiv2D subdiv(rect);

  face_landmark_element = face_landmark_list_head;
  while (face_landmark_element != NULL) {
        subdiv.insert(Point2f(face_landmark_element->x, face_landmark_element->y));
        face_landmark_element = face_landmark_element->next;
  }
  draw_delaunay (source_image, subdiv, delaunay_color);
  face_landmark_element = face_landmark_list_head;
  while (face_landmark_element != NULL) {
    draw_point (source_image, Point2f(face_landmark_element->x, face_landmark_element->y), points_color);
    face_landmark_element = face_landmark_element->next;
  }
      return source_image;
}

face_landmark_node * add_face_landmark_element (face_landmark_node *face_landmark_list_head, int frame, int indice, float pixel_location_x, float pixel_location_y)
{
  face_landmark_node *new_face_landmark_element, *face_landmark_element, *previous_face_landmark_element;

  new_face_landmark_element = (face_landmark_node *) malloc (sizeof (face_landmark_node));
  if (new_face_landmark_element != NULL)
  {
    new_face_landmark_element->frame = frame;
    new_face_landmark_element->indice = indice;
    new_face_landmark_element->x = pixel_location_x;
    new_face_landmark_element->y = pixel_location_y;
    new_face_landmark_element->next = NULL;
    if (face_landmark_list_head != NULL)
    {
      face_landmark_element = face_landmark_list_head;
      while (face_landmark_element->next != NULL)
      {
        face_landmark_element = face_landmark_element->next;
      }
      face_landmark_element->next = new_face_landmark_element;
    }
    else
    {
      face_landmark_list_head = new_face_landmark_element;
    }
  }
  return face_landmark_list_head;
}

int main (int argc, char *argv[])
{

    /**
        Connect to postgres database
    */
    char db_statement[10000], conninfo[1280];
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
    char delaunay_triangulation_directory[100];
    char delaunay_triangulation_img_directory[100];
    snprintf(&delaunay_triangulation_directory[0], sizeof(delaunay_triangulation_directory) - 1, "../../Output/video_id_%d/delaunay_triangulation", video_id);
    snprintf(&delaunay_triangulation_img_directory[0], sizeof(delaunay_triangulation_img_directory) - 1, "%s/delaunay_triangulation_img", delaunay_triangulation_directory);
    mkdir (delaunay_triangulation_directory, 0755);
    mkdir (delaunay_triangulation_img_directory, 0755);

    // query for stasm data
    char stasm_query_format[2000] = "SELECT data_point_0, data_point_1, data_point_2, data_point_3, data_point_4, data_point_5, data_point_6, data_point_7, data_point_8, data_point_9, data_point_1, data_point_2, data_point_3, data_point_4, data_point_5, data_point_6, data_point_7, data_point_8, data_point_9, data_point_10, data_point_11, data_point_12, data_point_13, data_point_14, data_point_15, data_point_16, data_point_17, data_point_18, data_point_19, data_point_20, data_point_21, data_point_22, data_point_23, data_point_24, data_point_25, data_point_26, data_point_27, data_point_28, data_point_29, data_point_30, data_point_31, data_point_32, data_point_33, data_point_34, data_point_35, data_point_36, data_point_37, data_point_38, data_point_39, data_point_40, data_point_41, data_point_42, data_point_43, data_point_44, data_point_45, data_point_46, data_point_47, data_point_48, data_point_49, data_point_50, data_point_51, data_point_52, data_point_53, data_point_54, data_point_55, data_point_56, data_point_57, data_point_58, data_point_59, data_point_60, data_point_61, data_point_62, data_point_63, data_point_64, data_point_65, data_point_66, data_point_67, data_point_68, data_point_69, data_point_70, data_point_71, data_point_72, data_point_73, data_point_74, data_point_75, data_point_76 FROM stasm_data WHERE video_id = %d AND frame_id = %d";
    char stasm_query[2000];
    char stasm_pt[50];
    int stasm_x[77], stasm_y[77];

    /**
        For each frame draw fash mesh
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
        cout << "Drawing delaunay_triangulation video_id_" << video_id << " [" << bar << "] " << (static_cast<int>(100 * i/num_frames)) << "% " << "(" << i << "/" << num_frames << ")";
        flush(cout); // Required

        source_image = imread (&input_filename[0], 1);
        if (!source_image.empty()) {


            // Query database for 77 STASM facial data points
            sprintf(stasm_query, stasm_query_format, video_id, i);
            strncpy (&db_statement[0], stasm_query, 5000);
            db_result = PQexecParams (db_connection, db_statement, 0, NULL, 0, 0, 0, 0);
            if (PQresultStatus(db_result) != PGRES_TUPLES_OK)
            {
                printf ("Postgres SELECT error: %s\n", PQerrorMessage(db_connection));
            }
            else
            {
                if (PQntuples(db_result) == 1)
                {
                    for (int r = 0; r <= 76; r++) {
                        strncpy (&res[0], PQgetvalue(db_result, 0, r), 25);
                        sscanf(res, "%s", stasm_pt);
                        sscanf(stasm_pt, "(%i,%i)", &stasm_x[r], &stasm_y[r]);
                    }
                }
                PQclear(db_result);
            }

            // If 77 STASM data points exist use OpenCV to draw Delaunay triangles
            face_landmark_node *face_landmark_list_head, *face_landmark_element;
            face_landmark_list_head = NULL;

            // Set face landmark list for drawing Delaunay triangles
            for (int s = 0; s <= 76; s++) {
              face_landmark_list_head = add_face_landmark_element (face_landmark_list_head, i, s, (float)stasm_y[s], (float)stasm_x[s]);
            }

            source_image = run (face_landmark_list_head, source_image);
            while (face_landmark_list_head != NULL) {
                face_landmark_element = face_landmark_list_head;
                face_landmark_list_head = face_landmark_list_head->next;
                free (face_landmark_element);
                face_landmark_element = NULL;
            }

            // save image to directory delaunay_triangulation/delaunay_triangulation_img
            snprintf (&output_filename[0], sizeof(output_filename) - 1, "%s/delaunay_triangulation_video_id_%d_%d.png", delaunay_triangulation_img_directory, video_id, i);
            imwrite (output_filename, source_image);

        }
    }

    /**
         Create MP4 delaunay_triangulation_video_id_*.mp4
    */

    snprintf (&output_filename[0], sizeof(output_filename) - 1,  "%s/delaunay_triangulation_video_id_%d.mp4", delaunay_triangulation_directory, video_id);
    char video_export_command[1280];
    snprintf (&video_export_command[0], sizeof(video_export_command) - 1,  "ffmpeg -r %d -start_number 1 -f image2 -i %s/delaunay_triangulation_video_id_%d_%%d.png -c:v libx264 %s/delaunay_triangulation_video_id_%d.mp4", frame_rate, delaunay_triangulation_img_directory, video_id, delaunay_triangulation_directory, video_id);
    printf("%s", video_export_command);
    FILE *pipe_fp;
    pipe_fp = popen(video_export_command, "r");
    pclose(pipe_fp);

    cout << "\nCompleted.\n";
}
