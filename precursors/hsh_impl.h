// hsh_impl.h - Auto-Generated (Mon Jan 8 14:49:00 2018): Switch entries for the 'hsh' module. Include in the sm_get_entity() function switch statement, in your module source file.

case CITY_HASH: return (uint64_t)sm_load_entity(context, 1, city_hash_data, city_hash_size, &city_hash_key, &city_hash_crc);
case MURMUR_3_HASH: return (uint64_t)sm_load_entity(context, 1, murmur_3_hash_data, murmur_3_hash_size, &murmur_3_hash_key, &murmur_3_hash_crc);
case MURMUR_3_32_HASH: return (uint64_t)sm_load_entity(context, 1, murmur_3_32_hash_data, murmur_3_32_hash_size, &murmur_3_32_hash_key, &murmur_3_32_hash_crc);
case FNV1A_HASH: return (uint64_t)sm_load_entity(context, 1, fnv1a_hash_data, fnv1a_hash_size, &fnv1a_hash_key, &fnv1a_hash_crc);
case SHA_3_STATE: return sha_3_state;
case SHA_3_RESULT: return sha_3_result;
case SHA_3_KEYED: return sha_3_keyed;
case SHA_3_HASH: return (uint64_t)sm_load_entity(context, 1, sha_3_hash_data, sha_3_hash_size, &sha_3_hash_key, &sha_3_hash_crc);
case SIP_STATE: return sip_state;
case SIP_RESULT: return sip_result;
case SIP_KEYED: return sip_keyed;
case SIP_HASH: return (uint64_t)sm_load_entity(context, 1, sip_hash_data, sip_hash_size, &sip_hash_key, &sip_hash_crc);
case HIGHWAY_STATE: return highway_state;
case HIGHWAY_RESULT: return highway_result;
case HIGHWAY_KEYED: return highway_keyed;
case HIGHWAY_HASH: return (uint64_t)sm_load_entity(context, 1, highway_hash_data, highway_hash_size, &highway_hash_key, &highway_hash_crc);
