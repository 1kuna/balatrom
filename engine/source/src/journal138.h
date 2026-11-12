#ifndef PUB138_API
#define PUB138_API
#endif
#include <stdint.h>
/* Pattern/map92-byte logical journal. Physical lease:80bytesDBB0..DBFF,
 * metadata12bytesDC60..DC6B; descriptorsDC00..DC53. */
uint16_t pub138_bucket(uint8_t bank,uint16_t address) PUB138_API;
uint8_t pub138_make_jobs(const uint8_t *bitmap,uint8_t *jobs,uint8_t *count,uint8_t *blocks) PUB138_API;
uint8_t pub138_cost_admits(uint8_t count,uint8_t blocks) PUB138_API;
