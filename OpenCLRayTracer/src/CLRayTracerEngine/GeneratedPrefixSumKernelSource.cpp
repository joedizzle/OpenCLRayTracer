const char* PrefixSumKernelSource = 
"/**********************************************************************\n"
"Copyright �2015 Advanced Micro Devices, Inc. All rights reserved.\n"
"\n"
"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:\n"
"\n"
"�	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.\n"
"�	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or\n"
" other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n"
" WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY\n"
" DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS\n"
" OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
" NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
"********************************************************************/\n"
"\n"
"/*\n"
" * Work-efficient compute implementation of scan, one thread per 2 elements\n"
" * O(log(n)) stepas and O(n) adds using shared memory\n"
" * Uses a balanced tree algorithm. See Belloch, 1990 \"Prefix Sums\n"
" * and Their Applications\"\n"
" * @param output	output data \n"
" * @param input		input data\n"
" * @param block		local memory used in the kernel\n"
" * @param length	lenght of the input data\n"
" * @param idxOffset	offset between two consecutive index.\n"
" */\n"
"__kernel \n"
"void group_prefixSum(__global uint * output,\n"
"					 __global uint * input,\n"
"					 const int idxOffset,\n"
"					  const int size,\n"
"					 __local  uint * block) {\n"
"	int localId = get_local_id(0);\n"
"	int localSize = get_local_size(0);\n"
"	int globalIdx = get_group_id(0);\n"
"\n"
"	// Cache the computational window in shared memory\n"
"	globalIdx = (idxOffset *(2 *(globalIdx*localSize + localId) +1)) - 1;\n"
"	if(globalIdx < size)             { block[2*localId]     = input[globalIdx];				}\n"
"    if(globalIdx + idxOffset < size) { block[2*localId + 1] = input[globalIdx + idxOffset];	}\n"
"\n"
"	// Build up tree \n"
"	int offset = 1;\n"
"	for(int l = size>>1; l > 0; l >>= 1)\n"
"	{\n"
"	  barrier(CLK_LOCAL_MEM_FENCE);\n"
"	  if(localId < l) {\n"
"            int ai = offset*(2*localId + 1) - 1;\n"
"            int bi = offset*(2*localId + 2) - 1;\n"
"            block[bi] += block[ai];\n"
"         }\n"
"         offset <<= 1;\n"
"	}\n"
"		 \n"
"	if (size > 2)\n"
"	{\n"
"		if(offset < size) { offset <<= 1; }\n"
"\n"
"		// Build down tree\n"
"		int maxThread = offset>>1;\n"
"		for(int d = 0; d < maxThread; d<<=1)\n"
"		{\n"
"			d += 1;\n"
"			offset >>=1;\n"
"			barrier(CLK_LOCAL_MEM_FENCE);\n"
"\n"
"			if(localId < d) {\n"
"				int ai = offset*(localId + 1) - 1;\n"
"				int bi = ai + (offset>>1);\n"
"				block[bi] += block[ai];\n"
"			}\n"
"		}\n"
"	}\n"
"	barrier(CLK_LOCAL_MEM_FENCE);\n"
"	\n"
"    // write the results back to global memory\n"
"    if(globalIdx < size)           { output[globalIdx]             = block[2*localId];		}\n"
"    if(globalIdx+idxOffset < size) { output[globalIdx + idxOffset] = block[2*localId + 1];	}\n"
"}\n"
"\n"
"/*\n"
" * Work-efficient compute implementation of scan, one thread per 2 elements\n"
" * O(log(n)) stepas and O(n) adds using shared memory\n"
" * Uses a balanced tree algorithm. See Belloch, 1990 \"Prefix Sums\n"
" * and Their Applications\"\n"
" * @param buffer	input/output data \n"
" * @param offset	Multiple of Offset positions are already updated by group_prefixSum kernel\n"
" * @param length	lenght of the input data\n"
" */\n"
"__kernel\n"
"void global_prefixSum(__global uint * buffer,\n"
"                      const int offset,\n"
"					  const int size) {\n"
"	int localSize = get_local_size(0);\n"
"    int groupIdx  = get_group_id(0);\n"
"\n"
"	int sortedLocalBlocks = offset / localSize;		// sorted groups per block\n"
"	// Map the gids to unsorted local blocks.\n"
"	int gidToUnsortedBlocks = groupIdx + (groupIdx / ((offset<<1) - sortedLocalBlocks) +1) * sortedLocalBlocks;\n"
"\n"
"	// Get the corresponding global index\n"
"    int globalIdx = (gidToUnsortedBlocks*localSize + get_local_id(0));\n"
"	if(((globalIdx+1) % offset != 0) && (globalIdx < size))\n"
"		buffer[globalIdx] += buffer[globalIdx - (globalIdx%offset + 1)];\n"
"}\n"
"\n"
"\n"
;
