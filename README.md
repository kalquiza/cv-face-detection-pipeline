# Computer Vision Face Detection Pipeline

## Overview
A face detection pipeline using open source software capable of applying various face detection techniques to an input video.

#### Objectives
- Create a command line based tool for face detection.
- Design a scalable, relational database schema in Postgres for storing data and connect using the Postgres C application interface.
- Utilize FFmpeg tools for the handling of multimedia files.
- Implement face detection techniques (Haar Cascades, Gradient Based Eye Center Tracking, and Active Shape Modelling) adapted from open source software to fit the requirements of the pipeline.
- Use OpenCV drawing functions to draw results into still image files and stich them into a new video, providing meaningful and observable output.

#### Processes
This computer vision pipeline consists of the following processes/stages:
1. Image Extraction
2. Haar Cascades for Feature Detection
3. Draw Bounding Boxes
4. Gradient Based Eye Center Tracking
5. Draw Pupil Crosshairs
6. Active Shape Modelling for Facial Landmarks
7. Draw Face Mesh using Delaunay Triangulation

IMPORTANT: Each of these processes can be ran independently in this modular system, however image analysis processes require extracted still images for reference and drawing processes rely on the previous analysis step for significant results in the output video file. For example, if you only require a face mesh video one only needs to run processes 1, 6, and 7.

## Requirements
This computer vision pipeline uses a number of open source software and libraries:

- [OpenCV](http://opencv.org/)
- [PostgreSQL](https://www.postgresql.org)
- [FFmpeg](https://ffmpeg.org/)
- [eyelike](https://github.com/trishume/eyeLike) by Tristan Hume
- [STASM](http://www.milbo.users.sonic.net/stasm/) by Stephen Milborrow

## Usage
#### Preparing the database
In order to run the application, it is necessary to create a SQL database to store the results. The schema for this application can be found in the file **db.sql** in the folder **/Contents/database**.

Once the database is initialized, change the information found in **conninfo** in the same folder **/Contents/database** to match your own database credentials.
```sh
host = 'localhost' dbname = 'cv_face_detection_pipeline' user = 'kris' password = 'cvface'
```
#### Building the Application
To build the application, open your terminal and run the included makefile from the **Contents** folder.
```sh
$ cd Contents
Contents $ make
```
Run each stage of the pipeline independently according to your needs from the **/Contents/build** folder:

#### Extracting images
In order to process a video the first step is to extract images and create a unique video id to identify the process throughout the pipeline.
```sh
Contents $ cd build
build $ ./img_extract filename.mp4
```

#### Image Processing
After extracting images from a video file the application will return a video id to be used to identify that particular video in subsequent stages of the pipeline. The output of files can be found in the **/Output** folder created in the root directory under the video's corresponding video id.

Following image extraction calls to subsequent stages in the pipeline will take in the video id as the argument. For example, if the video id is 1:
```sh
build $ ./bounding_boxes 1
build $ ./haar_cascades 1
build $ ./bounding_boxes 1
build $ ./pupil_tracking 1
build $ ./pupil_crosshairs 1
build $ ./active_shape_modelling 1
build $ ./delaunay_triangulation 1
```
The results of these processes can then be viewed as stored data in your Postgres database and as video and still images in the **/Output** directory.

### Readings
- *Accurate eye centre localisation by means of gradients* by Fabian Timm and Erhardt Barth
- *Active Shape Models - Their Training and Application* by T. F. Cootes, D. Cooper, C. J. Taylor, and J. Graham
