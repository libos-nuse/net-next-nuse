// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright IBM Corp. 2018
 */

#define KMSG_COMPONENT "qeth"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/ethtool.h>
#include "qeth_core.h"


#define QETH_TXQ_STAT(_name, _stat) { \
	.name = _name, \
	.offset = offsetof(struct qeth_out_q_stats, _stat) \
}

#define QETH_CARD_STAT(_name, _stat) { \
	.name = _name, \
	.offset = offsetof(struct qeth_card_stats, _stat) \
}

struct qeth_stats {
	char name[ETH_GSTRING_LEN];
	unsigned int offset;
};

static const struct qeth_stats txq_stats[] = {
	QETH_TXQ_STAT("IO buffers", bufs),
	QETH_TXQ_STAT("IO buffer elements", buf_elements),
	QETH_TXQ_STAT("packed IO buffers", bufs_pack),
	QETH_TXQ_STAT("skbs", tx_packets),
	QETH_TXQ_STAT("packed skbs", skbs_pack),
	QETH_TXQ_STAT("SG skbs", skbs_sg),
	QETH_TXQ_STAT("HW csum skbs", skbs_csum),
	QETH_TXQ_STAT("TSO skbs", skbs_tso),
	QETH_TXQ_STAT("linearized skbs", skbs_linearized),
	QETH_TXQ_STAT("linearized+error skbs", skbs_linearized_fail),
	QETH_TXQ_STAT("TSO bytes", tso_bytes),
	QETH_TXQ_STAT("Packing mode switches", packing_mode_switch),
	QETH_TXQ_STAT("Queue stopped", stopped),
	QETH_TXQ_STAT("Doorbell", doorbell),
	QETH_TXQ_STAT("IRQ for frames", coal_frames),
	QETH_TXQ_STAT("Completion yield", completion_yield),
	QETH_TXQ_STAT("Completion timer", completion_timer),
};

static const struct qeth_stats card_stats[] = {
	QETH_CARD_STAT("rx0 IO buffers", rx_bufs),
	QETH_CARD_STAT("rx0 HW csum skbs", rx_skb_csum),
	QETH_CARD_STAT("rx0 SG skbs", rx_sg_skbs),
	QETH_CARD_STAT("rx0 SG page frags", rx_sg_frags),
	QETH_CARD_STAT("rx0 SG page allocs", rx_sg_alloc_page),
	QETH_CARD_STAT("rx0 dropped, no memory", rx_dropped_nomem),
	QETH_CARD_STAT("rx0 dropped, bad format", rx_dropped_notsupp),
	QETH_CARD_STAT("rx0 dropped, runt", rx_dropped_runt),
};

#define TXQ_STATS_LEN	ARRAY_SIZE(txq_stats)
#define CARD_STATS_LEN	ARRAY_SIZE(card_stats)

static void qeth_add_stat_data(u64 **dst, void *src,
			       const struct qeth_stats stats[],
			       unsigned int size)
{
	unsigned int i;
	char *stat;

	for (i = 0; i < size; i++) {
		stat = (char *)src + stats[i].offset;
		**dst = *(u64 *)stat;
		(*dst)++;
	}
}

static void qeth_add_stat_strings(u8 **data, const char *prefix,
				  const struct qeth_stats stats[],
				  unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++) {
		snprintf(*data, ETH_GSTRING_LEN, "%s%s", prefix, stats[i].name);
		*data += ETH_GSTRING_LEN;
	}
}

static int qeth_get_sset_count(struct net_device *dev, int stringset)
{
	struct qeth_card *card = dev->ml_priv;

	switch (stringset) {
	case ETH_SS_STATS:
		return CARD_STATS_LEN +
		       card->qdio.no_out_queues * TXQ_STATS_LEN;
	default:
		return -EINVAL;
	}
}

static void qeth_get_ethtool_stats(struct net_device *dev,
				   struct ethtool_stats *stats, u64 *data)
{
	struct qeth_card *card = dev->ml_priv;
	unsigned int i;

	qeth_add_stat_data(&data, &card->stats, card_stats, CARD_STATS_LEN);
	for (i = 0; i < card->qdio.no_out_queues; i++)
		qeth_add_stat_data(&data, &card->qdio.out_qs[i]->stats,
				   txq_stats, TXQ_STATS_LEN);
}

static void __qeth_set_coalesce(struct net_device *dev,
				struct qeth_qdio_out_q *queue,
				struct ethtool_coalesce *coal)
{
	WRITE_ONCE(queue->coalesce_usecs, coal->tx_coalesce_usecs);
	WRITE_ONCE(queue->max_coalesced_frames, coal->tx_max_coalesced_frames);

