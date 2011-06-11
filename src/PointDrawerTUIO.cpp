/*
 Point Viewer with TUIO output

 Copyright (c) 2011 ZD Studios Inc.

 Author: Matt Cook <matt@lookitscook.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PointDrawerTUIO.h"
using namespace TUIO;


void PointDrawerTUIO::initKalman(void) {
    memset(tuioPointSmoothed, 0, 32*3*sizeof(ofxCvKalman*));
}

bool PointDrawerTUIO::updateKalman(int id, XnPoint3D &p) {
	if (id>=32) return false;
	if(tuioPointSmoothed[id*3] == NULL) {
	    printf("Creado");
		tuioPointSmoothed[id*3] = new ofxCvKalman(p.X);
		tuioPointSmoothed[id*3+1] = new ofxCvKalman(p.Y);
		//tuioPointSmoothed[id*3+2] = new ofxCvKalman(p.z);
	} else {
            p.X=tuioPointSmoothed[id*3]->correct(p.X);
            p.Y=tuioPointSmoothed[id*3+1]->correct(p.Y);
		//p.z=tuioPointSmoothed[id*3+2]->correct(p.z);
		printf("Corregido");
	}

	return true;
}
void PointDrawerTUIO::clearKalman(int id) {
	if (id>=32) return;
	if(tuioPointSmoothed[id*3]) {
		delete tuioPointSmoothed[id*3];
		tuioPointSmoothed[id*3] = NULL;
		delete tuioPointSmoothed[id*3+1];
		tuioPointSmoothed[id*3+1] = NULL;
		//delete tuioPointSmoothed[id*3+2];
		//tuioPointSmoothed[id*3+2] = NULL;
	}
}

// Constructor. Receives the number of previous positions to store per hand,
// and a source for depth map
PointDrawerTUIO::PointDrawerTUIO(XnUInt32 nHistory, xn::DepthGenerator depthGenerator, const char *host, int port) :
	XnVPointDrawer(nHistory, depthGenerator)
{
	width = 640;
	height = 480;
	cursor = NULL;
	if ((strcmp(host,"default")==0) && (port==0)) tuioServer = new TuioServer();
	else tuioServer = new TuioServer(host, port);
	currentTime = TuioTime::getSessionTime();


	initKalman();

}

// Destructor. Clear all data structures
PointDrawerTUIO::~PointDrawerTUIO()
{
	delete tuioServer;
}

// Handle a new Message
void PointDrawerTUIO::Update(XnVMessage* pMessage)
{
	currentTime = TuioTime::getSessionTime();
	tuioServer->initFrame(currentTime);
	XnVPointDrawer::Update(pMessage);
	//tuioServer->stopUntouchedMovingCursors();
	tuioServer->commitFrame();
}

// Handle detection of new hand, currently only supports single hand
void PointDrawerTUIO::OnPointCreate(const XnVHandPointContext* cxt)
{
	if(cursor==NULL){
		XnPoint3D pt(cxt->ptPosition);
		m_DepthGenerator.ConvertRealWorldToProjective(1, &pt, &pt);
		pt.X/=width;
		pt.Y/=height;
		cursor = tuioServer->addTuioCursor(0.5+(pt.X-0.5)*2,0.5+(pt.Y-0.5)*2);
	}
	XnVPointDrawer::OnPointCreate(cxt);
}

// Handle new position of an existing hand
void PointDrawerTUIO::OnPointUpdate(const XnVHandPointContext* cxt)
{
	if (cursor!=NULL && cursor->getTuioTime()!=currentTime){
		XnPoint3D pt(cxt->ptPosition);
		m_DepthGenerator.ConvertRealWorldToProjective(1, &pt, &pt);
		pt.X/=width;
		pt.Y/=height;
		//cursor->update(cursor,0.5+(pt.X-0.5)*2,0.5+(y-0.5)*2);
        updateKalman(cursor->getCursorID(), pt);
        //tuioServer->updateCursor(cursor);
		tuioServer->updateTuioCursor(cursor,0.5+(pt.X-0.5)*2,0.5+(pt.Y-0.5)*2);
	}
	XnVPointDrawer::OnPointUpdate(cxt);


}

// Handle destruction of an existing hand
void PointDrawerTUIO::OnPointDestroy(XnUInt32 nID)
{
	if (cursor!=NULL){
        clearKalman(cursor->getCursorID());
		tuioServer->removeTuioCursor(cursor);
		cursor = NULL;
	}
	XnVPointDrawer::OnPointDestroy(nID);


}
