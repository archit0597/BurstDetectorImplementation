#ifndef _BURST_H_
#define _BURST_H_

#include <cstdint>

template<typename UNIQUE_ID_TYPE>
class Burst {
public:
	uint32_t startWindow;
	uint32_t endWindow;
	UNIQUE_ID_TYPE burstId;

	Burst(){};
	Burst(uint32_t start, uint32_t end, UNIQUE_ID_TYPE _id)
	{
		startWindow = start;
		endWindow = end;
		burstId = _id;
	}
};

#endif