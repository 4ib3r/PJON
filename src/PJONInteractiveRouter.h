
 /*-O//\         __     __
   |-gfo\       |__| | |  | |\ | ®
   |!y°o:\      |  __| |__| | \| v10.1
   |y"s§+`\     multi-master, multi-media bus network protocol
  /so+:-..`\    Copyright 2010-2018 by Giovanni Blu Mitolo gioscarab@gmail.com
  |+/:ngr-*.`\
  |5/:%&-a3f.:;\
  \+//u/+g%{osv,,\
    \=+&/osw+olds.\\
       \:/+-.-°-:+oss\
        | |       \oy\\
        > <
 ______-| |-__________________________________________________________________

PJONInteractiveRouter has been contributed by Fred Larsen.

This class adds functionality to the PJONSwitch, PJONRouter, PJONDynamicRouter
and potential future classes derived from them. This functionality allows a
switch or router to have it's own device id and send and receive packets as a
normal device, but to and from multiple buses.

It also allows the device to listen to all packets passing through between
buses.

Probably it is wise to use this functionality only on routers using
strategies that are not timing-critical, for example on buffered media like
serial or Ethernet. If used on timing-critical strategies like SWBB, the
receiver callback should be really fast.

If you believe in this project and you appreciate our work, please, make a
donation. The PJON Foundation is entirely financed by contributions of wise
people like you and its resources are solely invested to cover the development
and maintainance costs.
- Paypal:   https://www.paypal.me/PJON
- Bitcoin:  1FupxAyDTuAMGz33PtwnhwBm4ppc7VLwpD
- Ethereum: 0xf34AEAF3B149454522019781668F9a2d1762559b
Thank you and happy tinkering!
 _____________________________________________________________________________

Copyright 2010-2018 by Giovanni Blu Mitolo gioscarab@gmail.com

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once
#include <PJONDynamicRouter.h>

template<class RouterClass>
class PJONInteractiveRouter : public RouterClass {
protected:
  void *custom_pointer = NULL;
  PJON_Receiver receiver = NULL;
  bool router = false;

  virtual void dynamic_receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
    // Handle packets to this device, with user-supplied callback and custom ptr
    bool packet_is_for_me = (
      RouterClass::buses[RouterClass::current_bus]->device_id() != PJON_NOT_ASSIGNED &&
      memcmp(RouterClass::buses[RouterClass::current_bus]->bus_id, packet_info.receiver_bus_id, 4) == 0 &&
      RouterClass::buses[RouterClass::current_bus]->device_id() == packet_info.receiver_id
    );

    // Take care of other's packets
    if(!packet_is_for_me)
      RouterClass::dynamic_receiver_function(payload, length, packet_info);

    // Call the receive callback _after_ the packet has been delivered
    if(router || packet_is_for_me) {
       // The packet is for ME :-)
        PJON_Packet_Info p_i;
        memcpy(&p_i, &packet_info, sizeof(PJON_Packet_Info));
        p_i.custom_pointer = custom_pointer;
        if(receiver) receiver(payload, length, p_i);
        if(!router) return;
    }
  }

public:
  PJONInteractiveRouter() : RouterClass() {}
  PJONInteractiveRouter(
    uint8_t bus_count,
    PJONAny*buses[],
    uint8_t default_gateway = PJON_NOT_ASSIGNED)
    : RouterClass(bus_count, buses, default_gateway) {}

  void set_receiver(PJON_Receiver r, void *custom_ptr = NULL) {
      receiver = r;
      custom_pointer = custom_ptr;
  };

  void send_packet(
    const uint8_t *payload,
    uint16_t length,
    const PJON_Packet_Info &packet_info
  ) {
    dynamic_receiver_function(payload, length, packet_info);
  };

  // Deliver every packet to receiver callback, or just for this device?
  void set_router(bool on) { router = on; };
};
