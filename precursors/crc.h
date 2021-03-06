// crc.h - CRC LUTs.


#ifndef INCLUDE_CRC_H
#define INCLUDE_CRC_H 1


// CRC 32 LUT.
static const uint32_t crc_32_tab[256] =
{
	0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU,
	0xE963A535U, 0x9E6495A3U, 0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U,
	0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D07U, 0x90BF1D91U, 0x1DB71064U, 0x6AB020F2U,
	0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U,
	0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU,	0x14015C4FU, 0x63066CD9U,
	0xFA0F3D63U, 0x8D080DF5U, 0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U,
	0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU,	0x35B5A8FAU, 0x42B2986CU,
	0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U,
	0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U,
	0xCFBA9599U, 0xB8BDA50FU, 0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U,
	0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU,	0x76DC4190U, 0x01DB7106U,
	0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U,
	0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU, 0x086D3D2DU,
	0x91646C97U, 0xE6635C01U, 0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU,
	0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U, 0x65B0D9C6U, 0x12B7E950U,
	0x8BBEB8EAU, 0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U,
	0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U,
	0xA4D1C46DU, 0xD3D6F4FBU, 0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U,
	0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U, 0x5005713CU, 0x270241AAU,
	0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU,
	0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U,
	0xB7BD5C3BU, 0xC0BA6CADU, 0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU,
	0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U, 0xE3630B12U, 0x94643B84U,
	0x0D6D6A3EU, 0x7A6A5AA8U, 0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U,
	0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU, 0xF762575DU, 0x806567CBU,
	0x196C3671U, 0x6E6B06E7U, 0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU,
	0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U, 0xD6D6A3E8U, 0xA1D1937EU,
	0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
	0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U,
	0x316E8EEFU, 0x4669BE79U, 0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U,
	0xCC0C7795U, 0xBB0B4703U, 0x220216B9U, 0x5505262FU, 0xC5BA3BBEU, 0xB2BD0B28U,
	0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU,
	0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU, 0x9C0906A9U, 0xEB0E363FU,
	0x72076785U, 0x05005713U, 0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U,
	0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U, 0x86D3D2D4U, 0xF1D4E242U,
	0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U,
	0x88085AE6U, 0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U,
	0x616BFFD3U, 0x166CCF45U, 0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U,
	0xA7672661U, 0xD06016F7U, 0x4969474DU, 0x3E6E77DBU, 0xAED16A4AU, 0xD9D65ADCU,
	0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U, 0x47B2CF7FU, 0x30B5FFE9U,
	0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U, 0xCDD70693U,
	0x54DE5729U, 0x23D967BFU, 0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U,
	0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU
};


