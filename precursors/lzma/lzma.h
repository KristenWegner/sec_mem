// lzma.h


#ifndef INCLUDE_LZMA_H
#define INCLUDE_LZMA_H 1


#include "../../config.h"


#define LZMA_API_IMPORT


#ifndef LZMA_API_CALL
#if defined(_WIN32) && !defined(__CYGWIN__)
#define LZMA_API_CALL __stdcall
#else
#define LZMA_API_CALL
#endif
#endif


#ifndef LZMA_API
#define LZMA_API(type) LZMA_API_IMPORT type LZMA_API_CALL
#endif


#ifndef lzma_nothrow
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define lzma_nothrow __attribute__((__nothrow__))
#else
#define lzma_nothrow
#endif
#endif


#if __GNUC__ >= 3
#ifndef lzma_attribute
#define lzma_attribute(attr) __attribute__(attr)
#endif
#ifndef lzma_attr_warn_unused_result
#if __GNUC__ == 3 && __GNUC_MINOR__ < 4
#define lzma_attr_warn_unused_result
#endif
#endif


#else
#ifndef lzma_attribute
#define lzma_attribute(attr)
#endif
#endif


#ifndef lzma_attr_pure
#define lzma_attr_pure lzma_attribute((__pure__))
#endif


#ifndef lzma_attr_const
#define lzma_attr_const lzma_attribute((__const__))
#endif


#ifndef lzma_attr_warn_unused_result
#define lzma_attr_warn_unused_result lzma_attribute((__warn_unused_result__))
#endif



/**
* \brief       Boolean
*
* This is here because C89 doesn't have stdbool.h. To set a value for
* variables having type lzma_bool, you can use
*   - C99's `true' and `false' from stdbool.h;
*   - C++'s internal `true' and `false'; or
*   - integers one (true) and zero (false).
*/
typedef unsigned char lzma_bool;


/**
* \brief       Type of reserved enumeration variable in structures
*
* To avoid breaking library ABI when new features are added, several
* structures contain extra variables that may be used in future. Since
* sizeof(enum) can be different than sizeof(int), and sizeof(enum) may
* even vary depending on the range of enumeration constants, we specify
* a separate type to be used for reserved enumeration variables. All
* enumeration constants in liblzma API will be non-negative and less
* than 128, which should guarantee that the ABI won't break even when
* new constants are added to existing enumerations.
*/
typedef enum {
	LZMA_RESERVED_ENUM = 0
} lzma_reserved_enum;


/**
* \brief       Return values used by several functions in liblzma
*
* Check the descriptions of specific functions to find out which return
* values they can return. With some functions the return values may have
* more specific meanings than described here; those differences are
* described per-function basis.
*/
typedef enum {
	LZMA_OK = 0,
	/**<
	* \brief       Operation completed successfully
	*/

	LZMA_STREAM_END = 1,
	/**<
	* \brief       End of stream was reached
	*
	* In encoder, LZMA_SYNC_FLUSH, LZMA_FULL_FLUSH, or
	* LZMA_FINISH was finished. In decoder, this indicates
	* that all the data was successfully decoded.
	*
	* In all cases, when LZMA_STREAM_END is returned, the last
	* output bytes should be picked from strm->next_out.
	*/

	LZMA_NO_CHECK = 2,
	/**<
	* \brief       Input stream has no integrity check
	*
	* This return value can be returned only if the
	* LZMA_TELL_NO_CHECK flag was used when initializing
	* the decoder. LZMA_NO_CHECK is just a warning, and
	* the decoding can be continued normally.
	*
	* It is possible to call lzma_get_check() immediately after
	* lzma_code has returned LZMA_NO_CHECK. The result will
	* naturally be LZMA_CHECK_NONE, but the possibility to call
	* lzma_get_check() may be convenient in some applications.
	*/

	LZMA_UNSUPPORTED_CHECK = 3,
	/**<
	* \brief       Cannot calculate the integrity check
	*
	* The usage of this return value is different in encoders
	* and decoders.
	*
	* Encoders can return this value only from the initialization
	* function. If initialization fails with this value, the
	* encoding cannot be done, because there's no way to produce
	* output with the correct integrity check.
	*
	* Decoders can return this value only from lzma_code() and
	* only if the LZMA_TELL_UNSUPPORTED_CHECK flag was used when
	* initializing the decoder. The decoding can still be
	* continued normally even if the check type is unsupported,
	* but naturally the check will not be validated, and possible
	* errors may go undetected.
	*
	* With decoder, it is possible to call lzma_get_check()
	* immediately after lzma_code() has returned
	* LZMA_UNSUPPORTED_CHECK. This way it is possible to find
	* out what the unsupported Check ID was.
	*/

	LZMA_GET_CHECK = 4,
	/**<
	* \brief       Integrity check type is now available
	*
	* This value can be returned only by the lzma_code() function
	* and only if the decoder was initialized with the
	* LZMA_TELL_ANY_CHECK flag. LZMA_GET_CHECK tells the
	* application that it may now call lzma_get_check() to find
	* out the Check ID. This can be used, for example, to
	* implement a decoder that accepts only files that have
	* strong enough integrity check.
	*/

	LZMA_MEM_ERROR = 5,
	/**<
	* \brief       Cannot allocate memory
	*
	* Memory allocation failed, or the size of the allocation
	* would be greater than SIZE_MAX.
	*
	* Due to internal implementation reasons, the coding cannot
	* be continued even if more memory were made available after
	* LZMA_MEM_ERROR.
	*/

	LZMA_MEMLIMIT_ERROR = 6,
	/**
	* \brief       Memory usage limit was reached
	*
	* Decoder would need more memory than allowed by the
	* specified memory usage limit. To continue decoding,
	* the memory usage limit has to be increased with
	* lzma_memlimit_set().
	*/

	LZMA_FORMAT_ERROR = 7,
	/**<
	* \brief       File format not recognized
	*
	* The decoder did not recognize the input as supported file
	* format. This error can occur, for example, when trying to
	* decode .lzma format file with lzma_stream_decoder,
	* because lzma_stream_decoder accepts only the .xz format.
	*/

	LZMA_OPTIONS_ERROR = 8,
	/**<
	* \brief       Invalid or unsupported options
	*
	* Invalid or unsupported options, for example
	*  - unsupported filter(s) or filter options; or
	*  - reserved bits set in headers (decoder only).
	*
	* Rebuilding liblzma with more features enabled, or
	* upgrading to a newer version of liblzma may help.
	*/

	LZMA_DATA_ERROR = 9,
	/**<
	* \brief       Data is corrupt
	*
	* The usage of this return value is different in encoders
	* and decoders. In both encoder and decoder, the coding
	* cannot continue after this error.
	*
	* Encoders return this if size limits of the target file
	* format would be exceeded. These limits are huge, thus
	* getting this error from an encoder is mostly theoretical.
	* For example, the maximum compressed and uncompressed
	* size of a .xz Stream is roughly 8 EiB (2^63 bytes).
	*
	* Decoders return this error if the input data is corrupt.
	* This can mean, for example, invalid CRC32 in headers
	* or invalid check of uncompressed data.
	*/

	LZMA_BUF_ERROR = 10,
	/**<
	* \brief       No progress is possible
	*
	* This error code is returned when the coder cannot consume
	* any new input and produce any new output. The most common
	* reason for this error is that the input stream being
	* decoded is truncated or corrupt.
	*
	* This error is not fatal. Coding can be continued normally
	* by providing more input and/or more output space, if
	* possible.
	*
	* Typically the first call to lzma_code() that can do no
	* progress returns LZMA_OK instead of LZMA_BUF_ERROR. Only
	* the second consecutive call doing no progress will return
	* LZMA_BUF_ERROR. This is intentional.
	*
	* With zlib, Z_BUF_ERROR may be returned even if the
	* application is doing nothing wrong, so apps will need
	* to handle Z_BUF_ERROR specially. The above hack
	* guarantees that liblzma never returns LZMA_BUF_ERROR
	* to properly written applications unless the input file
	* is truncated or corrupt. This should simplify the
	* applications a little.
	*/

	LZMA_PROG_ERROR = 11,
	/**<
	* \brief       Programming error
	*
	* This indicates that the arguments given to the function are
	* invalid or the internal state of the decoder is corrupt.
	*   - Function arguments are invalid or the structures
	*     pointed by the argument pointers are invalid
	*     e.g. if strm->next_out has been set to NULL and
	*     strm->avail_out > 0 when calling lzma_code().
	*   - lzma_* functions have been called in wrong order
	*     e.g. lzma_code() was called right after lzma_end().
	*   - If errors occur randomly, the reason might be flaky
	*     hardware.
	*
	* If you think that your code is correct, this error code
	* can be a sign of a bug in liblzma. See the documentation
	* how to report bugs.
	*/
} lzma_ret;


