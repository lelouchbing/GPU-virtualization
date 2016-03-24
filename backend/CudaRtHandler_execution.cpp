/*
 * gVirtuS -- A GPGPU transparent virtualization component.
 *
 * Copyright (C) 2009-2010  The University of Napoli Parthenope at Naples.
 *
 * This file is part of gVirtuS.
 *
 * gVirtuS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gVirtuS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gVirtuS; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Written by: Giuseppe Coviello <giuseppe.coviello@uniparthenope.it>,
 *             Department of Applied Science
 */

#include "CudaRtHandler.h"
#include <stdio.h>

CUDA_ROUTINE_HANDLER(ConfigureCall) {
    /* cudaError_t cudaConfigureCall(dim3 gridDim, dim3 blockDim,
     * size_t sharedMem, cudaStream_t stream) */
    dim3 gridDim = input_buffer->Get<dim3>();
    dim3 blockDim = input_buffer->Get<dim3>();
    size_t sharedMem = input_buffer->Get<size_t>();
    cudaStream_t stream = input_buffer->Get<cudaStream_t>();

    cudaError_t exit_code = cudaConfigureCall(gridDim, blockDim, sharedMem,
            stream);

    return new Result(exit_code);
}

#ifndef CUDART_VERSION
#error CUDART_VERSION not defined
#endif
#if CUDART_VERSION >= 2030
CUDA_ROUTINE_HANDLER(FuncGetAttributes) {
    cudaFuncAttributes *guestAttr = input_buffer->Assign<cudaFuncAttributes>();
    char *handler = input_buffer->AssignString();
    const char *entry = pThis->GetDeviceFunction(handler);
    Buffer * out = new Buffer();
    cudaFuncAttributes *attr = out->Delegate<cudaFuncAttributes>();
    memmove(attr, guestAttr, sizeof(cudaFuncAttributes));
    
    cudaError_t exit_code = cudaFuncGetAttributes(attr, entry);

    return new Result(exit_code, out);
}
#endif

CUDA_ROUTINE_HANDLER(Launch) {
    int ctrl;

    // cudaConfigureCall
    ctrl = input_buffer->Get<int>();
    if(ctrl != 0x434e34c)
        throw "Expecting cudaConfigureCall";

    dim3 gridDim = input_buffer->Get<dim3>();
    dim3 blockDim = input_buffer->Get<dim3>();
    size_t sharedMem = input_buffer->Get<size_t>();
    cudaStream_t stream = input_buffer->Get<cudaStream_t>();

    cudaError_t exit_code = cudaConfigureCall(gridDim, blockDim, sharedMem,
            stream);

    if(exit_code != cudaSuccess)
        return new Result(exit_code);

	ctrl = input_buffer->Get<int>();
    // cudaSetupArgument
    while((ctrl = input_buffer->Get<int>()) == 0x53544147) {
   
        void *arg = input_buffer->AssignAll<char>();
        size_t size = input_buffer->Get<size_t>();
        size_t offset = input_buffer->Get<size_t>();
	//whb
	//printf("%s %d %d\n", (char*)arg, size, offset);


        exit_code = cudaSetupArgument(arg, size, offset);
	
        if(exit_code != cudaSuccess)
            return new Result(exit_code);
    }

	//ctrl = input_buffer->Get<int>();
    // cudaLaunch
    if(ctrl != 0x4c41554e)
	{
	//printf("%d",ctrl);
        throw "Expecting cudaLaunch";
	}

    char *handler = input_buffer->AssignString();
    const char *entry = pThis->GetDeviceFunction(handler);
    exit_code = cudaLaunch(entry);

    return new Result(exit_code);
}

CUDA_ROUTINE_HANDLER(SetDoubleForDevice) {
    double *guestD = input_buffer->Assign<double>();
    Buffer *out = new Buffer();
    double *d = out->Delegate<double>();
    memmove(d, guestD, sizeof(double));

    cudaError_t exit_code = cudaSetDoubleForDevice(d);

    return new Result(exit_code, out);
}

CUDA_ROUTINE_HANDLER(SetDoubleForHost) {
    double *guestD = input_buffer->Assign<double>();
    Buffer *out = new Buffer();
    double *d = out->Delegate<double>();
    memmove(d, guestD, sizeof(double));

    cudaError_t exit_code = cudaSetDoubleForHost(d);

    return new Result(exit_code, out);
}

CUDA_ROUTINE_HANDLER(SetupArgument) {
    /* cudaError_t cudaSetupArgument(const void *arg, size_t size, size_t offset) */
    size_t offset = input_buffer->BackGet<size_t>();
    size_t size = input_buffer->BackGet<size_t>();
    void *arg = input_buffer->Assign<char>(size);

    cudaError_t exit_code = cudaSetupArgument(arg, size, offset);

    return new Result(exit_code);
}
