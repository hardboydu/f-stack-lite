#ifndef __FF_MESSAGE_H__
#define __FF_MESSAGE_H__

#include <rte_ring.h>
#include <rte_mempool.h>

#include "ff_api.h"
#include "ff_msg.h"

#define MSG_RING_SIZE 32

struct ff_msg_ring {
    char ring_name[FF_MSG_NUM][RTE_RING_NAMESIZE];
    /* ring[0] for lcore recv msg, other send */
    /* ring[1] for lcore send msg, other read */
    struct rte_ring *ring[FF_MSG_NUM];
} __rte_cache_aligned;

static struct ff_msg_ring msg_ring[RTE_MAX_LCORE];
static struct rte_mempool *message_pool;

static struct ff_top_args ff_top_status;
static struct ff_traffic_args ff_traffic;

static void
ff_msg_init(struct rte_mempool *mp,
    __attribute__((unused)) void *opaque_arg,
    void *obj, __attribute__((unused)) unsigned i)
{
    struct ff_msg *msg = (struct ff_msg *)obj;
    msg->msg_type = FF_UNKNOWN;
    msg->buf_addr = (char *)msg + sizeof(struct ff_msg);
    msg->buf_len = mp->elt_size - sizeof(struct ff_msg);
}

static inline struct rte_ring *
create_ring(const char *name, unsigned count, int socket_id, unsigned flags)
{
    struct rte_ring *ring;

    if (name == NULL) {
        rte_exit(EXIT_FAILURE, "create ring failed, no name!\n");
    }

    if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
        ring = rte_ring_create(name, count, socket_id, flags);
    } else {
        ring = rte_ring_lookup(name);
    }

    if (ring == NULL) {
        rte_exit(EXIT_FAILURE, "create ring:%s failed!\n", name);
    }

    return ring;
}

static inline int
init_msg_ring(void)
{
    uint16_t i, j;
    uint16_t nb_procs = 1;
    unsigned socketid = 0;

    /* Create message buffer pool */
    if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
        message_pool = rte_mempool_create(FF_MSG_POOL,
           MSG_RING_SIZE * 2 * nb_procs,
           MAX_MSG_BUF_SIZE, MSG_RING_SIZE / 2, 0,
           NULL, NULL, ff_msg_init, NULL,
           socketid, 0);
    } else {
        message_pool = rte_mempool_lookup(FF_MSG_POOL);
    }

    if (message_pool == NULL) {
        rte_panic("Create msg mempool failed\n");
    }

    for(i = 0; i < nb_procs; ++i) {
        snprintf(msg_ring[i].ring_name[0], RTE_RING_NAMESIZE,
            "%s%u", FF_MSG_RING_IN, i);
        msg_ring[i].ring[0] = create_ring(msg_ring[i].ring_name[0],
            MSG_RING_SIZE, socketid, RING_F_SP_ENQ | RING_F_SC_DEQ);
        if (msg_ring[i].ring[0] == NULL)
            rte_panic("create ring::%s failed!\n", msg_ring[i].ring_name[0]);

        for (j = FF_SYSCTL; j < FF_MSG_NUM; j++) {
            snprintf(msg_ring[i].ring_name[j], RTE_RING_NAMESIZE,
                "%s%u_%u", FF_MSG_RING_OUT, i, j);
            msg_ring[i].ring[j] = create_ring(msg_ring[i].ring_name[j],
                MSG_RING_SIZE, socketid, RING_F_SP_ENQ | RING_F_SC_DEQ);
            if (msg_ring[i].ring[j] == NULL)
                rte_panic("create ring::%s failed!\n", msg_ring[i].ring_name[j]);
        }
    }

    return 0;
}

static inline void
handle_sysctl_msg(struct ff_msg *msg)
{
    int ret = ff_sysctl(msg->sysctl.name, msg->sysctl.namelen,
        msg->sysctl.old, msg->sysctl.oldlenp, msg->sysctl.new,
        msg->sysctl.newlen);

    if (ret < 0) {
        msg->result = errno;
    } else {
        msg->result = 0;
    }
}

