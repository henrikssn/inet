//
// Copyright (C) 2014 OpenSim Ltd.
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

#ifndef __INET_IEEE80211LAYEREDENCODER_H_
#define __INET_IEEE80211LAYEREDENCODER_H_

#include "inet/physicallayer/contract/IEncoder.h"
#include "inet/physicallayer/contract/ISerializer.h"
#include "inet/physicallayer/layered/LayeredEncoder.h"
#include "inet/physicallayer/contract/IFECCoder.h"
#include "inet/physicallayer/contract/IScrambler.h"
#include "inet/physicallayer/contract/IInterleaver.h"
#include "inet/physicallayer/layered/SignalPacketModel.h"
#include "inet/physicallayer/layered/SignalBitModel.h"

namespace inet {
namespace physicallayer {

class INET_API Ieee80211LayeredEncoder : public LayeredEncoder
{
    protected:
        const IFECCoder *signalFECEncoder;
        const IInterleaver *signalInterleaver;
        const IFECCoder *dataFECEncoder;

    protected:
        virtual void initialize(int stage);
        virtual int numInitStages() const { return NUM_INIT_STAGES; }
        virtual BitVector signalFieldEncode(const BitVector& signalField) const;
        virtual BitVector dataFieldEncode(const BitVector& dataField) const;

    public:
        virtual const ITransmissionBitModel *encode(const ITransmissionPacketModel *packetModel) const;
        virtual void printToStream(std::ostream& stream) const { stream << "IEEE80211 Layered Encoder"; } // TODO
};

} /* namespace physicallayer */
} /* namespace inet */

#endif /* __INET_IEEE80211LAYEREDENCODER_H_ */