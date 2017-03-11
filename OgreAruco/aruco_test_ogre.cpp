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

#include <iostream>
 
#include "Ogre.h"
#include <OIS/OIS.h>
 
#include "aruco/aruco.h"  
#include "aruco/highlyreliablemarkers.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
 
 
#define MAX_MARKERS 30 // maximun number of markers we assume they will be detected simultaneously
 
/// Ogre general variables
Ogre::Root* root;
OIS::InputManager* im;
OIS::Keyboard* keyboard;

/// Ogre background variables
Ogre::PixelBox mPixelBox;
Ogre::TexturePtr mTexture;
 
/// Ogre scene variables
Ogre::SceneNode* ogreNode[MAX_MARKERS];
Ogre::AnimationState *baseAnim[MAX_MARKERS], *topAnim[MAX_MARKERS];
 
/// ArUco variables
cv::VideoCapture TheVideoCapturer;
cv::Mat TheInputImage, TheInputImageUnd;
aruco::CameraParameters CameraParams, CameraParamsUnd;
float TheMarkerSize=1;
aruco::MarkerDetector TheMarkerDetector;
std::vector<aruco::Marker> TheMarkers;
 
// ArUco Dictionaries
aruco::Dictionary Dictionary;
  
int initOgreAR(aruco::CameraParameters camParams, unsigned char* buffer, std::string resourcePath="");
bool readParameters(int argc, char** argv);

void usage()
{
    cout<<" This program test Ogre version of ArUco (single marker version) \n\n";
    cout<<" Usage <video.avi>|live <camera.yml> <markersize>"<<endl;
    cout<<" <video.avi>|live: specifies a input video file. Use 'live' to capture from camera"<<endl;
    cout<<" <camera.yml>: camera calibration file"<<endl;
    cout<<" <markersize>: in meters "<<endl;
}

 /**
  * 
  */
int main(int argc, char** argv)
{
  
      /// READ PARAMETERS
      if(!readParameters(argc, argv))
	return false;   

      /// CREATE UNDISTORTED CAMERA PARAMS
      CameraParamsUnd=CameraParams;
      CameraParamsUnd.Distorsion=cv::Mat::zeros(4,1,CV_32F);      
      
      /// CAPTURE FIRST FRAME
      TheVideoCapturer.grab();
      TheVideoCapturer.retrieve ( TheInputImage );
      cv::undistort( TheInputImage, TheInputImageUnd, CameraParams.CameraMatrix, CameraParams.Distorsion);      
      
	  // load dictionary
	  std::string dictionaryFile = "reduced-dict-plastic.yml";
	  if (Dictionary.fromFile(dictionaryFile) == false) {
		  cerr << "Could not open dictionary" << endl;
		  Sleep(2000);
		  return -1;
	  }
	  else{
		  cout << "Dictionary loaded" << endl;
	  }

	  // load the dictionary to marker detector
	  aruco::HighlyReliableMarkers::loadDictionary(Dictionary);
	  // set detector params
	  TheMarkerDetector.setMakerDetectorFunction(aruco::HighlyReliableMarkers::detect);
	  TheMarkerDetector.setWarpSize((Dictionary[0].n() + 2) * 8);

      /// INIT OGRE
      //initOgreAR(CameraParamsUnd, TheInputImageUnd.ptr<uchar>(0));
	  initOgreAR(CameraParamsUnd, TheInputImageUnd.ptr<uchar>(0));

      while (TheVideoCapturer.grab())
      {
	      /// READ AND UNDISTORT IMAGE
	      TheVideoCapturer.retrieve ( TheInputImage );

	      cv::undistort(TheInputImage,TheInputImageUnd,CameraParams.CameraMatrix,CameraParams.Distorsion);
	      
	      /// DETECT MARKERS
	      TheMarkerDetector.detect(TheInputImageUnd, TheMarkers, CameraParamsUnd, TheMarkerSize);
	      
	      /// UPDATE BACKGROUND IMAGE
	      mTexture->getBuffer()->blitFromMemory(mPixelBox);	      
	      
	      /// UPDATE SCENE
	      int i;
	      double position[3], orientation[4];
		  //exit(11);
	      // show nodes for detected markers
		  for (i = 0; i < TheMarkers.size() && i < MAX_MARKERS; i++) {
			  TheMarkers[i].OgreGetPoseParameters(position, orientation);
			  ogreNode[i]->setPosition(position[0], position[1], position[2]);
			  ogreNode[i]->setOrientation(orientation[0], orientation[1], orientation[2], orientation[3]);
			  ogreNode[i]->setVisible(true);
		  }

		  // hide rest of nodes
	      for( ; i<MAX_MARKERS; i++) ogreNode[i]->setVisible(false);

	      // Update animation
	      double deltaTime = 1.2*root->getTimer()->getMilliseconds()/1000.;
	      for(int i=0; i<MAX_MARKERS; i++) {
	        baseAnim[i]->addTime(deltaTime);
	        topAnim[i]->addTime(deltaTime);
	      }
	      root->getTimer()->reset();
      
	      /// RENDER FRAME
	      if(root->renderOneFrame() == false) break;
	      Ogre::WindowEventUtilities::messagePump();

	      /// KEYBOARD INPUT
	      keyboard->capture();
	      if (keyboard->isKeyDown(OIS::KC_ESCAPE)) break;	      
	      
	      
      }

      im->destroyInputObject(keyboard);
      im->destroyInputSystem(im);
      im = 0;

      delete root;
      return 0;
}
 

 /**
  * 
  */
