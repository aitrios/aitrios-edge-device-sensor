/*
 * SPDX-FileCopyrightText: 2023-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __NuttX__
#error Confirm that you want to use stub with TARGET DEVICE
#endif

#ifndef ESF_MEMORY_MANAGER_INCLUDE_MEMORY_MANAGER_H_
#define ESF_MEMORY_MANAGER_INCLUDE_MEMORY_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>  // for free
#include <string.h>  // for memcpy
#include <sys/types.h>
#if 0
#include <sys/queue.h>
#include <wasm_export.h>  // for wasm_exec_env_t, wasm_runtim...
#else
struct WASMExecEnv;
typedef struct WASMExecEnv *wasm_exec_env_t;
#endif

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kEsfMemoryManagerResultSuccess = 0,
  kEsfMemoryManagerResultParamError,
  kEsfMemoryManagerResultAllocationError,
  kEsfMemoryManagerResultMapError,
  kEsfMemoryManagerResultFileIoError,
  kEsfMemoryManagerResultNotSupport,
  kEsfMemoryManagerResultOperationError,
  kEsfMemoryManagerResultOtherError
} EsfMemoryManagerResult;

// This code defines an enumeration for specifying regions when allocating
// memory.
typedef enum {
  kEsfMemoryManagerTargetLargeHeap = 0,
  kEsfMemoryManagerTargetDma,
  kEsfMemoryManagerTargetWasmHeap,
  kEsfMemoryManagerTargetOtherHeap
} EsfMemoryManagerTargetArea;

// This code defines an enumeration for specifying whether or not Map features
// are supported.
typedef enum {
  kEsfMemoryManagerMapIsSupport = 0,
  kEsfMemoryManagerMapIsNotSupport
} EsfMemoryManagerMapSupport;

// This code defines an enumeration for specifying memory usage when allocating
// Wasm memory.
typedef enum {
  kEsfMemoryManagerWasmAllocForRuntime = 0,
  kEsfMemoryManagerWasmAllocForLinearMemory
} EsfMemoryManagerWasmMemoryUsage;

// Definition of memory operation handles for LargeHeap/DMA/WasmHeap area.
typedef uint32_t EsfMemoryManagerHandle;
// EsfMemoryManagerHandle details
//  |<---- 7bit ----->|<---------------------- 25bit ---------------------->|
//  +-----------------+-----------------+-----------------+-----------------+
//  |    handle_id    |                   address_offset                    |
//  +-----------------+-----------------+-----------------+-----------------+
//  handle_id      : 0,1-127
//  address_offset : 0x0000000-0x1FFFFFF(32MB)

// macro for getting handle_id.
#define MEMORY_MANAGER_HANDLE_ID_MASK(handle) (uint32_t)(handle & 0xfe000000)
#define MEMORY_MANAGER_HANDLE_ID(handle) \
  (uint8_t)((handle >> 25) & (0x0000007f))
// macro for getting address_offset.
#define MEMORY_MANAGER_OFFSET_MASK (uint32_t)(0x01ffffff)
#define MEMORY_MANAGER_HANDLE_OFFSET(handle) \
  (uint32_t)(handle & MEMORY_MANAGER_OFFSET_MASK)

// Definition of memory pointer to the AppMemory area.
typedef void *EsfMemoryManagerAppMemory;

// Definition of memory operation handle information (area, size)
typedef struct {
  EsfMemoryManagerTargetArea target_area;  // target memory area
  int32_t allocate_size;                   // allocated memory size
} EsfMemoryManagerHandleInfo;

// """ Memory allocation.
// This function is used to allocate memory in the specified target area
// (WasmHeap, LargeHeap or DMA) of the system. The allocated memory can be
// identified by a handle which will be returned after successful allocation.

// Args:
//    target_area: An enum value specifying the target area for memory
//      allocation. Can be one of kEsfMemoryManagerTargetWasmHeap,
//      kEsfMemoryManagerTargetLargeHeap or kEsfMemoryManagerTargetDMA.
//    exec_env: A pointer to a wasm execution environment structure
//      (wasm_exec_env_t). This is required for WasmHeap allocation and should
//      be NULL for other target areas.
//    size: The requested memory size in bytes. Must be greater than zero.
//    handle: A pointer to an EsfMemoryManagerHandle variable where the
//      allocated memory's handle will be stored upon successful allocation.

// Returns:
//    EsfMemoryManagerResult:
//    - kEsfMemoryManagerResultSuccess: Memory was successfully allocated and
//    its handle is stored in 'handle'.
//    - kEsfMemoryManagerResultParamError: Invalid parameters were passed (e.g.,
//    NULL pointer for 'handle' or non-positive 'size').
//    - kEsfMemoryManagerResultOtherError: An internal error occurred, such as a
//    mutex locking failure.
//    - Other EsfMemoryManagerResult values may be returned by the called
//    subfunctions (e.g., EsfMemoryManagerAllocateWasm or
//    EsfMemoryManagerGenerateMemoryInfo).

// Note:
// """
EsfMemoryManagerResult EsfMemoryManagerAllocate(
    EsfMemoryManagerTargetArea target_area, const wasm_exec_env_t *exec_env,
    int32_t size, EsfMemoryManagerHandle *handle);

// """ Free memory.
// This function is used to free memory allocated by the EsfMemoryManager. It
// takes two parameters, a handle and an execution environment. The function
// first checks if the provided handle is valid or not. If it's invalid, it
// returns kEsfMemoryManagerResultParamError. Then, it checks if the memory
// manager has been initialized or not. If not, it returns
// kEsfMemoryManagerResultOtherError. After that, it locks a mutex to ensure
// thread safety and unlocks it at the end of the function. The function then
// discards WasmHeap or LargeHeap/DMA memory based on the handle provided. If an
// error occurs during this process, it returns
// kEsfMemoryManagerResultOtherError. Finally, if no errors occur, it deletes
// the used handle ID and returns kEsfMemoryManagerResultSuccess.

// Args:
//    handle: A handle to the memory that needs to be freed. It is an input
//      parameter.
//    exec_env: The execution environment of WASM. It is an input parameter.

// Returns:
//    EsfMemoryManagerResult:
//      An enum value indicating whether the operation was successful or not.

// Raises:
//    kEsfMemoryManagerResultSuccess: Successful processing.
//    kEsfMemoryManagerResultParamError: Parameter error.
//                        (If the memory operation handle entered is invalid)
//    kEsfMemoryManagerResultOperationError: Operation error.(Not unmapped.)

// Note:
// """
EsfMemoryManagerResult EsfMemoryManagerFree(EsfMemoryManagerHandle handle,
                                            const wasm_exec_env_t *exec_env);

// """ Map memory.
// Maps the memory held by the memory operation handle.
// EsfMemoryManagerMap is a function that maps memory for the given handle,
// execution environment and size.

// Args:
//    handle (EsfMemoryManagerHandle): a memory operation handle
//      Please specify the memory operation handle obtained
//      with EsfMemoryManagerAllocate().
//    exec_env (const wasm_exec_env_t*):
//    size (int32_t):
//    address (void **):
//      When map successful : Set the start address (virtual memory address)
//        of the map destination memory.
//      When map fails : Set NULL.

// Returns:
//     EsfMemoryManagerResult:
//        The code returns one of the values EsfMemoryManagerResult depending
//        on the execution result.

// Raises:
//     kEsfMemoryManagerResultSuccess: Successful processing.
//     kEsfMemoryManagerResultParamError: Parameter error.
//                    (If the memory operation handle entered is invalid)
//     kEsfMemoryManagerResultMapError: Memory map error
//     kEsfMemoryManagerResultOperationError: Operation error.
//                    (Map operations on map states.)

// Note:
//  If the target area is "LargeHeap" or "DMA", specifying "MAP_ALL_AREA(1)"
//  will map the allocated size (it is recommended to obtain the size using
//  MemoryMangerGetHandleInfo()).
// """
#define MAP_ALL_AREA (1)
EsfMemoryManagerResult EsfMemoryManagerMap(EsfMemoryManagerHandle handle,
                                           const wasm_exec_env_t *exec_env,
                                           int32_t size, void **address);

// """ Unmap memory.
// Unmap memory address from the specified handle.
// This function unmap a memory address associated with the given
// EsfMemoryManagerHandle. It checks for valid parameters, initializes if
// necessary, and then proceeds to unmap the memory based on the type of heap
// (WasmHeap or LargeHeap/DMA). If an error occurs during this process, it
// returns an appropriate error code.

// Args:
//    handle (EsfMemoryManagerHandle): The EsfMemoryManagerHandle associated
//      with the memory address to be unmapped.
//    address (void **): A pointer to the void * value representing the memory
//    address to be unmapped. Must not be NULL and must contain a non-zero
//    value.

// Returns:
//    EsfMemoryManagerResult:
//      An EsfMemoryManagerResult indicating the success or failure of the
//      operation, as well as any specific error codes that may have occurred
//      during execution.

// Raises:
//     kEsfMemoryManagerResultSuccess: Successful processing.
//     kEsfMemoryManagerResultParamError: Parameter error.
//                    (If the memory operation handle entered is invalid)
//     kEsfMemoryManagerResultOperationError: Operation error.
//                    (Unmap operations on unmap states.)

// Note:
// """
EsfMemoryManagerResult EsfMemoryManagerUnmap(EsfMemoryManagerHandle handle,
                                             void **address);

// """ Wasm Memory allocation.
// Reallocates memory in the application's memory space based on the specified
// usage and size.

// Args:
//    usage (EsfMemoryManagerWasmMemoryUsage): The intended usage of the
//      reallocated memory. Can be either kEsfMemoryManagerWasmAllocForRuntime
//      or kEsfMemoryManagerWasmAllocForLinearMemory.
//    size  : signed 32-bit integer 'size' indicating the size of the requested
//      memory.

// Returns:
//    The function returns a pointer of type 'EsfMemoryManagerAppMemory'
//    pointing to the allocated memory. If the size is less than or equal to
//    zero, or if an invalid usage value was provided, the function will return
//    NULL.

// Note:
//  This function is used to allocate memory based on the provided usage and
//  size. The function takes two parameters.
//  The function first checks if the provided size is greater than zero. If not,
//  it returns NULL without allocating any memory. Then, based on the usage
//  value, it sets an enumeration variable 'osal_usage' to either kOsalAppHeap
//  or kOsalAppLinearMemory. Finally, it calls OsalAppmemMalloc with
//  'osal_usage' and the provided size (converted to unsigned 32-bit integer) as
//  arguments and returns the allocated memory address casted to
//  EsfMemoryManagerAppMemory type. If the allocation fails, NULL is returned.
// """
EsfMemoryManagerAppMemory EsfMemoryManagerWasmAllocate(
    EsfMemoryManagerWasmMemoryUsage usage, int32_t size);

// """ Wasm Memory reallocation.
// Allocates memory in the application's memory space based on the specified
// usage and size.

// Args:
//    usage (EsfMemoryManagerWasmMemoryUsage): The intended usage of the
//      reallocated memory. Can be either kEsfMemoryManagerWasmAllocForRuntime
//      or kEsfMemoryManagerWasmAllocForLinearMemory.
//    old_memory (EsfMemoryManagerAppMemory) : Pointer to the existing memory
//      block that needs to be resized.
//    size (int32_t) : The new size of the memory block, in bytes. Must be
//      greater than 0.

// Returns:
//    EsfMemoryManagerAppMemory: A pointer to the newly allocated or resized
//    memory block. If reallocation fails, returns NULL.

// Note:
//  This function checks if the input parameters are valid and then attempts to
//  reallocate the specified memory block based on the given usage and size. The
//  new memory block is returned as a result of this operation. In case of any
//  errors or invalid inputs, the function returns NULL.
// """
EsfMemoryManagerAppMemory EsfMemoryManagerWasmReallocate(
    EsfMemoryManagerWasmMemoryUsage usage,
    const EsfMemoryManagerAppMemory old_memory, int32_t size);

// """ Frees the wasm memory.
// Args:
//    usage (EsfMemoryManagerWasmMemoryUsage): The type of memory allocation.
//      It can be either kEsfMemoryManagerWasmAllocForRuntime or
//      kEsfMemoryManagerWasmAllocForLinearMemory.
//    memory (EsfMemoryManagerAppMemory): Pointer to the allocated memory that
//      needs to be freed.

// Returns:
//    None

// Note:
//  This function checks if the input parameters are valid. If not, it returns
//  without performing any operation. It also converts the usage parameter into
//  an equivalent OsalAppmemUsage value and calls the OsalAppmemFree() function
//  with this converted value and the memory pointer to free the allocated
//  memory.
// """
void EsfMemoryManagerWasmFree(EsfMemoryManagerWasmMemoryUsage usage,
                              EsfMemoryManagerAppMemory memory);

// """ EsfMemoryManagerInitialize
// Initialize the EsfMemoryManager.
// This function initializes the EsfMemoryManager, which is responsible for
// managing memory resources in the system. It performs various tasks such as
// initializing OSAL LargeHeap, OSAL AppMemory, and setting the division number
// of OSAL AppMemory based on input parameters. If any error occurs during
// initialization, it returns an appropriate error code.

// Args:
//    app_mem_div_num:  The division number for OSAL AppMemory. This value
//      should be greater than 0.

// Returns:
//    EsfMemoryManagerResult An enumeration that indicates the result of the
//    operation. If successful, kEsfMemoryManagerResultSuccess is returned;
//    otherwise, an appropriate error code is returned.
// """
EsfMemoryManagerResult EsfMemoryManagerInitialize(int32_t app_mem_blocks);

// """ EsfMemoryManagerFinalize
// Finalize the EsfMemoryManager.
// This function finalizes the EsfMemoryManager, releasing all resources and
// making it unusable until initialized again. It is safe to call this function
// even if the memory manager has not been initialized or has already been
// finalized.

// Args:
//    none

// Returns:
//    EsfMemoryManagerResult An enumeration that indicates the result of the
//    operation. If successful, kEsfMemoryManagerResultSuccess is returned;
//    otherwise, an appropriate error code is returned.
// """
EsfMemoryManagerResult EsfMemoryManagerFinalize(void);

// """ EsfMemoryManagerFopen
// This function is used to open a file in the memory manager.

// Args:
//    handle (EsfMemoryManagerHandle): A handle that represents the memory
//    manager instance.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. It can be one of the following values:
//      kEsfMemoryManagerResultSuccess: The file was opened successfully.
//      kEsfMemoryManagerResultOtherError: There was an error while opening the
//      file, such as a locking issue or an uninitialized memory manager.
//      kEsfMemoryManagerResultNotSupport: The handle provided does not support
//      File I/O operations (e.g., WasmHeap).
// """
EsfMemoryManagerResult EsfMemoryManagerFopen(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerFclose
// This C language function is used to close a file associated with the given
// handle in the memory manager system. The function first checks if the memory
// manager has been initialized, and returns an error if it hasn't. It then
// acquires a lock on the shared memory mutex before performing the operation.
// If the provided handle is for WasmHeap, which doesn't support file I/O
// functions, the function returns kEsfMemoryManagerResultNotSupport. Otherwise,
// it calls EsfMemoryManagerFileIoFunc with the given parameters and releases
// the lock afterward.

// Args:
//    handle - The memory manager handle associated with the file to be closed.

// Returns:
//    EsfMemoryManagerResult - An enum value indicating the result of the
//    operation. It can be one of:
//      kEsfMemoryManagerResultSuccess: The function executed successfully and
//      the file was closed. kEsfMemoryManagerResultNotSupport: The handle
//      provided is for WasmHeap, which doesn't support file I/O functions.
//      kEsfMemoryManagerResultOtherError: An error occurred during execution
//      (e.g., memory manager not initialized or mutex locking failed).
// """
EsfMemoryManagerResult EsfMemoryManagerFclose(EsfMemoryManagerHandle handle);

// """ EsfMemoryManagerFseek
// This function is used to set the file position indicator for a given memory
// manager handle. This function takes four parameters - handle, offset, whence
// and result_offset. The 'handle' parameter represents the memory manager
// handle, 'offset' specifies the number of bytes to offset from the position
// indicated by 'whence'. The 'whence' parameter can be one of SEEK_SET,
// SEEK_CUR or SEEK_END. Finally, 'result_offset' is a pointer to an off_t
// object where the resulting file position indicator will be stored.

// Args:
//    handle Memory manager handle.
//    offset Number of bytes to offset from the position indicated by whence.
//    whence SEEK_SET, SEEK_CUR or SEEK_END value that determines starting point
//    for file position indicator movement. result_offset Pointer to an off_t
//    object where resulting file position indicator will be stored.

// Returns:
//    EsfMemoryManagerResult enum which can have the following values:
//      kEsfMemoryManagerResultSuccess: Function executed successfully.
//      kEsfMemoryManagerResultParamError: One or more parameters are invalid.
//      kEsfMemoryManagerResultOtherError: Other error occurred during function
//      execution. kEsfMemoryManagerResultNotSupport: The specified handle does
//      not support FileIO functions (e.g., WasmHeap).
// """
EsfMemoryManagerResult EsfMemoryManagerFseek(EsfMemoryManagerHandle handle,
                                             off_t offset, int whence,
                                             off_t *result_offset);

// """ EsfMemoryManagerFwrite
// This function is used to write data from a buffer into the file associated
// with the specified handle.

// Args:
//    handle (EsfMemoryManagerHandle): Handle of the memory manager instance.
//    buff (const void*): Pointer to the source buffer containing the data to be
//    written. size (size_t): Number of bytes to write from the buffer. If this
//    value is 0, no data will be written and the function returns immediately.
//    rsize (size_t*): Pointer to a variable that receives the number of bytes
//    successfully written by the function call. This parameter can be NULL if
//    you do not need this information.

// Returns:
//    EsfMemoryManagerResult: The return value indicates whether the operation
//    was successful or an error occurred. Possible values are:
//      kEsfMemoryManagerResultSuccess: Operation completed successfully.
//      kEsfMemoryManagerResultParamError: Invalid parameter passed to the
//      function (e.g., NULL pointer where it is not allowed).
//      kEsfMemoryManagerResultOtherError: An unknown error occurred during
//      execution. kEsfMemoryManagerResultNotSupport: The specified handle does
//      not support file I/O operations.
// """
EsfMemoryManagerResult EsfMemoryManagerFwrite(EsfMemoryManagerHandle handle,
                                              const void *buff, size_t size,
                                              size_t *rsize);

// """ EsfMemoryManagerFread
// This function reads data from a file associated with the specified handle.
// The function takes four parameters, namely 'handle', 'buff', 'size' and
// 'rsize'. It returns an enumeration value of type EsfMemoryManagerResult
// indicating the result of the operation.

// Args:
//    handle (EsfMemoryManagerHandle): A file handle obtained from a previous
//    call to EsfMemoryManagerFopen. buff (const void*): Pointer to a buffer
//    where the read data will be stored. Must not be NULL. size (size_t):
//    Number of bytes to be read. If 'size' is greater than zero, this function
//    may block until all requested bytes are read or an error occurs. rsize
//    (size_t*): Pointer to a location where the number of bytes actually read
//    will be stored. Must not be NULL.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. Possible values are:
//      kEsfMemoryManagerResultSuccess: The function completed successfully and
//      read some data. kEsfMemoryManagerResultParamError: Invalid parameters
//      were passed to the function (NULL pointer or non-positive size).
//      kEsfMemoryManagerResultOtherError: An unknown error occurred during
//      execution of the function. kEsfMemoryManagerResultNotSupport: The
//      specified handle does not support file I/O functions, such as WasmHeap
//      handles.
// """
EsfMemoryManagerResult EsfMemoryManagerFread(EsfMemoryManagerHandle handle,
                                             const void *buff, size_t size,
                                             size_t *rsize);
// """ EsfMemoryManagerIsMapSupport
// This function checks if the memory manager with a given handle supports map
// operations.

// Args:
//    handle (EsfMemoryManagerHandle): A file handle obtained from a previous
//    call to EsfMemoryManagerFopen. support : A pointer to an
//    EsfMemoryManagerMapSupport variable that will store the result.
//              If the memory manager supports mapping, this value is set to
//              kEsfMemoryManagerMapIsSupport; otherwise, it's set to
//              kEsfMemoryManagerMapIsNotSupport.

// Returns:
//    EsfMemoryManagerResult: An enumeration value indicating the result of the
//    operation. Possible values are:
//      kEsfMemoryManagerResultSuccess: The function completed successfully.
//      kEsfMemoryManagerResultParamError: Invalid parameters were passed to the
//      function (NULL pointer or non-positive size).
//      kEsfMemoryManagerResultOtherError: If memory manager is not initialized.
//      In case of any other error.
// """
EsfMemoryManagerResult EsfMemoryManagerIsMapSupport(
    EsfMemoryManagerHandle handle, EsfMemoryManagerMapSupport *support);

// """ EsfMemoryManagerGetHandleInfo
// This function retrieves detailed information about a specific memory manager
// handle. The returned information includes target area (WasmHeap, LargeHeap,
// DMA or OtherArea) and allocated size.

// Args:
//    [in] handle Memory manager handle to get the information for.
//    [out] info Pointer to an EsfMemoryManagerHandleInfo structure where the
//    retrieved information will be stored. Must not be NULL.

// Returns:
//    EsfMemoryManagerResult Result of the operation,
//    kEsfMemoryManagerResultSuccess if successful, otherwise an error code.
// """
EsfMemoryManagerResult EsfMemoryManagerGetHandleInfo(
    uint32_t handle, EsfMemoryManagerHandleInfo *info);

#ifdef __cplusplus
}
#endif

#endif  // ESF_MEMORY_MANAGER_INCLUDE_MEMORY_MANAGER_H_
