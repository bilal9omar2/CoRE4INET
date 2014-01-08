#ifndef __CoRE4INET_AVBCLASSASHAPER_H
#define __CoRE4INET_AVBCLASSASHAPER_H

#include <AVBBuffer.h>

namespace CoRE4INET {

/**
 * @brief A Shaper for AVB-Messages.
 *
 * The TTShaper only allows lower priorities to transmit when the credit is <= 0.
 *
 * @author Philipp Meyer
 */
template <class TC>
class AVBClassAShaper : public TC
{
    public:
        /**
         * @brief Constructor
         */
        AVBClassAShaper();
        /**
         * @brief Destructor
         */
        ~AVBClassAShaper();


    private:
        /**
         * @brief Queue for AVB-Messages. Will be only filled when there AVB bursts possible.
         */
        cQueue avbQueue;

        AVBBuffer *avbBuffer;

    protected:
        /**
         * @brief Signal that is emitted when the queue length of time-triggered messages changes.
         */
        static simsignal_t avbQueueLengthSignal;

    protected:
        /**
         * @brief Initialization of the module
         *
         * @param stage The stages. Module initializes when stage==0
         */
        virtual void initialize(int stage);


        /**
         * @brief Forwards the messages from the different buffers and LLC
         * according to the specification for AVB ClassA Messages.
         *
         * AVB Class A messages are send immediately, lower priority frames are delayed
         * if the credit is positive.
         * If the mac layer is idle, messages are picked from the queues according
         * to the priorities, using the template class.
         *
         * @param msg the incoming message
         */
        virtual void handleMessage(cMessage *msg);

        /**
         * @brief Queues messages in the correct queue
         *
         * AVB messages are queued in this module, other messages are forwarded to the
         * template classes enqueueMessage method
         *
         * @param msg the incoming message
         */
        virtual void enqueueMessage(cMessage *msg);

        /**
         * @brief this method is invoked when the underlying mac is idle.
         *
         * When this method is invoked the module sends a new message when there is
         * one. Else it saves the state and sends the message immediately when it is
         * received.
         *
         * @param msg the message to be queued
         */
        virtual void requestPacket();

        /**
         * @brief Returns true when there are no pending messages.
         *
         * @return true if all queues are empty.
         */
        virtual bool isEmpty();

        /**
         * @brief Clears all queued packets and stored requests.
         */
        virtual void clear();

        /**
         * @brief Returns a frame directly from the queues, bypassing the primary,
         * send-on-request mechanism. Returns NULL if the queue is empty.
         *
         * @return the message with the highest priority from any queue. NULL if the
         * queues are empty or cannot send due to the traffic policies.
         */
        virtual cMessage *pop();

        /**
         * @brief Returns a pointer to a frame directly from the queues.
         *
         * front must return a pointer to the same message pop() would return.
         *
         * @return pointer to the message with the highest priority from any queue. NULL if the
         * queues are empty
         */
        virtual cMessage *front();
};

template <class TC>
simsignal_t AVBClassAShaper<TC>::avbQueueLengthSignal = SIMSIGNAL_NULL;

template <class TC>
AVBClassAShaper<TC>::AVBClassAShaper(){
    avbQueue.setName("AVB Messages");
}

template <class TC>
AVBClassAShaper<TC>::~AVBClassAShaper(){
    avbQueue.clear();
}

template <class TC>
void AVBClassAShaper<TC>::initialize(int stage)
{
    TC::initialize(stage);
    if(stage==0){
        avbQueueLengthSignal = cComponent::registerSignal("avbQueueLength");

        int portIndex = cModule::getParentModule()->getIndex();
        avbBuffer = dynamic_cast<AVBBuffer*> (cModule::getParentModule()->getParentModule()->getSubmodule("avbBuffer", portIndex));
    }
}

template <class TC>
void AVBClassAShaper<TC>::handleMessage(cMessage *msg)
{
    if(msg->arrivedOn("AVBin"))
    {
        if(TC::getNumPendingRequests() && avbBuffer->getCredit() >= 0)
        {
            TC::framesRequested--;
            cSimpleModule::send(msg, cModule::gateBaseId("out"));
            SimTime duration = TC::outChannel->calculateDuration(msg);
            //duration += (INTERFRAME_GAP_BITS + ((PREAMBLE_BYTES + SFD_BYTES) * 8)) / TC::outChannel->getNominalDatarate();
            avbBuffer->sendSlope(duration);
        }
        else
        {
            enqueueMessage(msg);
        }
    }
    else{
        if(avbBuffer->getCredit() <= 0){
            TC::handleMessage(msg);
        }else{
            TC::enqueueMessage(msg);
        }
    }
}

template <class TC>
void AVBClassAShaper<TC>::enqueueMessage(cMessage *msg){
    if(msg->arrivedOn("AVBin")){
        avbQueue.insert(msg);
        cComponent::emit(avbQueueLengthSignal, (unsigned int)avbQueue.length());
        TC::notifyListeners();
    }
    else{
        TC::enqueueMessage(msg);
    }
}

template <class TC>
void AVBClassAShaper<TC>::requestPacket()
{
    Enter_Method("requestPacket()");
    //Feed the MAC layer with the next frame
    TC::framesRequested++;

    if (cMessage *msg = pop())
    {
        TC::framesRequested--;
        cSimpleModule::send(msg, cModule::gateBaseId("out"));
    }
}

template <class TC>
cMessage* AVBClassAShaper<TC>::pop()
{
    Enter_Method("pop()");
    //AVBFrames
    if(avbBuffer->initialized()) avbBuffer->refresh();
    if (!avbQueue.isEmpty() && avbBuffer->getCredit() >= 0)
    {
        cMessage *msg = (cMessage*) avbQueue.pop();
        cComponent::emit(avbQueueLengthSignal, (unsigned int)avbQueue.length());
        SimTime duration = TC::outChannel->calculateDuration(msg);
        avbBuffer->sendSlope(duration);
        return msg;
    }
    else if(avbBuffer->getCredit() <= 0){
        return TC::pop();
    }
    return NULL;
}

template <class TC>
cMessage* AVBClassAShaper<TC>::front()
{
    Enter_Method("front()");
    //AVBFrames
    if (!avbQueue.isEmpty())
    {
        cMessage *msg = (cMessage*) avbQueue.front();
        return msg;
    }
    return TC::front();
}

template <class TC>
bool AVBClassAShaper<TC>::isEmpty()
{
        return avbQueue.isEmpty() && TC::isEmpty();
}

template <class TC>
void AVBClassAShaper<TC>::clear()
{
        TC::clear();
        avbQueue.clear();
}

}

#endif