/**
* \brief       The `action' argument for lzma_code()
*
* After the first use of LZMA_SYNC_FLUSH, LZMA_FULL_FLUSH, LZMA_FULL_BARRIER,
* or LZMA_FINISH, the same `action' must is used until lzma_code() returns
* LZMA_STREAM_END. Also, the amount of input (that is, strm->avail_in) must
* not be modified by the application until lzma_code() returns
* LZMA_STREAM_END. Changing the `action' or modifying the amount of input
* will make lzma_code() return LZMA_PROG_ERROR.
*/
typedef enum {
	LZMA_RUN = 0,
	/**<
	* \brief       Continue coding
	*
	* Encoder: Encode as much input as possible. Some internal
	* buffering will probably be done (depends on the filter
	* chain in use), which causes latency: the input used won't
	* usually be decodeable from the output of the same
	* lzma_code() call.
	*
	* Decoder: Decode as much input as possible and produce as
	* much output as possible.
	*/

	LZMA_SYNC_FLUSH = 1,
	/**<
	* \brief       Make all the input available at output
	*
	* Normally the encoder introduces some latency.
	* LZMA_SYNC_FLUSH forces all the buffered data to be
	* available at output without resetting the internal
	* state of the encoder. This way it is possible to use
	* compressed stream for example for communication over
	* network.
	*
	* Only some filters support LZMA_SYNC_FLUSH. Trying to use
	* LZMA_SYNC_FLUSH with filters that don't support it will
	* make lzma_code() return LZMA_OPTIONS_ERROR. For example,
	* LZMA1 doesn't support LZMA_SYNC_FLUSH but LZMA2 does.
	*
	* Using LZMA_SYNC_FLUSH very often can dramatically reduce
	* the compression ratio. With some filters (for example,
	* LZMA2), fine-tuning the compression options may help
	* mitigate this problem significantly (for example,
	* match finder with LZMA2).
	*
	* Decoders don't support LZMA_SYNC_FLUSH.
	*/

	LZMA_FULL_FLUSH = 2,
	/**<
	* \brief       Finish encoding of the current Block
	*
	* All the input data going to the current Block must have
	* been given to the encoder (the last bytes can still be
	* pending in *next_in). Call lzma_code() with LZMA_FULL_FLUSH
	* until it returns LZMA_STREAM_END. Then continue normally
	* with LZMA_RUN or finish the Stream with LZMA_FINISH.
	*
	* This action is currently supported only by Stream encoder
	* and easy encoder (which uses Stream encoder). If there is
	* no unfinished Block, no empty Block is created.
	*/

	LZMA_FULL_BARRIER = 4,
	/**<
	* \brief       Finish encoding of the current Block
	*
	* This is like LZMA_FULL_FLUSH except that this doesn't
	* necessarily wait until all the input has been made
	* available via the output buffer. That is, lzma_code()
	* might return LZMA_STREAM_END as soon as all the input
	* has been consumed (avail_in == 0).
	*
	* LZMA_FULL_BARRIER is useful with a threaded encoder if
	* one wants to split the .xz Stream into Blocks at specific
	* offsets but doesn't care if the output isn't flushed
	* immediately. Using LZMA_FULL_BARRIER allows keeping
	* the threads busy while LZMA_FULL_FLUSH would make
	* lzma_code() wait until all the threads have finished
	* until more data could be passed to the encoder.
	*
	* With a lzma_stream initialized with the single-threaded
	* lzma_stream_encoder() or lzma_easy_encoder(),
	* LZMA_FULL_BARRIER is an alias for LZMA_FULL_FLUSH.
	*/

	LZMA_FINISH = 3
	/**<
	* \brief       Finish the coding operation
	*
	* All the input data must have been given to the encoder
	* (the last bytes can still be pending in next_in).
	* Call lzma_code() with LZMA_FINISH until it returns
	* LZMA_STREAM_END. Once LZMA_FINISH has been used,
	* the amount of input must no longer be changed by
	* the application.
	*
	* When decoding, using LZMA_FINISH is optional unless the
	* LZMA_CONCATENATED flag was used when the decoder was
	* initialized. When LZMA_CONCATENATED was not used, the only
	* effect of LZMA_FINISH is that the amount of input must not
	* be changed just like in the encoder.
	*/
}
lzma_action;


/**
* \brief       Custom functions for memory handling
*
* A pointer to lzma_allocator may be passed via lzma_stream structure
* to liblzma, and some advanced functions take a pointer to lzma_allocator
* as a separate function argument. The library will use the functions
* specified in lzma_allocator for memory handling instead of the default
* malloc() and free(). C++ users should note that the custom memory
* handling functions must not throw exceptions.
*
* Single-threaded mode only: liblzma doesn't make an internal copy of
* lzma_allocator. Thus, it is OK to change these function pointers in
* the middle of the coding process, but obviously it must be done
* carefully to make sure that the replacement `free' can deallocate
* memory allocated by the earlier `allocate' function(s).
*
* Multithreaded mode: liblzma might internally store pointers to the
* lzma_allocator given via the lzma_stream structure. The application
* must not change the allocator pointer in lzma_stream or the contents
* of the pointed lzma_allocator structure until lzma_end() has been used
* to free the memory associated with that lzma_stream. The allocation
* functions might be called simultaneously from multiple threads, and
* thus they must be thread safe.
*/
typedef struct {
	/**
	* \brief       Pointer to a custom memory allocation function
	*
	* If you don't want a custom allocator, but still want
	* custom free(), set this to NULL and liblzma will use
	* the standard malloc().
	*
	* \param       opaque  lzma_allocator.opaque (see below)
	* \param       nmemb   Number of elements like in calloc(). liblzma
	*                      will always set nmemb to 1, so it is safe to
	*                      ignore nmemb in a custom allocator if you like.
	*                      The nmemb argument exists only for
	*                      compatibility with zlib and libbzip2.
	* \param       size    Size of an element in bytes.
	*                      liblzma never sets this to zero.
	*
	* \return      Pointer to the beginning of a memory block of
	*              `size' bytes, or NULL if allocation fails
	*              for some reason. When allocation fails, functions
	*              of liblzma return LZMA_MEM_ERROR.
	*
	* The allocator should not waste time zeroing the allocated buffers.
	* This is not only about speed, but also memory usage, since the
	* operating system kernel doesn't necessarily allocate the requested
	* memory in physical memory until it is actually used. With small
	* input files, liblzma may actually need only a fraction of the
	* memory that it requested for allocation.
	*
	* \note        LZMA_MEM_ERROR is also used when the size of the
	*              allocation would be greater than SIZE_MAX. Thus,
	*              don't assume that the custom allocator must have
	*              returned NULL if some function from liblzma
	*              returns LZMA_MEM_ERROR.
	*/
	void *(LZMA_API_CALL *allocate)(void *opaque, size_t nmemb, size_t size);

	/**
	* \brief       Pointer to a custom memory freeing function
	*
	* If you don't want a custom freeing function, but still
	* want a custom allocator, set this to NULL and liblzma
	* will use the standard free().
	*
	* \param       opaque  lzma_allocator.opaque (see below)
	* \param       ptr     Pointer returned by lzma_allocator.allocate(),
	*                      or when it is set to NULL, a pointer returned
	*                      by the standard malloc().
	*/
	void (LZMA_API_CALL *release)(void *opaque, void *ptr);

	/**
	* \brief       Pointer passed to .allocate() and .free()
	*
	* opaque is passed as the first argument to lzma_allocator.allocate()
	* and lzma_allocator.free(). This intended to ease implementing
	* custom memory allocation functions for use with liblzma.
	*
	* If you don't need this, you should set this to NULL.
	*/
	void *opaque;

}
lzma_allocator;


/**
* \brief       Internal data structure
*
* The contents of this structure is not visible outside the library.
*/
typedef struct lzma_internal_s lzma_internal;


/**
* \brief       Passing data to and from liblzma
*
* The lzma_stream structure is used for
*  - passing pointers to input and output buffers to liblzma;
*  - defining custom memory hander functions; and
*  - holding a pointer to coder-specific internal data structures.
*
* Typical usage:
*
*  - After allocating lzma_stream (on stack or with malloc()), it must be
*    initialized to LZMA_STREAM_INIT (see LZMA_STREAM_INIT for details).
*
*  - Initialize a coder to the lzma_stream, for example by using
*    lzma_easy_encoder() or lzma_auto_decoder(). Some notes:
*      - In contrast to zlib, strm->next_in and strm->next_out are
*        ignored by all initialization functions, thus it is safe
*        to not initialize them yet.
*      - The initialization functions always set strm->total_in and
*        strm->total_out to zero.
*      - If the initialization function fails, no memory is left allocated
*        that would require freeing with lzma_end() even if some memory was
*        associated with the lzma_stream structure when the initialization
*        function was called.
*
*  - Use lzma_code() to do the actual work.
*
*  - Once the coding has been finished, the existing lzma_stream can be
*    reused. It is OK to reuse lzma_stream with different initialization
*    function without calling lzma_end() first. Old allocations are
*    automatically freed.
*
*  - Finally, use lzma_end() to free the allocated memory. lzma_end() never
*    frees the lzma_stream structure itself.
*
* Application may modify the values of total_in and total_out as it wants.
* They are updated by liblzma to match the amount of data read and
* written but aren't used for anything else except as a possible return
* values from lzma_get_progress().
*/
typedef struct
{
	const uint8_t *next_in; /**< Pointer to the next input byte. */
	size_t avail_in;    /**< Number of available input bytes in next_in. */
	uint64_t total_in;  /**< Total number of bytes read by liblzma. */
	uint8_t *next_out;  /**< Pointer to the next output position. */
	size_t avail_out;   /**< Amount of free space in next_out. */
	uint64_t total_out; /**< Total number of bytes written by liblzma. */
	const lzma_allocator *allocator;
	lzma_internal *internal;
}
lzma_stream;


/**
* \brief       Initialization for lzma_stream
*
* When you declare an instance of lzma_stream, you can immediately
* initialize it so that initialization functions know that no memory
* has been allocated yet:
*
*     lzma_stream strm = LZMA_STREAM_INIT;
*
* If you need to initialize a dynamically allocated lzma_stream, you can use
* memset(strm_pointer, 0, sizeof(lzma_stream)). Strictly speaking, this
* violates the C standard since NULL may have different internal
* representation than zero, but it should be portable enough in practice.
* Anyway, for maximum portability, you can use something like this:
*
*     lzma_stream tmp = LZMA_STREAM_INIT;
*     *strm = tmp;
*/
#define LZMA_STREAM_INIT { NULL, 0, 0, NULL, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, LZMA_RESERVED_ENUM, LZMA_RESERVED_ENUM }


/**
* \brief       Encode or decode data
*
* Once the lzma_stream has been successfully initialized (e.g. with
* lzma_stream_encoder()), the actual encoding or decoding is done
* using this function. The application has to update strm->next_in,
* strm->avail_in, strm->next_out, and strm->avail_out to pass input
* to and get output from liblzma.
*
* See the description of the coder-specific initialization function to find
* out what `action' values are supported by the coder.
*/
extern LZMA_API(lzma_ret) lzma_code(lzma_stream *strm, lzma_action action) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Free memory allocated for the coder data structures
*
* \param       strm    Pointer to lzma_stream that is at least initialized
*                      with LZMA_STREAM_INIT.
*
* After lzma_end(strm), strm->internal is guaranteed to be NULL. No other
* members of the lzma_stream structure are touched.
*
* \note        zlib indicates an error if application end()s unfinished
*              stream structure. liblzma doesn't do this, and assumes that
*              application knows what it is doing.
*/
extern LZMA_API(void) lzma_end(lzma_stream *strm) lzma_nothrow;


/**
* \brief       Get progress information
*
* In single-threaded mode, applications can get progress information from
* strm->total_in and strm->total_out. In multi-threaded mode this is less
* useful because a significant amount of both input and output data gets
* buffered internally by liblzma. This makes total_in and total_out give
* misleading information and also makes the progress indicator updates
* non-smooth.
*
* This function gives realistic progress information also in multi-threaded
* mode by taking into account the progress made by each thread. In
* single-threaded mode *progress_in and *progress_out are set to
* strm->total_in and strm->total_out, respectively.
*/
extern LZMA_API(void) lzma_get_progress(lzma_stream *strm, uint64_t *progress_in, uint64_t *progress_out) lzma_nothrow;


