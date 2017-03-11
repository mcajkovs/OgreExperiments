/*****************************
Copyright 2011 Rafael Muñoz Salinas. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY Rafael Muñoz Salinas ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Rafael Muñoz Salinas OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Rafael Muñoz Salinas.
********************************/
#include "area.h"
#include <opencv2/calib3d/calib3d.hpp>
#include <fstream>
using namespace std;
using namespace cv;
namespace aruco {

	/**
	*
	*
	*/
	AreaConfiguration::AreaConfiguration() { mInfoType = NONE; cameraId = 0; }
	/**
	*
	*
	*/
	AreaConfiguration::AreaConfiguration(string filePath) throw(cv::Exception) {
		mInfoType = NONE;
		cameraId = 0;
		//readFromFile(filePath);
	}
	/**
	*
	*
	*/
	AreaConfiguration::AreaConfiguration(const AreaConfiguration &T) : vector< MyMarkerInfo >(T) {
		//     MarkersInfo=T.MarkersInfo;
		mInfoType = T.mInfoType;
		cameraId = T.cameraId;
	}

	/**
	*
	*
	*/
	AreaConfiguration &AreaConfiguration::operator=(const AreaConfiguration &T) {
		//     MarkersInfo=T.MarkersInfo;
		vector< aruco::MyMarkerInfo >::operator=(T);
		mInfoType = T.mInfoType;
		cameraId = T.cameraId;
		return *this;
	}
	/**
	*
	*
	*/
	void AreaConfiguration::saveToFile(string sfile) throw(cv::Exception) {

		cv::FileStorage fs(sfile, cv::FileStorage::WRITE);
		saveToFile(fs);
	}
	/**Saves the Area info to a file
	*/
	void AreaConfiguration::saveToFile(cv::FileStorage &fs) throw(cv::Exception) {
		fs << "aruco_ac_areaId" << (int)areaId;
		fs << "aruco_ac_cameraId" << (int)cameraId;
		fs << "aruco_ac_nmarkers" << (int)size();
		fs << "aruco_ac_mInfoType" << (int)mInfoType;
		fs << "aruco_ac_markers"
			<< "[";
		for (size_t i = 0; i < size(); i++) {
			fs << "{:"
				<< "id" << at(i).id;

			fs << "translation";
			fs << at(i);
			fs << "}";
		}
		fs << "]";
	}

	/**
	*
	*
	*/
	void AreaConfiguration::readFromFile(string sfile) throw(cv::Exception) {
		try {
			cv::FileStorage fs(sfile, cv::FileStorage::READ);
			readFromFile(fs);
		}
		catch (std::exception &ex) {
			throw cv::Exception(81818, "AreaConfiguration::readFromFile", ex.what() + string(" file=)") + sfile, __FILE__, __LINE__);
		}
	}


	/**Reads board info from a file
	*/
	void AreaConfiguration::readFromFile(cv::FileStorage &fs) throw(cv::Exception) {
		int aux = 0;
		// look for the nmarkers
		if (fs["aruco_ac_nmarkers"].name() != "aruco_ac_nmarkers")
			throw cv::Exception(81818, "AreaConfiguration::readFromFile", "invalid file type", __FILE__, __LINE__);
		fs["aruco_ac_nmarkers"] >> aux;
		resize(aux);
		fs["aruco_ac_mInfoType"] >> mInfoType;
		cv::FileNode markers = fs["aruco_ac_markers"];
		int i = 0;
		for (FileNodeIterator it = markers.begin(); it != markers.end(); ++it, i++) {
			at(i).id = (*it)["id"];
			vector< float > coordinates3d;
			(*it)["translation"] >> coordinates3d;
			if (coordinates3d.size() != 3)
				throw cv::Exception(81818, "AreaConfiguration::readFromFile", "invalid file type 3", __FILE__, __LINE__);
			at(i).x = coordinates3d[0];
			at(i).y = coordinates3d[1];
			at(i).z = coordinates3d[2];
		}
		// custom data
		fs["aruco_ac_areaId"] >> areaId;
		fs["aruco_ac_cameraId"] >> cameraId;
	}

	/**
	 */
	int AreaConfiguration::getIndexOfMarkerId(int id) const {

		for (size_t i = 0; i < size(); i++)
			if (at(i).id == id)
				return i;
		return -1;
	}

	/**
	 */
	const MyMarkerInfo &AreaConfiguration::getMarkerInfo(int id) const throw(cv::Exception) {
		for (size_t i = 0; i < size(); i++)
			if (at(i).id == id)
				return at(i);
		throw cv::Exception(111, "AreaConfiguration::getMarkerInfo", "Marker with the id given is not found", __FILE__, __LINE__);
	}

	/**
	*/
	std::vector<cv::Point2f> AreaConfiguration::getAreaXYcountours() {

		vector<Point2f> contours;
		for (size_t i = 0; i < size(); i++){
			cv::Point2f tempPoint;
			tempPoint.x = at(i).x;
			tempPoint.y = at(i).z;
			contours.push_back(tempPoint);
		}

		return contours;
	}

