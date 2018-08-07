/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_SERIALIZE_H
#define __NX_SERIALIZE_H

// TODO see macros in http://www.intel.com/design/intarch/papers/endian.pdf

#define NX_SWAP16(num)							\
    ({									\
	uint16_t _retval;						\
									\
	((uint8_t *)&_retval)[0] = ((uint8_t *)(num))[1];		\
	((uint8_t *)&_retval)[1] = ((uint8_t *)(num))[0];		\
									\
	_retval;							\
    })

#define NX_SWAP32(num)							\
    ({									\
	apr_uint32_t _retval;						\
									\
	((uint8_t *)&_retval)[0] = ((uint8_t *)(num))[3];		\
	((uint8_t *)&_retval)[1] = ((uint8_t *)(num))[2];		\
	((uint8_t *)&_retval)[2] = ((uint8_t *)(num))[1];		\
	((uint8_t *)&_retval)[3] = ((uint8_t *)(num))[0];		\
									\
	_retval;							\
    })

#define NX_SWAP64(num)							\
    ({									\
	apr_uint64_t _retval;						\
									\
	((uint8_t *)&_retval)[0] = ((uint8_t *)(num))[7];		\
	((uint8_t *)&_retval)[1] = ((uint8_t *)(num))[6];		\
	((uint8_t *)&_retval)[2] = ((uint8_t *)(num))[5];		\
	((uint8_t *)&_retval)[3] = ((uint8_t *)(num))[4];		\
	((uint8_t *)&_retval)[4] = ((uint8_t *)(num))[3];		\
	((uint8_t *)&_retval)[5] = ((uint8_t *)(num))[2];		\
	((uint8_t *)&_retval)[6] = ((uint8_t *)(num))[1];		\
	((uint8_t *)&_retval)[7] = ((uint8_t *)(num))[0];		\
									\
	_retval;							\
    })

#define NX_GET16(num)							\
    ({									\
	apr_uint16_t _retval;						\
									\
	((uint8_t *)&_retval)[0] = ((const uint8_t *)(num))[0];		\
	((uint8_t *)&_retval)[1] = ((const uint8_t *)(num))[1];		\
									\
	_retval;							\
    })

#define NX_GET32(num)							\
    ({									\
	apr_uint32_t _retval;						\
									\
	((uint8_t *)&_retval)[0] = ((const uint8_t *)(num))[0];		\
	((uint8_t *)&_retval)[1] = ((const uint8_t *)(num))[1];		\
	((uint8_t *)&_retval)[2] = ((const uint8_t *)(num))[2];		\
	((uint8_t *)&_retval)[3] = ((const uint8_t *)(num))[3];		\
									\
	_retval;							\
    })

#define NX_GET64(num)							\
    ({									\
	apr_uint64_t _retval;						\
									\
	((uint8_t *)&_retval)[0] = ((const uint8_t *)(num))[0];		\
	((uint8_t *)&_retval)[1] = ((const uint8_t *)(num))[1];		\
	((uint8_t *)&_retval)[2] = ((const uint8_t *)(num))[2];		\
	((uint8_t *)&_retval)[3] = ((const uint8_t *)(num))[3];		\
	((uint8_t *)&_retval)[4] = ((const uint8_t *)(num))[4];		\
	((uint8_t *)&_retval)[5] = ((const uint8_t *)(num))[5];		\
	((uint8_t *)&_retval)[6] = ((const uint8_t *)(num))[6];		\
	((uint8_t *)&_retval)[7] = ((const uint8_t *)(num))[7];		\
									\
	_retval;							\
    })



#define NX_COPY16(dst, src)						\
	((uint8_t *)(dst))[0]	= ((const uint8_t *)(src))[0];		\
	((uint8_t *)(dst))[1]	= ((const uint8_t *)(src))[1];

#define NX_COPY32(dst, src)						\
	((uint8_t *)(dst))[0]	= ((const uint8_t *)(src))[0];		\
	((uint8_t *)(dst))[1]	= ((const uint8_t *)(src))[1];		\
	((uint8_t *)(dst))[2]	= ((const uint8_t *)(src))[2];		\
	((uint8_t *)(dst))[3]	= ((const uint8_t *)(src))[3];

#define NX_COPY64(dst, src)						\
	((uint8_t *)(dst))[0]	= ((const uint8_t *)(src))[0];		\
	((uint8_t *)(dst))[1]	= ((const uint8_t *)(src))[1];		\
	((uint8_t *)(dst))[2]	= ((const uint8_t *)(src))[2];		\
	((uint8_t *)(dst))[3]	= ((const uint8_t *)(src))[3];		\
	((uint8_t *)(dst))[4]	= ((const uint8_t *)(src))[4];		\
	((uint8_t *)(dst))[5]	= ((const uint8_t *)(src))[5];		\
	((uint8_t *)(dst))[6]	= ((const uint8_t *)(src))[6];		\
	((uint8_t *)(dst))[7]	= ((const uint8_t *)(src))[7];


#if APR_IS_BIGENDIAN == 1
# define nx_int16_to_le(dst, src)					\
	((uint8_t *)(dst))[0]	= ((uint8_t *)(src))[1];		\
	((uint8_t *)(dst))[1]	= ((uint8_t *)(src))[0];
#else
#  define nx_int16_to_le(dst, src) NX_COPY16(dst, src)
#endif



#if APR_IS_BIGENDIAN == 1
# define nx_int32_to_le(dst, src)					\
	((uint8_t *)(dst))[0]	= ((uint8_t *)(src))[3];		\
	((uint8_t *)(dst))[1]	= ((uint8_t *)(src))[2];		\
	((uint8_t *)(dst))[2]	= ((uint8_t *)(src))[1];		\
	((uint8_t *)(dst))[3]	= ((uint8_t *)(src))[0];		
#else
#  define nx_int32_to_le(dst, src) NX_COPY32(dst, src)
#endif

#if APR_IS_BIGENDIAN == 1
# define nx_int64_to_le(dst, src)					\
	((uint8_t *)(dst))[0]	= ((uint8_t *)(src))[7];		\
	((uint8_t *)(dst))[1]	= ((uint8_t *)(src))[6];		\
	((uint8_t *)(dst))[2]	= ((uint8_t *)(src))[5];		\
	((uint8_t *)(dst))[3]	= ((uint8_t *)(src))[4];		\
	((uint8_t *)(dst))[4]	= ((uint8_t *)(src))[3];		\
	((uint8_t *)(dst))[5]	= ((uint8_t *)(src))[2];		\
	((uint8_t *)(dst))[6]	= ((uint8_t *)(src))[1];		\
	((uint8_t *)(dst))[7]	= ((uint8_t *)(src))[0];		
#else
#  define nx_int64_to_le(dst, src) NX_COPY64(dst, src)
#endif


#if APR_IS_BIGENDIAN == 1
# define nx_int16_from_le(num) NX_SWAP16(num)
# define nx_int32_from_le(num) NX_SWAP32(num)
# define nx_int64_from_le(num) NX_SWAP64(num)
# define nx_int16_from_be(num) NX_GET16(num)
# define nx_int32_from_be(num) NX_GET32(num)
# define nx_int64_from_be(num) NX_GET64(num)
#else
# define nx_int16_from_le(num) NX_GET16(num)
# define nx_int32_from_le(num) NX_GET32(num)
# define nx_int64_from_le(num) NX_GET64(num)
# define nx_int16_from_be(num) NX_SWAP16(num)
# define nx_int32_from_be(num) NX_SWAP32(num)
# define nx_int64_from_be(num) NX_SWAP64(num)
#endif


#endif	/* __NX_SERIALIZE_H */
   