/**
* \brief       Get the memory usage of decoder filter chain
*
* This function is currently supported only when *strm has been initialized
* with a function that takes a memlimit argument. With other functions, you
* should use e.g. lzma_raw_encoder_memusage() or lzma_raw_decoder_memusage()
* to estimate the memory requirements.
*
* This function is useful e.g. after LZMA_MEMLIMIT_ERROR to find out how big
* the memory usage limit should have been to decode the input. Note that
* this may give misleading information if decoding .xz Streams that have
* multiple Blocks, because each Block can have different memory requirements.
*
* \return      How much memory is currently allocated for the filter
*              decoders. If no filter chain is currently allocated,
*              some non-zero value is still returned, which is less than
*              or equal to what any filter chain would indicate as its
*              memory requirement.
*
*              If this function isn't supported by *strm or some other error
*              occurs, zero is returned.
*/
extern LZMA_API(uint64_t) lzma_memusage(const lzma_stream *strm) lzma_nothrow lzma_attr_pure;


/**
* \brief       Get the current memory usage limit
*
* This function is supported only when *strm has been initialized with
* a function that takes a memlimit argument.
*
* \return      On success, the current memory usage limit is returned
*              (always non-zero). On error, zero is returned.
*/
extern LZMA_API(uint64_t) lzma_memlimit_get(const lzma_stream *strm) lzma_nothrow lzma_attr_pure;


/**
* \brief       Set the memory usage limit
*
* This function is supported only when *strm has been initialized with
* a function that takes a memlimit argument.
*
* \return      - LZMA_OK: New memory usage limit successfully set.
*              - LZMA_MEMLIMIT_ERROR: The new limit is too small.
*                The limit was not changed.
*              - LZMA_PROG_ERROR: Invalid arguments, e.g. *strm doesn't
*                support memory usage limit or memlimit was zero.
*/
extern LZMA_API(lzma_ret) lzma_memlimit_set(lzma_stream *strm, uint64_t memlimit) lzma_nothrow;


/**
* \brief       Maximum supported value of a variable-length integer
*/
#define LZMA_VLI_MAX (UINT64_MAX / 2)

/**
* \brief       VLI value to denote that the value is unknown
*/
#define LZMA_VLI_UNKNOWN UINT64_MAX

/**
* \brief       Maximum supported encoded length of variable length integers
*/
#define LZMA_VLI_BYTES_MAX 9

/**
* \brief       VLI constant suffix
*/
#define LZMA_VLI_C(n) UINT64_C(n)


/**
* \brief       Variable-length integer type
*
* Valid VLI values are in the range [0, LZMA_VLI_MAX]. Unknown value is
* indicated with LZMA_VLI_UNKNOWN, which is the maximum value of the
* underlaying integer type.
*
* lzma_vli will be uint64_t for the foreseeable future. If a bigger size
* is needed in the future, it is guaranteed that 2 * LZMA_VLI_MAX will
* not overflow lzma_vli. This simplifies integer overflow detection.
*/
typedef uint64_t lzma_vli;


/**
* \brief       Validate a variable-length integer
*
* This is useful to test that application has given acceptable values
* for example in the uncompressed_size and compressed_size variables.
*
* \return      True if the integer is representable as VLI or if it
*              indicates unknown value.
*/
#define lzma_vli_is_valid(vli) ((vli) <= LZMA_VLI_MAX || (vli) == LZMA_VLI_UNKNOWN)


/**
* \brief       Encode a variable-length integer
*
* This function has two modes: single-call and multi-call. Single-call mode
* encodes the whole integer at once; it is an error if the output buffer is
* too small. Multi-call mode saves the position in *vli_pos, and thus it is
* possible to continue encoding if the buffer becomes full before the whole
* integer has been encoded.
*
* \param       vli       Integer to be encoded
* \param       vli_pos   How many VLI-encoded bytes have already been written
*                        out. When starting to encode a new integer in
*                        multi-call mode, *vli_pos must be set to zero.
*                        To use single-call encoding, set vli_pos to NULL.
* \param       out       Beginning of the output buffer
* \param       out_pos   The next byte will be written to out[*out_pos].
* \param       out_size  Size of the out buffer; the first byte into
*                        which no data is written to is out[out_size].
*
* \return      Slightly different return values are used in multi-call and
*              single-call modes.
*
*              Single-call (vli_pos == NULL):
*              - LZMA_OK: Integer successfully encoded.
*              - LZMA_PROG_ERROR: Arguments are not sane. This can be due
*                to too little output space; single-call mode doesn't use
*                LZMA_BUF_ERROR, since the application should have checked
*                the encoded size with lzma_vli_size().
*
*              Multi-call (vli_pos != NULL):
*              - LZMA_OK: So far all OK, but the integer is not
*                completely written out yet.
*              - LZMA_STREAM_END: Integer successfully encoded.
*              - LZMA_BUF_ERROR: No output space was provided.
*              - LZMA_PROG_ERROR: Arguments are not sane.
*/
extern LZMA_API(lzma_ret) lzma_vli_encode(lzma_vli vli, size_t *vli_pos, uint8_t *out, size_t *out_pos, size_t out_size) lzma_nothrow;


/**
* \brief       Decode a variable-length integer
*
* Like lzma_vli_encode(), this function has single-call and multi-call modes.
*
* \param       vli       Pointer to decoded integer. The decoder will
*                        initialize it to zero when *vli_pos == 0, so
*                        application isn't required to initialize *vli.
* \param       vli_pos   How many bytes have already been decoded. When
*                        starting to decode a new integer in multi-call
*                        mode, *vli_pos must be initialized to zero. To
*                        use single-call decoding, set vli_pos to NULL.
* \param       in        Beginning of the input buffer
* \param       in_pos    The next byte will be read from in[*in_pos].
* \param       in_size   Size of the input buffer; the first byte that
*                        won't be read is in[in_size].
*
* \return      Slightly different return values are used in multi-call and
*              single-call modes.
*
*              Single-call (vli_pos == NULL):
*              - LZMA_OK: Integer successfully decoded.
*              - LZMA_DATA_ERROR: Integer is corrupt. This includes hitting
*                the end of the input buffer before the whole integer was
*                decoded; providing no input at all will use LZMA_DATA_ERROR.
*              - LZMA_PROG_ERROR: Arguments are not sane.
*
*              Multi-call (vli_pos != NULL):
*              - LZMA_OK: So far all OK, but the integer is not
*                completely decoded yet.
*              - LZMA_STREAM_END: Integer successfully decoded.
*              - LZMA_DATA_ERROR: Integer is corrupt.
*              - LZMA_BUF_ERROR: No input was provided.
*              - LZMA_PROG_ERROR: Arguments are not sane.
*/
extern LZMA_API(lzma_ret) lzma_vli_decode(lzma_vli *vli, size_t *vli_pos, const uint8_t *in, size_t *in_pos, size_t in_size) lzma_nothrow;


/**
* \brief       Get the number of bytes required to encode a VLI
* \return      Number of bytes on success (1-9). If vli isn't valid, zero is returned.
*/
extern LZMA_API(uint32_t) lzma_vli_size(lzma_vli vli) lzma_nothrow lzma_attr_pure;


/**
* \brief Type of the integrity check (Check ID)
* The .xz format supports multiple types of checks that are calculated
* from the uncompressed data. They vary in both speed and ability to
* detect errors.
*/
typedef enum
{
	LZMA_CHECK_NONE = 0,
	/**<
	* No Check is calculated.
	* Size of the Check field: 0 bytes
	*/

	LZMA_CHECK_CRC32 = 1,
	/**<
	* CRC32 using the polynomial from the IEEE 802.3 standard
	* Size of the Check field: 4 bytes
	*/

	LZMA_CHECK_CRC64 = 4,
	/**<
	* CRC64 using the polynomial from the ECMA-182 standard
	* Size of the Check field: 8 bytes
	*/

	LZMA_CHECK_SHA256 = 10
	/**<
	* SHA-256
	* Size of the Check field: 32 bytes
	*/
}
lzma_check;


/**
* \brief Maximum valid Check ID
* The .xz file format specification specifies 16 Check IDs (0-15). Some
* of them are only reserved, that is, no actual Check algorithm has been
* assigned. When decoding, liblzma still accepts unknown Check IDs for
* future compatibility. If a valid but unsupported Check ID is detected,
* liblzma can indicate a warning; see the flags LZMA_TELL_NO_CHECK,
* LZMA_TELL_UNSUPPORTED_CHECK, and LZMA_TELL_ANY_CHECK in container.h.
*/
#define LZMA_CHECK_ID_MAX 15


/**
* \brief Test if the given Check ID is supported
* Return true if the given Check ID is supported by this liblzma build.
* Otherwise false is returned. It is safe to call this with a value that
* is not in the range [0, 15]; in that case the return value is always false.
* You can assume that LZMA_CHECK_NONE and LZMA_CHECK_CRC32 are always
* supported (even if liblzma is built with limited features).
*/
extern LZMA_API(lzma_bool) lzma_check_is_supported(lzma_check check) lzma_nothrow lzma_attr_const;


/**
* \brief Get the size of the Check field with the given Check ID
* Although not all Check IDs have a check algorithm associated, the size of
* every Check is already frozen. This function returns the size (in bytes) of
* the Check field with the specified Check ID. The values are:
* { 0, 4, 4, 4, 8, 8, 8, 16, 16, 16, 32, 32, 32, 64, 64, 64 }
* If the argument is not in the range [0, 15], UINT32_MAX is returned.
*/
extern LZMA_API(uint32_t) lzma_check_size(lzma_check check) lzma_nothrow lzma_attr_const;


/**
* \brief Maximum size of a Check field
*/
#define LZMA_CHECK_SIZE_MAX 64


/**
* \brief Calculate CRC32
* Calculate CRC32 using the polynomial from the IEEE 802.3 standard.
* \param buf Pointer to the input buffer
* \param size Size of the input buffer
* \param crc Previously returned CRC value. This is used to calculate the CRC of a big buffer in smaller chunks. Set to zero when starting a new calculation.
* \return Updated CRC value, which can be passed to this function again to continue CRC calculation.
*/
extern LZMA_API(uint32_t) lzma_crc32(const uint8_t *buf, size_t size, uint32_t crc) lzma_nothrow lzma_attr_pure;


