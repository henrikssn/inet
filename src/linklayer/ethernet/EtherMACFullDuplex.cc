//
// Copyright (C) 2006 Levente Meszaros
// Copyright (C) 2011 Zoltan Bojthe
// Copyright (C) 2014 RWTH Aachen University, Chair of Communication and Distributed Systems
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "EtherMACFullDuplex.h"

#include "EtherFrame.h"
#include "IPassiveQueue.h"
#include "NotificationBoard.h"
#include "NotifierConsts.h"
#include "InterfaceEntry.h"
#include "MACBase.h"

// TODO: refactor using a statemachine that is present in a single function
// TODO: this helps understanding what interactions are there and how they affect the state

Define_Module(EtherMACFullDuplex);
Register_ParallelInitialization_Module(EtherMACFullDuplex);

EtherMACFullDuplex::EtherMACFullDuplex()
{
}

cState& EtherMACFullDuplex::initialize(int stage, cState& state)
{
    currentState = state.getState()==NULL ? new EtherMACState() : dynamic_cast<EtherMACState*>(state.getState());
        if(currentState==NULL) // we are the first who get the status
            throw cRuntimeError("the state '%s' can't be casted to EtherMACState. ", state.getState()->getClassName());

        MACBase::initialize(stage);

        if(stage == 0) //stage 0: publish gate ids
        {
            //initialize
            EtherMACBase::initialize(stage);



            cGate *physOutEndGate = physOutGate->getPathEndGate();
            cModule *physOutEndOwnerMod = physOutEndGate->getOwnerModule();

            cGate *physInStartGate = physInGate->getPathStartGate();
            cModule *physInStartOwnerMod = physInStartGate->getOwnerModule();

            if(physOutEndOwnerMod->isPlaceholder() && physInStartOwnerMod->isPlaceholder()) //physOut->getPathEndGate is a proxygate -> we add our question to the map
            {

                currentState->addConnectionState(new ConnectionState(physOutEndOwnerMod->getId(),physOutEndGate->getId()));
                currentState->addConnectionState(new ConnectionState(physInStartOwnerMod->getId(),physInStartGate->getId()));

            }

        }
        else if (stage == 1) //stage 1: get gate ip
        {

            cArray* connectionStates = currentState->getConnectionStates();
            for (cArray::Iterator i(*connectionStates,true); !i.end(); i++)
            {
                ConnectionState *connectionState = dynamic_cast<ConnectionState*>(i());

                ASSERT(connectionState);

                //check physOut
                const cGate *g;
                for (g=physOutGate; g->getNextGate()!=NULL; g=g->getNextGate())
                {
                    if(connectionState->getModuleId() == g->getOwnerModule()->getId() && connectionState->getGateId() == g->getId())
                    {
                        connectionState->setConnected(true);
                    }
                }

                //check physIn
                for (g=physInGate; g->getPreviousGate()!=NULL; g=g->getPreviousGate())
                {
                    if(connectionState->getModuleId() == g->getOwnerModule()->getId() && connectionState->getGateId() == g->getId())
                    {
                        connectionState->setConnected(true);
                    }
                }

            }
        }
        else if (stage == 2) //lookup if gate connected on other LP
        {
            cGate *physOutEndGate = physOutGate->getPathEndGate();
            cModule *physOutEndOwnerMod = physOutEndGate->getOwnerModule();

            cGate *physInStartGate = physInGate->getPathStartGate();
            cModule *physInStartOwnerMod = physInStartGate->getOwnerModule();

            if(!physOutEndOwnerMod->isPlaceholder() && !physInStartOwnerMod->isPlaceholder()) //we can set connected directly (local modules)
            {
                connected = physOutEndGate->isConnected() && physInStartGate->isConnected();
            }
            else if(physOutEndOwnerMod->isPlaceholder() && physInStartOwnerMod->isPlaceholder())
            {

                bool physInConnected = false;
                bool physOutConnected = false;

                cArray* connectionStates = currentState->getConnectionStates();
                for (cArray::Iterator i(*connectionStates,true); !i.end(); i++)
                {
                    ConnectionState *connectionState = dynamic_cast<ConnectionState*>(i());
                    ASSERT(connectionState);

                    if(physOutEndOwnerMod->getId() == connectionState->getModuleId() &&
                           physOutEndGate->getId() == connectionState->getGateId())
                    {
                        physOutConnected = true;
                    }

                    if(physInStartOwnerMod->getId() == connectionState->getModuleId() &&
                           physInStartGate->getId() == connectionState->getGateId())
                    {
                        physInConnected = true;
                    }

                    if(physInConnected && physOutConnected)
                    {
                        connected = true;
                        break;
                    }

                }


            }

            if (!connected)
                EV << "MAC not connected to a network.\n";

            readChannelParameters(true);
            if (!par("duplexMode").boolValue())
                    throw cRuntimeError("Half duplex operation is not supported by EtherMACFullDuplex, use the EtherMAC module for that! (Please enable csmacdSupport on EthernetInterface)");

            beginSendFrames();
        }




    state.setState(currentState);
    return state;

}

