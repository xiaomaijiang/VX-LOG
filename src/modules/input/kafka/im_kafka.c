#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/alloc.h"
#include "im_kafka.h"
#include <apr_lib.h>
#include <librdkafka/rdkafka.h>

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static int run = 1;
static int wait_eof = 0;

static void im_kafka_add_poll_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_im_kafka_conf_t *imconf;

    imconf = (nx_im_kafka_conf_t *)module->config;
    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->type = NX_EVENT_READ;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
}
static void im_kafka_start(nx_module_t *module)
{
    nx_event_t *event;
    nx_im_kafka_conf_t *imconf;

    imconf = (nx_im_kafka_conf_t *)module->config;

    ASSERT(imconf->event == NULL);

    im_kafka_add_poll_event(module);
}

static void im_kafka_stop(nx_module_t *module)
{
    log_debug("Kafka module stop");
    ASSERT(module != NULL);
    nx_im_kafka_conf_t *modconf;
    modconf = (nx_im_kafka_conf_t *)module->config;
    rd_kafka_resp_err_t err;

    err = rd_kafka_consumer_close(modconf->rk);
    if (err)
    {
        log_error("%% Failed to close consumer: %s\n",
                  rd_kafka_err2str(err));
    }
    else
    {
        log_info("%% Consumer closed\n", stderr);
    }

    rd_kafka_topic_partition_list_destroy(modconf->topics);

    /* Destroy handle */
    rd_kafka_destroy(modconf->rk);

    /* Let background threads clean up and terminate cleanly. */
    run = 5;
    while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1)
        printf("Waiting for librdkafka to decommission\n");
}

static void print_partition_list(FILE *fp,
                                 const rd_kafka_topic_partition_list_t
                                     *partitions)
{
    int i;
    for (i = 0; i < partitions->cnt; i++)
    {
        fprintf(stderr, "%s %s [%" PRId32 "] offset %" PRId64,
                i > 0 ? "," : "",
                partitions->elems[i].topic,
                partitions->elems[i].partition,
                partitions->elems[i].offset);
    }
    fprintf(stderr, "\n");
}
static void rebalance_cb(rd_kafka_t *rk,
                         rd_kafka_resp_err_t err,
                         rd_kafka_topic_partition_list_t *partitions,
                         void *opaque)
{

    log_info("Consumer group rebalanced:");
    switch (err)
    {
    case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
        fprintf(stderr, "assigned:\n");
        print_partition_list(stderr, partitions);
        rd_kafka_assign(rk, partitions);
        wait_eof += partitions->cnt;
        break;

    case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
        fprintf(stderr, "revoked:\n");
        print_partition_list(stderr, partitions);
        rd_kafka_assign(rk, NULL);
        wait_eof = 0;
        break;

    default:
        fprintf(stderr, "failed: %s\n",
                rd_kafka_err2str(err));
        rd_kafka_assign(rk, NULL);
        break;
    }
}
/**
 * Message delivery report callback.
 * Called once for each message.
 * See rdkafka.h for more information.
 */
static void msg_delivered(rd_kafka_t *rk, void *payload, size_t len, int error_code, void *opaque, void *msg_opaque)
{
    if (error_code)
    {
        log_error("Message delivery failed: %s\n", rd_kafka_err2str(error_code));
    }
    else
    {
        //log_debug(stderr, "Message delivered (%zd bytes)\n", len);
        log_debug("Message delivered (%zd bytes)\n", len);
    }
}

