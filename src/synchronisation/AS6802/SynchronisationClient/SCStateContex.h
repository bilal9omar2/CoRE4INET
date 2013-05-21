/*
 * StateContex.h
 *
 *  Created on: Feb 27, 2012
 *      Author: lazar
 */

#ifndef SCSTATECONTEX_H_
#define SCSTATECONTEX_H_

#include "SCState.h"

namespace TTEthernetModel {

class StateContex{

	public:
		SCState *pState;

	public:

	StateContex(){}
	virtual ~StateContex(){}

	virtual  void setState(SCState *firstState){
		pState=firstState;
	}

	virtual void parseMessage(cMessage *message){

		pState->handleMessage(message);

	}
};

}

#endif /* SCSTATECONTEX_H_ */