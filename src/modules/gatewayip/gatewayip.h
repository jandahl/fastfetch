#pragma once

#include "fastfetch.h"

#define FF_GATEWAYIP_MODULE_NAME "GatewayIp"

void ffPrintGatewayIp(FFGatewayIpOptions* options);
void ffInitGatewayIpOptions(FFGatewayIpOptions* options);
void ffDestroyGatewayIpOptions(FFGatewayIpOptions* options);
