#ifndef NTCH
	#define NTCH
	#define NTCAVERAGING 1
	#define NR_NTCS		 3

	float calcNTC( uint32_t adcVal);
	float updateNTC( uint32_t ID, uint32_t adcVal );
	float resToTemp(float Rntc);

	extern float temperature[NR_NTCS];

	extern float NTCreferenceValue;

	#define ERRORTEMP -999.999

#endif	