void EtherMACFullDuplex::initializeMACAddress()
{
    const char *addrstr = par("address");

    if (!strcmp(addrstr, "auto"))
    {
        ASSERT(currentState);

        // assign automatic address
        address = MACAddress::generateAutoAddress(currentState->getLastAddress());

        // change module parameter from "auto" to concrete address
        par("address").setStringValue(address.str().c_str());
    }
    else
    {
        address.setAddress(addrstr);
    }
}

void EtherMACFullDuplex::initializeStatistics()
{
    EtherMACBase::initializeStatistics();

    // initialize statistics
    totalSuccessfulRxTime = 0.0;
}

void EtherMACFullDuplex::initializeFlags()
{
    EtherMACBase::initializeFlags();

    duplexMode = true;
    physInGate->setDeliverOnReceptionStart(false);
}

void EtherMACFullDuplex::handleMessage(cMessage *msg)
{
    if (!isOperational)
    {
        handleMessageWhenDown(msg);
        return;
    }

    if (channelsDiffer)
        readChannelParameters(true);

    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else if (msg->getArrivalGate() == upperLayerInGate)
        processFrameFromUpperLayer(check_and_cast<EtherFrame *>(msg));
    else if (msg->getArrivalGate() == physInGate)
        processMsgFromNetwork(check_and_cast<EtherTraffic *>(msg));
    else
        throw cRuntimeError("Message received from unknown gate!");

    if (ev.isGUI())
        updateDisplayString();
}

void EtherMACFullDuplex::handleSelfMessage(cMessage *msg)
{
    EV << "Self-message " << msg << " received\n";

    if (msg == endTxMsg)
        handleEndTxPeriod();
    else if (msg == endIFGMsg)
        handleEndIFGPeriod();
    else if (msg == endPauseMsg)
        handleEndPausePeriod();
    else
        throw cRuntimeError("Unknown self message received!");
}

void EtherMACFullDuplex::startFrameTransmission()
{
    ASSERT(curTxFrame);
    EV << "Transmitting a copy of frame " << curTxFrame << endl;

    EtherFrame *frame = curTxFrame->dup();  // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()

    if (frame->getSrc().isUnspecified())
        frame->setSrc(address);

    if (frame->getByteLength() < curEtherDescr->frameMinBytes)
        frame->setByteLength(curEtherDescr->frameMinBytes);

    // add preamble and SFD (Starting Frame Delimiter), then send out
    frame->addByteLength(PREAMBLE_BYTES+SFD_BYTES);

    // send
    EV << "Starting transmission of " << frame << endl;
    send(frame, physOutGate);

    scheduleAt(transmissionChannel->getTransmissionFinishTime(), endTxMsg);
    transmitState = TRANSMITTING_STATE;
}

