#ifndef __NX_OM_KAFKA_H
#define __NX_OM_KAFKA_H

#include "../../../common/types.h"
#include <librdkafka/rdkafka.h>

typedef struct nx_im_kafka_option_t
{
    const char *name;
    const char *value;
} nx_im_kafka_option_t;

typedef struct nx_im_kafka_conf_t
{
	char *brokerlist;
	char *topic;
	char *compression;
	int partition;
	apr_array_header_t *options;
	rd_kafka_topic_partition_list_t *topics;
	rd_kafka_conf_t *kafka_conf;
	rd_kafka_topic_conf_t *topic_conf;
	rd_kafka_t *rk;
	rd_kafka_topic_t *rkt;
	nx_event_t *event;
} nx_im_kafka_conf_t;

#endif /* __NX_OM_KAFKA_H */