	///****
	// *
	// */
	//void Area::OgreGetPoseParameters(double position[3], double orientation[4]) throw(cv::Exception) {
	//    // check if paremeters are valid
	//    bool invalid = false;
	//    for (int i = 0; i < 3 && !invalid; i++) {
	//        if (Tvec.at< float >(i, 0) != -999999)
	//            invalid |= false;
	//        if (Rvec.at< float >(i, 0) != -999999)
	//            invalid |= false;
	//    }
	//    if (invalid)
	//        throw cv::Exception(9003, "extrinsic parameters are not set", "Marker::getModelViewMatrix", __FILE__, __LINE__);
	//
	//    // calculate position vector
	//    position[0] = -Tvec.ptr< float >(0)[0];
	//    position[1] = -Tvec.ptr< float >(0)[1];
	//    position[2] = +Tvec.ptr< float >(0)[2];
	//
	//    // now calculare orientation quaternion
	//    cv::Mat Rot(3, 3, CV_32FC1);
	//    cv::Rodrigues(Rvec, Rot);
	//
	//    // calculate axes for quaternion
	//    double stAxes[3][3];
	//    // x axis
	//    stAxes[0][0] = -Rot.at< float >(0, 0);
	//    stAxes[0][1] = -Rot.at< float >(1, 0);
	//    stAxes[0][2] = +Rot.at< float >(2, 0);
	//    // y axis
	//    stAxes[1][0] = -Rot.at< float >(0, 1);
	//    stAxes[1][1] = -Rot.at< float >(1, 1);
	//    stAxes[1][2] = +Rot.at< float >(2, 1);
	//    // for z axis, we use cross product
	//    stAxes[2][0] = stAxes[0][1] * stAxes[1][2] - stAxes[0][2] * stAxes[1][1];
	//    stAxes[2][1] = -stAxes[0][0] * stAxes[1][2] + stAxes[0][2] * stAxes[1][0];
	//    stAxes[2][2] = stAxes[0][0] * stAxes[1][1] - stAxes[0][1] * stAxes[1][0];
	//
	//    // transposed matrix
	//    double axes[3][3];
	//    axes[0][0] = stAxes[0][0];
	//    axes[1][0] = stAxes[0][1];
	//    axes[2][0] = stAxes[0][2];
	//
	//    axes[0][1] = stAxes[1][0];
	//    axes[1][1] = stAxes[1][1];
	//    axes[2][1] = stAxes[1][2];
	//
	//    axes[0][2] = stAxes[2][0];
	//    axes[1][2] = stAxes[2][1];
	//    axes[2][2] = stAxes[2][2];
	//
	//    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
	//    // article "Quaternion Calculus and Fast Animation".
	//    double fTrace = axes[0][0] + axes[1][1] + axes[2][2];
	//    double fRoot;
	//
	//    if (fTrace > 0.0) {
	//        // |w| > 1/2, may as well choose w > 1/2
	//        fRoot = sqrt(fTrace + 1.0); // 2w
	//        orientation[0] = 0.5 * fRoot;
	//        fRoot = 0.5 / fRoot; // 1/(4w)
	//        orientation[1] = (axes[2][1] - axes[1][2]) * fRoot;
	//        orientation[2] = (axes[0][2] - axes[2][0]) * fRoot;
	//        orientation[3] = (axes[1][0] - axes[0][1]) * fRoot;
	//    } else {
	//        // |w| <= 1/2
	//        static unsigned int s_iNext[3] = {1, 2, 0};
	//        unsigned int i = 0;
	//        if (axes[1][1] > axes[0][0])
	//            i = 1;
	//        if (axes[2][2] > axes[i][i])
	//            i = 2;
	//        unsigned int j = s_iNext[i];
	//        unsigned int k = s_iNext[j];
	//
	//        fRoot = sqrt(axes[i][i] - axes[j][j] - axes[k][k] + 1.0);
	//        double *apkQuat[3] = {&orientation[1], &orientation[2], &orientation[3]};
	//        *apkQuat[i] = 0.5 * fRoot;
	//        fRoot = 0.5 / fRoot;
	//        orientation[0] = (axes[k][j] - axes[j][k]) * fRoot;
	//        *apkQuat[j] = (axes[j][i] + axes[i][j]) * fRoot;
	//        *apkQuat[k] = (axes[k][i] + axes[i][k]) * fRoot;
	//    }
	//}
	//
	///**
	// */
	//void Area::draw(cv::Mat &im, cv::Scalar color, int lineWidth, bool writeId) {
	//    for (size_t i = 0; i < size(); i++) {
	//        at(i).draw(im, color, lineWidth, writeId);
	//    }
	//}
	//

	/**
	*/
	void AreaConfiguration::getIdList(std::vector< int > &ids, bool append) const {
		if (!append)
			ids.clear();
		for (size_t i = 0; i < size(); i++)
			ids.push_back(at(i).id);
	}
};