void EtherMACFullDuplex::processFrameFromUpperLayer(EtherFrame *frame)
{
    if (frame->getByteLength() < MIN_ETHERNET_FRAME_BYTES)
        frame->setByteLength(MIN_ETHERNET_FRAME_BYTES);  // "padding"

    frame->setFrameByteLength(frame->getByteLength());

    EV << "Received frame from upper layer: " << frame << endl;

    emit(packetReceivedFromUpperSignal, frame);

    if (frame->getDest().equals(address))
    {
        error("logic error: frame %s from higher layer has local MAC address as dest (%s)",
                frame->getFullName(), frame->getDest().str().c_str());
    }

    if (frame->getByteLength() > MAX_ETHERNET_FRAME_BYTES)
    {
        error("packet from higher layer (%d bytes) exceeds maximum Ethernet frame size (%d)",
                (int)(frame->getByteLength()), MAX_ETHERNET_FRAME_BYTES);
    }

    if (!connected || disabled)
    {
        EV << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping packet " << frame << endl;
        emit(dropPkFromHLIfaceDownSignal, frame);
        numDroppedPkFromHLIfaceDown++;
        delete frame;

        requestNextFrameFromExtQueue();
        return;
    }

    // fill in src address if not set
    if (frame->getSrc().isUnspecified())
        frame->setSrc(address);

    bool isPauseFrame = (dynamic_cast<EtherPauseFrame*>(frame) != NULL);

    if (!isPauseFrame)
    {
        numFramesFromHL++;
        emit(rxPkFromHLSignal, frame);
    }

    if (txQueue.extQueue)
    {
        ASSERT(curTxFrame == NULL);
        curTxFrame = frame;
    }
    else
    {
        if (txQueue.innerQueue->isFull())
            error("txQueue length exceeds %d -- this is probably due to "
                  "a bogus app model generating excessive traffic "
                  "(or if this is normal, increase txQueueLimit!)",
                  txQueue.innerQueue->getQueueLimit());
        // store frame and possibly begin transmitting
        EV << "Frame " << frame << " arrived from higher layers, enqueueing\n";
        txQueue.innerQueue->insertFrame(frame);

        if (!curTxFrame && !txQueue.innerQueue->empty())
            curTxFrame = (EtherFrame*)txQueue.innerQueue->pop();
    }

    if (transmitState == TX_IDLE_STATE)
        startFrameTransmission();
}

void EtherMACFullDuplex::processMsgFromNetwork(EtherTraffic *msg)
{
    EV << "Received frame from network: " << msg << endl;

    if (!connected || disabled)
    {
        EV << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping msg " << msg << endl;
        if (dynamic_cast<EtherFrame *>(msg))    // do not count JAM and IFG packets
        {
            emit(dropPkIfaceDownSignal, msg);
            numDroppedIfaceDown++;
        }
        delete msg;

        return;
    }

    EtherFrame *frame = dynamic_cast<EtherFrame *>(msg);
    if (!frame)
    {
        if (dynamic_cast<EtherIFG *>(msg))
            throw cRuntimeError("There is no burst mode in full-duplex operation: EtherIFG is unexpected");
        check_and_cast<EtherFrame *>(msg);
    }

    totalSuccessfulRxTime += frame->getDuration();

    // bit errors
    if (frame->hasBitError())
    {
        numDroppedBitError++;
        emit(dropPkBitErrorSignal, frame);
        delete frame;
        return;
    }

    if (!dropFrameNotForUs(frame))
    {
        if (dynamic_cast<EtherPauseFrame*>(frame) != NULL)
        {
            int pauseUnits = ((EtherPauseFrame*)frame)->getPauseTime();
            delete frame;
            numPauseFramesRcvd++;
            emit(rxPausePkUnitsSignal, pauseUnits);
            processPauseCommand(pauseUnits);
        }
        else
        {
            processReceivedDataFrame((EtherFrame *)frame);
        }
    }
}

void EtherMACFullDuplex::handleEndIFGPeriod()
{
    if (transmitState != WAIT_IFG_STATE)
        error("Not in WAIT_IFG_STATE at the end of IFG period");

    // End of IFG period, okay to transmit
    EV << "IFG elapsed" << endl;

    beginSendFrames();
}

