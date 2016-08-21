
//nsw
#include "map_handler.h"

//std/stl
#include <iostream>
#include <string>
#include <tuple>

using std::cout;
using std::endl;

//////////////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------ //
//  MapHandler
// ------------------------------------------------------------------------ //
//////////////////////////////////////////////////////////////////////////////
MapHandler::MapHandler(QObject *parent) :
    QObject(parent),
    m_initialized(false),
    m_map_loaded(false),
    m_daqConfig(nullptr)
{
}
// ------------------------------------------------------------------------ //
std::string MapHandler::boardIDfromIP(std::string ipstring)
{
    return config().febConfig().boardIdFromIp(ipstring);
}
// ------------------------------------------------------------------------ //
bool MapHandler::loadDaqConfiguration(std::string filename)
{
    if(m_daqConfig) delete m_daqConfig;

    m_daqConfig = new DaqConfiguration();

    bool ok = m_daqConfig->loadDaqXml(filename);
    if(ok) {
        cout << "MapHandler::loadDaqConfiguration    DAQ xml loaded: " << filename << endl;
    }
    else {
        cout << "MapHandler::loadDaqConfiguration    Unable to load DAQ xml: " << filename << endl;
        return false;
    }
    ok = m_daqConfig->loadFEB();
    if(ok) {
        cout << "MapHandler::loadDaqConfiguration    FEB loaded" << endl;
    }
    else {
        cout << "MapHandler::loadDaqConfiguration    Unable to load FEB configuration" << endl;
        return false;
    }
    ok = m_daqConfig->loadDetectorSetup();
    if(ok) {
        cout << "MapHandler::loadDaqConfiguration     Detector setup loaded" << endl;
    }
    else {
        cout << "MapHandler::loadDaqConfiguration    Unable to laod detector setup" << endl;
        return false;
    }
    m_initialized = ok;

    if(!ok) delete m_daqConfig;
    return ok;
}
// ------------------------------------------------------------------------ //
std::string MapHandler::firstIP()
{
    // FEB ID starts at 1
    std::string ip_ = "";
    for(int ifeb = 0; ifeb< config().febConfig().nFeb(); ifeb++) {
        FEB feb = config().febConfig().getFEB(ifeb);
        if(feb.id()=="1") {
            ip_ = feb.ip();
            break;
        }
    }
    return ip_;
}
// ------------------------------------------------------------------------ //
void MapHandler::clearMaps()
{
    m_daq_map.clear();
    //m_daq_map_mini2.clear();
}
// ------------------------------------------------------------------------ //
void MapHandler::buildMapping()
{
    if(m_map_loaded) clearMaps(); //m_daq_map.clear();

    using namespace std;

    bool ok = true;

    int n_element_loaded = 0;
    for(int ifeb = 0; ifeb < config().febConfig().nFeb(); ifeb++) {
        FEB feb = config().febConfig().getFEB(ifeb);
        string feb_id = feb.id();
        string feb_name = feb.name();
        string feb_type = feb.type();

        nsw_map::channel_map feb_channel_map; // [ feb_channel : Element ]

        for(tuple<int, int, int> vmmdata : feb.vmm_map) {
            int vmm_id = get<0>(vmmdata);
            int vmm_channel = get<1>(vmmdata);
            int feb_channel = get<2>(vmmdata);

            // find connector <-- layer <-- multilayer <-- chamber
            // that has this FEB and then find the associated
            // detector element (strip) readout by FEB channel
            for(int ichamber = 0; ichamber < config().detConfig().nChamber(); ichamber++) {
                Chamber chamber = config().detConfig().getChamber(ichamber);

                // only look at chamber that holds FEB we are currently mapping
                if(chamber.hasFEB(feb_name)) {
                    string chamber_name = chamber.name();
                    string chamber_id   = chamber.id();

                    // name of connector on this chamber that contains
                    // our FEB
                    string connector_name = "";
                    if(feb_type == "mmfe8")
                        connector_name = chamber.connectorNameFromFEBName(feb_name);
                    else if(feb_type =="mini2") {
                        string vmm_name = feb.VMMNameFromId(vmm_id);
                        if(vmm_name=="") { ok = false; break; }
                        connector_name = chamber.connectorNameFromVMMName(vmm_name);
                    }
                        
                    for(int iML = 0; iML < chamber.nMultiLayer(); iML++) {
                        string multilayer_name = chamber.multiLayer(iML).name();
                        string multilayer_id   = chamber.multiLayer(iML).id();

                        for(int iL = 0; iL < chamber.multiLayer(iML).nLayer(); iL++) {
                            Layer layer = chamber.multiLayer(iML).getLayer(iL);

                            // only check readout layer that has the connector
                            // that our FEB is attached to
                            if(layer.hasConnector(connector_name)) {
                                string layer_name = layer.name();
                                string layer_id   = layer.id();

                                // grab the spec for this connector
                                // i.e. which detector elements connect
                                //      to which FEB channel
                                ConnectorInfo connector_info = layer.getConnectorInfo(connector_name);
                                int strip_number = connector_info.stripNumberFromFEBChannel(feb_channel);

                                // negative means that this element (strip) is not
                                // read out by this FEB
                                if(!(strip_number<0)) {

                                    //std::cout << "feb name: " << feb_name << " (vmmid, vmmchan, febchan) = (" << vmm_id << ", " << vmm_channel << " , " << feb_channel <<     ")   chamber: " << chamber_name << "  connector: " << connector_name << "  strip: " << strip_number << std::endl;


                                    Element current_element;
                                    current_element.init(n_element_loaded); n_element_loaded++; // give elements unique #
                                    

                                    /////////////////////////////////////
                                    // vmm to strip mapping
                                    /////////////////////////////////////
                                    current_element.setVMMID(to_string(vmm_id))
                                                   .setVMMChan(to_string(vmm_channel))
                                                   .setFEBChan(to_string(feb_channel))
                                                   .setStripNumber(to_string(strip_number));

                                    /////////////////////////////////////
                                    // feb address
                                    /////////////////////////////////////
                                    current_element.setParentIP(feb.ip())
                                                   .setParentID(feb.id())
                                                   .setParentName(feb.name());
                
                                    /////////////////////////////////////
                                    // readout info
                                    /////////////////////////////////////
                                    string readout_type = "";
                                    if((feb.type() == "mmfe8") || (feb.type() == "mini2")) readout_type = "MM";
                                    else { readout_type = "XXX"; }
                                    current_element.setElementType(feb.type())
                                                   .setReadoutType(readout_type);

                                    /////////////////////////////////////
                                    // chamber info
                                    /////////////////////////////////////
                                    current_element.setMultiLayerID(multilayer_id)
                                                   .setMultiLayerName(multilayer_name)
                                                   .setLayerID(layer_id)
                                                   .setLayerName(layer_name)
                                                   .setChamberID(chamber_id)
                                                   .setChamberName(chamber_name);

                                    /////////////////////////////////////
                                    // add this element to map
                                    /////////////////////////////////////
                                    feb_channel_map[to_string(feb_channel)] = current_element; 
                                }
                            }
                        } // iL
                    } // iML
                }
            } // ichamber, loop over chambers
        } // vmmdata, loop over vmmdata for FEB ifeb

        /////////////////////////////////////
        // add this FEB to map
        /////////////////////////////////////
        m_daq_map[feb_id] = feb_channel_map;
    } // ifeb, loop over FEB

    #warning ADD SANITY CHECK ON MAP

    //cout << " [1][222] = " << m_daq_map["1"]["222"].stripNumber() << std::endl;
    //cout << "febChannel(1, 3, 32) = " << febChannel("1", 3, 32) << endl;
    //cout << "[1][febChannel(1,3,32)] = " << m_daq_map["1"][std::to_string(febChannel("1",3,32))].stripNumber() << std::endl;

    m_map_loaded = true;
}
// ------------------------------------------------------------------------ //
int MapHandler::febChannel(std::string boardid, int vmmid, int vmmchan)
{
    FEB feb = config().febConfig().getFEBwithId(boardid);
    return feb.getFEBChannel(vmmid, vmmchan);
}

int MapHandler::elementStripNumber(std::string boardid, std::string vmmid, std::string vmmchan)
{
    std::string feb_chan = std::to_string(this->febChannel(boardid, stoi(vmmid), stoi(vmmchan)));
    return stoi(m_daq_map[boardid][feb_chan].stripNumber());
}

std::string MapHandler::getChipName(int boardId, int vmmId)
{
    std::string out_ = "";
    FEB feb = config().febConfig().getFEBwithId(std::to_string(boardId));
    out_ = feb.VMMNameFromId(vmmId);
    return out_;
}

