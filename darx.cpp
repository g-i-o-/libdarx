#include "darx.h"
#include <iostream>
#include <assert.h>

// magic number for darx files
#define DARX_MAGIC "DARX"
#define DARX_MAGIC_LEN 4
// endianness test "LIVE" is big endian, "EVIL" is little endian
#define DARX_MAGIC_BE 0x4c495645

namespace darx{
	int VERBOSE;
	CustomElementTypeStruct UNKNOWN_TYPE(TYPE_CUSTOM, 1, 8, "unknown");
	
	const char* errors[]={
		"success", "unsupported element type", "unsuported compression type",
		"invalid structure"
	};
	
	
	// structure used for mixed types
	MixedElementTypeStruct::MixedElementTypeStruct(
		ElementType _type, uint8_t _components, uint8_t _bit_width
	): ElementTypeStruct(_type, _components, _bit_width){
		subtypes = new ElementTypeStruct*[_components];
	}
	MixedElementTypeStruct::~MixedElementTypeStruct(){
		if(subtypes){
			delete[] subtypes;
			subtypes = 0;
		}
	};	
	// structure used for custom types
	CustomElementTypeStruct::CustomElementTypeStruct(
		ElementType _type, uint8_t _components, uint8_t _bit_width, const char* _type_name
	): ElementTypeStruct(_type,_components,_bit_width) {
		int len=strlen(_type_name);
		type_name=new char[len+1];
		for(int i=0; i<len; i++){type_name[i] = _type_name[i];}
	}
	
	typedef struct {
		bool swapEndian;
		uint8_t int_size;
		uint8_t long_size;
	} data_type_info;
	
