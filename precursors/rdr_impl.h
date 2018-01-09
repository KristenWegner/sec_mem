// rdr_impl.h - Auto-Generated (Wed Jan 3 16:52:28 2018): Switch entries for the 'rdr' module. Include in the sm_get_entity() function switch statement, in your module source file.

case HAVE_RDRAND: return (uint64_t)sm_load_entity(context, 1, have_rdrand_data, have_rdrand_size, &have_rdrand_key, &have_rdrand_crc);
case NEXT_RDRAND: return (uint64_t)sm_load_entity(context, 1, next_rdrand_data, next_rdrand_size, &next_rdrand_key, &next_rdrand_crc);
