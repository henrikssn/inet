//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

package inet.tutorials.wireless;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.StandardHost;
import inet.physicallayer.idealradio.IdealRadioMedium;


// - drop two standard hosts named hostR1 and hostR2 at the 100,400 and 400,100 positions
network Wireless03
{
    @display("bgb=500,500");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=61,24");
        }
        radioMedium: IdealRadioMedium {
            @display("p=158,16");
        }
        hostA: StandardHost {
            @display("p=100,100");
        }
        hostB: StandardHost {
            @display("p=400,400");
        }
        hostR1: StandardHost {
            @display("p=100,400");
        }
        hostR2: StandardHost {
            @display("p=400,100");
        }
}
