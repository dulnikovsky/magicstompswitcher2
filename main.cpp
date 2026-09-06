#include <iostream>
#include <thread>
#include <map>
#include <vector>
#include <array>
#include <alsa/asoundlib.h>
#include "magicstomp.h"

using namespace std;

bool operator < (const snd_seq_addr_t& l, const snd_seq_addr_t& r)
{
    return pair(l.client, l.port) < std::pair(r.client, r.port);
}

map<snd_seq_addr_t, vector<uint8_t>> msMap;
int queue;
uint8_t currentProgram{0};
snd_seq_t *handle;
snd_seq_addr_t selfInAddr, selfOutAddr;
struct pollfd seqPollFd;

int subscribePort(snd_seq_t *handle, const snd_seq_addr_t &src, const snd_seq_addr_t &dest)
{
    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &src);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    return snd_seq_subscribe_port(handle, subs);
}

int unSubscribePort(snd_seq_t *handle, const snd_seq_addr_t &src, const snd_seq_addr_t &dest)
{
    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &src);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    return snd_seq_unsubscribe_port(handle, subs);
}

void init()
{
    snd_seq_open(&handle, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
    selfInAddr.client = selfOutAddr.client = snd_seq_client_id(handle);
    snd_seq_set_client_name(handle, "msswitcher");

    queue = snd_seq_alloc_queue(handle);

    selfInAddr.port = snd_seq_create_simple_port(handle, "IN",
                                                      SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    selfOutAddr.port = snd_seq_create_simple_port(handle, "OUT",
                                                       SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC );


    int count = snd_seq_poll_descriptors_count(handle, POLLIN);
    if(count != 1) {
        cout << "Poll descriptor count higher than 1????" << endl;
    }
    count = snd_seq_poll_descriptors(handle, &seqPollFd, 1, POLLIN);
    seqPollFd.events = POLLIN;
    seqPollFd.revents = 0;

    snd_seq_addr_t systemAddr;
    systemAddr.client = SND_SEQ_CLIENT_SYSTEM;
    systemAddr.port = SND_SEQ_PORT_SYSTEM_ANNOUNCE;
    subscribePort(handle, systemAddr, selfInAddr);
}

bool isMagicstomp(const char *clientName, const char *portName)
{
    const char *ub99ClientName = "UB99";
    const char *ub99PortName = "UB99 MIDI 1";

    return(strncmp(portName, ub99PortName, 11) == 0 && strncmp(clientName, ub99ClientName, 4) == 0);
}

void scan()
{
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);
    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(handle, cinfo) >= 0)
    {
        int clientId = snd_seq_client_info_get_client(cinfo);
        if( (clientId == SND_SEQ_CLIENT_SYSTEM) || (clientId == snd_seq_client_id(handle)))
            continue;

        snd_seq_port_info_set_client(pinfo, clientId);
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(handle, pinfo) >= 0)
        {
            unsigned int cap = snd_seq_port_info_get_capability(pinfo);
            if( isMagicstomp(snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo)))
            {
                snd_seq_addr_t msAddr;
                msAddr.client = clientId;
                msAddr.port = snd_seq_port_info_get_port(pinfo);
                msMap.insert(std::pair<snd_seq_addr_t, vector<unsigned char>>(msAddr, vector<unsigned char>()));
                if(cap & (SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE)) {
                    subscribePort(handle, selfOutAddr, msAddr);
                }
                if(cap & (SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) {
                    subscribePort(handle, msAddr, selfInAddr);
                }
                cout << "Magicstomp found[" << clientId << "," << snd_seq_port_info_get_port(pinfo) << "]" << endl;
            }
            else if((snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_HARDWARE) == SND_SEQ_PORT_TYPE_HARDWARE &&
                     cap & (SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ))
            {
                snd_seq_addr_t hrdwrControllerAddr;
                hrdwrControllerAddr.client = clientId;
                hrdwrControllerAddr.port = snd_seq_port_info_get_port(pinfo);
                if(cap & (SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) {
                    subscribePort( handle, hrdwrControllerAddr, selfInAddr);
                }
                cout << "Hardware MIDI IN device found[" << clientId << "," << snd_seq_port_info_get_port(pinfo) << "]" << endl;
                if(/*midiThrough*/ 0)
                {
                    // subscribePort( handle, clientId, snd_seq_port_info_get_port(pinfo), 14, 0);
                    // if((snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_HARDWARE) == SND_SEQ_PORT_TYPE_HARDWARE &&
                    //     cap & (SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE))
                    //     subscribePort( handle, 14, 0, clientId, snd_seq_port_info_get_port(pinfo));
                }
            }
        }
    }
}