void EtherMACFullDuplex::handleEndTxPeriod()
{
    // we only get here if transmission has finished successfully
    if (transmitState != TRANSMITTING_STATE)
        error("End of transmission, and incorrect state detected");

    if (NULL == curTxFrame)
        error("Frame under transmission cannot be found");

    emit(packetSentToLowerSignal, curTxFrame);  //consider: emit with start time of frame

    if (dynamic_cast<EtherPauseFrame*>(curTxFrame) != NULL)
    {
        numPauseFramesSent++;
        emit(txPausePkUnitsSignal, ((EtherPauseFrame*)curTxFrame)->getPauseTime());
    }
    else
    {
        unsigned long curBytes = curTxFrame->getFrameByteLength();
        numFramesSent++;
        numBytesSent += curBytes;
        emit(txPkSignal, curTxFrame);
    }

    EV << "Transmission of " << curTxFrame << " successfully completed\n";
    delete curTxFrame;
    curTxFrame = NULL;
    lastTxFinishTime = simTime();
    getNextFrameFromQueue();

    if (pauseUnitsRequested > 0)
    {
        // if we received a PAUSE frame recently, go into PAUSE state
        EV << "Going to PAUSE mode for " << pauseUnitsRequested << " time units\n";

        scheduleEndPausePeriod(pauseUnitsRequested);
        pauseUnitsRequested = 0;
    }
    else
    {
        EV << "Start IFG period\n";
        scheduleEndIFGPeriod();
    }
}

void EtherMACFullDuplex::finish()
{
    EtherMACBase::finish();

    simtime_t t = simTime();
    simtime_t totalRxChannelIdleTime = t - totalSuccessfulRxTime;
    recordScalar("rx channel idle (%)", 100 * (totalRxChannelIdleTime / t));
    recordScalar("rx channel utilization (%)", 100 * (totalSuccessfulRxTime / t));
}

void EtherMACFullDuplex::handleEndPausePeriod()
{
    if (transmitState != PAUSE_STATE)
        error("End of PAUSE event occurred when not in PAUSE_STATE!");

    EV << "Pause finished, resuming transmissions\n";
    beginSendFrames();
}

void EtherMACFullDuplex::processReceivedDataFrame(EtherFrame *frame)
{
    emit(packetReceivedFromLowerSignal, frame);

    // strip physical layer overhead (preamble, SFD) from frame
    frame->setByteLength(frame->getFrameByteLength());

    // statistics
    unsigned long curBytes = frame->getByteLength();
    numFramesReceivedOK++;
    numBytesReceivedOK += curBytes;
    emit(rxPkOkSignal, frame);

    numFramesPassedToHL++;
    emit(packetSentToUpperSignal, frame);
    // pass up to upper layer
    send(frame, "upperLayerOut");
}

void EtherMACFullDuplex::processPauseCommand(int pauseUnits)
{
    if (transmitState == TX_IDLE_STATE)
    {
        EV << "PAUSE frame received, pausing for " << pauseUnitsRequested << " time units\n";
        if (pauseUnits > 0)
            scheduleEndPausePeriod(pauseUnits);
    }
    else if (transmitState == PAUSE_STATE)
    {
        EV << "PAUSE frame received, pausing for " << pauseUnitsRequested
           << " more time units from now\n";
        cancelEvent(endPauseMsg);

        if (pauseUnits > 0)
            scheduleEndPausePeriod(pauseUnits);
    }
    else
    {
        // transmitter busy -- wait until it finishes with current frame (endTx)
        // and then it'll go to PAUSE state
        EV << "PAUSE frame received, storing pause request\n";
        pauseUnitsRequested = pauseUnits;
    }
}

void EtherMACFullDuplex::scheduleEndIFGPeriod()
{
    transmitState = WAIT_IFG_STATE;
    simtime_t endIFGTime = simTime() + (INTERFRAME_GAP_BITS / curEtherDescr->txrate);
    scheduleAt(endIFGTime, endIFGMsg);
}

void EtherMACFullDuplex::scheduleEndPausePeriod(int pauseUnits)
{
    // length is interpreted as 512-bit-time units
    simtime_t pausePeriod = ((pauseUnits * PAUSE_UNIT_BITS) / curEtherDescr->txrate);
    scheduleAt(simTime() + pausePeriod, endPauseMsg);
    transmitState = PAUSE_STATE;
}

void EtherMACFullDuplex::beginSendFrames()
{
    if (curTxFrame)
    {
        // Other frames are queued, transmit next frame
        EV << "Transmit next frame in output queue\n";
        startFrameTransmission();
    }
    else
    {
        // No more frames set transmitter to idle
        transmitState = TX_IDLE_STATE;
        if (!txQueue.extQueue){
            // Output only for internal queue (we cannot be shure that there
            //are no other frames in external queue)
            EV << "No more frames to send, transmitter set to idle\n";
        }
    }
}

