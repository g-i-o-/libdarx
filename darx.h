/**
 * File for implementing reading and storing of darx data archive files.
 * A darx file is a set of data tensors, associated with some archive metadata.
 * Each tensor specifies it's own dimmensions and datatype. The purpose of
 * such a file is to store associated data of any type into one single archive.
 * Each tensor's data can be compressed individually, allowing the header to still be read.
 * Also, the format provides built in support for tensors holding mixed and custom data types.
 * The mixed data type specifies elements as a tuple of elements, each with it's own
 * recursively defined data type, whereas the custom data type specifies an application-dependent
 * formatting of the data.
 * 
 */
#ifndef DARX_IO_H
#define DARX_IO_H

#include <inttypes.h>
#include <stdio.h>

/**
 * Utility Namespace for .darx format I/O functions and structures.
 * darx : data archive
 */
namespace darx{
	// whether this librsary outputs any diagnostic messages while reading/writting the darx file or not.
	extern int VERBOSE;
	
	/** Describes the available types and sizes of pixels in an image.
	 *  Each type has an associated size and data structure.
	 */
	enum ElementType {
		// simple data types
		TYPE_INT=0,  TYPE_UINT=1, TYPE_FLOAT=2, TYPE_CHAR=3,
		// extensible data types
		TYPE_MIXED=4, TYPE_CUSTOM=5
	};
	/** Available Compression algorithms.
	 */
	enum CompressionType {
		UNCOMPRESSED=0
	};
	// structure used for simple types
	class ElementTypeStruct{
		public:
		ElementType type;
		uint8_t components;
		uint8_t bit_width;
		inline ElementTypeStruct(ElementType _type, uint8_t _components, uint8_t _bit_width):
			type(_type), components(_components), bit_width(_bit_width){}
	};
	// structure used for mixed types
	class MixedElementTypeStruct: public ElementTypeStruct{
		public:
		ElementTypeStruct** subtypes;
		
		MixedElementTypeStruct(ElementType _type, uint8_t _components, uint8_t _bit_width);
		~MixedElementTypeStruct();
	};
	// structure used for custom types
	class CustomElementTypeStruct: public ElementTypeStruct{
		public:
		char* type_name;
		CustomElementTypeStruct(ElementType _type, uint8_t _components, uint8_t _bit_width, const char* _type_name);
	};
	
	/** Low-level structure representing a tensor of data.
	 * a tensor is a (possibly) multi-dimmensional array of data,
	 * with each element of the tensor belong to some datatype.
	 * This tensor has an associated name, rank (it's dimmensionality),
	 * and size (in all it's dimmensions).
	 * The data may be compressed using one of the available algorithms
	 * (none at the moment).
	 */
	typedef struct {
		// name associated to this data tensor
		const char* name;
		// number of dimmensions in this data tensor.
		uint8_t rank;
		// the lengths of each dimmension in the tensor.
		unsigned int* lengths;
		// types of each element in the struct
		ElementTypeStruct* type;
		// Compression algorithm
		CompressionType compression;
		// size of stored data
		unsigned int data_size;
		// The tensor's data
		void* data;
	} datatensor;
	
	typedef struct {
		// Wether this structure is valid or not. (runtime flag)
		bool valid;
		// Wether this archive was stored in a big endian computer, or not
		bool isBigEndian;
		// number of tensors in the data archive
		uint16_t number_of_tensors;
		// size of the header's metadata
		uint16_t metadata_size;
		// archive metadata string
		char* metadata;
		// array of tensors
		datatensor* tensors;
	} darx;
	
	/** Error codes returned from the load_image_from() function.
	 */
	enum ErrorCode{
		SUCCESS=0,
		UNSUPPORTED_ELEMENT_TYPE,
		UNSUPPORTED_COMPRESS_TYPE,
		INVALID_STRUCT
	};
	
	/** Array of textual error code representations.
	 */
	extern const char* errors[];
	
	
	extern CustomElementTypeStruct UNKNOWN_TYPE;
	
	/** Indicates whether a given open file contains .darx data.
	 * 
	 * @return true if the data pointed by the file pointer is a .darx file,
	 * 	false otherwise.
	 */
	bool is_darx(FILE* file);
	inline bool is_image(FILE* file){
		return is_darx(file);
	}
	
	/** Loads a darx data archive from a file into the structure.
	 * 
	 * @param[in] darx - reference to the darx structure to fill
	 * @param[in] file - pointer to the file containing the darx data
	 * 
	 * @returns an error code indicating success or reson of failure
	 * @note reads only uncompressed 8,16 and 32 bit-per-pixel bmp images.
	 */
	ErrorCode load_image_from(darx& darx, FILE* file);
	
	
	/** Saves a darx data archive to a file.
	 * 
	 * @param[in] darx - reference to the darx structure to store
	 * @param[in] file - pointer to the file where to store the darx data
	 * 
	 * @returns true if the darx file could be saved
	 */
	bool save_image_to(darx& darx, FILE* file);
};

#endif