int requestPatch(unsigned char index, const snd_seq_addr_t &src, const snd_seq_addr_t &dest, unsigned int delayNs)
{
    array<unsigned char, 10> request{0xF0, 0x43, 0x7D, 0x50, 0x55, 0x42, 0x30, 0x01, 0x00, 0xF7};
    request[8] = index;

    snd_seq_event_t sendev;
    snd_seq_ev_clear(&sendev);
    snd_seq_ev_set_source(&sendev, src.port);
    snd_seq_ev_set_dest(&sendev, dest.client, dest.port);

    snd_seq_ev_set_variable(&sendev, request.size(), (void *) &request.at(0));
    sendev.type=SND_SEQ_EVENT_SYSEX;

    snd_seq_real_time_t delay_time;
    delay_time.tv_sec = 0;
    delay_time.tv_nsec = delayNs;

    snd_seq_ev_schedule_real(&sendev, queue, 1, &delay_time);

    int ret;
    ret = snd_seq_event_output(handle, &sendev);
    if( ret <= 0)
        return ret;
    snd_seq_control_queue(handle, queue, SND_SEQ_EVENT_SETPOS_TIME, 0, NULL);
    snd_seq_control_queue(handle, queue, SND_SEQ_EVENT_START, 0, NULL);
    ret = snd_seq_drain_output(handle);
    snd_seq_control_queue(handle, queue, SND_SEQ_EVENT_STOP, 0, NULL);
    return ret;
}

uint8_t calcChecksum(const uint8_t *data, int dataLength)
{
    char checkSum{0};
    for (int i = 0; i < dataLength; ++i) {
        checkSum += *data++;
    }
    return ((-checkSum) & 0x7f);
}