bool readParameters(int argc, char** argv)
{

	if (argc < 3) {
		usage();
		return false;
	}

	// read input video  
	//if (argv[1]=="live") TheVideoCapturer.open(0);
	//else TheVideoCapturer.open(argv[1]);

	// read intrinsic file
	try {
		CameraParams.readFromXMLFile(argv[2]);
	}
	catch (std::exception &ex) {
		cout << ex.what() << endl;
		return false;
	}

	TheVideoCapturer.open(0);
	TheVideoCapturer.set(CV_CAP_PROP_FRAME_WIDTH, CameraParams.CamSize.width);
	TheVideoCapturer.set(CV_CAP_PROP_FRAME_HEIGHT, CameraParams.CamSize.height);


	if (!TheVideoCapturer.isOpened())
	{
		cerr << "Could not open video" << endl;
		return false;
	}


	if (argc > 3) TheMarkerSize = atof(argv[3]);
	else TheMarkerSize = 1.;
}

 
 
  /**
  * 
  */
int initOgreAR(aruco::CameraParameters camParams, unsigned char* buffer, std::string resourcePath)
{
  
	/// INIT OGRE FUNCTIONS
  	//root = new Ogre::Root(resourcePath + "plugins.cfg", resourcePath + "ogre.cfg");
	root = new Ogre::Root();
  	if (!root->showConfigDialog()) return -1;
	Ogre::SceneManager* smgr = root->createSceneManager(Ogre::ST_GENERIC);
	
	
	/// CREATE WINDOW, CAMERA AND VIEWPORT
        Ogre::RenderWindow* window = root->initialise(true);
	Ogre::Camera *camera;
	Ogre::SceneNode* cameraNode;
	camera = smgr->createCamera("camera");
	camera->setNearClipDistance(0.01f);
	camera->setFarClipDistance(10.0f);
	camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
	camera->setPosition(0, 0, 0);
	camera->lookAt(0, 0, 1);
	double pMatrix[16];
	camParams.OgreGetProjectionMatrix(camParams.CamSize,camParams.CamSize, pMatrix, 0.05,10, false);
	Ogre::Matrix4 PM(pMatrix[0], pMatrix[1], pMatrix[2] , pMatrix[3],
			pMatrix[4], pMatrix[5], pMatrix[6] , pMatrix[7],
			pMatrix[8], pMatrix[9], pMatrix[10], pMatrix[11],
			pMatrix[12], pMatrix[13], pMatrix[14], pMatrix[15]);	
	camera->setCustomProjectionMatrix(true, PM);
	camera->setCustomViewMatrix(true, Ogre::Matrix4::IDENTITY);		
	window->addViewport(camera);
	cameraNode = smgr->getRootSceneNode()->createChildSceneNode("cameraNode");
	cameraNode->attachObject(camera);
		
	
	/// CREATE BACKGROUND FROM CAMERA IMAGE
	int width = camParams.CamSize.width;
	int height = camParams.CamSize.height;
	// create background camera image
	mPixelBox = Ogre::PixelBox(width, height, 1, Ogre::PF_R8G8B8, buffer);
	// Create Texture
	mTexture = Ogre::TextureManager::getSingleton().createManual("CameraTexture",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		      Ogre::TEX_TYPE_2D,width,height,0,Ogre::PF_R8G8B8,Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);	

	//Create Camera Material
	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("CameraMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Technique *technique = material->createTechnique();
	technique->createPass();
	material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	material->getTechnique(0)->getPass(0)->createTextureUnitState("CameraTexture");	
 
	Ogre::Rectangle2D* rect = new Ogre::Rectangle2D(true);
	rect->setCorners(-1.0, 1.0, 1.0, -1.0);
	rect->setMaterial("CameraMaterial");
 
	// Render the background before everything else
	rect->setRenderQueueGroup(Ogre::RENDER_QUEUE_BACKGROUND);
 
	// Hacky, but we need to set the bounding box to something big, use infinite AAB to always stay visible
	Ogre::AxisAlignedBox aabInf;
	aabInf.setInfinite();
	rect->setBoundingBox(aabInf);
 
	// Attach background to the scene
	Ogre::SceneNode* node = smgr->getRootSceneNode()->createChildSceneNode("Background");
	node->attachObject(rect);		
	
	
	/// CREATE SIMPLE OGRE SCENE
	// add sinbad.mesh
	Ogre::ResourceGroupManager::getSingleton().addResourceLocation("Sinbad.zip", "Zip", "Popular");
 	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();	
	for(int i=0; i<MAX_MARKERS; i++) {
	  Ogre::String entityName = "Marker_" + Ogre::StringConverter::toString(i);
	  Ogre::Entity* ogreEntity = smgr->createEntity(entityName, "Sinbad.mesh");
	  Ogre::Real offset = ogreEntity->getBoundingBox().getHalfSize().y;	  
	  ogreNode[i] = smgr->getRootSceneNode()->createChildSceneNode();
	  // add entity to a child node to correct position (this way, entity axis is on feet of sinbad)
	  Ogre::SceneNode *ogreNodeChild = ogreNode[i]->createChildSceneNode();
	  ogreNodeChild->attachObject(ogreEntity);
	  // Sinbad is placed along Y axis, we need to rotate to put it along Z axis so it stands up over the marker
	  // first rotate along X axis, then add offset in Z dir so it is over the marker and not in the middle of it 
	  ogreNodeChild->rotate(Ogre::Vector3(1,0,0), Ogre::Radian(Ogre::Degree(90)));	
	  ogreNodeChild->translate(0,0,offset,Ogre::Node::TS_PARENT);
	  // mesh is too big, rescale!
	  const float scale = 0.006675f;
	  ogreNode[i]->setScale(scale, scale, scale);	  
	  
	    // Init animation
	  ogreEntity->getSkeleton()->setBlendMode(Ogre::ANIMBLEND_CUMULATIVE);
	  baseAnim[i] = ogreEntity->getAnimationState("RunBase");
	  topAnim[i] = ogreEntity->getAnimationState("RunTop");
	  baseAnim[i]->setLoop(true);
	  topAnim[i]->setLoop(true);
	  baseAnim[i]->setEnabled(true);
	  topAnim[i]->setEnabled(true);	  
	}	
	
	
	// setup some basic lighting for our scene
	smgr->setAmbientLight(Ogre::ColourValue(1, 1, 1));

 	/// KEYBOARD INPUT READING
 	size_t windowHnd = 0;
 	window->getCustomAttribute("WINDOW", &windowHnd);
 	im = OIS::InputManager::createInputSystem(windowHnd);
 	keyboard = static_cast<OIS::Keyboard*>(im->createInputObject(OIS::OISKeyboard, true));
	
	return 1;
}








