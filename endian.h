#ifndef G_ENDIANNESS_H
#define G_ENDIANNESS_H

#include <inttypes.h>

// change from Host Endian to Network Byte Order
#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
// Little endian swaps byte order. 0xA0B0C0D0  --> 0xD0C0B0A0
template<class realType> uint8_t _NBO8(realType val){
	return val & 0xFF;
}
template<class realType> uint16_t _NBO16(realType val){
	return ((val&0xFF) << 8) | ((val&0xFF00)>>8);
}
template<class realType> uint32_t _NBO32(realType val){
	return ((val&0xFF) << 24) | ((val&0xFF00) << 8) | ((val&0xFF0000) >> 8) | ((val&0xFF000000)>>24);
}
template<class realType> uint64_t _NBO64(realType val){
	return ((val&0xFF) << 56) | ((val&0xFF00) << 40) | ((val&0xFF0000) << 24) | ((val&0xFF000000) << 8) |
			((val&0xFF00000000) >> 8) | ((val&0xFF0000000000) >> 24) | ((val&0xFF000000000000)>>40) | ((val&0xFF00000000000000) >> 56);
}

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
template<class realType> uint32_t _NBO8(realType val){return val;}
template<class realType> uint32_t _NBO16(realType val){return val;}
template<class realType> uint32_t _NBO32(realType val){return val;}
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
// PDP swaps word order.  0xA0B0C0D0 --> 0xC0D0A0B0
template<class realType> uint8_t _NBO8(realType val){
	return val & 0xFF;
}
template<class realType> uint16_t _NBO16(realType val){
	return val&0xFFFF;
}
template<class realType> uint32_t _NBO32(realType val){
	return ((val&0xFFFF) << 24) | ((val&0xFFFF0000) >> 24);
}
#endif

template<class realType> uint32_t _NBOfloat(realType val){
	float val2=val;
	return _NBO32(*((uint32_t*)&val2));
}

template<class realType> uint64_t _NBOdouble(realType val){
	float val2=val;
	return _NBO64(*((uint64_t*)&val2));
}

namespace endian{
	// switch from little endian to big endian and viceverse (the operation in the same)
	template<class dataType> dataType convert(dataType val){
		int s=sizeof(dataType);
		const unsigned char* ptval = (const unsigned char*)&val;
		unsigned char cval[s];
		for(int i=0; i<s; i++){
			cval[i] = ptval[s-i-1];
		}
		return *((dataType*)cval);
	}
	template<> inline uint8_t convert<uint8_t> (uint8_t val){ return val; }
	template<> inline uint16_t convert<uint16_t>(uint16_t val){ return ((val&0xFF) << 8) | ((val&0xFF00)>>8); }
	template<> inline uint32_t convert<uint32_t>(uint32_t val){ return ((val&0xFF) << 24) | ((val&0xFF00) << 8) | ((val&0xFF0000) >> 8) | ((val&0xFF000000)>>24); }
}



#endif