	if (coal->tx_coalesce_usecs &&
	    netif_running(dev) &&
	    !qeth_out_queue_is_empty(queue))
		qeth_tx_arm_timer(queue, coal->tx_coalesce_usecs);
}

static int qeth_set_coalesce(struct net_device *dev,
			     struct ethtool_coalesce *coal)
{
	struct qeth_card *card = dev->ml_priv;
	struct qeth_qdio_out_q *queue;
	unsigned int i;

	if (!IS_IQD(card))
		return -EOPNOTSUPP;

	if (!coal->tx_coalesce_usecs && !coal->tx_max_coalesced_frames)
		return -EINVAL;

	qeth_for_each_output_queue(card, queue, i)
		__qeth_set_coalesce(dev, queue, coal);

	return 0;
}

static void qeth_get_ringparam(struct net_device *dev,
			       struct ethtool_ringparam *param)
{
	struct qeth_card *card = dev->ml_priv;

	param->rx_max_pending = QDIO_MAX_BUFFERS_PER_Q;
	param->rx_mini_max_pending = 0;
	param->rx_jumbo_max_pending = 0;
	param->tx_max_pending = QDIO_MAX_BUFFERS_PER_Q;

	param->rx_pending = card->qdio.in_buf_pool.buf_count;
	param->rx_mini_pending = 0;
	param->rx_jumbo_pending = 0;
	param->tx_pending = QDIO_MAX_BUFFERS_PER_Q;
}

static void qeth_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	struct qeth_card *card = dev->ml_priv;
	char prefix[ETH_GSTRING_LEN] = "";
	unsigned int i;

	switch (stringset) {
	case ETH_SS_STATS:
		qeth_add_stat_strings(&data, prefix, card_stats,
				      CARD_STATS_LEN);
		for (i = 0; i < card->qdio.no_out_queues; i++) {
			snprintf(prefix, ETH_GSTRING_LEN, "tx%u ", i);
			qeth_add_stat_strings(&data, prefix, txq_stats,
					      TXQ_STATS_LEN);
		}
		break;
	default:
		WARN_ON(1);
		break;
	}
}

static void qeth_get_drvinfo(struct net_device *dev,
			     struct ethtool_drvinfo *info)
{
	struct qeth_card *card = dev->ml_priv;

	strlcpy(info->driver, IS_LAYER2(card) ? "qeth_l2" : "qeth_l3",
		sizeof(info->driver));
	strlcpy(info->fw_version, card->info.mcl_level,
		sizeof(info->fw_version));
	snprintf(info->bus_info, sizeof(info->bus_info), "%s/%s/%s",
		 CARD_RDEV_ID(card), CARD_WDEV_ID(card), CARD_DDEV_ID(card));
}

static void qeth_get_channels(struct net_device *dev,
			      struct ethtool_channels *channels)
{
	struct qeth_card *card = dev->ml_priv;

	channels->max_rx = dev->num_rx_queues;
	channels->max_tx = card->qdio.no_out_queues;
	channels->max_other = 0;
	channels->max_combined = 0;
	channels->rx_count = dev->real_num_rx_queues;
	channels->tx_count = dev->real_num_tx_queues;
	channels->other_count = 0;
	channels->combined_count = 0;
}

static int qeth_set_channels(struct net_device *dev,
			     struct ethtool_channels *channels)
{
	struct qeth_priv *priv = netdev_priv(dev);
	struct qeth_card *card = dev->ml_priv;
	int rc;

	if (channels->rx_count == 0 || channels->tx_count == 0)
		return -EINVAL;
	if (channels->tx_count > card->qdio.no_out_queues)
		return -EINVAL;

	/* Prio-queueing needs all TX queues: */
	if (qeth_uses_tx_prio_queueing(card))
		return -EPERM;

	if (IS_IQD(card)) {
		if (channels->tx_count < QETH_IQD_MIN_TXQ)
			return -EINVAL;

		/* Reject downgrade while running. It could push displaced
		 * ucast flows onto txq0, which is reserved for mcast.
		 */
		if (netif_running(dev) &&
		    channels->tx_count < dev->real_num_tx_queues)
			return -EPERM;
	}

	rc = qeth_set_real_num_tx_queues(card, channels->tx_count);
	if (!rc)
		priv->tx_wanted_queues = channels->tx_count;

	return rc;
}

static int qeth_get_ts_info(struct net_device *dev,
			    struct ethtool_ts_info *info)
{
	struct qeth_card *card = dev->ml_priv;

	if (!IS_IQD(card))
		return -EOPNOTSUPP;

	return ethtool_op_get_ts_info(dev, info);
}