/**
* \brief       Calculate CRC64
*
* Calculate CRC64 using the polynomial from the ECMA-182 standard.
*
* This function is used similarly to lzma_crc32(). See its documentation.
*/
extern LZMA_API(uint64_t) lzma_crc64(const uint8_t *buf, size_t size, uint64_t crc) lzma_nothrow lzma_attr_pure;


/*
* SHA-256 functions are currently not exported to public API.
* Contact Lasse Collin if you think it should be.
*/


/**
* \brief Get the type of the integrity check
* This function can be called only immediately after lzma_code() has
* returned LZMA_NO_CHECK, LZMA_UNSUPPORTED_CHECK, or LZMA_GET_CHECK.
* Calling this function in any other situation has undefined behavior.
*/
extern LZMA_API(lzma_check) lzma_get_check(const lzma_stream *strm) lzma_nothrow;


/**
* \brief Maximum number of filters in a chain
* A filter chain can have 1-4 filters, of which three are allowed to change
* the size of the data. Usually only one or two filters are needed.
*/
#define LZMA_FILTERS_MAX 4


/**
* \brief Filter options
*
* This structure is used to pass Filter ID and a pointer filter's
* options to liblzma. A few functions work with a single lzma_filter
* structure, while most functions expect a filter chain.
* A filter chain is indicated with an array of lzma_filter structures.
* The array is terminated with .id = LZMA_VLI_UNKNOWN. Thus, the filter
* array must have LZMA_FILTERS_MAX + 1 elements (that is, five) to
* be able to hold any arbitrary filter chain. This is important when
* using lzma_block_header_decode() from block.h, because too small
* array would make liblzma write past the end of the filters array.
*/
typedef struct
{
	/**
	* \brief Filter ID
	* Use constants whose name begin with `LZMA_FILTER_' to specify
	* different filters. In an array of lzma_filter structures, use
	* LZMA_VLI_UNKNOWN to indicate end of filters.
	* \note This is not an enum, because on some systems enums cannot be 64-bit.
	*/
	lzma_vli id;

	/**
	* \brief Pointer to filter-specific options structure
	* If the filter doesn't need options, set this to NULL. If id is
	* set to LZMA_VLI_UNKNOWN, options is ignored, and thus
	* doesn't need be initialized.
	*/
	void *options;
}
lzma_filter;


/**
* \brief       Test if the given Filter ID is supported for encoding
*
* Return true if the give Filter ID is supported for encoding by this
* liblzma build. Otherwise false is returned.
*
* There is no way to list which filters are available in this particular
* liblzma version and build. It would be useless, because the application
* couldn't know what kind of options the filter would need.
*/
extern LZMA_API(lzma_bool) lzma_filter_encoder_is_supported(lzma_vli id) lzma_nothrow lzma_attr_const;


/**
* \brief       Test if the given Filter ID is supported for decoding
*
* Return true if the give Filter ID is supported for decoding by this
* liblzma build. Otherwise false is returned.
*/
extern LZMA_API(lzma_bool) lzma_filter_decoder_is_supported(lzma_vli id) lzma_nothrow lzma_attr_const;


/**
* \brief       Copy the filters array
*
* Copy the Filter IDs and filter-specific options from src to dest.
* Up to LZMA_FILTERS_MAX filters are copied, plus the terminating
* .id == LZMA_VLI_UNKNOWN. Thus, dest should have at least
* LZMA_FILTERS_MAX + 1 elements space unless the caller knows that
* src is smaller than that.
*
* Unless the filter-specific options is NULL, the Filter ID has to be
* supported by liblzma, because liblzma needs to know the size of every
* filter-specific options structure. The filter-specific options are not
* validated. If options is NULL, any unsupported Filter IDs are copied
* without returning an error.
*
* Old filter-specific options in dest are not freed, so dest doesn't
* need to be initialized by the caller in any way.
*
* If an error occurs, memory possibly already allocated by this function
* is always freed.
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_OPTIONS_ERROR: Unsupported Filter ID and its options
*                is not NULL.
*              - LZMA_PROG_ERROR: src or dest is NULL.
*/
extern LZMA_API(lzma_ret) lzma_filters_copy(const lzma_filter *src, lzma_filter *dest, const lzma_allocator *allocator) lzma_nothrow;


/**
* \brief       Calculate approximate memory requirements for raw encoder
*
* This function can be used to calculate the memory requirements for
* Block and Stream encoders too because Block and Stream encoders don't
* need significantly more memory than raw encoder.
*
* \param       filters     Array of filters terminated with
*                          .id == LZMA_VLI_UNKNOWN.
*
* \return      Number of bytes of memory required for the given
*              filter chain when encoding. If an error occurs,
*              for example due to unsupported filter chain,
*              UINT64_MAX is returned.
*/
extern LZMA_API(uint64_t) lzma_raw_encoder_memusage(const lzma_filter *filters) lzma_nothrow lzma_attr_pure;


/**
* \brief       Calculate approximate memory requirements for raw decoder
*
* This function can be used to calculate the memory requirements for
* Block and Stream decoders too because Block and Stream decoders don't
* need significantly more memory than raw decoder.
*
* \param       filters     Array of filters terminated with
*                          .id == LZMA_VLI_UNKNOWN.
*
* \return      Number of bytes of memory required for the given
*              filter chain when decoding. If an error occurs,
*              for example due to unsupported filter chain,
*              UINT64_MAX is returned.
*/
extern LZMA_API(uint64_t) lzma_raw_decoder_memusage(const lzma_filter *filters) lzma_nothrow lzma_attr_pure;


/**
* \brief       Initialize raw encoder
*
* This function may be useful when implementing custom file formats.
*
* \param       strm    Pointer to properly prepared lzma_stream
* \param       filters Array of lzma_filter structures. The end of the
*                      array must be marked with .id = LZMA_VLI_UNKNOWN.
*
* The `action' with lzma_code() can be LZMA_RUN, LZMA_SYNC_FLUSH (if the
* filter chain supports it), or LZMA_FINISH.
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_raw_encoder(lzma_stream *strm, const lzma_filter *filters) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Initialize raw decoder
*
* The initialization of raw decoder goes similarly to raw encoder.
*
* The `action' with lzma_code() can be LZMA_RUN or LZMA_FINISH. Using
* LZMA_FINISH is not required, it is supported just for convenience.
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_raw_decoder(lzma_stream *strm, const lzma_filter *filters) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Update the filter chain in the encoder
*
* This function is for advanced users only. This function has two slightly
* different purposes:
*
*  - After LZMA_FULL_FLUSH when using Stream encoder: Set a new filter
*    chain, which will be used starting from the next Block.
*
*  - After LZMA_SYNC_FLUSH using Raw, Block, or Stream encoder: Change
*    the filter-specific options in the middle of encoding. The actual
*    filters in the chain (Filter IDs) cannot be changed. In the future,
*    it might become possible to change the filter options without
*    using LZMA_SYNC_FLUSH.
*
* While rarely useful, this function may be called also when no data has
* been compressed yet. In that case, this function will behave as if
* LZMA_FULL_FLUSH (Stream encoder) or LZMA_SYNC_FLUSH (Raw or Block
* encoder) had been used right before calling this function.
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_MEMLIMIT_ERROR
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_filters_update(lzma_stream *strm, const lzma_filter *filters) lzma_nothrow;


/**
* \brief       Single-call raw encoder
*
* \param       filters     Array of lzma_filter structures. The end of the
*                          array must be marked with .id = LZMA_VLI_UNKNOWN.
* \param       allocator   lzma_allocator for custom allocator functions.
*                          Set to NULL to use malloc() and free().
* \param       in          Beginning of the input buffer
* \param       in_size     Size of the input buffer
* \param       out         Beginning of the output buffer
* \param       out_pos     The next byte will be written to out[*out_pos].
*                          *out_pos is updated only if encoding succeeds.
* \param       out_size    Size of the out buffer; the first byte into
*                          which no data is written to is out[out_size].
*
* \return      - LZMA_OK: Encoding was successful.
*              - LZMA_BUF_ERROR: Not enough output buffer space.
*              - LZMA_OPTIONS_ERROR
*              - LZMA_MEM_ERROR
*              - LZMA_DATA_ERROR
*              - LZMA_PROG_ERROR
*
* \note        There is no function to calculate how big output buffer
*              would surely be big enough. (lzma_stream_buffer_bound()
*              works only for lzma_stream_buffer_encode(); raw encoder
*              won't necessarily meet that bound.)
*/
extern LZMA_API(lzma_ret) lzma_raw_buffer_encode(const lzma_filter *filters, const lzma_allocator *allocator, const uint8_t *in, size_t in_size, uint8_t *out, size_t *out_pos, size_t out_size) lzma_nothrow;


/**
* \brief       Single-call raw decoder
*
* \param       filters     Array of lzma_filter structures. The end of the
*                          array must be marked with .id = LZMA_VLI_UNKNOWN.
* \param       allocator   lzma_allocator for custom allocator functions.
*                          Set to NULL to use malloc() and free().
* \param       in          Beginning of the input buffer
* \param       in_pos      The next byte will be read from in[*in_pos].
*                          *in_pos is updated only if decoding succeeds.
* \param       in_size     Size of the input buffer; the first byte that
*                          won't be read is in[in_size].
* \param       out         Beginning of the output buffer
* \param       out_pos     The next byte will be written to out[*out_pos].
*                          *out_pos is updated only if encoding succeeds.
* \param       out_size    Size of the out buffer; the first byte into
*                          which no data is written to is out[out_size].
*/
extern LZMA_API(lzma_ret) lzma_raw_buffer_decode(const lzma_filter *filters, const lzma_allocator *allocator, const uint8_t *in, size_t *in_pos, size_t in_size, uint8_t *out, size_t *out_pos, size_t out_size) lzma_nothrow;


/**
* \brief       Get the size of the Filter Properties field
*
* This function may be useful when implementing custom file formats
* using the raw encoder and decoder.
*
* \param       size    Pointer to uint32_t to hold the size of the properties
* \param       filter  Filter ID and options (the size of the properties may
*                      vary depending on the options)
*
* \return      - LZMA_OK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*
* \note        This function validates the Filter ID, but does not
*              necessarily validate the options. Thus, it is possible
*              that this returns LZMA_OK while the following call to
*              lzma_properties_encode() returns LZMA_OPTIONS_ERROR.
*/
extern LZMA_API(lzma_ret) lzma_properties_size(uint32_t *size, const lzma_filter *filter) lzma_nothrow;


