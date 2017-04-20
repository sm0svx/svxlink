/**
@author Artem Prilutskiy / R3ABM
**/

#ifndef REWIND_H
#define REWIND_H

#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

#pragma pack(push, 1)

#define REWIND_KEEP_ALIVE_INTERVAL   5

#define REWIND_SIGN_LENGTH           8
#define REWIND_PROTOCOL_SIGN         "REWIND01"

#define REWIND_CLASS_REWIND_CONTROL  0x0000
#define REWIND_CLASS_SYSTEM_CONSOLE  0x0100
#define REWIND_CLASS_SERVER_NOTICE   0x0200
#define REWIND_CLASS_DEVICE_DATA     0x0800
#define REWIND_CLASS_APPLICATION     0x0900

#define REWIND_CLASS_KAIROS_DATA       (REWIND_CLASS_DEVICE_DATA + 0x00)
#define REWIND_CLASS_HYTERA_DATA       (REWIND_CLASS_DEVICE_DATA + 0x10)

#define REWIND_TYPE_KEEP_ALIVE         (REWIND_CLASS_REWIND_CONTROL + 0)
#define REWIND_TYPE_CLOSE              (REWIND_CLASS_REWIND_CONTROL + 1)
#define REWIND_TYPE_CHALLENGE          (REWIND_CLASS_REWIND_CONTROL + 2)
#define REWIND_TYPE_AUTHENTICATION     (REWIND_CLASS_REWIND_CONTROL + 3)

#define REWIND_TYPE_REPORT             (REWIND_CLASS_SYSTEM_CONSOLE + 0)

#define REWIND_TYPE_BUSY_NOTICE        (REWIND_CLASS_SERVER_NOTICE + 0)
#define REWIND_TYPE_ADDRESS_NOTICE     (REWIND_CLASS_SERVER_NOTICE + 1)
#define REWIND_TYPE_BINDING_NOTICE     (REWIND_CLASS_SERVER_NOTICE + 2)

#define REWIND_TYPE_EXTERNAL_SERVER    (REWIND_CLASS_KAIROS_DATA + 0)
#define REWIND_TYPE_REMOTE_CONTROL     (REWIND_CLASS_KAIROS_DATA + 1)
#define REWIND_TYPE_SNMP_TRAP          (REWIND_CLASS_KAIROS_DATA + 2)

#define REWIND_TYPE_PEER_DATA          (REWIND_CLASS_HYTERA_DATA + 0)
#define REWIND_TYPE_RDAC_DATA          (REWIND_CLASS_HYTERA_DATA + 1)
#define REWIND_TYPE_MEDIA_DATA         (REWIND_CLASS_HYTERA_DATA + 2)

#define REWIND_TYPE_CONFIGURATION      (REWIND_CLASS_APPLICATION + 0x00)
#define REWIND_TYPE_SUBSCRIPTION       (REWIND_CLASS_APPLICATION + 0x01)
#define REWIND_TYPE_CANCELLING         (REWIND_CLASS_APPLICATION + 0x02)
#define REWIND_TYPE_DMR_DATA_BASE      (REWIND_CLASS_APPLICATION + 0x10)
#define REWIND_TYPE_DMR_START_FRAME    (REWIND_CLASS_APPLICATION + 0x11)
#define REWIND_TYPE_DMR_STOP_FRAME     (REWIND_CLASS_APPLICATION + 0x12)
#define REWIND_TYPE_DMR_AUDIO_FRAME    (REWIND_CLASS_APPLICATION + 0x20)
#define REWIND_TYPE_DMR_EMBEDDED_DATA  (REWIND_CLASS_APPLICATION + 0x27)
#define REWIND_TYPE_SUPER_HEADER       (REWIND_CLASS_APPLICATION + 0x28)
#define REWIND_TYPE_FAILURE_CODE       (REWIND_CLASS_APPLICATION + 0x29)

#define REWIND_FLAG_NONE             0
#define REWIND_FLAG_REAL_TIME_1      (1 << 0)
#define REWIND_FLAG_REAL_TIME_2      (1 << 1)
#define REWIND_FLAG_DEFAULT_SET      REWIND_FLAG_NONE

#define REWIND_ROLE_REPEATER_AGENT   0x10
#define REWIND_ROLE_APPLICATION      0x20

#define REWIND_SERVICE_CRONOS_AGENT        (REWIND_ROLE_REPEATER_AGENT + 0)
#define REWIND_SERVICE_TELLUS_AGENT        (REWIND_ROLE_REPEATER_AGENT + 1)
#define REWIND_SERVICE_SIMPLE_APPLICATION  (REWIND_ROLE_APPLICATION    + 0)
#define SHA256_DIGEST_LENGTH               32
#define BUFFER_SIZE                        2048
#define REWIND_CALL_LENGTH                 10

#define REWIND_OPTION_SUPER_HEADER  (1 << 0)
#define REWIND_OPTION_LINEAR_FRAME  (1 << 1)

#define REWIND_CALL_LENGTH  10

struct RewindVersionData
{
  uint32_t number;      // Remote ID
  uint8_t service;      // REWIND_SERVICE_*
  char description[0];  // Software name and version
};

// Generic Data Structures

struct RewindAddressData
{
  struct in_addr address;
  uint16_t port;
};

struct RewindBindingData
{
  uint16_t ports[0];
};

// Simple Application Protocol

struct RewindConfigurationData
{
  uint32_t options;  // REWIND_OPTION_*
};

struct RewindSubscriptionData
{
  uint32_t type;    // SESSION_TYPE_*
  uint32_t number;  // Destination ID
};

struct RewindSuperHeader
{
  uint32_t type;                             // SESSION_TYPE_*
  uint32_t sourceID;                         // Source ID or 0
  uint32_t destinationID;                    // Destination ID or 0
  char sourceCall[REWIND_CALL_LENGTH];       // Source Call or zeros
  char destinationCall[REWIND_CALL_LENGTH];  // Destination Call or zeros
};

// Rewind Transport Layer

struct RewindData
{
  char sign[REWIND_SIGN_LENGTH];
  uint16_t type;    // REWIND_TYPE_*
  uint16_t flags;   // REWIND_FLAG_*
  uint32_t number;  // Packet sequence number
  uint16_t length;  // Length of following data
  uint8_t data[0];  //
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif
