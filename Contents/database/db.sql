CREATE DATABASE cv_face_detection_pipeline;

CREATE TABLE input_video_metadata(
	video_id				bigserial not null primary key
	frame_rate				real
	num_frames				integer
	height					integer
	width					integer
);

CREATE TABLE bounding_box_data(
	frame_id				bigint not null
	video_id				bigint not null
	face_upper_left			point
	face_width				integer
	face_height				integer
	left_eye_upper_left		point
	left_eye_width			integer
	left_eye_height			integer
	right_eye_upper_left	point
	right_eye_width			integer
	right_eye_height		integer
	nose_upper_left			point
	nose_width				integer
	nose_height				integer
	mouth_upper_left		point
	mouth_width				integer
	mouth_height			integer
);

CREATE TABLE pupil_data(
	frame_id				bigint not null
	video_id				bigint not null
	left_pupil				point
	right_pupil				point
);

CREATE TABLE stasm_data(
	frame_id				bigint not null
	video_id				bigint not null
	data_point_0			point
	data_point_1			point
	data_point_2			point
	data_point_3			point
	data_point_4			point
	data_point_5 			point
	data_point_6			point
	data_point_7			point
	data_point_8			point
	data_point_9			point
	data_point_10			point
	data_point_11			point
	data_point_12			point
	data_point_13			point
	data_point_14			point
	data_point_15 			point
	data_point_16			point
	data_point_17			point
	data_point_18			point
	data_point_19			point
	data_point_20			point
	data_point_21			point
	data_point_22			point
	data_point_23			point
	data_point_24			point
	data_point_25 			point
	data_point_26			point
	data_point_27			point
	data_point_28			point
	data_point_29			point
	data_point_30			point
	data_point_31			point
	data_point_32			point
	data_point_33			point
	data_point_34			point
	data_point_35 			point
	data_point_36			point
	data_point_37			point
	data_point_38			point
	data_point_39			point
	data_point_40			point
	data_point_41			point
	data_point_42			point
	data_point_43			point
	data_point_44			point
	data_point_45 			point
	data_point_46			point
	data_point_47			point
	data_point_48			point
	data_point_49			point
	data_point_50			point
	data_point_51			point
	data_point_52			point
	data_point_53			point
	data_point_54			point
	data_point_55 			point
	data_point_56			point
	data_point_57			point
	data_point_58			point
	data_point_59			point
	data_point_60			point
	data_point_61			point
	data_point_62			point
	data_point_63			point
	data_point_64			point
	data_point_65 			point
	data_point_66			point
	data_point_67			point
	data_point_68			point
	data_point_69			point
	data_point_70			point
	data_point_71			point
	data_point_72			point
	data_point_73			point
	data_point_74			point
	data_point_75 			point
	data_point_76			point
);