/**
* \brief       Encode the Filter Properties field
*
* \param       filter  Filter ID and options
* \param       props   Buffer to hold the encoded options. The size of
*                      buffer must have been already determined with
*                      lzma_properties_size().
*
* \return      - LZMA_OK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*
* \note        Even this function won't validate more options than actually
*              necessary. Thus, it is possible that encoding the properties
*              succeeds but using the same options to initialize the encoder
*              will fail.
*
* \note        If lzma_properties_size() indicated that the size
*              of the Filter Properties field is zero, calling
*              lzma_properties_encode() is not required, but it
*              won't do any harm either.
*/
extern LZMA_API(lzma_ret) lzma_properties_encode(const lzma_filter *filter, uint8_t *props) lzma_nothrow;


/**
* \brief       Decode the Filter Properties field
*
* \param       filter      filter->id must have been set to the correct
*                          Filter ID. filter->options doesn't need to be
*                          initialized (it's not freed by this function). The
*                          decoded options will be stored to filter->options.
*                          filter->options is set to NULL if there are no
*                          properties or if an error occurs.
* \param       allocator   Custom memory allocator used to allocate the
*                          options. Set to NULL to use the default malloc(),
*                          and in case of an error, also free().
* \param       props       Input buffer containing the properties.
* \param       props_size  Size of the properties. This must be the exact
*                          size; giving too much or too little input will
*                          return LZMA_OPTIONS_ERROR.
*
* \return      - LZMA_OK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_MEM_ERROR
*/
extern LZMA_API(lzma_ret) lzma_properties_decode(lzma_filter *filter, const lzma_allocator *allocator, const uint8_t *props, size_t props_size) lzma_nothrow;


/**
* \brief Calculate encoded size of a Filter Flags field
*
* Knowing the size of Filter Flags is useful to know when allocating
* memory to hold the encoded Filter Flags.
*
* \param       size    Pointer to integer to hold the calculated size
* \param       filter  Filter ID and associated options whose encoded
*                      size is to be calculated
*
* \return      - LZMA_OK: *size set successfully. Note that this doesn't
*                guarantee that filter->options is valid, thus
*                lzma_filter_flags_encode() may still fail.
*              - LZMA_OPTIONS_ERROR: Unknown Filter ID or unsupported options.
*              - LZMA_PROG_ERROR: Invalid options
*
* \note        If you need to calculate size of List of Filter Flags,
*              you need to loop over every lzma_filter entry.
*/
extern LZMA_API(lzma_ret) lzma_filter_flags_size(uint32_t *size, const lzma_filter *filter) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief Encode Filter Flags into given buffer
*
* In contrast to some functions, this doesn't allocate the needed buffer.
* This is due to how this function is used internally by liblzma.
*
* \param       filter      Filter ID and options to be encoded
* \param       out         Beginning of the output buffer
* \param       out_pos     out[*out_pos] is the next write position. This
*                          is updated by the encoder.
* \param       out_size    out[out_size] is the first byte to not write.
*
* \return      - LZMA_OK: Encoding was successful.
*              - LZMA_OPTIONS_ERROR: Invalid or unsupported options.
*              - LZMA_PROG_ERROR: Invalid options or not enough output
*                buffer space (you should have checked it with
*                lzma_filter_flags_size()).
*/
extern LZMA_API(lzma_ret) lzma_filter_flags_encode(const lzma_filter *filter, uint8_t *out, size_t *out_pos, size_t out_size) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief Decode Filter Flags from given buffer
*
* The decoded result is stored into *filter. The old value of
* filter->options is not free()d.
*
* \return      - LZMA_OK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_MEM_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_filter_flags_decode(lzma_filter *filter, const lzma_allocator *allocator, const uint8_t *in, size_t *in_pos, size_t in_size) lzma_nothrow lzma_attr_warn_unused_result;


/* Filter IDs for lzma_filter.id */

#define LZMA_FILTER_X86 LZMA_VLI_C(0x04)
/**<
* Filter for x86 binaries
*/

#define LZMA_FILTER_POWERPC LZMA_VLI_C(0x05)
/**<
* Filter for Big endian PowerPC binaries
*/

#define LZMA_FILTER_IA64 LZMA_VLI_C(0x06)
/**<
* Filter for IA-64 (Itanium) binaries.
*/

#define LZMA_FILTER_ARM LZMA_VLI_C(0x07)
/**<
* Filter for ARM binaries.
*/

#define LZMA_FILTER_ARMTHUMB LZMA_VLI_C(0x08)
/**<
* Filter for ARM-Thumb binaries.
*/

#define LZMA_FILTER_SPARC LZMA_VLI_C(0x09)
/**<
* Filter for SPARC binaries.
*/


/**
* \brief Options for BCJ filters
*
* The BCJ filters never change the size of the data. Specifying options
* for them is optional: if pointer to options is NULL, default value is
* used. You probably never need to specify options to BCJ filters, so just
* set the options pointer to NULL and be happy.
*
* If options with non-default values have been specified when encoding,
* the same options must also be specified when decoding.
*
* \note        At the moment, none of the BCJ filters support
*              LZMA_SYNC_FLUSH. If LZMA_SYNC_FLUSH is specified,
*              LZMA_OPTIONS_ERROR will be returned. If there is need,
*              partial support for LZMA_SYNC_FLUSH can be added in future.
*              Partial means that flushing would be possible only at
*              offsets that are multiple of 2, 4, or 16 depending on
*              the filter, except x86 which cannot be made to support
*              LZMA_SYNC_FLUSH predictably.
*/
typedef struct
{
	/**
	* \brief Start offset for conversions
	* This setting is useful only when the same filter is used
	* _separately_ for multiple sections of the same executable file,
	* and the sections contain cross-section branch/call/jump
	* instructions. In that case it is beneficial to set the start
	* offset of the non-first sections so that the relative addresses
	* of the cross-section branch/call/jump instructions will use the
	* same absolute addresses as in the first section.
	* When the pointer to options is NULL, the default value (zero)
	* is used.
	*/
	uint32_t start_offset;

}
lzma_options_bcj;


/**
* \brief Filter ID
*
* Filter ID of the Delta filter. This is used as lzma_filter.id.
*/
#define LZMA_FILTER_DELTA LZMA_VLI_C(0x03)


/**
* \brief Type of the delta calculation
*
* Currently only byte-wise delta is supported. Other possible types could
* be, for example, delta of 16/32/64-bit little/big endian integers, but
* these are not currently planned since byte-wise delta is almost as good.
*/
typedef enum {
	LZMA_DELTA_TYPE_BYTE
} lzma_delta_type;


/**
* \brief Options for the Delta filter
* These options are needed by both encoder and decoder.
*/
typedef struct {
	/** For now, this must always be LZMA_DELTA_TYPE_BYTE. */
	lzma_delta_type type;

	/**
	* \brief Delta distance
	* With the only currently supported type, LZMA_DELTA_TYPE_BYTE,
	* the distance is as bytes.
	* Examples:
	*  - 16-bit stereo audio: distance = 4 bytes
	*  - 24-bit RGB image data: distance = 3 bytes
	*/
	uint32_t dist;
#define LZMA_DELTA_DIST_MIN 1
#define LZMA_DELTA_DIST_MAX 256
}
lzma_options_delta;


/**
* \brief LZMA1 Filter ID
* LZMA1 is the very same thing as what was called just LZMA in LZMA Utils,
* 7-Zip, and LZMA SDK. It's called LZMA1 here to prevent developers from
* accidentally using LZMA when they actually want LZMA2.
* LZMA1 shouldn't be used for new applications unless you _really_ know
* what you are doing. LZMA2 is almost always a better choice.
*/
#define LZMA_FILTER_LZMA1 LZMA_VLI_C(0x4000000000000001)

/**
* \brief LZMA2 Filter ID
* Usually you want this instead of LZMA1. Compared to LZMA1, LZMA2 adds
* support for LZMA_SYNC_FLUSH, uncompressed chunks (smaller expansion
* when trying to compress uncompressible data), possibility to change
* lc/lp/pb in the middle of encoding, and some other internal improvements.
*/
#define LZMA_FILTER_LZMA2 LZMA_VLI_C(0x21)


/**
* \brief Match finders
*
* Match finder has major effect on both speed and compression ratio.
* Usually hash chains are faster than binary trees.
* If you will use LZMA_SYNC_FLUSH often, the hash chains may be a better
* choice, because binary trees get much higher compression ratio penalty
* with LZMA_SYNC_FLUSH.
* The memory usage formulas are only rough estimates, which are closest to
* reality when dict_size is a power of two. The formulas are  more complex
* in reality, and can also change a little between liblzma versions. Use
* lzma_raw_encoder_memusage() to get more accurate estimate of memory usage.
*/
typedef enum
{
	LZMA_MF_HC3 = 0x03,
	/**<
	* \brief Hash Chain with 2- and 3-byte hashing
	* Minimum nice_len: 3
	* Memory usage:
	*  - dict_size <= 16 MiB: dict_size * 7.5
	*  - dict_size > 16 MiB: dict_size * 5.5 + 64 MiB
	*/

	LZMA_MF_HC4 = 0x04,
	/**<
	* \brief Hash Chain with 2-, 3-, and 4-byte hashing
	* Minimum nice_len: 4
	* Memory usage:
	*  - dict_size <= 32 MiB: dict_size * 7.5
	*  - dict_size > 32 MiB: dict_size * 6.5
	*/

	LZMA_MF_BT2 = 0x12,
	/**<
	* \brief Binary Tree with 2-byte hashing
	* Minimum nice_len: 2
	* Memory usage: dict_size * 9.5
	*/

	LZMA_MF_BT3 = 0x13,
	/**<
	* \brief Binary Tree with 2- and 3-byte hashing
	* Minimum nice_len: 3
	* Memory usage:
	*  - dict_size <= 16 MiB: dict_size * 11.5
	*  - dict_size > 16 MiB: dict_size * 9.5 + 64 MiB
	*/

	LZMA_MF_BT4 = 0x14
	/**<
	* \brief Binary Tree with 2-, 3-, and 4-byte hashing
	* Minimum nice_len: 4
	* Memory usage:
	*  - dict_size <= 32 MiB: dict_size * 11.5
	*  - dict_size > 32 MiB: dict_size * 10.5
	*/
}
lzma_match_finder;


