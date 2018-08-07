/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_CHARCONV_H
#define __NX_CHARCONV_H

#include "../../../common/types.h"

#include <iconv.h>

const char  *nx_get_locale_charset(void);

iconv_t	nx_iconv_open(const char *to_encoding,
		      const char *from_encoding);
int32_t nx_convert(char		**outptr,
		   int32_t	*outbytesleft,
		   const char	**inptr,
		   int32_t	*inbytesleft,
		   const char	*to_encoding,
		   const char	*from_encoding,
		   boolean	convert_invalid);
int32_t nx_convert_ctx(char		**outptr,
		       int32_t		*outbytesleft,
		       const char	**inptr,
		       int32_t		*inbytesleft,
		       iconv_t		*iconv_ctx,
		       boolean		convert_invalid);
int32_t nx_convert_auto(char		**outptr,
			int32_t		*outbytesleft,
			const char	**inptr,
			int32_t		*inbytesleft,
			const char	*to_encoding,
			int32_t		num_encoding,
			const char	*from_encodings[],
			const char	**ok_encoding);

#endif /* __NX_CHARCONV_H */

