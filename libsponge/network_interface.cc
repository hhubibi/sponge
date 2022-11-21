#include "network_interface.hh"

#include "arp_message.hh"
#include "buffer.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "ipv4_datagram.hh"
#include "parser.hh"

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    if (_ip_to_mac.count(next_hop_ip)) {
        send_frame(dgram, next_hop_ip);
    } else {
        if (_arps_for_reply.count(next_hop_ip) && _arps_for_reply[next_hop_ip] < 5 * 1000) {
            return;
        }
        send_arp(ARPMessage::OPCODE_REQUEST, ETHERNET_BROADCAST, next_hop_ip);
        _unsented_datagrams[next_hop_ip].push(dgram);
        _arps_for_reply[next_hop_ip] = 0;
    }
}

void NetworkInterface::send_frame(const InternetDatagram &dgram, const uint32_t &target_ip) {
    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.header().dst = _ip_to_mac[target_ip].first;
    frame.payload() = dgram.serialize();
    _frames_out.push(frame);
}

void NetworkInterface::send_arp(const uint16_t &opcode, const EthernetAddress &dst, const uint32_t &target_ip) {
    ARPMessage arp;
    arp.opcode = opcode;
    arp.sender_ethernet_address = _ethernet_address;
    arp.sender_ip_address = _ip_address.ipv4_numeric();
    if (opcode != ARPMessage::OPCODE_REQUEST) {
        arp.target_ethernet_address = dst;
    }
    arp.target_ip_address = target_ip;

    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().dst = dst;
    frame.header().type = EthernetHeader::TYPE_ARP;
    frame.payload() = Buffer(arp.serialize());
    _frames_out.push(frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    if (frame.payload().buffers().empty()) {
        return {};
    }

    if (frame.header().dst == _ethernet_address
            && frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        std::deque<Buffer> buffers = frame.payload().buffers();
        while (!buffers.empty()) {
            if (dgram.parse(buffers.front()) == ParseResult::NoError) {
                buffers.pop_front();
            } else {
                return {};
            }
        }
        return dgram;
    }
    if (frame.header().type == EthernetHeader::TYPE_ARP
            && frame.payload().buffers().size() == 1) {
        ARPMessage arp;
        std::deque<Buffer> buffers = frame.payload().buffers();
        if (arp.parse(buffers.front()) == ParseResult::NoError) {
            buffers.pop_front();
        } else {
            return {};
        }

        if (frame.header().dst == ETHERNET_BROADCAST
            && arp.opcode == ARPMessage::OPCODE_REQUEST
            && _ip_address.ipv4_numeric() == arp.target_ip_address) {
            send_arp(ARPMessage::OPCODE_REPLY, arp.sender_ethernet_address, arp.sender_ip_address);
            _ip_to_mac[arp.sender_ip_address] = std::make_pair(arp.sender_ethernet_address, 0);
        } else if (frame.header().dst == _ethernet_address
            && arp.opcode == ARPMessage::OPCODE_REPLY
            && _arps_for_reply.count(arp.sender_ip_address)) {
            _ip_to_mac[arp.sender_ip_address] = std::make_pair(arp.sender_ethernet_address, 0);
            _arps_for_reply.erase(arp.sender_ip_address);
            auto dgrams = _unsented_datagrams[arp.sender_ip_address];
            while (!dgrams.empty()) {
                send_frame(dgrams.front(), arp.sender_ip_address);
                dgrams.pop();
            }
            _unsented_datagrams.erase(arp.sender_ip_address);
        } else {
            return {};
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    auto it = _arps_for_reply.begin();
    while (it != _arps_for_reply.end()) {
        it->second += ms_since_last_tick;
        it++;
    }

    auto iter = _ip_to_mac.begin();
    while (iter != _ip_to_mac.end()) {
        iter->second.second += ms_since_last_tick;
        if (iter->second.second >= 30 * 1000) {
            iter = _ip_to_mac.erase(iter);
        } else {
            iter++;
        }
    }
}
