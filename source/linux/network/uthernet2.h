#pragma once

// this register the IOCalls for Uthernet 2 on slot 3
// if TFE is not enabled (or available) all operations will timeout
void registerUthernet2();
void unRegisterUthernet2();
void processEventsUthernet2(uint32_t timeout);