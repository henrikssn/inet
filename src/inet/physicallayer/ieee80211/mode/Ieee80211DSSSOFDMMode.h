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

#ifndef __INET_IEEE80211DSSSOFDMMODE_H
#define __INET_IEEE80211DSSSOFDMMODE_H

#include "inet/physicallayer/ieee80211/mode/Ieee80211DSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211OFDMMode.h"

namespace inet {

namespace physicallayer {

/**
 * Represents a Direct Sequence Spread Spectrum with Orthogonal Frequency Division
 * Multiplexing PHY mode as described in IEEE 802.11-2012 specification subclause
 * 19.3.2.6.
 */
class INET_API Ieee80211DsssOfdmMode
{
  protected:
    const Ieee80211DsssPreambleMode dsssPreambleMode;
    const Ieee80211DsssHeaderMode *dsssHeaderMode;
    const Ieee80211OFDMPreambleMode *ofdmPreambleMode;
    const Ieee80211OFDMSignalMode *ofdmSignalMode;
    const Ieee80211OFDMDataMode *ofdmDataMode;
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_IEEE80211DSSSOFDMMODE_H