	bool write_tensor_type(ElementTypeStruct* tensor_type, FILE* file){
		if(!tensor_type){
			return false;
		}
		uint8_t ttt = (uint8_t)tensor_type->type;
if(VERBOSE){ std::cout << "#    element type : " << ((int)ttt) << std::endl; }
		fwrite(&ttt, sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#       comps : " << ((int)tensor_type->components) << std::endl; }
		fwrite(&(tensor_type->components), sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#       bitwidth : " << ((int)tensor_type->bit_width) << std::endl; }
		fwrite(&(tensor_type->bit_width ), sizeof(uint8_t), 1, file);
		switch(tensor_type->type){
			case TYPE_INT: case TYPE_UINT: case TYPE_FLOAT: case TYPE_CHAR:
			break;
			case TYPE_MIXED:{
				int components = tensor_type->components;
				MixedElementTypeStruct* mixedtype = (MixedElementTypeStruct*)tensor_type;
				for(int comp_idx=0; comp_idx < components; comp_idx++){
					write_tensor_type(mixedtype->subtypes[comp_idx], file);
				}
			} break;
			case TYPE_CUSTOM:{
				CustomElementTypeStruct* customtype = (CustomElementTypeStruct*)tensor_type;
				const char* type_name = customtype->type_name ? customtype->type_name : "Unknown";
				uint8_t namelen = strlen(type_name);
				fwrite(&namelen, sizeof(uint8_t), 1, file);
				fwrite(type_name, sizeof(char), namelen, file);
			} break;
			default:
				return UNSUPPORTED_ELEMENT_TYPE;
		}
		return true;
	}
	
	ElementTypeStruct* read_tensor_type(darx& darx, FILE* file, data_type_info& dtinfo){
		uint8_t tensor_type_tag, components, bit_width;
		fread(&tensor_type_tag, sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#    element type : " << ((int)tensor_type_tag) << std::endl; }
		fread(&(components), sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#       comps : " << ((int)components) << std::endl; }
		fread(&(bit_width ), sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#       bitwidth : " << ((int)bit_width) << std::endl; }
		switch(tensor_type_tag){
			case TYPE_INT: case TYPE_UINT: case TYPE_FLOAT: case TYPE_CHAR:
				return new ElementTypeStruct((ElementType)tensor_type_tag, components, bit_width);
			break;
			case TYPE_MIXED:{
				MixedElementTypeStruct* mixedtype = new MixedElementTypeStruct((ElementType)tensor_type_tag, components, bit_width);
				for(int comp_idx=0; comp_idx < components; comp_idx++){
					mixedtype->subtypes[comp_idx] = read_tensor_type(darx, file, dtinfo);
				}
				return mixedtype;
			} break;
			case TYPE_CUSTOM:{
				uint8_t namelen;
				fread(&namelen, sizeof(uint8_t), 1, file);
				char* type_name = new char[namelen+1];
				fread(type_name, sizeof(char), namelen, file);
				type_name[namelen] = 0;
				
				CustomElementTypeStruct* customtype = new CustomElementTypeStruct(
					(ElementType)tensor_type_tag, components, bit_width, type_name
				);
				return customtype;
			} break;
			default:
				return 0;
		}		
	}
	
	bool compress_data(datatensor& tensor, uint8_t** cdata, unsigned int* cdata_len, bool* cdata_is_temp);
	bool decompress_data(datatensor& tensor, uint8_t** cdata, unsigned int* cdata_len, bool* cdata_is_temp);
	
	int write_tensor(datatensor& tensor, FILE* file){
		// write tensor's name
		uint8_t namelen = tensor.name ? strlen(tensor.name) : 0;
		fwrite(&namelen, sizeof(uint8_t), 1, file);
		if(tensor.name){
if(VERBOSE){ std::cout << "#    name : " << tensor.name << std::endl; }
			fwrite(tensor.name, sizeof(char), namelen, file);
		}
		// write the tensor's rank
if(VERBOSE){ std::cout << "#    rank : " << ((int)tensor.rank) << std::endl; }
		fwrite(&(tensor.rank), sizeof(uint8_t), 1, file);
		// write the length of each dimmension 
if(VERBOSE){ std::cout << "#    lengths : "; for(int i=0; i < tensor.rank; i++){ std::cout << tensor.lengths[i] << "   "; } std::cout << std::endl; }
		fwrite(tensor.lengths, sizeof(unsigned int), tensor.rank, file);
		// write the element type stuct
		if(!write_tensor_type(tensor.type, file)){
			return UNSUPPORTED_ELEMENT_TYPE;
		}
		// write the data (possibly compressed)
		{
			bool cdata_is_temp=false;
			unsigned int cdata_length=0;
			uint8_t* cdata=0;
			if(!tensor.data){
				return INVALID_STRUCT;
			}
			if(!compress_data(tensor, &cdata, &cdata_length, &cdata_is_temp)){
if(VERBOSE){ std::cout << "#    cdata size:  " << cdata_length << std::endl; }
				return UNSUPPORTED_COMPRESS_TYPE;
			}
			uint8_t ctype = (uint8_t)tensor.compression;
			fwrite(&ctype, sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#    cdata size:  " << cdata_length << std::endl; }
			fwrite(&cdata_length, sizeof(unsigned int), 1, file);
if(VERBOSE){ std::cout << "#    cdata :  " << ((void*)cdata) << std::endl; }
			fwrite(cdata, cdata_length, 1, file);
			if(cdata_is_temp){
if(VERBOSE){ std::cout << "#    deleting temp cdata...  " << cdata << std::endl; }
				delete[] cdata;
			}
		}
		return SUCCESS;
	}
	
	bool compress_data(datatensor& tensor, uint8_t** cdata, unsigned int* cdata_len, bool* cdata_is_temp){
		CompressionType compression = tensor.compression;
		if(compression == UNCOMPRESSED){
if(VERBOSE){ std::cout << "#    [no compression] " << std::endl; }
			(*cdata) = (uint8_t*)tensor.data;
			(*cdata_len) = tensor.data_size;
			(*cdata_is_temp) = false;
			return true;
		}
		return false;
	}

	bool decompress_data(datatensor& tensor, uint8_t** cdata, unsigned int* cdata_len, bool* cdata_is_temp){
		CompressionType compression = tensor.compression;
		if(compression == UNCOMPRESSED){
if(VERBOSE){ std::cout << "#    [no compression] " << std::endl; }
			tensor.data = (*cdata);
			tensor.data_size = (*cdata_len);
			(*cdata_is_temp) = false;
			return true;
		}
		return false;
	}
		
	/**
	 * Indicates whether a given open file contains .darx data.
	 * 
	 * @return true if the data pointed by the file pointer is a .darx file,
	 * 	false otherwise.
	 */
	bool read_tensor(datatensor& tensor, darx& darx, FILE* file, data_type_info& dtinfo){
if(VERBOSE){ std::cout << "# file pos:  " << ftell(file) << std::endl; }
		uint8_t namelen=0;
		fread(&namelen, sizeof(uint8_t), 1, file);
		if(namelen > 0){
			char* tensor_name = new char[namelen+1];
			tensor.name=tensor_name;
			fread(tensor_name, sizeof(uint8_t), namelen, file);
			tensor_name[namelen]=0;
if(VERBOSE){ std::cout << "#    name : " << tensor.name << std::endl; }
		} else {
if(VERBOSE){ std::cout << "#    (unnamed)" << std::endl; }
		}
		// write the tensor's rank
		fread(&(tensor.rank), sizeof(uint8_t), 1, file);
if(VERBOSE){ std::cout << "#    rank : " << ((int)tensor.rank) << std::endl; }
		// write the length of each dimmension
		tensor.lengths = new unsigned int[tensor.rank];
if(VERBOSE){ std::cout << "#    lengths : "; }
		for(int dim_idx=0; dim_idx < tensor.rank; dim_idx++){
			tensor.lengths[dim_idx]=0;
			fread(&(tensor.lengths[dim_idx]), dtinfo.int_size, 1, file);
			if(VERBOSE){ std::cout << tensor.lengths[dim_idx] << "   "; }
		}
if(VERBOSE){ std::cout << std::endl; }
		// write the element type stuct
		tensor.type = read_tensor_type(darx, file, dtinfo);
		if(!tensor.type){
			return UNSUPPORTED_ELEMENT_TYPE;
		}
		// write the data (possibly compressed)
		{
			uint8_t ctype;
			fread(&ctype, sizeof(uint8_t), 1, file);
			tensor.compression = (CompressionType)ctype;
if(VERBOSE){ std::cout << "#    compression :  " << ((int)ctype) << std::endl; }
			bool cdata_is_temp=false;
			unsigned int cdata_length;
			fread(&cdata_length, sizeof(unsigned int), 1, file);
if(VERBOSE){ std::cout << "#    cdata size:  " << cdata_length << std::endl; }
			uint8_t* cdata = new uint8_t[cdata_length];
			fread(cdata, cdata_length, 1, file);
if(VERBOSE){ std::cout << "#    cdata :  " << ((void*)cdata) << std::endl; }
			if(!decompress_data(tensor, &cdata, &cdata_length, &cdata_is_temp)){
				return UNSUPPORTED_COMPRESS_TYPE;
			}
			if(cdata_is_temp){
if(VERBOSE){ std::cout << "#    deleting temp cdata...  " << cdata << std::endl; }
				delete[] cdata;
			}
		}
		return SUCCESS;
	}
	
	bool is_darx(FILE* file){
		char magic[DARX_MAGIC_LEN];
		fread(magic, sizeof(char), DARX_MAGIC_LEN, file); // read magic number
		fseek(file, 0, SEEK_SET); // reset the file read position
		return !strncmp(magic, DARX_MAGIC, DARX_MAGIC_LEN);
	}
	
	
	/**  Loads a darx data archive from a file into the structure.
	 * 
	 * @param[in] darx - reference to the darx structure to fill
	 * @param[in] file - pointer to the file containing the darx data
	 * 
	 * @returns an error code indicating success or reson of failure
	 * @note reads only uncompressed 8,16 and 32 bit-per-pixel bmp images.
	 */
	ErrorCode load_image_from(darx& darx, FILE* file){
		data_type_info dtinfo;
		char magic[DARX_MAGIC_LEN+1];
		fread(magic, sizeof(char), DARX_MAGIC_LEN, file);
		magic[DARX_MAGIC_LEN] = 0;
if(VERBOSE){ std::cout << "# read magic number : '" << magic << "' == " << DARX_MAGIC << std::endl; }
if(VERBOSE){ std::cout << "#     " << magic[0] << magic[1] << magic[2] << magic[3] << std::endl; }
		if(strncmp(magic, DARX_MAGIC, DARX_MAGIC_LEN)){
			return INVALID_STRUCT;
		}
		char magic2[4];
		fread(&magic2, sizeof(char), 4, file);
		bool storedAsBE = ( magic2[3] ==  ((uint8_t)( DARX_MAGIC_BE&0xff)));
		darx.isBigEndian = storedAsBE;
if(VERBOSE){ std::cout << "# file endianness ["<<magic2[3]<<" == " << ((uint8_t)(DARX_MAGIC_BE&0xff)) << "]: " << (storedAsBE ? "big" : "little") << std::endl; }
		int endianness_i = 0x00010203;
		bool systemIsBE = ( ((char*)&endianness_i)[0] ==  0x00);
if(VERBOSE){ std::cout << "# system endianness : " << (systemIsBE ? "big" : "little") << std::endl; }
		
		dtinfo.swapEndian = (systemIsBE == storedAsBE);
		if(dtinfo.swapEndian){
if(VERBOSE){ std::cout << "# need to swap endianness.." << std::endl; }
		}
		fread(&dtinfo.int_size, sizeof(uint8_t), 1, file);
		fread(&dtinfo.long_size, sizeof(uint8_t), 1, file);
		
if(VERBOSE){ std::cout << "# data sizes [int:" << ((int)dtinfo.int_size) << ", long:" << ((long)dtinfo.long_size) << "]" << std::endl; }
		fread(&darx.number_of_tensors, sizeof(uint16_t), 1, file);
if(VERBOSE){ std::cout << "# tensors : " << darx.number_of_tensors << std::endl; }
		long int *tensor_indices  = new long int[darx.number_of_tensors];
		assert(sizeof(long int) >= dtinfo.long_size);
if(VERBOSE){ std::cout << "#  tensor file locations : ["; }
		for(int i=0; i < darx.number_of_tensors; i++){
			fread(&(tensor_indices[i]), dtinfo.long_size, 1, file);
if(VERBOSE){ std::cout << tensor_indices[i] << "   "; }
		}
if(VERBOSE){ std::cout << "]" << std::endl; }
		fread(&darx.metadata_size, sizeof(uint16_t), 1, file);
		if(darx.metadata_size > 0){
if(VERBOSE){ std::cout << "# metadata size:" << darx.metadata_size << std::endl; }
			darx.metadata = new char[darx.metadata_size];
			fread(&darx.metadata, 1, darx.metadata_size, file);
		} else {
if(VERBOSE){ std::cout << "# no metadata (size:0)." << std::endl; }
			darx.metadata = 0;
		}
if(VERBOSE){ std::cout << "# readin tensors ("<< darx.number_of_tensors <<")." << std::endl; }
		darx.tensors = new datatensor[darx.number_of_tensors];
		for(int tensor_idx=0; tensor_idx < darx.number_of_tensors; tensor_idx++){
			fseek(file, tensor_indices[tensor_idx], SEEK_SET);
			read_tensor(darx.tensors[tensor_idx], darx, file, dtinfo);
		}
		
if(VERBOSE){ std::cout << "# darx file read successfully." << std::endl; }
		delete[] tensor_indices;
		return SUCCESS;
	}
	
	
	/** Saves a darx data archive to a file.
	 * 
	 * @param[in] darx - reference to the darx structure to store
	 * @param[in] file - pointer to the file where to store the darx data
	 * 
	 * @returns true if the darx file could be saved
	 */
	bool save_image_to(darx& darx, FILE* file){
		if(!darx.valid){
			return false;
		}
		
		// store magic number
		const char* magic = DARX_MAGIC;
if(VERBOSE){ std::cout << "# image magic number : " << magic << std::endl; }
		fwrite(magic, sizeof(char), DARX_MAGIC_LEN, file);
		// store an endianness test in the file: 
		uint32_t int_magic2 = DARX_MAGIC_BE; // "LIVE" in hex
if(VERBOSE){ std::cout << "# image magic number, endianed : " << int_magic2 << std::endl; }
		fwrite(&int_magic2, sizeof(uint32_t), 1, file);
		// write the size of an integer (in an 8-bit int)
		uint8_t int_size = sizeof(unsigned int);
if(VERBOSE){ std::cout << "# int size : " << int_size << std::endl; }
		fwrite(&int_size, sizeof(uint8_t), 1, file);
		// write the size of a long int (in a 8-bit int)
		uint8_t long_size = sizeof(long int);
if(VERBOSE){ std::cout << "# long size : " << long_size << std::endl; }
		fwrite(&long_size, sizeof(uint8_t), 1, file);
		// count how many tensors we were given, and store the number
if(VERBOSE){ std::cout << "# tensors : " << darx.number_of_tensors << std::endl; }
		fwrite(&darx.number_of_tensors, sizeof(uint16_t), 1, file);
		// record the file offset of the archive's tensor index.
		long int tensors_index_pos = ftell(file);
		// leave a space in the file for the tensors index
if(VERBOSE){ std::cout << "# tensors index filepos : " << tensors_index_pos << std::endl; }
		fseek(file, darx.number_of_tensors * sizeof(long int), SEEK_CUR);
		// write out any metadata that may be added to the file
if(VERBOSE){ std::cout << "# metadata : " << ((void*)darx.metadata) << "(size : " << darx.metadata_size << ")" << std::endl; }
		fwrite(&darx.metadata_size, sizeof(uint16_t), 1, file);
		fwrite(darx.metadata, 1, darx.metadata_size, file);
		// write the tensors to the file, one by one
		for(int tensor_idx=0; tensor_idx < darx.number_of_tensors; tensor_idx++){
			long int tensor_pos = ftell(file);
if(VERBOSE){ std::cout << "# tensor["<<tensor_idx<<"] @ file pos : " << tensor_pos << std::endl; }
			fseek(file, tensors_index_pos, SEEK_SET); // get file position of tensor
			fwrite(&tensor_pos, sizeof(long int), 1, file); // write it in the index
			tensors_index_pos += sizeof(long int);
			fseek(file, tensor_pos, SEEK_SET); // seek back to the tensor's file position
			// write the tensor
			int write_result = write_tensor(darx.tensors[tensor_idx], file);
			if(write_result != SUCCESS){
				return write_result;
			}
		}
		return SUCCESS;
	}
	
	
	
	
};
