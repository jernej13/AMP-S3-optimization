#pragma once

#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#ifndef _LITTLE_ENDIAN
#define _LITTLE_ENDIAN 1234
#endif

#ifndef _BIG_ENDIAN
#define _BIG_ENDIAN 4321
#endif

#ifndef _BYTE_ORDER
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif

#endif
