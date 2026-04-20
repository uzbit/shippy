#ifndef _PDAUDIO_H_
#define _PDAUDIO_H_

bool pd_init();
void pd_shutdown();

void pd_send_float(const char* recv, float f);
void pd_send_bang(const char* recv);

#endif
