// embedded.c - Embedded machine code for obfuscated functions.


#include "config.h"
#include "embedded.h"
#include "allocator.h"
#include "mutex.h"
#include "sm.h"


#if defined(SM_OS_LINUX)

#include <sys/mman.h>

#define em_mp__ e050a98d0c544268
static int (*em_mp__)(void*,size_t,int) = mprotect;

// Makes the specified page executable, Linux version.
inline static bool sm_make_executable(void* p, size_t n)
{
	return (em_mp__(p, n, PROT_READ|PROT_WRITE|PROT_EXEC) == 0);
}

#elif defined(SM_OS_WINDOWS)

#include <windows.h>

#define em_mp__ e050a98d0c544268
static BOOL (__stdcall *em_mp__)(LPVOID, SIZE_T, DWORD, PDWORD) = VirtualProtect;

// Makes the specified page executable, Windows version.
inline static bool sm_make_executable(void* p, size_t n)
{
	DWORD d;
	return (em_mp__(p, n, PAGE_EXECUTE_READWRITE, &d) != 0);
}

#else
#error Dynamic code execution is not available.
#endif


// Machine Code


// Test for RDRAND support.
#define sm_have_rdrand_key b63c7ff2c7374891
#define sm_have_rdrand_size e99c50b3fd234e89
#define sm_have_rdrand_crc f99c50b3fd234e89
#define sm_have_rdrand a4321c43f880b202
static uint64_t sm_have_rdrand_key = 0xFBBA362F1CD8462F;
static uint64_t sm_have_rdrand_size = 0x28;
static uint64_t sm_have_rdrand_crc = 0xB3E3CFC3BB642FA8;
static uint8_t sm_have_rdrand[] = { 0x53, 0xB8, 0x01, 0x00, 0x00, 0x00, 0x0F, 0xA2, 0x0F, 0xBA, 0xE1, 0x1E, 0x73, 0x09, 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, 0x5B, 0xC3, 0x48, 0x2B, 0xC0, 0xEB, 0xF2 };


// Get next rand via RDRAND.
#define sm_next_rdrand_key bcfc1a5d78ab4d43
#define sm_next_rdrand_size a91c50b4fd237a82
#define sm_next_rdrand_crc af08ec0c647e4301
#define sm_next_rdrand e407dff8c7534790
static uint8_t sm_next_rdrand_key = 0x139D1AFC60F64C22;
static uint64_t sm_next_rdrand_size = 0x7;
static uint64_t sm_next_rdrand_crc = 0xB3E3CFC3BB642FA8;
static uint8_t sm_next_rdrand[] = { 0x48, 0x0F, 0xC7, 0xF0, 0x73, 0xFA, 0xC3 };


extern void* sm_xor_pass(void *restrict data, register size_t bytes, uint64_t key);


// Methods


// Identical to memcpy.
inline static void* sm_memcpy(void *restrict p, const void *restrict q, size_t n)
{
	register const uint8_t* s;
	register uint8_t* d;

	if (p < q)
	{
		s = (const uint8_t*)q;
		d = (uint8_t*)p;
		while (n--) *d++ = *s++;
	}
	else
	{
		s = (const uint8_t*)q + (n - 1);
		d = (uint8_t*)p + (n - 1);
		while (n--) *d-- = *s--;
	}

	return p;
}


// Loads an encrypted procedure and makes it executable: ptr is the buffer of the code, len is the length, and key is the key to use.
#define LODMPROC f88fb6d1
inline static void* LODMPROC(void *restrict ptr, register size_t len, uint32_t key)
{
	uint8_t ok;
	void* r = malloc(len);
	if (r == 0) return 0;
	sm_memcpy(r, ptr, len);
	MXORPASS(r, len, key);
	EMMKEXEC(r, len, ok);
	if (!ok) { free(r); r = 0; }
	return r;
}


extern sm_context_t callconv sm_allocator_create_context(size_t capacity, uint8_t locked);


// The global context structure.
typedef struct sm_op_context_s
{
	sm_mutex_t mutex; // Context/initialization mutex.
	sm_context_t allocator; // Global allocator.
	sm_err_f error_handler; // Error handler.
	uint8_t ran_cnt; // Count of RNG entries in tab.
	sm_srs64_f ran_srs_tab[256]; // RNG seed functions.
	sm_ran64_f ran_gen_tab[256]; // RNG functions.
	uint64_t ran_ssz_tab[256]; // RNG state vector sizes.
}
sm_op_context_t;


volatile sm_op_context_t* sm_op_context__ = NULL;
volatile sm_get64_f sm_op_have_rdrand_fn__ = NULL;
volatile sm_get64_f sm_op_next_rdrand_fn__ = NULL;


extern uint64_t callconv sm_master_rand();


