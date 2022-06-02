#include <otfcc.h>


extern "C" void otfcc_free_buffer(char *buffer) {
  free(buffer);
}
