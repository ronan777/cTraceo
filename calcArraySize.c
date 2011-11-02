#pragma once
#include <stdio.h>
#include <stdint.h>
#include "matlabOut.h"
#include "dataElementSize.c"

uint32_t	calcArraySize(mxArray* inArray);


uint32_t	calcArraySize(mxArray* inArray){
	/*
	 * Calculates the total size in Bytes which is required to
	 * contain a struct array and its children.
	 * NOTE: this function is recursive, as it will call itself to
	 * 		 determine the size required by the struct's children.
	 * NOTE: an mxArray's name is not known before the call to
	 * 		 matPutVariable which writes it to a file, so the
	 * 		 number of bytes needed to write an mxArray's name 
	 *  	 is not calculated in this function.
	 * 		 This is actually not very important, as array names
	 * 		 are only written for mxArrays which aren't members
	 * 		 in a structure.
	 */
	
	uintptr_t	nBytes = 0;
	uintptr_t	maxLengthFieldname;
	
	// Array Flags[16B] + Dimensions Array [16B]
	nBytes += 4*8;
	
	//Handle mxArrays which are structs:
	if (inArray->isStruct){
		/*
		 * Determine size required for the fieldnames:
		 */
		//get length of longest fieldname
		for (uintptr_t i=0; i<inArray->nFields; i++){
			maxLengthFieldname = max(maxLengthFieldname, strlen(inArray->fieldNames[i]));
		}
		
		//need to add 1B for the string's NULL terminator
		maxLengthFieldname += 1;
		
		nBytes += maxLengthFieldname * inArray->nFields;
		
		// determine padding required for the fieldnames_
		if (maxLengthFieldname * inArray->nFields % 8 > 0){
			//add padding to size
			nBytes += 8 - nBytes % 8;
		}
		
		/*
		 * Determine size required for the structure's children
		 */
		for (uintptr_t i=0; i<inArray->nFields; i++){
			nBytes += calcArraySize(&inArray->field[i]);
		}
	
	// Handle mxArrays which aren't structures:
	}else{
		// determine size required for the actual data:
		if (inArray->mxCLASS == mxCHAR_CLASS){
			/*
			 * NOTE: mxCHAR_CLASS is strange: although datatype
			 * 		 is 'char' (which should be 1B) 2B are
			 * 		 written per character
			 */
			nBytes += dataElementSize(2*sizeof(char), inArray->dims[0]*inArray->dims[1]);
		}else{
			/*
			 * All other data types [seem to] behave as expected
			 */
			nBytes += dataElementSize(inArray->dataElementSize, inArray->dims[0]*inArray->dims[1]);
			
			//if data is complex, get size of imaginary part
			if (inArray->numericType == mxCOMPLEX){
				nBytes += dataElementSize(inArray->dataElementSize, inArray->dims[0]*inArray->dims[1]);
			}
		}
	}
	
	//save the size of the mxArray's contents
	inArray->nBytes = nBytes;
	
	/*
	 * NOTE: the size given in an mxArray's header does not
	 * 		 include the 8 bytes of the header itself, thus
	 * 		 an additional 8B are actually required to write the
	 * 		 mxArray's header. However, i case of nested
	 * 		 structures, the 8B for the header are needed.
	 */
	return nBytes +8;
}