static int qeth_get_tunable(struct net_device *dev,
			    const struct ethtool_tunable *tuna, void *data)
{
	struct qeth_priv *priv = netdev_priv(dev);

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		*(u32 *)data = priv->rx_copybreak;
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static int qeth_set_tunable(struct net_device *dev,
			    const struct ethtool_tunable *tuna,
			    const void *data)
{
	struct qeth_priv *priv = netdev_priv(dev);

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		WRITE_ONCE(priv->rx_copybreak, *(u32 *)data);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static int qeth_get_per_queue_coalesce(struct net_device *dev, u32 __queue,
				       struct ethtool_coalesce *coal)
{
	struct qeth_card *card = dev->ml_priv;
	struct qeth_qdio_out_q *queue;

	if (!IS_IQD(card))
		return -EOPNOTSUPP;

	if (__queue >= card->qdio.no_out_queues)
		return -EINVAL;

	queue = card->qdio.out_qs[__queue];

	coal->tx_coalesce_usecs = queue->coalesce_usecs;
	coal->tx_max_coalesced_frames = queue->max_coalesced_frames;
	return 0;
}

static int qeth_set_per_queue_coalesce(struct net_device *dev, u32 queue,
				       struct ethtool_coalesce *coal)
{
	struct qeth_card *card = dev->ml_priv;

	if (!IS_IQD(card))
		return -EOPNOTSUPP;

	if (queue >= card->qdio.no_out_queues)
		return -EINVAL;

	if (!coal->tx_coalesce_usecs && !coal->tx_max_coalesced_frames)
		return -EINVAL;

	__qeth_set_coalesce(dev, card->qdio.out_qs[queue], coal);
	return 0;
}

/* Helper function to fill 'advertising' and 'supported' which are the same. */
/* Autoneg and full-duplex are supported and advertised unconditionally.     */
/* Always advertise and support all speeds up to specified, and only one     */
/* specified port type.							     */
static void qeth_set_cmd_adv_sup(struct ethtool_link_ksettings *cmd,
				int maxspeed, int porttype)
{
	ethtool_link_ksettings_zero_link_mode(cmd, supported);
	ethtool_link_ksettings_zero_link_mode(cmd, advertising);
	ethtool_link_ksettings_zero_link_mode(cmd, lp_advertising);

	ethtool_link_ksettings_add_link_mode(cmd, supported, Autoneg);
	ethtool_link_ksettings_add_link_mode(cmd, advertising, Autoneg);

	switch (porttype) {
	case PORT_TP:
		ethtool_link_ksettings_add_link_mode(cmd, supported, TP);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, TP);
		break;
	case PORT_FIBRE:
		ethtool_link_ksettings_add_link_mode(cmd, supported, FIBRE);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, FIBRE);
		break;
	default:
		ethtool_link_ksettings_add_link_mode(cmd, supported, TP);
		ethtool_link_ksettings_add_link_mode(cmd, advertising, TP);
		WARN_ON_ONCE(1);
	}

	/* partially does fall through, to also select lower speeds */
	switch (maxspeed) {
	case SPEED_25000:
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     25000baseSR_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     25000baseSR_Full);
		break;
	case SPEED_10000:
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     10000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     10000baseT_Full);
		fallthrough;
	case SPEED_1000:
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     1000baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     1000baseT_Half);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     1000baseT_Half);
		fallthrough;
	case SPEED_100:
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     100baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     100baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     100baseT_Half);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     100baseT_Half);
		fallthrough;
	case SPEED_10:
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     10baseT_Half);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     10baseT_Half);
		break;
	default:
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     10baseT_Full);
		ethtool_link_ksettings_add_link_mode(cmd, supported,
						     10baseT_Half);
		ethtool_link_ksettings_add_link_mode(cmd, advertising,
						     10baseT_Half);
		WARN_ON_ONCE(1);
	}
}


static int qeth_get_link_ksettings(struct net_device *netdev,
				   struct ethtool_link_ksettings *cmd)
{
	struct qeth_card *card = netdev->ml_priv;
	enum qeth_link_types link_type;
	struct carrier_info carrier_info;
	int rc;

	if (IS_IQD(card) || IS_VM_NIC(card))
		link_type = QETH_LINK_TYPE_10GBIT_ETH;
	else
		link_type = card->info.link_type;

	cmd->base.duplex = DUPLEX_FULL;
	cmd->base.autoneg = AUTONEG_ENABLE;
	cmd->base.phy_address = 0;
	cmd->base.mdio_support = 0;
	cmd->base.eth_tp_mdix = ETH_TP_MDI_INVALID;
	cmd->base.eth_tp_mdix_ctrl = ETH_TP_MDI_INVALID;