/**
* \brief Test if given match finder is supported
* Return true if the given match finder is supported by this liblzma build.
* Otherwise false is returned. It is safe to call this with a value that
* isn't listed in lzma_match_finder enumeration; the return value will be
* false.
* There is no way to list which match finders are available in this
* particular liblzma version and build. It would be useless, because
* a new match finder, which the application developer wasn't aware,
* could require giving additional options to the encoder that the older
* match finders don't need.
*/
extern LZMA_API(lzma_bool) lzma_mf_is_supported(lzma_match_finder match_finder) lzma_nothrow lzma_attr_const;


/**
* \brief Compression modes
* This selects the function used to analyze the data produced by the match
* finder.
*/
typedef enum
{
	LZMA_MODE_FAST = 1,
	/**<
	* \brief Fast compression
	* Fast mode is usually at its best when combined with
	* a hash chain match finder.
	*/

	LZMA_MODE_NORMAL = 2
	/**<
	* \brief Normal compression
	* This is usually notably slower than fast mode. Use this
	* together with binary tree match finders to expose the
	* full potential of the LZMA1 or LZMA2 encoder.
	*/
}
lzma_mode;


/**
* \brief Test if given compression mode is supported
*
* Return true if the given compression mode is supported by this liblzma
* build. Otherwise false is returned. It is safe to call this with a value
* that isn't listed in lzma_mode enumeration; the return value will be false.
* There is no way to list which modes are available in this particular
* liblzma version and build. It would be useless, because a new compression
* mode, which the application developer wasn't aware, could require giving
* additional options to the encoder that the older modes don't need.
*/
extern LZMA_API(lzma_bool) lzma_mode_is_supported(lzma_mode mode) lzma_nothrow lzma_attr_const;


/**
* \brief Options specific to the LZMA1 and LZMA2 filters
* Since LZMA1 and LZMA2 share most of the code, it's simplest to share
* the options structure too. For encoding, all but the reserved variables
* need to be initialized unless specifically mentioned otherwise.
* lzma_lzma_preset() can be used to get a good starting point.
* For raw decoding, both LZMA1 and LZMA2 need dict_size, preset_dict, and
* preset_dict_size (if preset_dict != NULL). LZMA1 needs also lc, lp, and pb.
*/
typedef struct
{
	/**
	* \brief Dictionary size in bytes
	* Dictionary size indicates how many bytes of the recently processed
	* uncompressed data is kept in memory. One method to reduce size of
	* the uncompressed data is to store distance-length pairs, which
	* indicate what data to repeat from the dictionary buffer. Thus,
	* the bigger the dictionary, the better the compression ratio
	* usually is.
	* Maximum size of the dictionary depends on multiple things:
	*  - Memory usage limit
	*  - Available address space (not a problem on 64-bit systems)
	*  - Selected match finder (encoder only)
	* Currently the maximum dictionary size for encoding is 1.5 GiB
	* (i.e. (UINT32_C(1) << 30) + (UINT32_C(1) << 29)) even on 64-bit
	* systems for certain match finder implementation reasons. In the
	* future, there may be match finders that support bigger
	* dictionaries.
	* Decoder already supports dictionaries up to 4 GiB - 1 B (i.e.
	* UINT32_MAX), so increasing the maximum dictionary size of the
	* encoder won't cause problems for old decoders.
	* Because extremely small dictionaries sizes would have unneeded
	* overhead in the decoder, the minimum dictionary size is 4096 bytes.
	* \note When decoding, too big dictionary does no other harm than wasting memory.
	*/
	uint32_t dict_size;
#define LZMA_DICT_SIZE_MIN       UINT32_C(4096)
#define LZMA_DICT_SIZE_DEFAULT   (UINT32_C(1) << 23)

	/**
	* \brief Pointer to an initial dictionary
	* It is possible to initialize the LZ77 history window using
	* a preset dictionary. It is useful when compressing many
	* similar, relatively small chunks of data independently from
	* each other. The preset dictionary should contain typical
	* strings that occur in the files being compressed. The most
	* probable strings should be near the end of the preset dictionary.
	* This feature should be used only in special situations. For
	* now, it works correctly only with raw encoding and decoding.
	* Currently none of the container formats supported by
	* liblzma allow preset dictionary when decoding, thus if
	* you create a .xz or .lzma file with preset dictionary, it
	* cannot be decoded with the regular decoder functions. In the
	* future, the .xz format will likely get support for preset
	* dictionary though.
	*/
	const uint8_t *preset_dict;

	/**
	* \brief Size of the preset dictionary
	* Specifies the size of the preset dictionary. If the size is
	* bigger than dict_size, only the last dict_size bytes are
	* processed.
	* This variable is read only when preset_dict is not NULL.
	* If preset_dict is not NULL but preset_dict_size is zero,
	* no preset dictionary is used (identical to only setting
	* preset_dict to NULL).
	*/
	uint32_t preset_dict_size;

	/**
	* \brief Number of literal context bits
	* How many of the highest bits of the previous uncompressed
	* eight-bit byte (also known as `literal') are taken into
	* account when predicting the bits of the next literal.
	* E.g. in typical English text, an upper-case letter is
	* often followed by a lower-case letter, and a lower-case
	* letter is usually followed by another lower-case letter.
	* In the US-ASCII character set, the highest three bits are 010
	* for upper-case letters and 011 for lower-case letters.
	* When lc is at least 3, the literal coding can take advantage of
	* this property in the uncompressed data.
	* There is a limit that applies to literal context bits and literal
	* position bits together: lc + lp <= 4. Without this limit the
	* decoding could become very slow, which could have security related
	* results in some cases like email servers doing virus scanning.
	* This limit also simplifies the internal implementation in liblzma.
	* There may be LZMA1 streams that have lc + lp > 4 (maximum possible
	* lc would be 8). It is not possible to decode such streams with
	* liblzma.
	*/
	uint32_t lc;
#define LZMA_LCLP_MIN    0
#define LZMA_LCLP_MAX    4
#define LZMA_LC_DEFAULT  3

	/**
	* \brief Number of literal position bits
	* lp affects what kind of alignment in the uncompressed data is
	* assumed when encoding literals. A literal is a single 8-bit byte.
	* See pb below for more information about alignment.
	*/
	uint32_t lp;
#	define LZMA_LP_DEFAULT  0

	/**
	* \brief Number of position bits
	* pb affects what kind of alignment in the uncompressed data is
	* assumed in general. The default means four-byte alignment
	* (2^ pb =2^2=4), which is often a good choice when there's
	* no better guess.
	* When the aligment is known, setting pb accordingly may reduce
	* the file size a little. E.g. with text files having one-byte
	* alignment (US-ASCII, ISO-8859-*, UTF-8), setting pb=0 can
	* improve compression slightly. For UTF-16 text, pb=1 is a good
	* choice. If the alignment is an odd number like 3 bytes, pb=0
	* might be the best choice.
	* Even though the assumed alignment can be adjusted with pb and
	* lp, LZMA1 and LZMA2 still slightly favor 16-byte alignment.
	* It might be worth taking into account when designing file formats
	* that are likely to be often compressed with LZMA1 or LZMA2.
	*/
	uint32_t pb;
#define LZMA_PB_MIN      0
#define LZMA_PB_MAX      4
#define LZMA_PB_DEFAULT  2

	/** Compression mode */
	lzma_mode mode;

	/**
	* \brief Nice length of a match
	*
	* This determines how many bytes the encoder compares from the match
	* candidates when looking for the best match. Once a match of at
	* least nice_len bytes long is found, the encoder stops looking for
	* better candidates and encodes the match. (Naturally, if the found
	* match is actually longer than nice_len, the actual length is
	* encoded; it's not truncated to nice_len.)
	*
	* Bigger values usually increase the compression ratio and
	* compression time. For most files, 32 to 128 is a good value,
	* which gives very good compression ratio at good speed.
	*
	* The exact minimum value depends on the match finder. The maximum
	* is 273, which is the maximum length of a match that LZMA1 and
	* LZMA2 can encode.
	*/
	uint32_t nice_len;

	/** Match finder ID */
	lzma_match_finder mf;

	/**
	* \brief Maximum search depth in the match finder
	*
	* For every input byte, match finder searches through the hash chain
	* or binary tree in a loop, each iteration going one step deeper in
	* the chain or tree. The searching stops if
	*  - a match of at least nice_len bytes long is found;
	*  - all match candidates from the hash chain or binary tree have
	*    been checked; or
	*  - maximum search depth is reached.
	*
	* Maximum search depth is needed to prevent the match finder from
	* wasting too much time in case there are lots of short match
	* candidates. On the other hand, stopping the search before all
	* candidates have been checked can reduce compression ratio.
	*
	* Setting depth to zero tells liblzma to use an automatic default
	* value, that depends on the selected match finder and nice_len.
	* The default is in the range [4, 200] or so (it may vary between
	* liblzma versions).
	*
	* Using a bigger depth value than the default can increase
	* compression ratio in some cases. There is no strict maximum value,
	* but high values (thousands or millions) should be used with care:
	* the encoder could remain fast enough with typical input, but
	* malicious input could cause the match finder to slow down
	* dramatically, possibly creating a denial of service attack.
	*/
	uint32_t depth;
}
lzma_options_lzma;


/**
* \brief Set a compression preset to lzma_options_lzma structure
* 0 is the fastest and 9 is the slowest. These match the switches -0 .. -9
* of the xz command line tool. In addition, it is possible to bitwise-or
* flags to the preset. Currently only LZMA_PRESET_EXTREME is supported.
* The flags are defined in container.h, because the flags are used also
* with lzma_easy_encoder().
* The preset values are subject to changes between liblzma versions.
* This function is available only if LZMA1 or LZMA2 encoder has been enabled
* when building liblzma.
* \return On success, false is returned. If the preset is not supported, true is returned.
*/
extern LZMA_API(lzma_bool) lzma_lzma_preset(lzma_options_lzma *options, uint32_t preset) lzma_nothrow;


/**
* \brief       Default compression preset
*
* It's not straightforward to recommend a default preset, because in some
* cases keeping the resource usage relatively low is more important that
* getting the maximum compression ratio.
*/
#define LZMA_PRESET_DEFAULT     UINT32_C(6)


/**
* \brief       Mask for preset level
*
* This is useful only if you need to extract the level from the preset
* variable. That should be rare.
*/
#define LZMA_PRESET_LEVEL_MASK  UINT32_C(0x1F)