int sendPatchToTemp(const snd_seq_addr_t &src, const snd_seq_addr_t &dest,
                                      const uint8_t *patchCommonData, const uint8_t *patchEffectData)
{
    vector<uint8_t> dataVector;
    dataVector.reserve(256);

    snd_seq_event_t sendev;
    int ret;

    dataVector.insert(dataVector.end(), ub99SysExHeader, ub99SysExHeader + ub99SysExHeaderSize);
    dataVector.push_back(0x00);
    dataVector.push_back(0x00);
    dataVector.push_back(0x30);
    dataVector.push_back(0x03);
    dataVector.push_back(currentProgram);
    dataVector.push_back( calcChecksum( &(*dataVector.cbegin()) + ub99SysExHeaderSize, dataVector.size() - ub99SysExHeaderSize));
    dataVector.push_back(0xF7);

    snd_seq_ev_clear(&sendev);
    snd_seq_ev_set_source(&sendev, src.port);
    snd_seq_ev_set_dest(&sendev, dest.client, dest.port);
    snd_seq_ev_set_direct(&sendev);
    snd_seq_ev_set_variable(&sendev, dataVector.size(), (void *) &(*dataVector.cbegin()));
    sendev.type=SND_SEQ_EVENT_SYSEX;
    ret = snd_seq_event_output(handle, &sendev);
    if( ret <= 0)
        return ret;

    dataVector.clear();
    dataVector.insert(dataVector.end(), ub99SysExHeader, ub99SysExHeader + ub99SysExHeaderSize);
    dataVector.push_back(0x00);
    dataVector.push_back(PatchCommonLength);
    dataVector.push_back(0x20);
    dataVector.push_back(0x00);
    dataVector.push_back(0x00);
    dataVector.insert(dataVector.end(), patchCommonData, patchCommonData + PatchCommonLength);
    dataVector.push_back( calcChecksum( &(*dataVector.cbegin()) + ub99SysExHeaderSize, dataVector.size() - ub99SysExHeaderSize));
    dataVector.push_back(0xF7);

    snd_seq_ev_clear(&sendev);
    snd_seq_ev_set_source(&sendev, src.port);
    snd_seq_ev_set_dest(&sendev, dest.client, dest.port);
    snd_seq_ev_set_direct(&sendev);
    snd_seq_ev_set_variable(&sendev, dataVector.size(), (void *) &(*dataVector.cbegin()));
    sendev.type=SND_SEQ_EVENT_SYSEX;
    ret = snd_seq_event_output(handle, &sendev);
    if( ret <= 0)
        return ret;

    dataVector.clear();
    dataVector.insert(dataVector.end(), ub99SysExHeader, ub99SysExHeader + ub99SysExHeaderSize);
    dataVector.push_back(0x00);
    dataVector.push_back(PatchEffectLength);
    dataVector.push_back(0x20);
    dataVector.push_back(0x01);
    dataVector.push_back(0x00);
    dataVector.insert(dataVector.end(), patchEffectData, patchEffectData + PatchEffectLength);
    dataVector.push_back( calcChecksum( &(*dataVector.cbegin()) + ub99SysExHeaderSize, dataVector.size() - ub99SysExHeaderSize));
    dataVector.push_back(0xF7);

    snd_seq_ev_clear(&sendev);
    snd_seq_ev_set_source(&sendev, src.port);
    snd_seq_ev_set_dest(&sendev, dest.client, dest.port);
    snd_seq_ev_set_direct(&sendev);
    snd_seq_ev_set_variable(&sendev, dataVector.size(), (void *) &(*dataVector.cbegin()));
    sendev.type=SND_SEQ_EVENT_SYSEX;
    ret = snd_seq_event_output(handle, &sendev);
    if( ret <= 0)
        return ret;

    dataVector.clear();
    dataVector.insert(dataVector.end(), ub99SysExHeader, ub99SysExHeader + ub99SysExHeaderSize);
    dataVector.push_back(0x00);
    dataVector.push_back(0x00);
    dataVector.push_back(0x30);
    dataVector.push_back(0x13);
    dataVector.push_back(currentProgram);
    dataVector.push_back( calcChecksum( &(*dataVector.cbegin()) + ub99SysExHeaderSize, dataVector.size() - ub99SysExHeaderSize));
    dataVector.push_back(0xF7);

    snd_seq_ev_clear(&sendev);
    snd_seq_ev_set_source(&sendev, src.port);
    snd_seq_ev_set_dest(&sendev, dest.client, dest.port);
    snd_seq_ev_set_direct(&sendev);
    snd_seq_ev_set_variable(&sendev, dataVector.size(), (void *) &(*dataVector.cbegin()));
    sendev.type=SND_SEQ_EVENT_SYSEX;
    ret = snd_seq_event_output(handle, &sendev);
    if( ret <= 0)
        return ret;

    ret = snd_seq_drain_output(handle);
    return ret;
}


void sendAllToTemp()
{
    for (const auto& it: msMap)
    {
        if( it.second.size() != numOfPatches*PatchTotalLength)
            continue; // Size does not match, means patches are not loaded properly
        sendPatchToTemp(selfOutAddr, it.first,
                        it.second.data() + PatchTotalLength*currentProgram,
                        it.second.data() + PatchTotalLength*currentProgram + PatchCommonLength);

    }
}

