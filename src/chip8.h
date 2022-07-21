#include <stdint.h>

void vm_init(void);
void vm_cycle(void);
int load_program(char *file);

extern uint8_t key[16];
extern uint8_t display[64 * 32];
extern uint8_t draw;
