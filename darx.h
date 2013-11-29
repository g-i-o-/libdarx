/**
 * @file
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
	/** Value indicating the verbosity when reading/writing a darx data archive.
	 * 0 - no verbosity
	 * non-zero - diagnostic messages
	 */
	extern int VERBOSE;
	
	/** Describes the available types and sizes of pixels in an image.
	 *  Each type has an associated size and data structure.
	 */
	enum ElementType {
		/** Type is specified by an ElementTypeStruct, elements are signed integers. */
		TYPE_INT=0,
		/** Type is specified by an ElementTypeStruct, elements are unsigned integers. */
		TYPE_UINT=1,
		/** Type is specified by an ElementTypeStruct, elements are floating point numbers (ieee format). */
		TYPE_FLOAT=2,
		/** Type is specified by an ElementTypeStruct, elements are characters (As in a string of...) */
		TYPE_CHAR=3,
		/** Type is specified by a MixedElementTypeStruct. */
		TYPE_MIXED=4,
		/** Type is specified by a CustomElementTypeStruct. */
		TYPE_CUSTOM=5
	};
	/** Available Compression algorithms.
	 */
	enum CompressionType {
		/** No compression is used for the data. */
		UNCOMPRESSED=0
	};
	
	/** Structure used for describing simple element types.
	 * Can be used for describing signed/unsigned integer, floating point and character string types.
	 */
	class ElementTypeStruct{
		public:
		/** Element Type enumeration describing the data type of each component in the data element.
		 *  (e.g. each value in an RGB pixel is an unsigned integer (TYPE_UINT) )
		 */
		ElementType type;
		/** Number of values of the given type per data element.
		 * A data tensor may have more than one value per data element, this number
		 * indicates how many of such values each data element has.
		 *  (e.g. an RGB pixel has three components per pixel)
		 */
		uint8_t components;
		/** Size, in bits, of each value described in this element type.
		 *  (e.g. each component in a 24-bit RGB pixel is 8 bits in size.)
		 */
		uint8_t bit_width;
		/** Construct an ElementTypeStruct for the given type, number of components and bit width.
		 */
		inline ElementTypeStruct(ElementType _type, uint8_t _components, uint8_t _bit_width):
			type(_type), components(_components), bit_width(_bit_width){}
	};
	/** Structure used for mixed types. A mixed type describes each data element as a sequence
	 *  of values, each described by it's own ElementTypeStruct.
	 */
	class MixedElementTypeStruct: public ElementTypeStruct{
		public:
		/** The subtypes describing each component in a data element.
		 *  This array is of size components, each entry describing the i'th component in the
		 *  data element.
		 */
		ElementTypeStruct** subtypes;
		
		/** Construct an MixedElementTypeStruct for the given type, number of components and bit width.
		 */
		MixedElementTypeStruct(ElementType _type, uint8_t _components, uint8_t _bit_width);
		~MixedElementTypeStruct();
	};
	/** Structure used for custom data element types.
	 *  A custom type may organize the data element however it sees fit. A name is used to
	 *  identify the actual type used, but this use is other-wise application-specific.
	 */
	class CustomElementTypeStruct: public ElementTypeStruct{
		public:
		/** Name uniquely describing the data element's custom type.
		 */
		char* type_name;
		
		/** Construct an CustomElementTypeStruct for the given type, number of components,d bit width and name.
		 */
		CustomElementTypeStruct(ElementType _type, uint8_t _components, uint8_t _bit_width, const char* _type_name);
	};
	
	/** Low-level structure representing a tensor of data.
	 * A tensor is a (possibly) multi-dimmensional array of data,
	 * with each element of the tensor belong to some datatype.
	 * This tensor has an associated name, rank (it's dimmensionality),
	 * and size (in all it's dimmensions).
	 * The data may be compressed using one of the available algorithms
	 * (none at the moment).
	 */
	typedef struct {
		/** name associated to this data tensor. */
		const char* name;
		/** number of dimmensions in this data tensor. */
		uint8_t rank;
		/** the lengths of each dimmension in the tensor. */
		unsigned int* lengths;
		/** types of each element in the struct. */
		ElementTypeStruct* type;
		/** Compression algorithm used on this tensor's data. */
		CompressionType compression;
		/** total size of the compressed stored data. */
		unsigned int data_size;
		/** The tensor's data. */
		void* data;
	} datatensor;
	
	/** Data structure representing a darx data archive.
	 */
	typedef struct {
		/** Wether this structure is valid or not. (runtime flag). */
		bool valid;
		/** Wether this archive was stored in a big endian computer, or not. */
		bool isBigEndian;
		/**  number of tensors in the data archive. */
		uint16_t number_of_tensors;
		/**  size of the header's metadata. */
		uint16_t metadata_size;
		/**  Archive metadata string. */
		char* metadata;
		/** array of tensor in this darx structure. */
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
	
	/** Constant specifying an Unknown Element Type.
	 * The default unknown type is just a CustomElementTypeStruct with name "unknown".
	 */
	extern CustomElementTypeStruct UNKNOWN_TYPE;
	
	/** Indicates whether a given open file contains .darx data.
	 * 
	 * @return true if the data pointed by the file pointer is a .darx file,
	 * 	false otherwise.
	 */
	bool is_darx(FILE* file);
		
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