int main(int argc, char* argv[])
{
    map<snd_seq_addr_t, vector<uint8_t>> sysExMap;
    uint8_t midiChannel{0};
    snd_seq_event_t *ev;
    int pollret;

    cout << "Magicstomp Switcher" << endl;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-c" || arg == "--channel") {
            if (i + 1 < argc) {
                string cString = argv[++i];
                midiChannel = stoi(cString);
                if(midiChannel > 16) {
                    cerr << "Error: " << arg << " - midi channel too high. Max value is 16 " << endl;
                    return 1;
                }
            } else {
                cerr << "Error: " << arg << " requires an argument." << endl;
                return 1;
            }
        }
    }


    init();
    scan();
    for (auto const& [msaddr, dataVector] : msMap) {
        requestPatch(dataVector.size() , selfOutAddr, msaddr, 0);
    }
    while (1) {

        pollret = poll(&seqPollFd, 1, 500);
        if (pollret < 0) {
            cout << "Poll error. Exiting thread" << endl;
            break;
        }
        if(pollret == 0) { //timeout
            continue;
        }
        while(snd_seq_event_input(handle, &ev) >= 0) {
            if(ev->type==SND_SEQ_EVENT_SYSEX) {
                auto msMapIt = msMap.find(ev->source);
                if( msMapIt == msMap.end()) {
                    cout << "Unexpected event addess??" << endl;
                    continue;
                }
                auto sysExMapIt = sysExMap.find(ev->data.addr);
                if(sysExMapIt == sysExMap.end()) {
                    sysExMapIt = sysExMap.insert(std::pair<snd_seq_addr_t, vector<uint8_t>>(ev->data.addr, vector<uint8_t>())).first;
                }
                vector<uint8_t> &sysExDataVecRef = sysExMapIt->second;
                sysExDataVecRef.insert(sysExDataVecRef.end(), static_cast<uint8_t *>(ev->data.ext.ptr), static_cast<uint8_t *>(ev->data.ext.ptr) + ev->data.ext.len);

                if( (! sysExDataVecRef.empty()) && sysExDataVecRef.at(0) == 0xF0 && sysExDataVecRef.at(sysExDataVecRef.size()-1) == 0xF7) {
                    if(sysExDataVecRef.size() >= 13 && equal(sysExDataVecRef.cbegin(), sysExDataVecRef.cbegin()+ub99SysExHeaderSize, ub99SysExHeader)) {
                        int8_t currentPatchInRequest = msMapIt->second.size() / PatchTotalLength;
                        uint8_t checkSum = calcChecksum( & sysExDataVecRef.at(ub99SysExHeaderSize), sysExDataVecRef.size() - ub99SysExHeaderSize-2);
                        if(checkSum == sysExDataVecRef.at(sysExDataVecRef.size()-2)) {
                            if( sysExDataVecRef.at(8)==0x00 && sysExDataVecRef.at(9)==0x00) {
                                if( sysExDataVecRef.at(10)==0x30 && sysExDataVecRef.at(11)==0x01 && currentPatchInRequest==sysExDataVecRef.at(12)) {
                                    ; //patch dump start message
                                } else if( sysExDataVecRef.at(10)==0x30 && sysExDataVecRef.at(11)==0x11 && (currentPatchInRequest-1)==sysExDataVecRef.at(12)) {
                                    //patch dump end message
                                    if(currentPatchInRequest >= (numOfPatches)) {
                                        sendPatchToTemp( selfOutAddr, msMapIt->first,
                                                         msMapIt->second.data() + PatchTotalLength*currentProgram,
                                                         msMapIt->second.data() + PatchTotalLength*currentProgram + PatchCommonLength);

                                    } else {
                                        requestPatch(currentPatchInRequest, selfOutAddr, msMapIt->first, 70000000);
                                    }
                                    const char *firstCharNameAddr = reinterpret_cast<const char *>(&(*(msMapIt->second.cbegin()+(PatchTotalLength*(currentPatchInRequest -1)) + PatchName)));
                                    std::string patchName(firstCharNameAddr, PatchNameLength);
                                    cout << "Received Patch " << static_cast<uint32_t>(currentPatchInRequest) << " " << patchName << " from Magicstomp at ["
                                         << static_cast<uint32_t>(msMapIt->first.client) << ","
                                         << static_cast<uint32_t>(msMapIt->first.port) << "]" << endl;
                                }
                            } else if( sysExDataVecRef.at(8)==0x00 && sysExDataVecRef.at(9)!=0x00) {
                                uint8_t length = sysExDataVecRef.at(9);
                                if( sysExDataVecRef.at(10)==0x20) {
                                    if( sysExDataVecRef.at(11)==0x00 && sysExDataVecRef.at(12)==0x00 && length==PatchCommonLength) {
                                        // Patch common data;
                                        msMapIt->second.insert(msMapIt->second.end(), &sysExDataVecRef.at(13), &sysExDataVecRef.at(13)+PatchCommonLength);
                                    }
                                    else if( sysExDataVecRef.at(11)==0x01 && sysExDataVecRef.at(12)==0x00 && length==PatchEffectLength) {
                                        // Patch effect data;
                                        msMapIt->second.insert(msMapIt->second.end(), &sysExDataVecRef.at(13), &sysExDataVecRef.at(13)+PatchEffectLength);
                                    }
                                }
                            }
                        }
                    }
                    sysExDataVecRef.clear();
                }
            } else if(ev->type==SND_SEQ_EVENT_PGMCHANGE) {
                if((midiChannel==0 || (ev->data.raw8.d[0] & 0x0F)+1 == midiChannel) && ev->data.raw8.d[8] < numOfPatches) {
                    currentProgram = ev->data.raw8.d[8];
                    cout << "Received Program Change " << currentProgram << endl;
                    sendAllToTemp();
                }
            } else if(ev->type==SND_SEQ_EVENT_PORT_START) {
                snd_seq_client_info_t *cinfo;
                snd_seq_port_info_t *pinfo;

                snd_seq_client_info_alloca(&cinfo);
                snd_seq_port_info_alloca(&pinfo);

                snd_seq_get_any_client_info(handle, ev->data.addr.client, cinfo);
                snd_seq_get_any_port_info(handle, ev->data.addr.client, ev->data.addr.port, pinfo);
                unsigned int cap = snd_seq_port_info_get_capability(pinfo);
                if(isMagicstomp(snd_seq_client_info_get_name(cinfo), snd_seq_port_info_get_name(pinfo))) {
                    auto findIter = msMap.find(ev->data.addr);
                    if(findIter == msMap.end()) {
                        subscribePort(handle, selfOutAddr, ev->data.addr);
                        subscribePort(handle, ev->data.addr, selfInAddr);
                        auto retPair = msMap.insert(std::pair<snd_seq_addr_t, vector<uint8_t>>(ev->data.addr, vector<uint8_t>()));
                        requestPatch( retPair.first->second.size(), selfOutAddr, retPair.first->first, 700000000);
                        cout << "Magicstomp connected[" << static_cast<uint32_t>(ev->data.addr.client)
                             << ":" << static_cast<uint32_t>(ev->data.addr.port) << "]" << endl;
                    }
                }
                else if((snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_HARDWARE) == SND_SEQ_PORT_TYPE_HARDWARE &&
                         cap & (SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) {
                    subscribePort( handle, ev->data.addr, selfInAddr);
                    cout << "Hardware MIDI IN device connected[" << static_cast<uint32_t>(ev->data.addr.client)
                         << ":" << static_cast<uint32_t>(ev->data.addr.port) << "]" << endl;

                    if(/*midiThrough*/ 0)
                    {
                        // subscribePort( handle, ev->data.addr.client, ev->data.addr.port, 14, 0);
                        // if((snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_HARDWARE) == SND_SEQ_PORT_TYPE_HARDWARE &&
                        //     cap & (SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE))
                        //     subscribePort( handle, 14, 0, ev->data.addr.client, ev->data.addr.port);
                    }
                }
            }
            else if(ev->type==SND_SEQ_EVENT_PORT_EXIT) {
                if(msMap.erase(ev->data.addr) == 1) {
                    cout << "Magicstomp disconnected[" << static_cast<uint32_t>(ev->data.addr.client)
                         << ":" << static_cast<uint32_t>(ev->data.addr.port) << "]" << endl;
                }
                sysExMap.erase(ev->data.addr);
            }
        }
    }
    return 0;
}
