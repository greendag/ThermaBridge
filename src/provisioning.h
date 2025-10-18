#ifndef PROVISIONING_H
#define PROVISIONING_H
#include "config.h"
#include <WebServer.h>
#include <DNSServer.h>

void startProvisioning();
void loopProvisioning();
bool isProvisioningActive();
void startStatusServer();
void loopStatusServer();

#endif // PROVISIONING_H