static bool sm_op_initialize(uint64_t bytes)
{
	if (sm_op_context__) return true;

	sm_context_t allocator = sm_allocator_create_context(bytes, 1);

	if (!allocator) return false;

	sm_op_context_t* context = sm_space_allocate(allocator, sizeof(sm_op_context_t));

	if (!context)
	{
		sm_allocator_destroy_context(allocator);
		return false;
	}

	register uint8_t* p = (uint8_t*)context;
	register size_t n = sizeof(sm_op_context_t);
	while (n-- > 0U) *p++ = 0;

	if (!sm_mutex_create(&context->mutex))
	{
		sm_space_free(allocator, context);
		sm_allocator_destroy_context(allocator);
		return false;
	}

	sm_mutex_lock(&context->mutex);

	context->allocator = allocator;

	sm_op_have_rdrand_fn__ = (sm_get64_f)sm_space_allocate(allocator, sm_have_rdrand_size);
	if (sm_op_have_rdrand_fn__)
	{
		uint64_t key2 = ~sm_yellow_64(sm_shuffle_64(sm_have_rdrand_key) ^ time(NULL));
		sm_xor_cross((void*)sm_op_have_rdrand_fn__, (void*)&sm_have_rdrand[0], sm_have_rdrand_size, sm_have_rdrand_key, key2);
		sm_have_rdrand_key = key2;
		if (!sm_make_executable(sm_op_have_rdrand_fn__, sm_have_rdrand_size))
		{
			sm_space_free(allocator, sm_op_have_rdrand_fn__);
			sm_op_have_rdrand_fn__ = NULL;
		}
	}

	sm_op_next_rdrand_fn__ = (sm_get64_f)sm_space_allocate(allocator, sm_next_rdrand_size);
	if (sm_op_next_rdrand_fn__)
	{
		uint64_t key2 = ~sm_yellow_64(sm_shuffle_64(sm_next_rdrand_key) ^ time(NULL));
		sm_xor_cross((void*)sm_op_next_rdrand_fn__, (void*)&sm_next_rdrand[0], sm_next_rdrand_size, sm_next_rdrand_key, key2);
		sm_next_rdrand_key = key2;
		if (!sm_make_executable(sm_op_next_rdrand_fn__, sm_next_rdrand_size))
		{
			sm_space_free(allocator, sm_op_next_rdrand_fn__);
			sm_op_next_rdrand_fn__ = NULL;
		}
	}

	sm_master_rand();

	sm_mutex_unlock(&context->mutex);

	sm_op_context__ = context;

	return true;
}


static void sm_op_uninitialize()
{
	if (!sm_op_context__) return;
	
	sm_op_context_t* context = sm_op_context__;

	sm_mutex_lock(&sm_op_context__->mutex);
	sm_op_context__ = NULL;

	if (sm_op_have_rdrand_fn__)
	{
		void* tmp = sm_op_have_rdrand_fn__;
		sm_op_have_rdrand_fn__ = NULL;
		sm_space_free(sm_op_context__->allocator, tmp);
	}
	
	if (sm_op_next_rdrand_fn__)
	{
		void* tmp = sm_op_next_rdrand_fn__;
		sm_op_next_rdrand_fn__ = NULL;
		sm_space_free(sm_op_context__->allocator, tmp);
	}

	sm_context_t allocator = context->allocator;

	sm_mutex_unlock(&sm_op_context__->mutex);
	sm_mutex_destroy(&sm_op_context__->mutex);

	register uint8_t* p = (uint8_t*)context;
	register size_t n = sizeof(sm_op_context_t);
	while (n-- > 0U) { *p ^= ~sm_yellow64(sm_shuffle_64(*p)); ++p; }

	sm_space_free(allocator, context);

	sm_allocator_destroy_context(allocator);
}


// Loads an encrypted procedure and makes it executable: dst is the buffer to receive the code from src, len is the length, and key is the key to use.
static void* sm_op_load_function(void *restrict dst, void *restrict src, register size_t len, uint64_t* key)
{
	void* r = NULL;
	uint64_t old_key = *key, new_key = sm_master_rand();

	



	*key = new_key;

	return r;
}


// Exported


sm_ref_t sm_op(uint16_t op, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
	switch (op)
	{
	case SM_OP_CREATE: 
	{ 
		if (!sm_op_context__)
		{
			if (arg1 == 0) arg1 = UINT64_C(0xFFFF);
			return sm_op_initialize(arg1) ? 1 : 0;
		}
		return sm_op_context__ ? 1 : 0;
	}
	case SM_OP_DESTROY: 
	{
		if (!sm_op_context__) return 0;
		sm_op_uninitialize(); 
		return 1; 
	}
	case SM_OP_SET_ERROR_HANDLER:
	{
		if (!sm_op_context__) return 0;
		sm_mutex_lock(&sm_op_context__->mutex);
		sm_op_context__->error_handler = (sm_err_f)arg1;
		sm_mutex_unlock(&sm_op_context__->mutex);
		return 1;
	}

	// Failure

	default:
		if (sm_op_context__ && sm_op_context__->error_handler)
			sm_op_context__->error_handler(SM_ERR_NO_SUCH_COMMAND);
		return 0ULL;
	}
}

