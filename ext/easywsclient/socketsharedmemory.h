#pragma once

#include <stdint.h>

namespace wsocket
{

class Wsocket;

Wsocket *createSocketSharedMemory(uint32_t port,bool isServer);

}
