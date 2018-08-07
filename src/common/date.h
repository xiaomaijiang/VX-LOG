#ifndef __NX_DATE_H
#define __NX_DATE_H

apr_status_t nx_date_parse_rfc3164(apr_time_t  *t, 
				   const char *date,
				   const char **dateend);
apr_status_t nx_date_parse_rfc1123(apr_time_t *t,
				   const char *date,
				   const char **dateend);
apr_status_t nx_date_parse_iso(apr_time_t  *t, 
			       const char *date,
			       const char **dateend);
apr_status_t nx_date_parse_apache(apr_time_t *t,
				  const char *date,
				  const char **dateend);
apr_status_t nx_date_parse_cisco(apr_time_t  *t, 
				 const char *date,
				 const char **dateend);
apr_status_t nx_date_parse_win(apr_time_t  *t, 
			       const char *date,
			       const char **dateend);
apr_status_t nx_date_parse_timestamp(apr_time_t  *t, 
				     const char *date,
				     const char **dateend);
apr_status_t nx_date_parse(apr_time_t *t, const char *date, const char **dateend);
apr_status_t nx_date_to_rfc3164(char *datestr,
				apr_size_t dstsize,
				apr_time_t timeval);
apr_status_t nx_date_to_rfc3164_wday_year(char *datestr,
					  apr_size_t dstsize,
					  apr_time_t timeval,
					  boolean spaceday);
apr_status_t nx_date_to_iso(char *dst,
			    apr_size_t dstsize,
			    apr_time_t t);
apr_status_t nx_date_to_rfc5424(char *dst,
				apr_size_t dstsize,
				boolean gmt,
				apr_time_t t);
apr_status_t nx_date_fix_year(apr_time_t *t);

#endif /* __NX_DATE_H */
