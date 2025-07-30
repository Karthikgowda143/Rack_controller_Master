#ifndef SLAVE_HANDLER_H
#define SLAVE_HANDLER_H

#include "slip.h"
#include "main.h"


// Declare buffer and slip handler
extern uint8_t buf[500];  // External buffer
extern slip_descriptor_s slip_descriptor;
extern slip_handler_s slip;

// Function declarations
void slip_init_handler();
void slip_process_byte(uint8_t byte);
void SlaveCommunicationTask(void *pvParameters);
void recv_message(uint8_t *data, uint32_t size);
// Function to process incoming SLIP messages
void processSLIP();
void sendDoorAndLightOpen();
void sendACK(const char* messageType, int status);
bool compareWithStoredPassword(int enteredPassword);
void sendInternalDisplayData();
void sendchangepasswordACK(int status);
void send_button_status(bool status);

void sendLightOn();
void sendLightOff();
void send_virgin_message();
void send_device_settings();
void send_restart_message();
#endif
