// crc_op_impl.h - Auto-Generated (Mon Jan  1 15:09:48 2018): Switch entries for the 'crc' module. Include in the sm_op() function switch statement, in your module source file.

case SM_CRC_32_TAB: return (uint64_t)sm_load_entity(context, 0, sm_crc_32_tab_data, sm_crc_32_tab_size, &sm_crc_32_tab_key, &sm_crc_32_tab_crc);
case SM_CRC_32: return (uint64_t)sm_load_entity(context, 1, sm_crc_32_data, sm_crc_32_size, &sm_crc_32_key, &sm_crc_32_crc);
case SM_CRC_64_TAB: return (uint64_t)sm_load_entity(context, 0, sm_crc_64_tab_data, sm_crc_64_tab_size, &sm_crc_64_tab_key, &sm_crc_64_tab_crc);
case SM_CRC_64: return (uint64_t)sm_load_entity(context, 1, sm_crc_64_data, sm_crc_64_size, &sm_crc_64_key, &sm_crc_64_crc);