static void im_kafka_init(nx_module_t *module)
{
    log_debug("Kafka module init entrypoint");
    char errstr[512];
    int i;
    nx_im_kafka_conf_t *modconf;
    modconf = (nx_im_kafka_conf_t *)module->config;
    nx_im_kafka_option_t *option;
    rd_kafka_conf_t *conf;
    rd_kafka_topic_conf_t *topic_conf;

    /* Kafka configuration */
    conf = rd_kafka_conf_new();

    /* Topic configuration */
    topic_conf = rd_kafka_topic_conf_new();

    rd_kafka_conf_set_dr_cb(conf, msg_delivered);

    if (rd_kafka_conf_set(conf, "compression.codec", modconf->compression, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        log_error("Unable to set compression codec %s", modconf->compression);
    }
    else
    {
        log_info("Kafka compression set to %s", modconf->compression);
    }

    for (i = 0; i < modconf->options->nelts; i++)
    {
        option = ((nx_im_kafka_option_t **)modconf->options->elts)[i];
        if (rd_kafka_conf_set(conf, option->name, option->value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
        {
            log_error("Unable to set Options %s=%s", option->name, option->value);
        }
        else
        {
            log_info("Kafka Options %s set to %s", option->name, option->value);
        }
    }
    if (rd_kafka_topic_conf_set(topic_conf, "offset.store.method",
                                "broker",
                                errstr, sizeof(errstr)) !=
        RD_KAFKA_CONF_OK)
    {
        fprintf(stderr, "%% %s\n", errstr);
        return -1;
    }

    rd_kafka_conf_set_default_topic_conf(conf, topic_conf);
    rd_kafka_conf_set_rebalance_cb(conf, rebalance_cb);

    if (!(modconf->rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr))))
    {
        log_error("Failed to create new consumer: %s\n", errstr);
    }

    if (rd_kafka_brokers_add(modconf->rk, modconf->brokerlist) == 0)
    {
        log_error("No valid brokers specified (%s)", modconf->brokerlist);
    }
    else
    {
        log_info("Kafka brokers set to %s", modconf->brokerlist);
    }
    modconf->topics = rd_kafka_topic_partition_list_new(1);
    rd_kafka_poll_set_consumer(modconf->rk);
    rd_kafka_topic_partition_list_add(modconf->topics, modconf->topic, modconf->partition);

    rd_kafka_resp_err_t err;
    if ((err = rd_kafka_subscribe(modconf->rk, modconf->topics)))
    {
        fprintf(stderr,
                "%% Failed to start consuming topics: %s\n",
                rd_kafka_err2str(err));
        exit(1);
    }

    modconf->kafka_conf = conf;
    modconf->topic_conf = topic_conf;
}

static boolean im_kafka_add_option(nx_module_t *module, char *optionstr)
{
    nx_im_kafka_option_t *option;
    nx_im_kafka_conf_t *imconf;
    char *ptr;

    imconf = (nx_im_kafka_conf_t *)module->config;

    for (ptr = optionstr; (*ptr != '\0') && (!apr_isspace(*ptr)); ptr++)
        ;
    while (apr_isspace(*ptr))
    {
        *ptr = '\0';
        ptr++;
    }

    if (*ptr == '\0')
    {
        return (FALSE);
    }
    log_debug("im_dbi option %s = %s", optionstr, ptr);

    option = apr_pcalloc(module->pool, sizeof(nx_im_kafka_option_t));
    option->name = optionstr;
    option->value = ptr;

    *((nx_im_kafka_option_t **)apr_array_push(imconf->options)) = option;

    return (TRUE);
}

static void im_kafka_config(nx_module_t *module)
{
    const nx_directive_t *volatile curr;
    nx_im_kafka_conf_t *volatile modconf;
    nx_exception_t e;
    curr = module->directives;
    modconf = apr_pcalloc(module->pool, sizeof(nx_im_kafka_conf_t));
    module->config = modconf;

    modconf->options = apr_array_make(module->pool, 5, sizeof(const nx_im_kafka_conf_t *));

    while (curr != NULL)
    {
        if (nx_module_common_keyword(curr->directive) == TRUE)
        { //ignore common configuration keywords
        }
        else if (strcasecmp(curr->directive, "brokerlist") == 0)
        {
            modconf->brokerlist = curr->args;
        }
        else if (strcasecmp(curr->directive, "topic") == 0)
        {
            modconf->topic = curr->args;
        }
        else if (strcasecmp(curr->directive, "compression") == 0)
        {
            modconf->compression = curr->args;
        }
        else if (strcasecmp(curr->directive, "partition") == 0)
        {
            modconf->partition = curr->args;
        }
        else if (strcasecmp(curr->directive, "option") == 0)
        {
            if (im_kafka_add_option(module, curr->args) == FALSE)
            {
                nx_conf_error(curr, "invalid option %s", curr->args);
            }
        }
        curr = curr->next;
    }
}

static void im_kafka_read(nx_module_t *module)
{
    nx_im_kafka_conf_t *imconf;
    nx_logdata_t *logdata;

    ASSERT(module != NULL);
    imconf = (nx_im_kafka_conf_t *)module->config;
    imconf->event = NULL;

    if (nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING)
    {
        log_debug("module %s not running, not reading any more data", module->name);
        return;
    }

    while (run)
    {
        rd_kafka_message_t *rkmessage;
        rkmessage = rd_kafka_consumer_poll(imconf->rk, 1000);

        if (rkmessage)
        {
            if (rkmessage->err)
            {
                if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
                {
                    continue;
                }

                if (rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION ||
                    rkmessage->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)
                {
                    run = 0;
                }
            }
            else
            {
                logdata = nx_logdata_new_logline((char *)rkmessage->payload, (int)rkmessage->len);
                nx_logdata_set_integer(logdata, "SeverityValue", NX_LOGLEVEL_INFO);
                nx_logdata_set_datetime(logdata, "EventTime", apr_time_now());
                nx_module_add_logdata_input(module, NULL, logdata);
            }
            rd_kafka_message_destroy(rkmessage);
        }
    }
}

static void im_kafka_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch (event->type)
    {
    case NX_EVENT_READ:
        im_kafka_read(module);
        break;
    default:
        nx_panic("invalid event type: %d", event->type);
    }
}

static void im_kafka_pause(nx_module_t *module)
{
    nx_im_kafka_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_kafka_conf_t *)module->config;

    im_kafka_stop(module);
    if (imconf->event != NULL)
    {
        nx_event_remove(imconf->event);
        nx_event_free(imconf->event);
        imconf->event = NULL;
    }
}

static void im_kafka_resume(nx_module_t *module)
{
    nx_im_kafka_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_kafka_conf_t *)module->config;

    if (imconf->event != NULL)
    {
        nx_event_remove(imconf->event);
        nx_event_free(imconf->event);
        imconf->event = NULL;
    }
    im_kafka_add_poll_event(module);
}

NX_MODULE_DECLARATION nx_im_kafka_module =
    {
        NX_MODULE_API_VERSION,
        NX_MODULE_TYPE_INPUT,
        NULL,
        im_kafka_config, // config
        im_kafka_start,  // start
        im_kafka_stop,   // stop
        im_kafka_pause,  // pause
        im_kafka_resume, // resume
        im_kafka_init,   // init
        NULL,            // shutdown
        im_kafka_event,  // event
        NULL,            // info
        NULL,            // exports
};
