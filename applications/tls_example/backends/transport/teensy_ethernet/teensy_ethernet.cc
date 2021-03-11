// Copyright 2020 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "teensy_ethernet.h"

#include <Arduino.h>

#include "pw_log/log.h"

// Choose a MAC address for your device.
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01};
// IP address will be resolved by DHCP. But in case it fails, provide a backup
// IP address to initialize the TCP/IP stack. The address should be based on
// your local network setup.
IPAddress backup_ip(10, 0, 0, 32);

static void InitializeEthernet() {
  // attempt a DHCP connection:
  PW_LOG_INFO("Attempting to get an IP address using DHCP:");
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
    PW_LOG_INFO(
        "failed to get an IP address using DHCP, using the backup address");
    Ethernet.begin(mac, backup_ip);
  }
  PW_LOG_INFO("My address:");
  Serial.println(Ethernet.localIP());
}

TeensyEthernetTransport::TeensyEthernetTransport() { InitializeEthernet(); }

int TeensyEthernetTransport::Connect(const char* ip, int port) {
  IPAddress ip_addr;
  ip_addr.fromString(String(ip));
  return client_.connect(ip_addr, port) ? 0 : -1;
}

int TeensyEthernetTransport::Write(const void* buffer, size_t size) {
  if (!client_.connected()) {
    PW_LOG_INFO("ethernet client is not connected\n");
    return -1;
  }
  int status = client_.write(static_cast<const char*>(buffer), size);
  return status ? status : -1;
}

int TeensyEthernetTransport::Read(void* buffer, size_t size) {
  if (!client_.available()) {
    return 0;
  }
  int status = client_.read(static_cast<uint8_t*>(buffer), size);
  return status;
}

TransportInterface* CreateTransport() { return new TeensyEthernetTransport(); }