	switch (link_type) {
	case QETH_LINK_TYPE_FAST_ETH:
	case QETH_LINK_TYPE_LANE_ETH100:
		cmd->base.speed = SPEED_100;
		cmd->base.port = PORT_TP;
		break;
	case QETH_LINK_TYPE_GBIT_ETH:
	case QETH_LINK_TYPE_LANE_ETH1000:
		cmd->base.speed = SPEED_1000;
		cmd->base.port = PORT_FIBRE;
		break;
	case QETH_LINK_TYPE_10GBIT_ETH:
		cmd->base.speed = SPEED_10000;
		cmd->base.port = PORT_FIBRE;
		break;
	case QETH_LINK_TYPE_25GBIT_ETH:
		cmd->base.speed = SPEED_25000;
		cmd->base.port = PORT_FIBRE;
		break;
	default:
		cmd->base.speed = SPEED_10;
		cmd->base.port = PORT_TP;
	}
	qeth_set_cmd_adv_sup(cmd, cmd->base.speed, cmd->base.port);

	/* Check if we can obtain more accurate information.	 */
	/* If QUERY_CARD_INFO command is not supported or fails, */
	/* just return the heuristics that was filled above.	 */
	rc = qeth_query_card_info(card, &carrier_info);
	if (rc == -EOPNOTSUPP) /* for old hardware, return heuristic */
		return 0;
	if (rc) /* report error from the hardware operation */
		return rc;
	/* on success, fill in the information got from the hardware */

	netdev_dbg(netdev,
	"card info: card_type=0x%02x, port_mode=0x%04x, port_speed=0x%08x\n",
			carrier_info.card_type,
			carrier_info.port_mode,
			carrier_info.port_speed);

	/* Update attributes for which we've obtained more authoritative */
	/* information, leave the rest the way they where filled above.  */
	switch (carrier_info.card_type) {
	case CARD_INFO_TYPE_1G_COPPER_A:
	case CARD_INFO_TYPE_1G_COPPER_B:
		cmd->base.port = PORT_TP;
		qeth_set_cmd_adv_sup(cmd, SPEED_1000, cmd->base.port);
		break;
	case CARD_INFO_TYPE_1G_FIBRE_A:
	case CARD_INFO_TYPE_1G_FIBRE_B:
		cmd->base.port = PORT_FIBRE;
		qeth_set_cmd_adv_sup(cmd, SPEED_1000, cmd->base.port);
		break;
	case CARD_INFO_TYPE_10G_FIBRE_A:
	case CARD_INFO_TYPE_10G_FIBRE_B:
		cmd->base.port = PORT_FIBRE;
		qeth_set_cmd_adv_sup(cmd, SPEED_10000, cmd->base.port);
		break;
	}

	switch (carrier_info.port_mode) {
	case CARD_INFO_PORTM_FULLDUPLEX:
		cmd->base.duplex = DUPLEX_FULL;
		break;
	case CARD_INFO_PORTM_HALFDUPLEX:
		cmd->base.duplex = DUPLEX_HALF;
		break;
	}

	switch (carrier_info.port_speed) {
	case CARD_INFO_PORTS_10M:
		cmd->base.speed = SPEED_10;
		break;
	case CARD_INFO_PORTS_100M:
		cmd->base.speed = SPEED_100;
		break;
	case CARD_INFO_PORTS_1G:
		cmd->base.speed = SPEED_1000;
		break;
	case CARD_INFO_PORTS_10G:
		cmd->base.speed = SPEED_10000;
		break;
	case CARD_INFO_PORTS_25G:
		cmd->base.speed = SPEED_25000;
		break;
	}

	return 0;
}

const struct ethtool_ops qeth_ethtool_ops = {
	.supported_coalesce_params = ETHTOOL_COALESCE_TX_USECS |
				     ETHTOOL_COALESCE_TX_MAX_FRAMES,
	.get_link = ethtool_op_get_link,
	.set_coalesce = qeth_set_coalesce,
	.get_ringparam = qeth_get_ringparam,
	.get_strings = qeth_get_strings,
	.get_ethtool_stats = qeth_get_ethtool_stats,
	.get_sset_count = qeth_get_sset_count,
	.get_drvinfo = qeth_get_drvinfo,
	.get_channels = qeth_get_channels,
	.set_channels = qeth_set_channels,
	.get_ts_info = qeth_get_ts_info,
	.get_tunable = qeth_get_tunable,
	.set_tunable = qeth_set_tunable,
	.get_per_queue_coalesce = qeth_get_per_queue_coalesce,
	.set_per_queue_coalesce = qeth_set_per_queue_coalesce,
	.get_link_ksettings = qeth_get_link_ksettings,
};

const struct ethtool_ops qeth_osn_ethtool_ops = {
	.get_strings = qeth_get_strings,
	.get_ethtool_stats = qeth_get_ethtool_stats,
	.get_sset_count = qeth_get_sset_count,
	.get_drvinfo = qeth_get_drvinfo,
};