/*
* Preset flags
*
* Currently only one flag is defined.
*/

/**
* \brief       Extreme compression preset
*
* This flag modifies the preset to make the encoding significantly slower
* while improving the compression ratio only marginally. This is useful
* when you don't mind wasting time to get as small result as possible.
*
* This flag doesn't affect the memory usage requirements of the decoder (at
* least not significantly). The memory usage of the encoder may be increased
* a little but only at the lowest preset levels (0-3).
*/
#define LZMA_PRESET_EXTREME       (UINT32_C(1) << 31)


/**
* \brief       Multithreading options
*/
typedef struct {
	/**
	* \brief       Flags
	*
	* Set this to zero if no flags are wanted.
	*
	* No flags are currently supported.
	*/
	uint32_t flags;

	/**
	* \brief       Number of worker threads to use
	*/
	uint32_t threads;

	/**
	* \brief       Maximum uncompressed size of a Block
	*
	* The encoder will start a new .xz Block every block_size bytes.
	* Using LZMA_FULL_FLUSH or LZMA_FULL_BARRIER with lzma_code()
	* the caller may tell liblzma to start a new Block earlier.
	*
	* With LZMA2, a recommended block size is 2-4 times the LZMA2
	* dictionary size. With very small dictionaries, it is recommended
	* to use at least 1 MiB block size for good compression ratio, even
	* if this is more than four times the dictionary size. Note that
	* these are only recommendations for typical use cases; feel free
	* to use other values. Just keep in mind that using a block size
	* less than the LZMA2 dictionary size is waste of RAM.
	*
	* Set this to 0 to let liblzma choose the block size depending
	* on the compression options. For LZMA2 it will be 3*dict_size
	* or 1 MiB, whichever is more.
	*
	* For each thread, about 3 * block_size bytes of memory will be
	* allocated. This may change in later liblzma versions. If so,
	* the memory usage will probably be reduced, not increased.
	*/
	uint64_t block_size;

	/**
	* \brief       Timeout to allow lzma_code() to return early
	*
	* Multithreading can make liblzma to consume input and produce
	* output in a very bursty way: it may first read a lot of input
	* to fill internal buffers, then no input or output occurs for
	* a while.
	*
	* In single-threaded mode, lzma_code() won't return until it has
	* either consumed all the input or filled the output buffer. If
	* this is done in multithreaded mode, it may cause a call
	* lzma_code() to take even tens of seconds, which isn't acceptable
	* in all applications.
	*
	* To avoid very long blocking times in lzma_code(), a timeout
	* (in milliseconds) may be set here. If lzma_code() would block
	* longer than this number of milliseconds, it will return with
	* LZMA_OK. Reasonable values are 100 ms or more. The xz command
	* line tool uses 300 ms.
	*
	* If long blocking times are fine for you, set timeout to a special
	* value of 0, which will disable the timeout mechanism and will make
	* lzma_code() block until all the input is consumed or the output
	* buffer has been filled.
	*
	* \note        Even with a timeout, lzma_code() might sometimes take
	*              somewhat long time to return. No timing guarantees
	*              are made.
	*/
	uint32_t timeout;

	/**
	* \brief       Compression preset (level and possible flags)
	*
	* The preset is set just like with lzma_easy_encoder().
	* The preset is ignored if filters below is non-NULL.
	*/
	uint32_t preset;

	/**
	* \brief       Filter chain (alternative to a preset)
	*
	* If this is NULL, the preset above is used. Otherwise the preset
	* is ignored and the filter chain specified here is used.
	*/
	const lzma_filter *filters;

	/**
	* \brief       Integrity check type
	*
	* See check.h for available checks. The xz command line tool
	* defaults to LZMA_CHECK_CRC64, which is a good choice if you
	* are unsure.
	*/
	lzma_check check;

	/*
	* Reserved space to allow possible future extensions without
	* breaking the ABI. You should not touch these, because the names
	* of these variables may change. These are and will never be used
	* with the currently supported options, so it is safe to leave these
	* uninitialized.
	*/
	lzma_reserved_enum reserved_enum1;
	lzma_reserved_enum reserved_enum2;
	lzma_reserved_enum reserved_enum3;
	uint32_t reserved_int1;
	uint32_t reserved_int2;
	uint32_t reserved_int3;
	uint32_t reserved_int4;
	uint64_t reserved_int5;
	uint64_t reserved_int6;
	uint64_t reserved_int7;
	uint64_t reserved_int8;
	void *reserved_ptr1;
	void *reserved_ptr2;
	void *reserved_ptr3;
	void *reserved_ptr4;

} lzma_mt;


/**
* \brief       Calculate approximate memory usage of easy encoder
*
* This function is a wrapper for lzma_raw_encoder_memusage().
*
* \param       preset  Compression preset (level and possible flags)
*
* \return      Number of bytes of memory required for the given
*              preset when encoding. If an error occurs, for example
*              due to unsupported preset, UINT64_MAX is returned.
*/
extern LZMA_API(uint64_t) lzma_easy_encoder_memusage(uint32_t preset)
lzma_nothrow lzma_attr_pure;


/**
* \brief       Calculate approximate decoder memory usage of a preset
*
* This function is a wrapper for lzma_raw_decoder_memusage().
*
* \param       preset  Compression preset (level and possible flags)
*
* \return      Number of bytes of memory required to decompress a file
*              that was compressed using the given preset. If an error
*              occurs, for example due to unsupported preset, UINT64_MAX
*              is returned.
*/
extern LZMA_API(uint64_t) lzma_easy_decoder_memusage(uint32_t preset)
lzma_nothrow lzma_attr_pure;


/**
* \brief       Initialize .xz Stream encoder using a preset number
*
* This function is intended for those who just want to use the basic features
* if liblzma (that is, most developers out there).
*
* \param       strm    Pointer to lzma_stream that is at least initialized
*                      with LZMA_STREAM_INIT.
* \param       preset  Compression preset to use. A preset consist of level
*                      number and zero or more flags. Usually flags aren't
*                      used, so preset is simply a number [0, 9] which match
*                      the options -0 ... -9 of the xz command line tool.
*                      Additional flags can be be set using bitwise-or with
*                      the preset level number, e.g. 6 | LZMA_PRESET_EXTREME.
* \param       check   Integrity check type to use. See check.h for available
*                      checks. The xz command line tool defaults to
*                      LZMA_CHECK_CRC64, which is a good choice if you are
*                      unsure. LZMA_CHECK_CRC32 is good too as long as the
*                      uncompressed file is not many gigabytes.
*
* \return      - LZMA_OK: Initialization succeeded. Use lzma_code() to
*                encode your data.
*              - LZMA_MEM_ERROR: Memory allocation failed.
*              - LZMA_OPTIONS_ERROR: The given compression preset is not
*                supported by this build of liblzma.
*              - LZMA_UNSUPPORTED_CHECK: The given check type is not
*                supported by this liblzma build.
*              - LZMA_PROG_ERROR: One or more of the parameters have values
*                that will never be valid. For example, strm == NULL.
*
* If initialization fails (return value is not LZMA_OK), all the memory
* allocated for *strm by liblzma is always freed. Thus, there is no need
* to call lzma_end() after failed initialization.
*
* If initialization succeeds, use lzma_code() to do the actual encoding.
* Valid values for `action' (the second argument of lzma_code()) are
* LZMA_RUN, LZMA_SYNC_FLUSH, LZMA_FULL_FLUSH, and LZMA_FINISH. In future,
* there may be compression levels or flags that don't support LZMA_SYNC_FLUSH.
*/
extern LZMA_API(lzma_ret) lzma_easy_encoder(
	lzma_stream *strm, uint32_t preset, lzma_check check)
	lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Single-call .xz Stream encoding using a preset number
*
* The maximum required output buffer size can be calculated with
* lzma_stream_buffer_bound().
*
* \param       preset      Compression preset to use. See the description
*                          in lzma_easy_encoder().
* \param       check       Type of the integrity check to calculate from
*                          uncompressed data.
* \param       allocator   lzma_allocator for custom allocator functions.
*                          Set to NULL to use malloc() and free().
* \param       in          Beginning of the input buffer
* \param       in_size     Size of the input buffer
* \param       out         Beginning of the output buffer
* \param       out_pos     The next byte will be written to out[*out_pos].
*                          *out_pos is updated only if encoding succeeds.
* \param       out_size    Size of the out buffer; the first byte into
*                          which no data is written to is out[out_size].
*
* \return      - LZMA_OK: Encoding was successful.
*              - LZMA_BUF_ERROR: Not enough output buffer space.
*              - LZMA_UNSUPPORTED_CHECK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_MEM_ERROR
*              - LZMA_DATA_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_easy_buffer_encode(
	uint32_t preset, lzma_check check,
	const lzma_allocator *allocator,
	const uint8_t *in, size_t in_size,
	uint8_t *out, size_t *out_pos, size_t out_size) lzma_nothrow;


/**
* \brief       Initialize .xz Stream encoder using a custom filter chain
*
* \param       strm    Pointer to properly prepared lzma_stream
* \param       filters Array of filters. This must be terminated with
*                      filters[n].id = LZMA_VLI_UNKNOWN. See filter.h for
*                      more information.
* \param       check   Type of the integrity check to calculate from
*                      uncompressed data.
*
* \return      - LZMA_OK: Initialization was successful.
*              - LZMA_MEM_ERROR
*              - LZMA_UNSUPPORTED_CHECK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_stream_encoder(lzma_stream *strm,
	const lzma_filter *filters, lzma_check check)
	lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Calculate approximate memory usage of multithreaded .xz encoder
*
* Since doing the encoding in threaded mode doesn't affect the memory
* requirements of single-threaded decompressor, you can use
* lzma_easy_decoder_memusage(options->preset) or
* lzma_raw_decoder_memusage(options->filters) to calculate
* the decompressor memory requirements.
*
* \param       options Compression options
*
* \return      Number of bytes of memory required for encoding with the
*              given options. If an error occurs, for example due to
*              unsupported preset or filter chain, UINT64_MAX is returned.
*/
extern LZMA_API(uint64_t) lzma_stream_encoder_mt_memusage(
	const lzma_mt *options) lzma_nothrow lzma_attr_pure;


