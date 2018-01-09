// crc_impl.h - Auto-Generated (Wed Jan 3 16:53:05 2018): Switch entries for the 'crc' module. Include in the sm_get_entity() function switch statement, in your module source file.

case CRC_32_TAB: return (uint64_t)sm_load_entity(context, 0, crc_32_tab_data, crc_32_tab_size, &crc_32_tab_key, &crc_32_tab_crc);
case CRC_32: return (uint64_t)sm_load_entity(context, 1, crc_32_data, crc_32_size, &crc_32_key, &crc_32_crc);
case CRC_64_TAB: return (uint64_t)sm_load_entity(context, 0, crc_64_tab_data, crc_64_tab_size, &crc_64_tab_key, &crc_64_tab_crc);
case CRC_64: return (uint64_t)sm_load_entity(context, 1, crc_64_data, crc_64_size, &crc_64_key, &crc_64_crc);
