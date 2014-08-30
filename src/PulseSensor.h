#ifndef __PulseSensor__
#define __PulseSensor__


void PulseSensorInterruptSetup(void);

boolean PulseSensorGetQS(void);
int PulseSensorGetBPM(void);  // resets QS


#endif