/**
* \brief       Initialize multithreaded .xz Stream encoder
*
* This provides the functionality of lzma_easy_encoder() and
* lzma_stream_encoder() as a single function for multithreaded use.
*
* The supported actions for lzma_code() are LZMA_RUN, LZMA_FULL_FLUSH,
* LZMA_FULL_BARRIER, and LZMA_FINISH. Support for LZMA_SYNC_FLUSH might be
* added in the future.
*
* \param       strm    Pointer to properly prepared lzma_stream
* \param       options Pointer to multithreaded compression options
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_UNSUPPORTED_CHECK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_stream_encoder_mt(
	lzma_stream *strm, const lzma_mt *options)
	lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Initialize .lzma encoder (legacy file format)
*
* The .lzma format is sometimes called the LZMA_Alone format, which is the
* reason for the name of this function. The .lzma format supports only the
* LZMA1 filter. There is no support for integrity checks like CRC32.
*
* Use this function if and only if you need to create files readable by
* legacy LZMA tools such as LZMA Utils 4.32.x. Moving to the .xz format
* is strongly recommended.
*
* The valid action values for lzma_code() are LZMA_RUN and LZMA_FINISH.
* No kind of flushing is supported, because the file format doesn't make
* it possible.
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_OPTIONS_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_alone_encoder(
	lzma_stream *strm, const lzma_options_lzma *options)
	lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Calculate output buffer size for single-call Stream encoder
*
* When trying to compress uncompressible data, the encoded size will be
* slightly bigger than the input data. This function calculates how much
* output buffer space is required to be sure that lzma_stream_buffer_encode()
* doesn't return LZMA_BUF_ERROR.
*
* The calculated value is not exact, but it is guaranteed to be big enough.
* The actual maximum output space required may be slightly smaller (up to
* about 100 bytes). This should not be a problem in practice.
*
* If the calculated maximum size doesn't fit into size_t or would make the
* Stream grow past LZMA_VLI_MAX (which should never happen in practice),
* zero is returned to indicate the error.
*
* \note        The limit calculated by this function applies only to
*              single-call encoding. Multi-call encoding may (and probably
*              will) have larger maximum expansion when encoding
*              uncompressible data. Currently there is no function to
*              calculate the maximum expansion of multi-call encoding.
*/
extern LZMA_API(size_t) lzma_stream_buffer_bound(size_t uncompressed_size)
lzma_nothrow;


/**
* \brief       Single-call .xz Stream encoder
*
* \param       filters     Array of filters. This must be terminated with
*                          filters[n].id = LZMA_VLI_UNKNOWN. See filter.h
*                          for more information.
* \param       check       Type of the integrity check to calculate from
*                          uncompressed data.
* \param       allocator   lzma_allocator for custom allocator functions.
*                          Set to NULL to use malloc() and free().
* \param       in          Beginning of the input buffer
* \param       in_size     Size of the input buffer
* \param       out         Beginning of the output buffer
* \param       out_pos     The next byte will be written to out[*out_pos].
*                          *out_pos is updated only if encoding succeeds.
* \param       out_size    Size of the out buffer; the first byte into
*                          which no data is written to is out[out_size].
*
* \return      - LZMA_OK: Encoding was successful.
*              - LZMA_BUF_ERROR: Not enough output buffer space.
*              - LZMA_UNSUPPORTED_CHECK
*              - LZMA_OPTIONS_ERROR
*              - LZMA_MEM_ERROR
*              - LZMA_DATA_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_stream_buffer_encode(
	lzma_filter *filters, lzma_check check,
	const lzma_allocator *allocator,
	const uint8_t *in, size_t in_size,
	uint8_t *out, size_t *out_pos, size_t out_size)
	lzma_nothrow lzma_attr_warn_unused_result;


/************
* Decoding *
************/

/**
* This flag makes lzma_code() return LZMA_NO_CHECK if the input stream
* being decoded has no integrity check. Note that when used with
* lzma_auto_decoder(), all .lzma files will trigger LZMA_NO_CHECK
* if LZMA_TELL_NO_CHECK is used.
*/
#define LZMA_TELL_NO_CHECK              UINT32_C(0x01)


/**
* This flag makes lzma_code() return LZMA_UNSUPPORTED_CHECK if the input
* stream has an integrity check, but the type of the integrity check is not
* supported by this liblzma version or build. Such files can still be
* decoded, but the integrity check cannot be verified.
*/
#define LZMA_TELL_UNSUPPORTED_CHECK     UINT32_C(0x02)


/**
* This flag makes lzma_code() return LZMA_GET_CHECK as soon as the type
* of the integrity check is known. The type can then be got with
* lzma_get_check().
*/
#define LZMA_TELL_ANY_CHECK             UINT32_C(0x04)


/**
* This flag makes lzma_code() not calculate and verify the integrity check
* of the compressed data in .xz files. This means that invalid integrity
* check values won't be detected and LZMA_DATA_ERROR won't be returned in
* such cases.
*
* This flag only affects the checks of the compressed data itself; the CRC32
* values in the .xz headers will still be verified normally.
*
* Don't use this flag unless you know what you are doing. Possible reasons
* to use this flag:
*
*   - Trying to recover data from a corrupt .xz file.
*
*   - Speeding up decompression, which matters mostly with SHA-256
*     or with files that have compressed extremely well. It's recommended
*     to not use this flag for this purpose unless the file integrity is
*     verified externally in some other way.
*
* Support for this flag was added in liblzma 5.1.4beta.
*/
#define LZMA_IGNORE_CHECK               UINT32_C(0x10)


/**
* This flag enables decoding of concatenated files with file formats that
* allow concatenating compressed files as is. From the formats currently
* supported by liblzma, only the .xz format allows concatenated files.
* Concatenated files are not allowed with the legacy .lzma format.
*
* This flag also affects the usage of the `action' argument for lzma_code().
* When LZMA_CONCATENATED is used, lzma_code() won't return LZMA_STREAM_END
* unless LZMA_FINISH is used as `action'. Thus, the application has to set
* LZMA_FINISH in the same way as it does when encoding.
*
* If LZMA_CONCATENATED is not used, the decoders still accept LZMA_FINISH
* as `action' for lzma_code(), but the usage of LZMA_FINISH isn't required.
*/
#define LZMA_CONCATENATED               UINT32_C(0x08)


/**
* \brief       Initialize .xz Stream decoder
*
* \param       strm        Pointer to properly prepared lzma_stream
* \param       memlimit    Memory usage limit as bytes. Use UINT64_MAX
*                          to effectively disable the limiter.
* \param       flags       Bitwise-or of zero or more of the decoder flags:
*                          LZMA_TELL_NO_CHECK, LZMA_TELL_UNSUPPORTED_CHECK,
*                          LZMA_TELL_ANY_CHECK, LZMA_CONCATENATED
*
* \return      - LZMA_OK: Initialization was successful.
*              - LZMA_MEM_ERROR: Cannot allocate memory.
*              - LZMA_OPTIONS_ERROR: Unsupported flags
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_stream_decoder(lzma_stream *strm, uint64_t memlimit, uint32_t flags) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Decode .xz Streams and .lzma files with autodetection
*
* This decoder autodetects between the .xz and .lzma file formats, and
* calls lzma_stream_decoder() or lzma_alone_decoder() once the type
* of the input file has been detected.
*
* \param       strm        Pointer to properly prepared lzma_stream
* \param       memlimit    Memory usage limit as bytes. Use UINT64_MAX
*                          to effectively disable the limiter.
* \param       flags       Bitwise-or of flags, or zero for no flags.
*
* \return      - LZMA_OK: Initialization was successful.
*              - LZMA_MEM_ERROR: Cannot allocate memory.
*              - LZMA_OPTIONS_ERROR: Unsupported flags
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_auto_decoder(lzma_stream *strm, uint64_t memlimit, uint32_t flags) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Initialize .lzma decoder (legacy file format)
*
* Valid `action' arguments to lzma_code() are LZMA_RUN and LZMA_FINISH.
* There is no need to use LZMA_FINISH, but allowing it may simplify
* certain types of applications.
*
* \return      - LZMA_OK
*              - LZMA_MEM_ERROR
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_alone_decoder(lzma_stream *strm, uint64_t memlimit) lzma_nothrow lzma_attr_warn_unused_result;


/**
* \brief       Single-call .xz Stream decoder
*
* \param       memlimit    Pointer to how much memory the decoder is allowed
*                          to allocate. The value pointed by this pointer is
*                          modified if and only if LZMA_MEMLIMIT_ERROR is
*                          returned.
* \param       flags       Bitwise-or of zero or more of the decoder flags:
*                          LZMA_TELL_NO_CHECK, LZMA_TELL_UNSUPPORTED_CHECK,
*                          LZMA_CONCATENATED. Note that LZMA_TELL_ANY_CHECK
*                          is not allowed and will return LZMA_PROG_ERROR.
* \param       allocator   lzma_allocator for custom allocator functions.
*                          Set to NULL to use malloc() and free().
* \param       in          Beginning of the input buffer
* \param       in_pos      The next byte will be read from in[*in_pos].
*                          *in_pos is updated only if decoding succeeds.
* \param       in_size     Size of the input buffer; the first byte that
*                          won't be read is in[in_size].
* \param       out         Beginning of the output buffer
* \param       out_pos     The next byte will be written to out[*out_pos].
*                          *out_pos is updated only if decoding succeeds.
* \param       out_size    Size of the out buffer; the first byte into
*                          which no data is written to is out[out_size].
*
* \return      - LZMA_OK: Decoding was successful.
*              - LZMA_FORMAT_ERROR
*              - LZMA_OPTIONS_ERROR
*              - LZMA_DATA_ERROR
*              - LZMA_NO_CHECK: This can be returned only if using
*                the LZMA_TELL_NO_CHECK flag.
*              - LZMA_UNSUPPORTED_CHECK: This can be returned only if using
*                the LZMA_TELL_UNSUPPORTED_CHECK flag.
*              - LZMA_MEM_ERROR
*              - LZMA_MEMLIMIT_ERROR: Memory usage limit was reached.
*                The minimum required memlimit value was stored to *memlimit.
*              - LZMA_BUF_ERROR: Output buffer was too small.
*              - LZMA_PROG_ERROR
*/
extern LZMA_API(lzma_ret) lzma_stream_buffer_decode(uint64_t *memlimit, uint32_t flags, const lzma_allocator *allocator, const uint8_t *in, size_t *in_pos, size_t in_size, uint8_t *out, size_t *out_pos, size_t out_size) lzma_nothrow lzma_attr_warn_unused_result;

#include "stream_flags.h"
#include "block.h"
#include "index.h"
#include "index_hash.h"
#include "hardware.h"


#endif // INCLUDE_LZMA_H