// CRC 64 LUT.
static const uint64_t crc_64_tab[256] =
{
	UINT64_C(0x0000000000000000), UINT64_C(0x7AD870C830358979), UINT64_C(0xF5B0E190606B12F2), UINT64_C(0x8F689158505E9B8B),
	UINT64_C(0xC038E5739841B68F), UINT64_C(0xBAE095BBA8743FF6), UINT64_C(0x358804E3F82AA47D), UINT64_C(0x4F50742BC81F2D04),
	UINT64_C(0xAB28ECB46814FE75), UINT64_C(0xD1F09C7C5821770C), UINT64_C(0x5E980D24087FEC87), UINT64_C(0x24407DEC384A65FE),
	UINT64_C(0x6B1009C7F05548FA), UINT64_C(0x11C8790FC060C183), UINT64_C(0x9EA0E857903E5A08), UINT64_C(0xE478989FA00BD371),
	UINT64_C(0x7D08FF3B88BE6F81), UINT64_C(0x07D08FF3B88BE6F8), UINT64_C(0x88B81EABE8D57D73), UINT64_C(0xF2606E63D8E0F40A),
	UINT64_C(0xBD301A4810FFD90E), UINT64_C(0xC7E86A8020CA5077), UINT64_C(0x4880FBD87094CBFC), UINT64_C(0x32588B1040A14285),
	UINT64_C(0xD620138FE0AA91F4), UINT64_C(0xACF86347D09F188D), UINT64_C(0x2390F21F80C18306), UINT64_C(0x594882D7B0F40A7F),
	UINT64_C(0x1618F6FC78EB277B), UINT64_C(0x6CC0863448DEAE02), UINT64_C(0xE3A8176C18803589), UINT64_C(0x997067A428B5BCF0),
	UINT64_C(0xFA11FE77117CDF02), UINT64_C(0x80C98EBF2149567B), UINT64_C(0x0FA11FE77117CDF0), UINT64_C(0x75796F2F41224489),
	UINT64_C(0x3A291B04893D698D), UINT64_C(0x40F16BCCB908E0F4), UINT64_C(0xCF99FA94E9567B7F), UINT64_C(0xB5418A5CD963F206),
	UINT64_C(0x513912C379682177), UINT64_C(0x2BE1620B495DA80E), UINT64_C(0xA489F35319033385), UINT64_C(0xDE51839B2936BAFC),
	UINT64_C(0x9101F7B0E12997F8), UINT64_C(0xEBD98778D11C1E81), UINT64_C(0x64B116208142850A), UINT64_C(0x1E6966E8B1770C73),
	UINT64_C(0x8719014C99C2B083), UINT64_C(0xFDC17184A9F739FA), UINT64_C(0x72A9E0DCF9A9A271), UINT64_C(0x08719014C99C2B08),
	UINT64_C(0x4721E43F0183060C), UINT64_C(0x3DF994F731B68F75), UINT64_C(0xB29105AF61E814FE), UINT64_C(0xC849756751DD9D87),
	UINT64_C(0x2C31EDF8F1D64EF6), UINT64_C(0x56E99D30C1E3C78F), UINT64_C(0xD9810C6891BD5C04), UINT64_C(0xA3597CA0A188D57D),
	UINT64_C(0xEC09088B6997F879), UINT64_C(0x96D1784359A27100), UINT64_C(0x19B9E91B09FCEA8B), UINT64_C(0x636199D339C963F2),
	UINT64_C(0xDF7ADABD7A6E2D6F), UINT64_C(0xA5A2AA754A5BA416), UINT64_C(0x2ACA3B2D1A053F9D), UINT64_C(0x50124BE52A30B6E4),
	UINT64_C(0x1F423FCEE22F9BE0), UINT64_C(0x659A4F06D21A1299), UINT64_C(0xEAF2DE5E82448912), UINT64_C(0x902AAE96B271006B),
	UINT64_C(0x74523609127AD31A), UINT64_C(0x0E8A46C1224F5A63), UINT64_C(0x81E2D7997211C1E8), UINT64_C(0xFB3AA75142244891),
	UINT64_C(0xB46AD37A8A3B6595), UINT64_C(0xCEB2A3B2BA0EECEC), UINT64_C(0x41DA32EAEA507767), UINT64_C(0x3B024222DA65FE1E),
	UINT64_C(0xA2722586F2D042EE), UINT64_C(0xD8AA554EC2E5CB97), UINT64_C(0x57C2C41692BB501C), UINT64_C(0x2D1AB4DEA28ED965),
	UINT64_C(0x624AC0F56A91F461), UINT64_C(0x1892B03D5AA47D18), UINT64_C(0x97FA21650AFAE693), UINT64_C(0xED2251AD3ACF6FEA),
	UINT64_C(0x095AC9329AC4BC9B), UINT64_C(0x7382B9FAAAF135E2), UINT64_C(0xFCEA28A2FAAFAE69), UINT64_C(0x8632586ACA9A2710),
	UINT64_C(0xC9622C4102850A14), UINT64_C(0xB3BA5C8932B0836D), UINT64_C(0x3CD2CDD162EE18E6), UINT64_C(0x460ABD1952DB919F),
	UINT64_C(0x256B24CA6B12F26D), UINT64_C(0x5FB354025B277B14), UINT64_C(0xD0DBC55A0B79E09F), UINT64_C(0xAA03B5923B4C69E6),
	UINT64_C(0xE553C1B9F35344E2), UINT64_C(0x9F8BB171C366CD9B), UINT64_C(0x10E3202993385610), UINT64_C(0x6A3B50E1A30DDF69),
	UINT64_C(0x8E43C87E03060C18), UINT64_C(0xF49BB8B633338561), UINT64_C(0x7BF329EE636D1EEA), UINT64_C(0x012B592653589793),
	UINT64_C(0x4E7B2D0D9B47BA97), UINT64_C(0x34A35DC5AB7233EE), UINT64_C(0xBBCBCC9DFB2CA865), UINT64_C(0xC113BC55CB19211C),
	UINT64_C(0x5863DBF1E3AC9DEC), UINT64_C(0x22BBAB39D3991495), UINT64_C(0xADD33A6183C78F1E), UINT64_C(0xD70B4AA9B3F20667),
	UINT64_C(0x985B3E827BED2B63), UINT64_C(0xE2834E4A4BD8A21A), UINT64_C(0x6DEBDF121B863991), UINT64_C(0x1733AFDA2BB3B0E8),
	UINT64_C(0xF34B37458BB86399), UINT64_C(0x8993478DBB8DEAE0), UINT64_C(0x06FBD6D5EBD3716B), UINT64_C(0x7C23A61DDBE6F812),
	UINT64_C(0x3373D23613F9D516), UINT64_C(0x49ABA2FE23CC5C6F), UINT64_C(0xC6C333A67392C7E4), UINT64_C(0xBC1B436E43A74E9D),
	UINT64_C(0x95AC9329AC4BC9B5), UINT64_C(0xEF74E3E19C7E40CC), UINT64_C(0x601C72B9CC20DB47), UINT64_C(0x1AC40271FC15523E),
	UINT64_C(0x5594765A340A7F3A), UINT64_C(0x2F4C0692043FF643), UINT64_C(0xA02497CA54616DC8), UINT64_C(0xDAFCE7026454E4B1),
	UINT64_C(0x3E847F9DC45F37C0), UINT64_C(0x445C0F55F46ABEB9), UINT64_C(0xCB349E0DA4342532), UINT64_C(0xB1ECEEC59401AC4B),
	UINT64_C(0xFEBC9AEE5C1E814F), UINT64_C(0x8464EA266C2B0836), UINT64_C(0x0B0C7B7E3C7593BD), UINT64_C(0x71D40BB60C401AC4),
	UINT64_C(0xE8A46C1224F5A634), UINT64_C(0x927C1CDA14C02F4D), UINT64_C(0x1D148D82449EB4C6), UINT64_C(0x67CCFD4A74AB3DBF),
	UINT64_C(0x289C8961BCB410BB), UINT64_C(0x5244F9A98C8199C2), UINT64_C(0xDD2C68F1DCDF0249), UINT64_C(0xA7F41839ECEA8B30),
	UINT64_C(0x438C80A64CE15841), UINT64_C(0x3954F06E7CD4D138), UINT64_C(0xB63C61362C8A4AB3), UINT64_C(0xCCE411FE1CBFC3CA),
	UINT64_C(0x83B465D5D4A0EECE), UINT64_C(0xF96C151DE49567B7), UINT64_C(0x76048445B4CBFC3C), UINT64_C(0x0CDCF48D84FE7545),
	UINT64_C(0x6FBD6D5EBD3716B7), UINT64_C(0x15651D968D029FCE), UINT64_C(0x9A0D8CCEDD5C0445), UINT64_C(0xE0D5FC06ED698D3C),
	UINT64_C(0xAF85882D2576A038), UINT64_C(0xD55DF8E515432941), UINT64_C(0x5A3569BD451DB2CA), UINT64_C(0x20ED197575283BB3),
	UINT64_C(0xC49581EAD523E8C2), UINT64_C(0xBE4DF122E51661BB), UINT64_C(0x3125607AB548FA30), UINT64_C(0x4BFD10B2857D7349),
	UINT64_C(0x04AD64994D625E4D), UINT64_C(0x7E7514517D57D734), UINT64_C(0xF11D85092D094CBF), UINT64_C(0x8BC5F5C11D3CC5C6),
	UINT64_C(0x12B5926535897936), UINT64_C(0x686DE2AD05BCF04F), UINT64_C(0xE70573F555E26BC4), UINT64_C(0x9DDD033D65D7E2BD),
	UINT64_C(0xD28D7716ADC8CFB9), UINT64_C(0xA85507DE9DFD46C0), UINT64_C(0x273D9686CDA3DD4B), UINT64_C(0x5DE5E64EFD965432),
	UINT64_C(0xB99D7ED15D9D8743), UINT64_C(0xC3450E196DA80E3A), UINT64_C(0x4C2D9F413DF695B1), UINT64_C(0x36F5EF890DC31CC8),
	UINT64_C(0x79A59BA2C5DC31CC), UINT64_C(0x037DEB6AF5E9B8B5), UINT64_C(0x8C157A32A5B7233E), UINT64_C(0xF6CD0AFA9582AA47),
	UINT64_C(0x4AD64994D625E4DA), UINT64_C(0x300E395CE6106DA3), UINT64_C(0xBF66A804B64EF628), UINT64_C(0xC5BED8CC867B7F51),
	UINT64_C(0x8AEEACE74E645255), UINT64_C(0xF036DC2F7E51DB2C), UINT64_C(0x7F5E4D772E0F40A7), UINT64_C(0x05863DBF1E3AC9DE),
	UINT64_C(0xE1FEA520BE311AAF), UINT64_C(0x9B26D5E88E0493D6), UINT64_C(0x144E44B0DE5A085D), UINT64_C(0x6E963478EE6F8124),
	UINT64_C(0x21C640532670AC20), UINT64_C(0x5B1E309B16452559), UINT64_C(0xD476A1C3461BBED2), UINT64_C(0xAEAED10B762E37AB),
	UINT64_C(0x37DEB6AF5E9B8B5B), UINT64_C(0x4D06C6676EAE0222), UINT64_C(0xC26E573F3EF099A9), UINT64_C(0xB8B627F70EC510D0),
	UINT64_C(0xF7E653DCC6DA3DD4), UINT64_C(0x8D3E2314F6EFB4AD), UINT64_C(0x0256B24CA6B12F26), UINT64_C(0x788EC2849684A65F),
	UINT64_C(0x9CF65A1B368F752E), UINT64_C(0xE62E2AD306BAFC57), UINT64_C(0x6946BB8B56E467DC), UINT64_C(0x139ECB4366D1EEA5),
	UINT64_C(0x5CCEBF68AECEC3A1), UINT64_C(0x2616CFA09EFB4AD8), UINT64_C(0xA97E5EF8CEA5D153), UINT64_C(0xD3A62E30FE90582A),
	UINT64_C(0xB0C7B7E3C7593BD8), UINT64_C(0xCA1FC72BF76CB2A1), UINT64_C(0x45775673A732292A), UINT64_C(0x3FAF26BB9707A053),
	UINT64_C(0x70FF52905F188D57), UINT64_C(0x0A2722586F2D042E), UINT64_C(0x854FB3003F739FA5), UINT64_C(0xFF97C3C80F4616DC),
	UINT64_C(0x1BEF5B57AF4DC5AD), UINT64_C(0x61372B9F9F784CD4), UINT64_C(0xEE5FBAC7CF26D75F), UINT64_C(0x9487CA0FFF135E26),
	UINT64_C(0xDBD7BE24370C7322), UINT64_C(0xA10FCEEC0739FA5B), UINT64_C(0x2E675FB4576761D0), UINT64_C(0x54BF2F7C6752E8A9),
	UINT64_C(0xCDCF48D84FE75459), UINT64_C(0xB71738107FD2DD20), UINT64_C(0x387FA9482F8C46AB), UINT64_C(0x42A7D9801FB9CFD2),
	UINT64_C(0x0DF7ADABD7A6E2D6), UINT64_C(0x772FDD63E7936BAF), UINT64_C(0xF8474C3BB7CDF024), UINT64_C(0x829F3CF387F8795D),
	UINT64_C(0x66E7A46C27F3AA2C), UINT64_C(0x1C3FD4A417C62355), UINT64_C(0x935745FC4798B8DE), UINT64_C(0xE98F353477AD31A7),
	UINT64_C(0xA6DF411FBFB21CA3), UINT64_C(0xDC0731D78F8795DA), UINT64_C(0x536FA08FDFD90E51), UINT64_C(0x29B7D047EFEC8728),
};


#endif // INCLUDE_CRC_H