static inline void
handle_ioctl_msg(struct ff_msg *msg)
{
    int fd, ret;
#ifdef INET6
    if (msg->msg_type == FF_IOCTL6) {
        fd = ff_socket(AF_INET6, SOCK_DGRAM, 0);
    } else
#endif
        fd = ff_socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        ret = -1;
        goto done;
    }

    ret = ff_ioctl_freebsd(fd, msg->ioctl.cmd, msg->ioctl.data);

    ff_close(fd);

done:
    if (ret < 0) {
        msg->result = errno;
    } else {
        msg->result = 0;
    }
}

static inline void
handle_route_msg(struct ff_msg *msg)
{
    int ret = ff_rtioctl(msg->route.fib, msg->route.data,
        &msg->route.len, msg->route.maxlen);
    if (ret < 0) {
        msg->result = errno;
    } else {
        msg->result = 0;
    }
}

static inline void
handle_top_msg(struct ff_msg *msg)
{
    msg->top = ff_top_status;
    msg->result = 0;
}

#ifdef FF_NETGRAPH
static inline void
handle_ngctl_msg(struct ff_msg *msg)
{
    int ret = ff_ngctl(msg->ngctl.cmd, msg->ngctl.data);
    if (ret < 0) {
        msg->result = errno;
    } else {
        msg->result = 0;
        msg->ngctl.ret = ret;
    }
}
#endif

#ifdef FF_IPFW
static inline void
handle_ipfw_msg(struct ff_msg *msg)
{
    int fd, ret;
    fd = ff_socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (fd < 0) {
        ret = -1;
        goto done;
    }

    switch (msg->ipfw.cmd) {
        case FF_IPFW_GET:
            ret = ff_getsockopt_freebsd(fd, msg->ipfw.level,
                msg->ipfw.optname, msg->ipfw.optval,
                msg->ipfw.optlen);
            break;
        case FF_IPFW_SET:
            ret = ff_setsockopt_freebsd(fd, msg->ipfw.level,
                msg->ipfw.optname, msg->ipfw.optval,
                *(msg->ipfw.optlen)); 
            break;
        default:
            ret = -1;
            errno = ENOTSUP;
            break;
    }

    ff_close(fd);

done:
    if (ret < 0) {
        msg->result = errno;
    } else {
        msg->result = 0;
    }
}
#endif

static inline void
handle_traffic_msg(struct ff_msg *msg)
{
    msg->traffic = ff_traffic;
    msg->result = 0;
}

static inline void
handle_default_msg(struct ff_msg *msg)
{
    msg->result = ENOTSUP;
}

static inline void
handle_msg(struct ff_msg *msg, uint16_t proc_id)
{
    switch (msg->msg_type) {
        case FF_SYSCTL:
            handle_sysctl_msg(msg);
            break;
        case FF_IOCTL:
#ifdef INET6
        case FF_IOCTL6:
#endif
            handle_ioctl_msg(msg);
            break;
        case FF_ROUTE:
            handle_route_msg(msg);
            break;
        case FF_TOP:
            handle_top_msg(msg);
            break;
#ifdef FF_NETGRAPH
        case FF_NGCTL:
            handle_ngctl_msg(msg);
            break;
#endif
#ifdef FF_IPFW
        case FF_IPFW_CTL:
            handle_ipfw_msg(msg);
            break;
#endif
        case FF_TRAFFIC:
            handle_traffic_msg(msg);
            break;
        default:
            handle_default_msg(msg);
            break;
    }
    rte_ring_enqueue(msg_ring[proc_id].ring[msg->msg_type], msg);
}

static inline int
process_msg_ring(uint16_t proc_id)
{
    void *msg;
    int ret = rte_ring_dequeue(msg_ring[proc_id].ring[0], &msg);

    if (unlikely(ret == 0)) {
        handle_msg((struct ff_msg *)msg, proc_id);
    }

    return 0;
}

#endif
