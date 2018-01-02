// rdrnd_op_impl.h - Auto-Generated (Sat Dec 30 15:11:23 2017): Switch entries for the 'rdrnd' module. Include in the sm_op() function switch statement, in your module source file.

case SM_HAVE_RDRAND: return (uint64_t)sm_load_entity(context, 1, sm_have_rdrand_data, sm_have_rdrand_size, &sm_have_rdrand_key, &sm_have_rdrand_crc);
case SM_NEXT_RDRAND: return (uint64_t)sm_load_entity(context, 1, sm_next_rdrand_data, sm_next_rdrand_size, &sm_next_rdrand_key, &sm_next_rdrand_crc);
