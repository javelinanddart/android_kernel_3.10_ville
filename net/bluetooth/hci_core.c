/*
   BlueZ - Bluetooth protocol stack for Linux
   Copyright (c) 2000-2001, 2010-2012 The Linux Foundation.  All rights reserved.

   Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
   SOFTWARE IS DISCLAIMED.
*/

/* Bluetooth HCI core. */

<<<<<<< HEAD
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/kmod.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/rfkill.h>
#include <linux/timer.h>
#include <linux/crypto.h>
#include <net/sock.h>

#include <asm/system.h>
#include <linux/uaccess.h>
#include <asm/unaligned.h>
=======
#include <linux/export.h>
#include <linux/idr.h>

#include <linux/rfkill.h>
>>>>>>> common/android-3.10.y

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

<<<<<<< HEAD
#define AUTO_OFF_TIMEOUT 2000

static void hci_cmd_task(unsigned long arg);
static void hci_rx_task(unsigned long arg);
static void hci_tx_task(unsigned long arg);

static DEFINE_RWLOCK(hci_task_lock);

static bool enable_smp = 1;
=======
static void hci_rx_work(struct work_struct *work);
static void hci_cmd_work(struct work_struct *work);
static void hci_tx_work(struct work_struct *work);
>>>>>>> common/android-3.10.y

/* HCI device list */
LIST_HEAD(hci_dev_list);
DEFINE_RWLOCK(hci_dev_list_lock);

/* HCI callback list */
LIST_HEAD(hci_cb_list);
DEFINE_RWLOCK(hci_cb_list_lock);

<<<<<<< HEAD
/* AMP Manager event callbacks */
LIST_HEAD(amp_mgr_cb_list);
DEFINE_RWLOCK(amp_mgr_cb_list_lock);

/* HCI protocols */
#define HCI_MAX_PROTO	2
struct hci_proto *hci_proto[HCI_MAX_PROTO];

/* HCI notifiers list */
static ATOMIC_NOTIFIER_HEAD(hci_notifier);
=======
/* HCI ID Numbering */
static DEFINE_IDA(hci_index_ida);
>>>>>>> common/android-3.10.y

/* ---- HCI notifications ---- */

int hci_register_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_register(&hci_notifier, nb);
}

int hci_unregister_notifier(struct notifier_block *nb)
{
	return atomic_notifier_chain_unregister(&hci_notifier, nb);
}

static void hci_notify(struct hci_dev *hdev, int event)
{
	atomic_notifier_call_chain(&hci_notifier, event, hdev);
}

/* ---- HCI requests ---- */

static void hci_req_sync_complete(struct hci_dev *hdev, u8 result)
{
<<<<<<< HEAD
	BT_DBG("%s command 0x%04x result 0x%2.2x", hdev->name, cmd, result);

	/* If this is the init phase check if the completed command matches
	 * the last init command, and if not just return.
	 */
	if (test_bit(HCI_INIT, &hdev->flags) && hdev->init_last_cmd != cmd)
		return;
=======
	BT_DBG("%s result 0x%2.2x", hdev->name, result);
>>>>>>> common/android-3.10.y

	if (hdev->req_status == HCI_REQ_PEND) {
		hdev->req_result = result;
		hdev->req_status = HCI_REQ_DONE;
		wake_up_interruptible(&hdev->req_wait_q);
	}
}

static void hci_req_cancel(struct hci_dev *hdev, int err)
{
	BT_DBG("%s err 0x%2.2x", hdev->name, err);

	if (hdev->req_status == HCI_REQ_PEND) {
		hdev->req_result = err;
		hdev->req_status = HCI_REQ_CANCELED;
		wake_up_interruptible(&hdev->req_wait_q);
	}
}

static struct sk_buff *hci_get_cmd_complete(struct hci_dev *hdev, u16 opcode,
					    u8 event)
{
	struct hci_ev_cmd_complete *ev;
	struct hci_event_hdr *hdr;
	struct sk_buff *skb;

	hci_dev_lock(hdev);

	skb = hdev->recv_evt;
	hdev->recv_evt = NULL;

	hci_dev_unlock(hdev);

	if (!skb)
		return ERR_PTR(-ENODATA);

	if (skb->len < sizeof(*hdr)) {
		BT_ERR("Too short HCI event");
		goto failed;
	}

	hdr = (void *) skb->data;
	skb_pull(skb, HCI_EVENT_HDR_SIZE);

	if (event) {
		if (hdr->evt != event)
			goto failed;
		return skb;
	}

	if (hdr->evt != HCI_EV_CMD_COMPLETE) {
		BT_DBG("Last event is not cmd complete (0x%2.2x)", hdr->evt);
		goto failed;
	}

	if (skb->len < sizeof(*ev)) {
		BT_ERR("Too short cmd_complete event");
		goto failed;
	}

	ev = (void *) skb->data;
	skb_pull(skb, sizeof(*ev));

	if (opcode == __le16_to_cpu(ev->opcode))
		return skb;

	BT_DBG("opcode doesn't match (0x%2.2x != 0x%2.2x)", opcode,
	       __le16_to_cpu(ev->opcode));

failed:
	kfree_skb(skb);
	return ERR_PTR(-ENODATA);
}

struct sk_buff *__hci_cmd_sync_ev(struct hci_dev *hdev, u16 opcode, u32 plen,
				  const void *param, u8 event, u32 timeout)
{
	DECLARE_WAITQUEUE(wait, current);
	struct hci_request req;
	int err = 0;

	BT_DBG("%s", hdev->name);

	hci_req_init(&req, hdev);

	hci_req_add_ev(&req, opcode, plen, param, event);

	hdev->req_status = HCI_REQ_PEND;

	err = hci_req_run(&req, hci_req_sync_complete);
	if (err < 0)
		return ERR_PTR(err);

	add_wait_queue(&hdev->req_wait_q, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	schedule_timeout(timeout);

	remove_wait_queue(&hdev->req_wait_q, &wait);

	if (signal_pending(current))
		return ERR_PTR(-EINTR);

	switch (hdev->req_status) {
	case HCI_REQ_DONE:
		err = -bt_to_errno(hdev->req_result);
		break;

	case HCI_REQ_CANCELED:
		err = -hdev->req_result;
		break;

	default:
		err = -ETIMEDOUT;
		break;
	}

	hdev->req_status = hdev->req_result = 0;

	BT_DBG("%s end: err %d", hdev->name, err);

	if (err < 0)
		return ERR_PTR(err);

	return hci_get_cmd_complete(hdev, opcode, event);
}
EXPORT_SYMBOL(__hci_cmd_sync_ev);

struct sk_buff *__hci_cmd_sync(struct hci_dev *hdev, u16 opcode, u32 plen,
			       const void *param, u32 timeout)
{
	return __hci_cmd_sync_ev(hdev, opcode, plen, param, 0, timeout);
}
EXPORT_SYMBOL(__hci_cmd_sync);

/* Execute request and wait for completion. */
static int __hci_req_sync(struct hci_dev *hdev,
			  void (*func)(struct hci_request *req,
				      unsigned long opt),
			  unsigned long opt, __u32 timeout)
{
	struct hci_request req;
	DECLARE_WAITQUEUE(wait, current);
	int err = 0;

	BT_DBG("%s start", hdev->name);

	hci_req_init(&req, hdev);

	hdev->req_status = HCI_REQ_PEND;

	func(&req, opt);

	err = hci_req_run(&req, hci_req_sync_complete);
	if (err < 0) {
		hdev->req_status = 0;

		/* ENODATA means the HCI request command queue is empty.
		 * This can happen when a request with conditionals doesn't
		 * trigger any commands to be sent. This is normal behavior
		 * and should not trigger an error return.
		 */
		if (err == -ENODATA)
			return 0;

		return err;
	}

	add_wait_queue(&hdev->req_wait_q, &wait);
	set_current_state(TASK_INTERRUPTIBLE);

	schedule_timeout(timeout);

	remove_wait_queue(&hdev->req_wait_q, &wait);

	if (signal_pending(current))
		return -EINTR;

	switch (hdev->req_status) {
	case HCI_REQ_DONE:
		err = -bt_err(hdev->req_result);
		break;

	case HCI_REQ_CANCELED:
		err = -hdev->req_result;
		break;

	default:
		err = -ETIMEDOUT;
		break;
	}

	hdev->req_status = hdev->req_result = 0;

	BT_DBG("%s end: err %d", hdev->name, err);

	return err;
}

static int hci_req_sync(struct hci_dev *hdev,
			void (*req)(struct hci_request *req,
				    unsigned long opt),
			unsigned long opt, __u32 timeout)
{
	int ret;

	if (!test_bit(HCI_UP, &hdev->flags))
		return -ENETDOWN;

	/* Serialize all requests */
	hci_req_lock(hdev);
	ret = __hci_req_sync(hdev, req, opt, timeout);
	hci_req_unlock(hdev);

	return ret;
}

static void hci_reset_req(struct hci_request *req, unsigned long opt)
{
	BT_DBG("%s %ld", req->hdev->name, opt);

	/* Reset device */
<<<<<<< HEAD
	set_bit(HCI_RESET, &hdev->flags);
	memset(&hdev->features, 0, sizeof(hdev->features));
	hci_send_cmd(hdev, HCI_OP_RESET, 0, NULL);
}

static void hci_init_req(struct hci_dev *hdev, unsigned long opt)
{
	struct hci_cp_delete_stored_link_key cp;
	struct sk_buff *skb;
	__le16 param;
	__u8 flt_type;

	BT_DBG("%s %ld", hdev->name, opt);

	/* Driver initialization */

	/* Special commands */
	while ((skb = skb_dequeue(&hdev->driver_init))) {
		bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
		skb->dev = (void *) hdev;

		skb_queue_tail(&hdev->cmd_q, skb);
		tasklet_schedule(&hdev->cmd_task);
	}
	skb_queue_purge(&hdev->driver_init);
=======
	set_bit(HCI_RESET, &req->hdev->flags);
	hci_req_add(req, HCI_OP_RESET, 0, NULL);
}

static void bredr_init(struct hci_request *req)
{
	req->hdev->flow_ctl_mode = HCI_FLOW_CTL_MODE_PACKET_BASED;

	/* Read Local Supported Features */
	hci_req_add(req, HCI_OP_READ_LOCAL_FEATURES, 0, NULL);
>>>>>>> common/android-3.10.y

	/* Read Local Version */
	hci_req_add(req, HCI_OP_READ_LOCAL_VERSION, 0, NULL);

<<<<<<< HEAD
	/* Reset */
	if (!test_bit(HCI_QUIRK_NO_RESET, &hdev->quirks)) {
			set_bit(HCI_RESET, &hdev->flags);
			hci_send_cmd(hdev, HCI_OP_RESET, 0, NULL);
	}

=======
	/* Read BD Address */
	hci_req_add(req, HCI_OP_READ_BD_ADDR, 0, NULL);
}

static void amp_init(struct hci_request *req)
{
	req->hdev->flow_ctl_mode = HCI_FLOW_CTL_MODE_BLOCK_BASED;

>>>>>>> common/android-3.10.y
	/* Read Local Version */
	hci_req_add(req, HCI_OP_READ_LOCAL_VERSION, 0, NULL);

<<<<<<< HEAD

	/* Set default HCI Flow Control Mode */
	if (hdev->dev_type == HCI_BREDR)
		hdev->flow_ctl_mode = HCI_PACKET_BASED_FLOW_CTL_MODE;
	else
		hdev->flow_ctl_mode = HCI_BLOCK_BASED_FLOW_CTL_MODE;

	/* Read HCI Flow Control Mode */
	hci_send_cmd(hdev, HCI_OP_READ_FLOW_CONTROL_MODE, 0, NULL);

	/* Read Buffer Size (ACL mtu, max pkt, etc.) */
	hci_send_cmd(hdev, HCI_OP_READ_BUFFER_SIZE, 0, NULL);

	/* Read Data Block Size (ACL mtu, max pkt, etc.) */
	hci_send_cmd(hdev, HCI_OP_READ_DATA_BLOCK_SIZE, 0, NULL);

#if 0
	/* Host buffer size */
	{
		struct hci_cp_host_buffer_size cp;
		cp.acl_mtu = cpu_to_le16(HCI_MAX_ACL_SIZE);
		cp.sco_mtu = HCI_MAX_SCO_SIZE;
		cp.acl_max_pkt = cpu_to_le16(0xffff);
		cp.sco_max_pkt = cpu_to_le16(0xffff);
		hci_send_cmd(hdev, HCI_OP_HOST_BUFFER_SIZE, sizeof(cp), &cp);
	}
#endif

	if (hdev->dev_type == HCI_BREDR) {
		/* BR-EDR initialization */

		/* Read Local Supported Features */
		hci_send_cmd(hdev, HCI_OP_READ_LOCAL_FEATURES, 0, NULL);

		/* Read BD Address */
		hci_send_cmd(hdev, HCI_OP_READ_BD_ADDR, 0, NULL);

		/* Read Class of Device */
		hci_send_cmd(hdev, HCI_OP_READ_CLASS_OF_DEV, 0, NULL);

		/* Read Local Name */
		hci_send_cmd(hdev, HCI_OP_READ_LOCAL_NAME, 0, NULL);

		/* Read Voice Setting */
		hci_send_cmd(hdev, HCI_OP_READ_VOICE_SETTING, 0, NULL);

		/* Optional initialization */
		/* Clear Event Filters */
		flt_type = HCI_FLT_CLEAR_ALL;
		hci_send_cmd(hdev, HCI_OP_SET_EVENT_FLT, 1, &flt_type);

		/* Connection accept timeout ~20 secs */
		param = cpu_to_le16(0x7d00);
		hci_send_cmd(hdev, HCI_OP_WRITE_CA_TIMEOUT, 2, &param);

		bacpy(&cp.bdaddr, BDADDR_ANY);
		cp.delete_all = 1;
		hci_send_cmd(hdev, HCI_OP_DELETE_STORED_LINK_KEY,
				sizeof(cp), &cp);
	} else {
		/* AMP initialization */
		/* Connection accept timeout ~5 secs */
		param = cpu_to_le16(0x1f40);
		hci_send_cmd(hdev, HCI_OP_WRITE_CA_TIMEOUT, 2, &param);

		/* Read AMP Info */
		hci_send_cmd(hdev, HCI_OP_READ_LOCAL_AMP_INFO, 0, NULL);
	}
=======
	/* Read Local AMP Info */
	hci_req_add(req, HCI_OP_READ_LOCAL_AMP_INFO, 0, NULL);

	/* Read Data Blk size */
	hci_req_add(req, HCI_OP_READ_DATA_BLOCK_SIZE, 0, NULL);
}

static void hci_init1_req(struct hci_request *req, unsigned long opt)
{
	struct hci_dev *hdev = req->hdev;

	BT_DBG("%s %ld", hdev->name, opt);

	/* Reset */
	if (!test_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks))
		hci_reset_req(req, 0);

	switch (hdev->dev_type) {
	case HCI_BREDR:
		bredr_init(req);
		break;

	case HCI_AMP:
		amp_init(req);
		break;

	default:
		BT_ERR("Unknown device type %d", hdev->dev_type);
		break;
	}
}

static void bredr_setup(struct hci_request *req)
{
	__le16 param;
	__u8 flt_type;

	/* Read Buffer Size (ACL mtu, max pkt, etc.) */
	hci_req_add(req, HCI_OP_READ_BUFFER_SIZE, 0, NULL);

	/* Read Class of Device */
	hci_req_add(req, HCI_OP_READ_CLASS_OF_DEV, 0, NULL);

	/* Read Local Name */
	hci_req_add(req, HCI_OP_READ_LOCAL_NAME, 0, NULL);

	/* Read Voice Setting */
	hci_req_add(req, HCI_OP_READ_VOICE_SETTING, 0, NULL);

	/* Clear Event Filters */
	flt_type = HCI_FLT_CLEAR_ALL;
	hci_req_add(req, HCI_OP_SET_EVENT_FLT, 1, &flt_type);

	/* Connection accept timeout ~20 secs */
	param = __constant_cpu_to_le16(0x7d00);
	hci_req_add(req, HCI_OP_WRITE_CA_TIMEOUT, 2, &param);

	/* Read page scan parameters */
	if (req->hdev->hci_ver > BLUETOOTH_VER_1_1) {
		hci_req_add(req, HCI_OP_READ_PAGE_SCAN_ACTIVITY, 0, NULL);
		hci_req_add(req, HCI_OP_READ_PAGE_SCAN_TYPE, 0, NULL);
	}
}

static void le_setup(struct hci_request *req)
{
	struct hci_dev *hdev = req->hdev;

	/* Read LE Buffer Size */
	hci_req_add(req, HCI_OP_LE_READ_BUFFER_SIZE, 0, NULL);

	/* Read LE Local Supported Features */
	hci_req_add(req, HCI_OP_LE_READ_LOCAL_FEATURES, 0, NULL);

	/* Read LE Advertising Channel TX Power */
	hci_req_add(req, HCI_OP_LE_READ_ADV_TX_POWER, 0, NULL);

	/* Read LE White List Size */
	hci_req_add(req, HCI_OP_LE_READ_WHITE_LIST_SIZE, 0, NULL);

	/* Read LE Supported States */
	hci_req_add(req, HCI_OP_LE_READ_SUPPORTED_STATES, 0, NULL);

	/* LE-only controllers have LE implicitly enabled */
	if (!lmp_bredr_capable(hdev))
		set_bit(HCI_LE_ENABLED, &hdev->dev_flags);
}

static u8 hci_get_inquiry_mode(struct hci_dev *hdev)
{
	if (lmp_ext_inq_capable(hdev))
		return 0x02;

	if (lmp_inq_rssi_capable(hdev))
		return 0x01;

	if (hdev->manufacturer == 11 && hdev->hci_rev == 0x00 &&
	    hdev->lmp_subver == 0x0757)
		return 0x01;

	if (hdev->manufacturer == 15) {
		if (hdev->hci_rev == 0x03 && hdev->lmp_subver == 0x6963)
			return 0x01;
		if (hdev->hci_rev == 0x09 && hdev->lmp_subver == 0x6963)
			return 0x01;
		if (hdev->hci_rev == 0x00 && hdev->lmp_subver == 0x6965)
			return 0x01;
	}

	if (hdev->manufacturer == 31 && hdev->hci_rev == 0x2005 &&
	    hdev->lmp_subver == 0x1805)
		return 0x01;

	return 0x00;
}

static void hci_setup_inquiry_mode(struct hci_request *req)
{
	u8 mode;

	mode = hci_get_inquiry_mode(req->hdev);

	hci_req_add(req, HCI_OP_WRITE_INQUIRY_MODE, 1, &mode);
}

static void hci_setup_event_mask(struct hci_request *req)
{
	struct hci_dev *hdev = req->hdev;

	/* The second byte is 0xff instead of 0x9f (two reserved bits
	 * disabled) since a Broadcom 1.2 dongle doesn't respond to the
	 * command otherwise.
	 */
	u8 events[8] = { 0xff, 0xff, 0xfb, 0xff, 0x00, 0x00, 0x00, 0x00 };

	/* CSR 1.1 dongles does not accept any bitfield so don't try to set
	 * any event mask for pre 1.2 devices.
	 */
	if (hdev->hci_ver < BLUETOOTH_VER_1_2)
		return;

	if (lmp_bredr_capable(hdev)) {
		events[4] |= 0x01; /* Flow Specification Complete */
		events[4] |= 0x02; /* Inquiry Result with RSSI */
		events[4] |= 0x04; /* Read Remote Extended Features Complete */
		events[5] |= 0x08; /* Synchronous Connection Complete */
		events[5] |= 0x10; /* Synchronous Connection Changed */
	}

	if (lmp_inq_rssi_capable(hdev))
		events[4] |= 0x02; /* Inquiry Result with RSSI */

	if (lmp_sniffsubr_capable(hdev))
		events[5] |= 0x20; /* Sniff Subrating */

	if (lmp_pause_enc_capable(hdev))
		events[5] |= 0x80; /* Encryption Key Refresh Complete */

	if (lmp_ext_inq_capable(hdev))
		events[5] |= 0x40; /* Extended Inquiry Result */

	if (lmp_no_flush_capable(hdev))
		events[7] |= 0x01; /* Enhanced Flush Complete */

	if (lmp_lsto_capable(hdev))
		events[6] |= 0x80; /* Link Supervision Timeout Changed */

	if (lmp_ssp_capable(hdev)) {
		events[6] |= 0x01;	/* IO Capability Request */
		events[6] |= 0x02;	/* IO Capability Response */
		events[6] |= 0x04;	/* User Confirmation Request */
		events[6] |= 0x08;	/* User Passkey Request */
		events[6] |= 0x10;	/* Remote OOB Data Request */
		events[6] |= 0x20;	/* Simple Pairing Complete */
		events[7] |= 0x04;	/* User Passkey Notification */
		events[7] |= 0x08;	/* Keypress Notification */
		events[7] |= 0x10;	/* Remote Host Supported
					 * Features Notification
					 */
	}

	if (lmp_le_capable(hdev))
		events[7] |= 0x20;	/* LE Meta-Event */

	hci_req_add(req, HCI_OP_SET_EVENT_MASK, sizeof(events), events);

	if (lmp_le_capable(hdev)) {
		memset(events, 0, sizeof(events));
		events[0] = 0x1f;
		hci_req_add(req, HCI_OP_LE_SET_EVENT_MASK,
			    sizeof(events), events);
	}
>>>>>>> common/android-3.10.y
}

static void hci_init2_req(struct hci_request *req, unsigned long opt)
{
	struct hci_dev *hdev = req->hdev;

	if (lmp_bredr_capable(hdev))
		bredr_setup(req);

	if (lmp_le_capable(hdev))
		le_setup(req);

	hci_setup_event_mask(req);

	if (hdev->hci_ver > BLUETOOTH_VER_1_1)
		hci_req_add(req, HCI_OP_READ_LOCAL_COMMANDS, 0, NULL);

	if (lmp_ssp_capable(hdev)) {
		if (test_bit(HCI_SSP_ENABLED, &hdev->dev_flags)) {
			u8 mode = 0x01;
			hci_req_add(req, HCI_OP_WRITE_SSP_MODE,
				    sizeof(mode), &mode);
		} else {
			struct hci_cp_write_eir cp;

			memset(hdev->eir, 0, sizeof(hdev->eir));
			memset(&cp, 0, sizeof(cp));

			hci_req_add(req, HCI_OP_WRITE_EIR, sizeof(cp), &cp);
		}
	}

	if (lmp_inq_rssi_capable(hdev))
		hci_setup_inquiry_mode(req);

	if (lmp_inq_tx_pwr_capable(hdev))
		hci_req_add(req, HCI_OP_READ_INQ_RSP_TX_POWER, 0, NULL);

	if (lmp_ext_feat_capable(hdev)) {
		struct hci_cp_read_local_ext_features cp;

		cp.page = 0x01;
		hci_req_add(req, HCI_OP_READ_LOCAL_EXT_FEATURES,
			    sizeof(cp), &cp);
	}

<<<<<<< HEAD
	/* Read LE buffer size */
	hci_send_cmd(hdev, HCI_OP_LE_READ_BUFFER_SIZE, 0, NULL);

	/* Read LE clear white list */
	hci_send_cmd(hdev, HCI_OP_LE_CLEAR_WHITE_LIST, 0, NULL);

	/* Read LE white list size */
	hci_send_cmd(hdev, HCI_OP_LE_READ_WHITE_LIST_SIZE, 0, NULL);
=======
	if (test_bit(HCI_LINK_SECURITY, &hdev->dev_flags)) {
		u8 enable = 1;
		hci_req_add(req, HCI_OP_WRITE_AUTH_ENABLE, sizeof(enable),
			    &enable);
	}
}

static void hci_setup_link_policy(struct hci_request *req)
{
	struct hci_dev *hdev = req->hdev;
	struct hci_cp_write_def_link_policy cp;
	u16 link_policy = 0;

	if (lmp_rswitch_capable(hdev))
		link_policy |= HCI_LP_RSWITCH;
	if (lmp_hold_capable(hdev))
		link_policy |= HCI_LP_HOLD;
	if (lmp_sniff_capable(hdev))
		link_policy |= HCI_LP_SNIFF;
	if (lmp_park_capable(hdev))
		link_policy |= HCI_LP_PARK;

	cp.policy = cpu_to_le16(link_policy);
	hci_req_add(req, HCI_OP_WRITE_DEF_LINK_POLICY, sizeof(cp), &cp);
}

static void hci_set_le_support(struct hci_request *req)
{
	struct hci_dev *hdev = req->hdev;
	struct hci_cp_write_le_host_supported cp;

	/* LE-only devices do not support explicit enablement */
	if (!lmp_bredr_capable(hdev))
		return;

	memset(&cp, 0, sizeof(cp));

	if (test_bit(HCI_LE_ENABLED, &hdev->dev_flags)) {
		cp.le = 0x01;
		cp.simul = lmp_le_br_capable(hdev);
	}

	if (cp.le != lmp_host_le_capable(hdev))
		hci_req_add(req, HCI_OP_WRITE_LE_HOST_SUPPORTED, sizeof(cp),
			    &cp);
}

static void hci_init3_req(struct hci_request *req, unsigned long opt)
{
	struct hci_dev *hdev = req->hdev;
	u8 p;

	/* Only send HCI_Delete_Stored_Link_Key if it is supported */
	if (hdev->commands[6] & 0x80) {
		struct hci_cp_delete_stored_link_key cp;

		bacpy(&cp.bdaddr, BDADDR_ANY);
		cp.delete_all = 0x01;
		hci_req_add(req, HCI_OP_DELETE_STORED_LINK_KEY,
			    sizeof(cp), &cp);
	}

	if (hdev->commands[5] & 0x10)
		hci_setup_link_policy(req);

	if (lmp_le_capable(hdev)) {
		hci_set_le_support(req);
		hci_update_ad(req);
	}

	/* Read features beyond page 1 if available */
	for (p = 2; p < HCI_MAX_PAGES && p <= hdev->max_page; p++) {
		struct hci_cp_read_local_ext_features cp;

		cp.page = p;
		hci_req_add(req, HCI_OP_READ_LOCAL_EXT_FEATURES,
			    sizeof(cp), &cp);
	}
>>>>>>> common/android-3.10.y
}

static int __hci_init(struct hci_dev *hdev)
{
	int err;

	err = __hci_req_sync(hdev, hci_init1_req, 0, HCI_INIT_TIMEOUT);
	if (err < 0)
		return err;

	/* HCI_BREDR covers both single-mode LE, BR/EDR and dual-mode
	 * BR/EDR/LE type controllers. AMP controllers only need the
	 * first stage init.
	 */
	if (hdev->dev_type != HCI_BREDR)
		return 0;

	err = __hci_req_sync(hdev, hci_init2_req, 0, HCI_INIT_TIMEOUT);
	if (err < 0)
		return err;

	return __hci_req_sync(hdev, hci_init3_req, 0, HCI_INIT_TIMEOUT);
}

static void hci_scan_req(struct hci_request *req, unsigned long opt)
{
	__u8 scan = opt;

	BT_DBG("%s %x", req->hdev->name, scan);

	/* Inquiry and Page scans */
	hci_req_add(req, HCI_OP_WRITE_SCAN_ENABLE, 1, &scan);
}

static void hci_auth_req(struct hci_request *req, unsigned long opt)
{
	__u8 auth = opt;

	BT_DBG("%s %x", req->hdev->name, auth);

	/* Authentication */
	hci_req_add(req, HCI_OP_WRITE_AUTH_ENABLE, 1, &auth);
}

static void hci_encrypt_req(struct hci_request *req, unsigned long opt)
{
	__u8 encrypt = opt;

	BT_DBG("%s %x", req->hdev->name, encrypt);

	/* Encryption */
	hci_req_add(req, HCI_OP_WRITE_ENCRYPT_MODE, 1, &encrypt);
}

static void hci_linkpol_req(struct hci_request *req, unsigned long opt)
{
	__le16 policy = cpu_to_le16(opt);

	BT_DBG("%s %x", req->hdev->name, policy);

	/* Default link policy */
	hci_req_add(req, HCI_OP_WRITE_DEF_LINK_POLICY, 2, &policy);
}

/* Get HCI device by index.
 * Device is held on return. */
struct hci_dev *hci_dev_get(int index)
{
	struct hci_dev *hdev = NULL;
	struct list_head *p;

	BT_DBG("%d", index);

	if (index < 0)
		return NULL;

	read_lock(&hci_dev_list_lock);
	list_for_each(p, &hci_dev_list) {
		struct hci_dev *d = list_entry(p, struct hci_dev, list);
		if (d->id == index) {
			hdev = hci_dev_hold(d);
			break;
		}
	}
	read_unlock(&hci_dev_list_lock);
	return hdev;
}
EXPORT_SYMBOL(hci_dev_get);

/* ---- Inquiry support ---- */
<<<<<<< HEAD
static void inquiry_cache_flush(struct hci_dev *hdev)
{
	struct inquiry_cache *cache = &hdev->inq_cache;
	struct inquiry_entry *next  = cache->list, *e;

	BT_DBG("cache %p", cache);
=======

bool hci_discovery_active(struct hci_dev *hdev)
{
	struct discovery_state *discov = &hdev->discovery;

	switch (discov->state) {
	case DISCOVERY_FINDING:
	case DISCOVERY_RESOLVING:
		return true;

	default:
		return false;
	}
}

void hci_discovery_set_state(struct hci_dev *hdev, int state)
{
	BT_DBG("%s state %u -> %u", hdev->name, hdev->discovery.state, state);

	if (hdev->discovery.state == state)
		return;

	switch (state) {
	case DISCOVERY_STOPPED:
		if (hdev->discovery.state != DISCOVERY_STARTING)
			mgmt_discovering(hdev, 0);
		break;
	case DISCOVERY_STARTING:
		break;
	case DISCOVERY_FINDING:
		mgmt_discovering(hdev, 1);
		break;
	case DISCOVERY_RESOLVING:
		break;
	case DISCOVERY_STOPPING:
		break;
	}

	hdev->discovery.state = state;
}

static void inquiry_cache_flush(struct hci_dev *hdev)
{
	struct discovery_state *cache = &hdev->discovery;
	struct inquiry_entry *p, *n;

	list_for_each_entry_safe(p, n, &cache->all, all) {
		list_del(&p->all);
		kfree(p);
	}

	INIT_LIST_HEAD(&cache->unknown);
	INIT_LIST_HEAD(&cache->resolve);
}

struct inquiry_entry *hci_inquiry_cache_lookup(struct hci_dev *hdev,
					       bdaddr_t *bdaddr)
{
	struct discovery_state *cache = &hdev->discovery;
	struct inquiry_entry *e;

	BT_DBG("cache %p, %pMR", cache, bdaddr);
>>>>>>> common/android-3.10.y

	cache->list = NULL;
	while ((e = next)) {
		next = e->next;
		kfree(e);
	}
}

struct inquiry_entry *hci_inquiry_cache_lookup(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct inquiry_cache *cache = &hdev->inq_cache;
	struct inquiry_entry *e;

	BT_DBG("cache %p, %pMR", cache, bdaddr);

<<<<<<< HEAD
	for (e = cache->list; e; e = e->next)
		if (!bacmp(&e->data.bdaddr, bdaddr))
=======
	list_for_each_entry(e, &cache->unknown, list) {
		if (!bacmp(&e->data.bdaddr, bdaddr))
			return e;
	}

	return NULL;
}

struct inquiry_entry *hci_inquiry_cache_lookup_resolve(struct hci_dev *hdev,
						       bdaddr_t *bdaddr,
						       int state)
{
	struct discovery_state *cache = &hdev->discovery;
	struct inquiry_entry *e;

	BT_DBG("cache %p bdaddr %pMR state %d", cache, bdaddr, state);

	list_for_each_entry(e, &cache->resolve, list) {
		if (!bacmp(bdaddr, BDADDR_ANY) && e->name_state == state)
			return e;
		if (!bacmp(&e->data.bdaddr, bdaddr))
			return e;
	}

	return NULL;
}

void hci_inquiry_cache_update_resolve(struct hci_dev *hdev,
				      struct inquiry_entry *ie)
{
	struct discovery_state *cache = &hdev->discovery;
	struct list_head *pos = &cache->resolve;
	struct inquiry_entry *p;

	list_del(&ie->list);

	list_for_each_entry(p, &cache->resolve, list) {
		if (p->name_state != NAME_PENDING &&
		    abs(p->data.rssi) >= abs(ie->data.rssi))
>>>>>>> common/android-3.10.y
			break;
	return e;
}

void hci_inquiry_cache_update(struct hci_dev *hdev, struct inquiry_data *data)
{
	struct inquiry_cache *cache = &hdev->inq_cache;
	struct inquiry_entry *ie;

	BT_DBG("cache %p, %pMR", cache, &data->bdaddr);

	hci_remove_remote_oob_data(hdev, &data->bdaddr);

	ie = hci_inquiry_cache_lookup(hdev, &data->bdaddr);
<<<<<<< HEAD
	if (!ie) {
		/* Entry not in the cache. Add new one. */
		ie = kzalloc(sizeof(struct inquiry_entry), GFP_ATOMIC);
		if (!ie)
			return;

		ie->next = cache->list;
		cache->list = ie;
=======
	if (ie) {
		if (ie->data.ssp_mode && ssp)
			*ssp = true;

		if (ie->name_state == NAME_NEEDED &&
		    data->rssi != ie->data.rssi) {
			ie->data.rssi = data->rssi;
			hci_inquiry_cache_update_resolve(hdev, ie);
		}

		goto update;
	}

	/* Entry not in the cache. Add new one. */
	ie = kzalloc(sizeof(struct inquiry_entry), GFP_ATOMIC);
	if (!ie)
		return false;

	list_add(&ie->all, &cache->all);

	if (name_known) {
		ie->name_state = NAME_KNOWN;
	} else {
		ie->name_state = NAME_NOT_KNOWN;
		list_add(&ie->list, &cache->unknown);
	}

update:
	if (name_known && ie->name_state != NAME_KNOWN &&
	    ie->name_state != NAME_PENDING) {
		ie->name_state = NAME_KNOWN;
		list_del(&ie->list);
>>>>>>> common/android-3.10.y
	}

	memcpy(&ie->data, data, sizeof(*data));
	ie->timestamp = jiffies;
	cache->timestamp = jiffies;
}

static int inquiry_cache_dump(struct hci_dev *hdev, int num, __u8 *buf)
{
	struct inquiry_cache *cache = &hdev->inq_cache;
	struct inquiry_info *info = (struct inquiry_info *) buf;
	struct inquiry_entry *e;
	int copied = 0;

	for (e = cache->list; e && copied < num; e = e->next, copied++) {
		struct inquiry_data *data = &e->data;
		bacpy(&info->bdaddr, &data->bdaddr);
		info->pscan_rep_mode	= data->pscan_rep_mode;
		info->pscan_period_mode	= data->pscan_period_mode;
		info->pscan_mode	= data->pscan_mode;
		memcpy(info->dev_class, data->dev_class, 3);
		info->clock_offset	= data->clock_offset;
		info++;
	}

	BT_DBG("cache %p, copied %d", cache, copied);
	return copied;
}

static void hci_inq_req(struct hci_request *req, unsigned long opt)
{
	struct hci_inquiry_req *ir = (struct hci_inquiry_req *) opt;
	struct hci_dev *hdev = req->hdev;
	struct hci_cp_inquiry cp;

	BT_DBG("%s", hdev->name);

	if (test_bit(HCI_INQUIRY, &hdev->flags))
		return;

	/* Start Inquiry */
	memcpy(&cp.lap, &ir->lap, 3);
	cp.length  = ir->length;
	cp.num_rsp = ir->num_rsp;
	hci_req_add(req, HCI_OP_INQUIRY, sizeof(cp), &cp);
}

static int wait_inquiry(void *word)
{
	schedule();
	return signal_pending(current);
}

int hci_inquiry(void __user *arg)
{
	__u8 __user *ptr = arg;
	struct hci_inquiry_req ir;
	struct hci_dev *hdev;
	int err = 0, do_inquiry = 0, max_rsp;
	long timeo;
	__u8 *buf;

	if (copy_from_user(&ir, ptr, sizeof(ir)))
		return -EFAULT;

	hdev = hci_dev_get(ir.dev_id);
	if (!hdev)
		return -ENODEV;

	hci_dev_lock_bh(hdev);
	if (inquiry_cache_age(hdev) > INQUIRY_CACHE_AGE_MAX ||
	    inquiry_cache_empty(hdev) || ir.flags & IREQ_CACHE_FLUSH) {
		inquiry_cache_flush(hdev);
		do_inquiry = 1;
	}
	hci_dev_unlock_bh(hdev);

	timeo = ir.length * msecs_to_jiffies(2000);

	if (do_inquiry) {
		err = hci_req_sync(hdev, hci_inq_req, (unsigned long) &ir,
				   timeo);
		if (err < 0)
			goto done;

		/* Wait until Inquiry procedure finishes (HCI_INQUIRY flag is
		 * cleared). If it is interrupted by a signal, return -EINTR.
		 */
		if (wait_on_bit(&hdev->flags, HCI_INQUIRY, wait_inquiry,
				TASK_INTERRUPTIBLE))
			return -EINTR;
	}

	/* for unlimited number of responses we will use buffer with
	 * 255 entries
	 */
	max_rsp = (ir.num_rsp == 0) ? 255 : ir.num_rsp;

	/* cache_dump can't sleep. Therefore we allocate temp buffer and then
	 * copy it to the user space.
	 */
	buf = kmalloc(sizeof(struct inquiry_info) * max_rsp, GFP_KERNEL);
	if (!buf) {
		err = -ENOMEM;
		goto done;
	}

	hci_dev_lock_bh(hdev);
	ir.num_rsp = inquiry_cache_dump(hdev, max_rsp, buf);
	hci_dev_unlock_bh(hdev);

	BT_DBG("num_rsp %d", ir.num_rsp);

	if (!copy_to_user(ptr, &ir, sizeof(ir))) {
		ptr += sizeof(ir);
		if (copy_to_user(ptr, buf, sizeof(struct inquiry_info) *
				 ir.num_rsp))
			err = -EFAULT;
	} else
		err = -EFAULT;

	kfree(buf);

done:
	hci_dev_put(hdev);
	return err;
}

static u8 create_ad(struct hci_dev *hdev, u8 *ptr)
{
	u8 ad_len = 0, flags = 0;
	size_t name_len;

	if (test_bit(HCI_LE_PERIPHERAL, &hdev->dev_flags))
		flags |= LE_AD_GENERAL;

	if (!lmp_bredr_capable(hdev))
		flags |= LE_AD_NO_BREDR;

	if (lmp_le_br_capable(hdev))
		flags |= LE_AD_SIM_LE_BREDR_CTRL;

	if (lmp_host_le_br_capable(hdev))
		flags |= LE_AD_SIM_LE_BREDR_HOST;

	if (flags) {
		BT_DBG("adv flags 0x%02x", flags);

		ptr[0] = 2;
		ptr[1] = EIR_FLAGS;
		ptr[2] = flags;

		ad_len += 3;
		ptr += 3;
	}

	if (hdev->adv_tx_power != HCI_TX_POWER_INVALID) {
		ptr[0] = 2;
		ptr[1] = EIR_TX_POWER;
		ptr[2] = (u8) hdev->adv_tx_power;

		ad_len += 3;
		ptr += 3;
	}

	name_len = strlen(hdev->dev_name);
	if (name_len > 0) {
		size_t max_len = HCI_MAX_AD_LENGTH - ad_len - 2;

		if (name_len > max_len) {
			name_len = max_len;
			ptr[1] = EIR_NAME_SHORT;
		} else
			ptr[1] = EIR_NAME_COMPLETE;

		ptr[0] = name_len + 1;

		memcpy(ptr + 2, hdev->dev_name, name_len);

		ad_len += (name_len + 2);
		ptr += (name_len + 2);
	}

	return ad_len;
}

void hci_update_ad(struct hci_request *req)
{
	struct hci_dev *hdev = req->hdev;
	struct hci_cp_le_set_adv_data cp;
	u8 len;

	if (!lmp_le_capable(hdev))
		return;

	memset(&cp, 0, sizeof(cp));

	len = create_ad(hdev, cp.data);

	if (hdev->adv_data_len == len &&
	    memcmp(cp.data, hdev->adv_data, len) == 0)
		return;

	memcpy(hdev->adv_data, cp.data, sizeof(cp.data));
	hdev->adv_data_len = len;

	cp.length = len;

	hci_req_add(req, HCI_OP_LE_SET_ADV_DATA, sizeof(cp), &cp);
}

/* ---- HCI ioctl helpers ---- */

int hci_dev_open(__u16 dev)
{
	struct hci_dev *hdev;
	int ret = 0;

	hdev = hci_dev_get(dev);
	if (!hdev)
		return -ENODEV;

	BT_DBG("%s %p", hdev->name, hdev);

	hci_req_lock(hdev);

<<<<<<< HEAD
	if (hdev->rfkill && rfkill_blocked(hdev->rfkill)) {
=======
	if (test_bit(HCI_UNREGISTER, &hdev->dev_flags)) {
		ret = -ENODEV;
		goto done;
	}

	/* Check for rfkill but allow the HCI setup stage to proceed
	 * (which in itself doesn't cause any RF activity).
	 */
	if (test_bit(HCI_RFKILLED, &hdev->dev_flags) &&
	    !test_bit(HCI_SETUP, &hdev->dev_flags)) {
>>>>>>> common/android-3.10.y
		ret = -ERFKILL;
		goto done;
	}

	if (test_bit(HCI_UP, &hdev->flags)) {
		ret = -EALREADY;
		goto done;
	}

<<<<<<< HEAD
	if (test_bit(HCI_QUIRK_RAW_DEVICE, &hdev->quirks))
		set_bit(HCI_RAW, &hdev->flags);

=======
>>>>>>> common/android-3.10.y
	if (hdev->open(hdev)) {
		ret = -EIO;
		goto done;
	}

<<<<<<< HEAD
	if (!skb_queue_empty(&hdev->cmd_q)) {
		BT_ERR("command queue is not empty, purging");
		skb_queue_purge(&hdev->cmd_q);
	}
	if (!skb_queue_empty(&hdev->rx_q)) {
		BT_ERR("rx queue is not empty, purging");
		skb_queue_purge(&hdev->rx_q);
	}
	if (!skb_queue_empty(&hdev->raw_q)) {
		BT_ERR("raw queue is not empty, purging");
		skb_queue_purge(&hdev->raw_q);
	}

	if (!test_bit(HCI_RAW, &hdev->flags)) {
		atomic_set(&hdev->cmd_cnt, 1);
		set_bit(HCI_INIT, &hdev->flags);
		hdev->init_last_cmd = 0;
=======
	atomic_set(&hdev->cmd_cnt, 1);
	set_bit(HCI_INIT, &hdev->flags);
>>>>>>> common/android-3.10.y

	if (hdev->setup && test_bit(HCI_SETUP, &hdev->dev_flags))
		ret = hdev->setup(hdev);

<<<<<<< HEAD
		if (lmp_le_capable(hdev))
			ret = __hci_request(hdev, hci_le_init_req, 0,
					msecs_to_jiffies(HCI_INIT_TIMEOUT));
=======
	if (!ret) {
		/* Treat all non BR/EDR controllers as raw devices if
		 * enable_hs is not set.
		 */
		if (hdev->dev_type != HCI_BREDR && !enable_hs)
			set_bit(HCI_RAW, &hdev->flags);
>>>>>>> common/android-3.10.y

		if (test_bit(HCI_QUIRK_RAW_DEVICE, &hdev->quirks))
			set_bit(HCI_RAW, &hdev->flags);

		if (!test_bit(HCI_RAW, &hdev->flags))
			ret = __hci_init(hdev);
	}

	clear_bit(HCI_INIT, &hdev->flags);

	if (!ret) {
		hci_dev_hold(hdev);
		set_bit(HCI_UP, &hdev->flags);
		hci_notify(hdev, HCI_DEV_UP);
<<<<<<< HEAD
		if (!test_bit(HCI_SETUP, &hdev->flags) &&
				hdev->dev_type == HCI_BREDR) {
			hci_dev_lock_bh(hdev);
			mgmt_powered(hdev->id, 1);
			hci_dev_unlock_bh(hdev);
=======
		if (!test_bit(HCI_SETUP, &hdev->dev_flags) &&
		    mgmt_valid_hdev(hdev)) {
			hci_dev_lock(hdev);
			mgmt_powered(hdev, 1);
			hci_dev_unlock(hdev);
>>>>>>> common/android-3.10.y
		}
	} else {
		/* Init failed, cleanup */
		tasklet_kill(&hdev->rx_task);
		tasklet_kill(&hdev->tx_task);
		tasklet_kill(&hdev->cmd_task);

		skb_queue_purge(&hdev->cmd_q);
		skb_queue_purge(&hdev->rx_q);

		if (hdev->flush)
			hdev->flush(hdev);

		if (hdev->sent_cmd) {
			kfree_skb(hdev->sent_cmd);
			hdev->sent_cmd = NULL;
		}

		hdev->close(hdev);
		hdev->flags = 0;
	}

done:
	hci_req_unlock(hdev);
	hci_dev_put(hdev);
	return ret;
}

static int hci_dev_do_close(struct hci_dev *hdev, u8 is_process)
{
	unsigned long keepflags = 0;

	BT_DBG("%s %p", hdev->name, hdev);

	cancel_delayed_work(&hdev->power_off);

	hci_req_cancel(hdev, ENODEV);
	hci_req_lock(hdev);

	if (!test_and_clear_bit(HCI_UP, &hdev->flags)) {
		del_timer_sync(&hdev->cmd_timer);
		hci_req_unlock(hdev);
		return 0;
	}

	/* Kill RX and TX tasks */
	tasklet_kill(&hdev->rx_task);
	tasklet_kill(&hdev->tx_task);

	hci_dev_lock_bh(hdev);
	inquiry_cache_flush(hdev);
	hci_conn_hash_flush(hdev, is_process);
	hci_dev_unlock_bh(hdev);

	hci_notify(hdev, HCI_DEV_DOWN);

	if (hdev->dev_type == HCI_BREDR) {
		hci_dev_lock_bh(hdev);
		mgmt_powered(hdev->id, 0);
		hci_dev_unlock_bh(hdev);
	}

	if (hdev->flush)
		hdev->flush(hdev);

	/* Reset device */
	skb_queue_purge(&hdev->cmd_q);
	atomic_set(&hdev->cmd_cnt, 1);
<<<<<<< HEAD
	if (!test_bit(HCI_RAW, &hdev->flags)) {
=======
	if (!test_bit(HCI_RAW, &hdev->flags) &&
	    test_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks)) {
>>>>>>> common/android-3.10.y
		set_bit(HCI_INIT, &hdev->flags);
		__hci_req_sync(hdev, hci_reset_req, 0, HCI_CMD_TIMEOUT);
		clear_bit(HCI_INIT, &hdev->flags);
	}

	/* Kill cmd task */
	tasklet_kill(&hdev->cmd_task);

	/* Drop queues */
	skb_queue_purge(&hdev->rx_q);
	skb_queue_purge(&hdev->cmd_q);
	skb_queue_purge(&hdev->raw_q);

	/* Drop last sent command */
	if (hdev->sent_cmd) {
		del_timer_sync(&hdev->cmd_timer);
		kfree_skb(hdev->sent_cmd);
		hdev->sent_cmd = NULL;
	}

	kfree_skb(hdev->recv_evt);
	hdev->recv_evt = NULL;

	/* After this point our queues are empty
	 * and no tasks are scheduled. */
	hdev->close(hdev);

<<<<<<< HEAD
	/* Clear only non-persistent flags */
	if (test_bit(HCI_MGMT, &hdev->flags))
		set_bit(HCI_MGMT, &keepflags);
	if (test_bit(HCI_LINK_KEYS, &hdev->flags))
		set_bit(HCI_LINK_KEYS, &keepflags);
	if (test_bit(HCI_DEBUG_KEYS, &hdev->flags))
		set_bit(HCI_DEBUG_KEYS, &keepflags);
=======
	/* Clear flags */
	hdev->flags = 0;
	hdev->dev_flags &= ~HCI_PERSISTENT_MASK;

	if (!test_and_clear_bit(HCI_AUTO_OFF, &hdev->dev_flags) &&
	    mgmt_valid_hdev(hdev)) {
		hci_dev_lock(hdev);
		mgmt_powered(hdev, 0);
		hci_dev_unlock(hdev);
	}

	/* Controller radio is available but is currently powered down */
	hdev->amp_status = 0;
>>>>>>> common/android-3.10.y

	hdev->flags = keepflags;

	hci_req_unlock(hdev);

	hci_dev_put(hdev);
	return 0;
}

int hci_dev_close(__u16 dev)
{
	struct hci_dev *hdev;
	int err;

	hdev = hci_dev_get(dev);
	if (!hdev)
		return -ENODEV;
	err = hci_dev_do_close(hdev, 1);
	hci_dev_put(hdev);
	return err;
}

int hci_dev_reset(__u16 dev)
{
	struct hci_dev *hdev;
	int ret = 0;

	hdev = hci_dev_get(dev);
	if (!hdev)
		return -ENODEV;

	hci_req_lock(hdev);
	tasklet_disable(&hdev->tx_task);

	if (!test_bit(HCI_UP, &hdev->flags))
		goto done;

	/* Drop queues */
	skb_queue_purge(&hdev->rx_q);
	skb_queue_purge(&hdev->cmd_q);

	hci_dev_lock_bh(hdev);
	inquiry_cache_flush(hdev);
	hci_conn_hash_flush(hdev, 0);
	hci_dev_unlock_bh(hdev);

	if (hdev->flush)
		hdev->flush(hdev);

	atomic_set(&hdev->cmd_cnt, 1);
	hdev->acl_cnt = 0; hdev->sco_cnt = 0; hdev->le_cnt = 0;

	if (!test_bit(HCI_RAW, &hdev->flags))
		ret = __hci_req_sync(hdev, hci_reset_req, 0, HCI_INIT_TIMEOUT);

done:
	tasklet_enable(&hdev->tx_task);
	hci_req_unlock(hdev);
	hci_dev_put(hdev);
	return ret;
}

int hci_dev_reset_stat(__u16 dev)
{
	struct hci_dev *hdev;
	int ret = 0;

	hdev = hci_dev_get(dev);
	if (!hdev)
		return -ENODEV;

	memset(&hdev->stat, 0, sizeof(struct hci_dev_stats));

	hci_dev_put(hdev);

	return ret;
}

int hci_dev_cmd(unsigned int cmd, void __user *arg)
{
	struct hci_dev *hdev;
	struct hci_dev_req dr;
	int err = 0;

	if (copy_from_user(&dr, arg, sizeof(dr)))
		return -EFAULT;

	hdev = hci_dev_get(dr.dev_id);
	if (!hdev)
		return -ENODEV;

	switch (cmd) {
	case HCISETAUTH:
		err = hci_req_sync(hdev, hci_auth_req, dr.dev_opt,
				   HCI_INIT_TIMEOUT);
		break;

	case HCISETENCRYPT:
		if (!lmp_encrypt_capable(hdev)) {
			err = -EOPNOTSUPP;
			break;
		}

		if (!test_bit(HCI_AUTH, &hdev->flags)) {
			/* Auth must be enabled first */
			err = hci_req_sync(hdev, hci_auth_req, dr.dev_opt,
					   HCI_INIT_TIMEOUT);
			if (err)
				break;
		}

		err = hci_req_sync(hdev, hci_encrypt_req, dr.dev_opt,
				   HCI_INIT_TIMEOUT);
		break;

	case HCISETSCAN:
		err = hci_req_sync(hdev, hci_scan_req, dr.dev_opt,
				   HCI_INIT_TIMEOUT);
		break;

	case HCISETLINKPOL:
		err = hci_req_sync(hdev, hci_linkpol_req, dr.dev_opt,
				   HCI_INIT_TIMEOUT);
		break;

	case HCISETLINKMODE:
		hdev->link_mode = ((__u16) dr.dev_opt) &
					(HCI_LM_MASTER | HCI_LM_ACCEPT);
		break;

	case HCISETPTYPE:
		hdev->pkt_type = (__u16) dr.dev_opt;
		break;

	case HCISETACLMTU:
		hdev->acl_mtu  = *((__u16 *) &dr.dev_opt + 1);
		hdev->acl_pkts = *((__u16 *) &dr.dev_opt + 0);
		break;

	case HCISETSCOMTU:
		hdev->sco_mtu  = *((__u16 *) &dr.dev_opt + 1);
		hdev->sco_pkts = *((__u16 *) &dr.dev_opt + 0);
		break;

	default:
		err = -EINVAL;
		break;
	}

	hci_dev_put(hdev);
	return err;
}

int hci_get_dev_list(void __user *arg)
{
	struct hci_dev_list_req *dl;
	struct hci_dev_req *dr;
	struct list_head *p;
	int n = 0, size, err;
	__u16 dev_num;

	if (get_user(dev_num, (__u16 __user *) arg))
		return -EFAULT;

	if (!dev_num || dev_num > (PAGE_SIZE * 2) / sizeof(*dr))
		return -EINVAL;

	size = sizeof(*dl) + dev_num * sizeof(*dr);

	dl = kzalloc(size, GFP_KERNEL);
	if (!dl)
		return -ENOMEM;

	dr = dl->dev_req;

	read_lock_bh(&hci_dev_list_lock);
	list_for_each(p, &hci_dev_list) {
		struct hci_dev *hdev;

		hdev = list_entry(p, struct hci_dev, list);

		hci_del_off_timer(hdev);

		if (!test_bit(HCI_MGMT, &hdev->flags))
			set_bit(HCI_PAIRABLE, &hdev->flags);

		(dr + n)->dev_id  = hdev->id;
		(dr + n)->dev_opt = hdev->flags;

		if (++n >= dev_num)
			break;
	}
	read_unlock_bh(&hci_dev_list_lock);

	dl->dev_num = n;
	size = sizeof(*dl) + n * sizeof(*dr);

	err = copy_to_user(arg, dl, size);
	kfree(dl);

	return err ? -EFAULT : 0;
}

int hci_get_dev_info(void __user *arg)
{
	struct hci_dev *hdev;
	struct hci_dev_info di;
	int err = 0;

	if (copy_from_user(&di, arg, sizeof(di)))
		return -EFAULT;

	hdev = hci_dev_get(di.dev_id);
	if (!hdev)
		return -ENODEV;

	hci_del_off_timer(hdev);

	if (!test_bit(HCI_MGMT, &hdev->flags))
		set_bit(HCI_PAIRABLE, &hdev->flags);

	strcpy(di.name, hdev->name);
	di.bdaddr   = hdev->bdaddr;
	di.type     = (hdev->bus & 0x0f) | (hdev->dev_type << 4);
	di.flags    = hdev->flags;
	di.pkt_type = hdev->pkt_type;
	if (lmp_bredr_capable(hdev)) {
		di.acl_mtu  = hdev->acl_mtu;
		di.acl_pkts = hdev->acl_pkts;
		di.sco_mtu  = hdev->sco_mtu;
		di.sco_pkts = hdev->sco_pkts;
	} else {
		di.acl_mtu  = hdev->le_mtu;
		di.acl_pkts = hdev->le_pkts;
		di.sco_mtu  = 0;
		di.sco_pkts = 0;
	}
	di.link_policy = hdev->link_policy;
	di.link_mode   = hdev->link_mode;

	memcpy(&di.stat, &hdev->stat, sizeof(di.stat));
	memcpy(&di.features, &hdev->features, sizeof(di.features));

	if (copy_to_user(arg, &di, sizeof(di)))
		err = -EFAULT;

	hci_dev_put(hdev);

	return err;
}

/* ---- Interface to HCI drivers ---- */

static int hci_rfkill_set_block(void *data, bool blocked)
{
	struct hci_dev *hdev = data;

	BT_DBG("%p name %s blocked %d", hdev, hdev->name, blocked);

<<<<<<< HEAD
	if (!blocked)
		return 0;

	hci_dev_do_close(hdev, 0);
=======
	if (blocked) {
		set_bit(HCI_RFKILLED, &hdev->dev_flags);
		if (!test_bit(HCI_SETUP, &hdev->dev_flags))
			hci_dev_do_close(hdev);
	} else {
		clear_bit(HCI_RFKILLED, &hdev->dev_flags);
}
>>>>>>> common/android-3.10.y

	return 0;
}

static const struct rfkill_ops hci_rfkill_ops = {
	.set_block = hci_rfkill_set_block,
};

<<<<<<< HEAD
/* Alloc HCI device */
struct hci_dev *hci_alloc_dev(void)
{
	struct hci_dev *hdev;

	hdev = kzalloc(sizeof(struct hci_dev), GFP_KERNEL);
	if (!hdev)
		return NULL;

	skb_queue_head_init(&hdev->driver_init);

	return hdev;
}
EXPORT_SYMBOL(hci_alloc_dev);

/* Free HCI device */
void hci_free_dev(struct hci_dev *hdev)
{
	skb_queue_purge(&hdev->driver_init);

	/* will free via device release */
	put_device(&hdev->dev);
}
EXPORT_SYMBOL(hci_free_dev);

=======
>>>>>>> common/android-3.10.y
static void hci_power_on(struct work_struct *work)
{
	struct hci_dev *hdev = container_of(work, struct hci_dev, power_on);
	int err;

	BT_DBG("%s", hdev->name);

	err = hci_dev_open(hdev->id);
<<<<<<< HEAD
	if (err && err != -EALREADY)
=======
	if (err < 0) {
		mgmt_set_powered_failed(hdev, err);
>>>>>>> common/android-3.10.y
		return;
	}

<<<<<<< HEAD
	if (test_bit(HCI_AUTO_OFF, &hdev->flags) &&
				hdev->dev_type == HCI_BREDR)
		mod_timer(&hdev->off_timer,
				jiffies + msecs_to_jiffies(AUTO_OFF_TIMEOUT));
=======
	if (test_bit(HCI_RFKILLED, &hdev->dev_flags)) {
		clear_bit(HCI_AUTO_OFF, &hdev->dev_flags);
		hci_dev_do_close(hdev);
	} else if (test_bit(HCI_AUTO_OFF, &hdev->dev_flags)) {
		queue_delayed_work(hdev->req_workqueue, &hdev->power_off,
				   HCI_AUTO_OFF_TIMEOUT);
	}
>>>>>>> common/android-3.10.y

	if (test_and_clear_bit(HCI_SETUP, &hdev->flags) &&
				hdev->dev_type == HCI_BREDR)
		mgmt_index_added(hdev->id);
}

static void hci_power_off(struct work_struct *work)
{
<<<<<<< HEAD
	struct hci_dev *hdev = container_of(work, struct hci_dev, power_off);
=======
	struct hci_dev *hdev = container_of(work, struct hci_dev,
					    power_off.work);
>>>>>>> common/android-3.10.y

	BT_DBG("%s", hdev->name);

	hci_dev_close(hdev->id);
}

static void hci_auto_off(unsigned long data)
{
	struct hci_dev *hdev = (struct hci_dev *) data;

	BT_DBG("%s", hdev->name);

	clear_bit(HCI_AUTO_OFF, &hdev->flags);

	queue_work(hdev->workqueue, &hdev->power_off);
}

void hci_del_off_timer(struct hci_dev *hdev)
{
	BT_DBG("%s", hdev->name);

	clear_bit(HCI_AUTO_OFF, &hdev->flags);
	del_timer(&hdev->off_timer);
}

int hci_uuids_clear(struct hci_dev *hdev)
{
	struct bt_uuid *uuid, *tmp;

	list_for_each_entry_safe(uuid, tmp, &hdev->uuids, list) {
		list_del(&uuid->list);
		kfree(uuid);
	}

	return 0;
}

int hci_link_keys_clear(struct hci_dev *hdev)
{
	struct list_head *p, *n;

	list_for_each_safe(p, n, &hdev->link_keys) {
		struct link_key *key;

		key = list_entry(p, struct link_key, list);

		list_del(p);
		kfree(key);
	}

	return 0;
}

struct link_key *hci_find_link_key(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct list_head *p;

	list_for_each(p, &hdev->link_keys) {
		struct link_key *k;

		k = list_entry(p, struct link_key, list);

		if (bacmp(bdaddr, &k->bdaddr) == 0)
			return k;
	}

	return NULL;
}

<<<<<<< HEAD
struct link_key *hci_find_ltk(struct hci_dev *hdev, __le16 ediv, u8 rand[8])
=======
static bool hci_persistent_key(struct hci_dev *hdev, struct hci_conn *conn,
			       u8 key_type, u8 old_key_type)
>>>>>>> common/android-3.10.y
{
	struct list_head *p;

	list_for_each(p, &hdev->link_keys) {
		struct link_key *k;
		struct key_master_id *id;

		k = list_entry(p, struct link_key, list);

		if (k->key_type != KEY_TYPE_LTK)
			continue;

<<<<<<< HEAD
		if (k->dlen != sizeof(*id))
=======
	list_for_each_entry(k, &hdev->long_term_keys, list) {
		if (k->ediv != ediv ||
		    memcmp(rand, k->rand, sizeof(k->rand)))
>>>>>>> common/android-3.10.y
			continue;

		id = (void *) &k->data;
		if (id->ediv == ediv &&
				(memcmp(rand, id->rand, sizeof(id->rand)) == 0))
			return k;
	}

	return NULL;
}

struct link_key *hci_find_link_key_type(struct hci_dev *hdev,
					bdaddr_t *bdaddr, u8 type)
{
	struct list_head *p;

	list_for_each(p, &hdev->link_keys) {
		struct link_key *k;

<<<<<<< HEAD
		k = list_entry(p, struct link_key, list);

		if ((k->key_type == type) && (bacmp(bdaddr, &k->bdaddr) == 0))
=======
	list_for_each_entry(k, &hdev->long_term_keys, list)
		if (addr_type == k->bdaddr_type &&
		    bacmp(bdaddr, &k->bdaddr) == 0)
>>>>>>> common/android-3.10.y
			return k;
	}

	return NULL;
}
<<<<<<< HEAD
EXPORT_SYMBOL(hci_find_link_key_type);
=======
>>>>>>> common/android-3.10.y

int hci_add_link_key(struct hci_dev *hdev, int new_key, bdaddr_t *bdaddr,
						u8 *val, u8 type, u8 pin_len)
{
	struct link_key *key, *old_key;
	struct hci_conn *conn;
	u8 old_key_type;
	u8 bonded = 0;

	old_key = hci_find_link_key(hdev, bdaddr);
	if (old_key) {
		old_key_type = old_key->key_type;
		key = old_key;
	} else {
		old_key_type = 0xff;
		key = kzalloc(sizeof(*key), GFP_ATOMIC);
		if (!key)
			return -ENOMEM;
		list_add(&key->list, &hdev->link_keys);
	}

	BT_DBG("%s key for %pMR type %u", hdev->name, bdaddr, type);

<<<<<<< HEAD
	bacpy(&key->bdaddr, bdaddr);
	memcpy(key->val, val, 16);
	key->auth = 0x01;
	key->key_type = type;
=======
	/* Some buggy controller combinations generate a changed
	 * combination key for legacy pairing even when there's no
	 * previous key */
	if (type == HCI_LK_CHANGED_COMBINATION &&
	    (!conn || conn->remote_auth == 0xff) && old_key_type == 0xff) {
		type = HCI_LK_COMBINATION;
		if (conn)
			conn->key_type = type;
	}

	bacpy(&key->bdaddr, bdaddr);
	memcpy(key->val, val, HCI_LINK_KEY_SIZE);
>>>>>>> common/android-3.10.y
	key->pin_len = pin_len;

	conn = hci_conn_hash_lookup_ba(hdev, ACL_LINK, bdaddr);
	/* Store the link key persistently if one of the following is true:
	 * 1. the remote side is using dedicated bonding since in that case
	 *    also the local requirements are set to dedicated bonding
	 * 2. the local side had dedicated bonding as a requirement
	 * 3. this is a legacy link key
	 * 4. this is a changed combination key and there was a previously
	 *    stored one
	 * If none of the above match only keep the link key around for
	 * this connection and set the temporary flag for the device.
	*/

	if (conn) {
		if ((conn->remote_auth > 0x01) ||
			(conn->auth_initiator && conn->auth_type > 0x01) ||
			(key->key_type < 0x03) ||
			(key->key_type == 0x06 && old_key_type != 0xff))
			bonded = 1;
	}

	if (new_key)
		mgmt_new_key(hdev->id, key, bonded);

	if (type == 0x06)
		key->key_type = old_key_type;

	return 0;
}

<<<<<<< HEAD
int hci_add_ltk(struct hci_dev *hdev, int new_key, bdaddr_t *bdaddr,
			u8 addr_type, u8 key_size, u8 auth,
			__le16 ediv, u8 rand[8], u8 ltk[16])
=======
int hci_add_ltk(struct hci_dev *hdev, bdaddr_t *bdaddr, u8 addr_type, u8 type,
		int new_key, u8 authenticated, u8 tk[16], u8 enc_size, __le16
		ediv, u8 rand[8])
>>>>>>> common/android-3.10.y
{
	struct link_key *key, *old_key;
	struct key_master_id *id;

	BT_DBG("%s Auth: %2.2X addr %s type: %d", hdev->name, auth,
						batostr(bdaddr), addr_type);

	old_key = hci_find_link_key_type(hdev, bdaddr, KEY_TYPE_LTK);
	if (old_key) {
		key = old_key;
	} else {
		key = kzalloc(sizeof(*key) + sizeof(*id), GFP_ATOMIC);
		if (!key)
			return -ENOMEM;
		list_add(&key->list, &hdev->link_keys);
	}

	key->dlen = sizeof(*id);

	bacpy(&key->bdaddr, bdaddr);
	key->addr_type = addr_type;
	memcpy(key->val, ltk, sizeof(key->val));
	key->key_type = KEY_TYPE_LTK;
	key->pin_len = key_size;
	key->auth = auth;

	id = (void *) &key->data;
	id->ediv = ediv;
	memcpy(id->rand, rand, sizeof(id->rand));

	if (new_key)
		mgmt_new_key(hdev->id, key, auth & 0x01);

	return 0;
}

int hci_remove_link_key(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct link_key *key;

	key = hci_find_link_key(hdev, bdaddr);
	if (!key)
		return -ENOENT;

	BT_DBG("%s removing %pMR", hdev->name, bdaddr);

	list_del(&key->list);
	kfree(key);

	return 0;
}

<<<<<<< HEAD
=======
int hci_remove_ltk(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct smp_ltk *k, *tmp;

	list_for_each_entry_safe(k, tmp, &hdev->long_term_keys, list) {
		if (bacmp(bdaddr, &k->bdaddr))
			continue;

		BT_DBG("%s removing %pMR", hdev->name, bdaddr);

		list_del(&k->list);
		kfree(k);
	}

	return 0;
}

>>>>>>> common/android-3.10.y
/* HCI command timer function */
static void hci_cmd_timeout(unsigned long arg)
{
	struct hci_dev *hdev = (void *) arg;

	if (hdev->sent_cmd) {
		struct hci_command_hdr *sent = (void *) hdev->sent_cmd->data;
		u16 opcode = __le16_to_cpu(sent->opcode);

		BT_ERR("%s command 0x%4.4x tx timeout", hdev->name, opcode);
	} else {
		BT_ERR("%s command tx timeout", hdev->name);
	}

	atomic_set(&hdev->cmd_cnt, 1);
	clear_bit(HCI_RESET, &hdev->flags);
	tasklet_schedule(&hdev->cmd_task);
}

struct oob_data *hci_find_remote_oob_data(struct hci_dev *hdev,
							bdaddr_t *bdaddr)
{
	struct oob_data *data;

	list_for_each_entry(data, &hdev->remote_oob_data, list)
		if (bacmp(bdaddr, &data->bdaddr) == 0)
			return data;

	return NULL;
}

int hci_remove_remote_oob_data(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct oob_data *data;

	data = hci_find_remote_oob_data(hdev, bdaddr);
	if (!data)
		return -ENOENT;

	BT_DBG("%s removing %pMR", hdev->name, bdaddr);

	list_del(&data->list);
	kfree(data);

	return 0;
}

int hci_remote_oob_data_clear(struct hci_dev *hdev)
{
	struct oob_data *data, *n;

	list_for_each_entry_safe(data, n, &hdev->remote_oob_data, list) {
		list_del(&data->list);
		kfree(data);
	}

	return 0;
}

static void hci_adv_clear(unsigned long arg)
{
	struct hci_dev *hdev = (void *) arg;

	hci_adv_entries_clear(hdev);
}

int hci_adv_entries_clear(struct hci_dev *hdev)
{
	struct list_head *p, *n;

	BT_DBG("");
	write_lock_bh(&hdev->adv_entries_lock);

	list_for_each_safe(p, n, &hdev->adv_entries) {
		struct adv_entry *entry;

<<<<<<< HEAD
		entry = list_entry(p, struct adv_entry, list);
=======
	BT_DBG("%s for %pMR", hdev->name, bdaddr);

	return 0;
}

struct bdaddr_list *hci_blacklist_lookup(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct bdaddr_list *b;

	list_for_each_entry(b, &hdev->blacklist, list)
		if (bacmp(bdaddr, &b->bdaddr) == 0)
			return b;

	return NULL;
}

int hci_blacklist_clear(struct hci_dev *hdev)
{
	struct list_head *p, *n;

	list_for_each_safe(p, n, &hdev->blacklist) {
		struct bdaddr_list *b;

		b = list_entry(p, struct bdaddr_list, list);
>>>>>>> common/android-3.10.y

		list_del(p);
		kfree(entry);
	}

	write_unlock_bh(&hdev->adv_entries_lock);

	return 0;
}

struct adv_entry *hci_find_adv_entry(struct hci_dev *hdev, bdaddr_t *bdaddr)
{
	struct list_head *p;
	struct adv_entry *res = NULL;

	BT_DBG("");
	read_lock_bh(&hdev->adv_entries_lock);

	list_for_each(p, &hdev->adv_entries) {
		struct adv_entry *entry;

		entry = list_entry(p, struct adv_entry, list);

		if (bacmp(bdaddr, &entry->bdaddr) == 0) {
			res = entry;
			goto out;
		}
	}
out:
	read_unlock_bh(&hdev->adv_entries_lock);
	return res;
}

static inline int is_connectable_adv(u8 evt_type)
{
	if (evt_type == ADV_IND || evt_type == ADV_DIRECT_IND)
		return 1;

	return 0;
}

<<<<<<< HEAD
int hci_add_remote_oob_data(struct hci_dev *hdev, bdaddr_t *bdaddr, u8 *hash,
								u8 *randomizer)
{
	struct oob_data *data;

	data = hci_find_remote_oob_data(hdev, bdaddr);

	if (!data) {
		data = kmalloc(sizeof(*data), GFP_ATOMIC);
		if (!data)
			return -ENOMEM;

		bacpy(&data->bdaddr, bdaddr);
		list_add(&data->list, &hdev->remote_oob_data);
	}

	memcpy(data->hash, hash, sizeof(data->hash));
	memcpy(data->randomizer, randomizer, sizeof(data->randomizer));

	BT_DBG("%s for %s", hdev->name, batostr(bdaddr));
=======
static void le_scan_param_req(struct hci_request *req, unsigned long opt)
{
	struct le_scan_params *param =  (struct le_scan_params *) opt;
	struct hci_cp_le_set_scan_param cp;

	memset(&cp, 0, sizeof(cp));
	cp.type = param->type;
	cp.interval = cpu_to_le16(param->interval);
	cp.window = cpu_to_le16(param->window);

	hci_req_add(req, HCI_OP_LE_SET_SCAN_PARAM, sizeof(cp), &cp);
}

static void le_scan_enable_req(struct hci_request *req, unsigned long opt)
{
	struct hci_cp_le_set_scan_enable cp;

	memset(&cp, 0, sizeof(cp));
	cp.enable = LE_SCAN_ENABLE;
	cp.filter_dup = LE_SCAN_FILTER_DUP_ENABLE;

	hci_req_add(req, HCI_OP_LE_SET_SCAN_ENABLE, sizeof(cp), &cp);
}

static int hci_do_le_scan(struct hci_dev *hdev, u8 type, u16 interval,
			  u16 window, int timeout)
{
	long timeo = msecs_to_jiffies(3000);
	struct le_scan_params param;
	int err;

	BT_DBG("%s", hdev->name);

	if (test_bit(HCI_LE_SCAN, &hdev->dev_flags))
		return -EINPROGRESS;

	param.type = type;
	param.interval = interval;
	param.window = window;

	hci_req_lock(hdev);

	err = __hci_req_sync(hdev, le_scan_param_req, (unsigned long) &param,
			     timeo);
	if (!err)
		err = __hci_req_sync(hdev, le_scan_enable_req, 0, timeo);

	hci_req_unlock(hdev);

	if (err < 0)
		return err;

	queue_delayed_work(hdev->workqueue, &hdev->le_scan_disable,
			   timeout);
>>>>>>> common/android-3.10.y

	return 0;
}

<<<<<<< HEAD
int hci_add_adv_entry(struct hci_dev *hdev,
					struct hci_ev_le_advertising_info *ev)
{
	struct adv_entry *entry;
	u8 flags = 0;
	int i;

	BT_DBG("");

	if (!is_connectable_adv(ev->evt_type))
		return -EINVAL;

	if (ev->data && ev->length) {
		for (i = 0; (i + 2) < ev->length; i++)
			if (ev->data[i+1] == 0x01) {
				flags = ev->data[i+2];
				BT_DBG("flags: %2.2x", flags);
				break;
			} else {
				i += ev->data[i];
			}
	}

	entry = hci_find_adv_entry(hdev, &ev->bdaddr);
	/* Only new entries should be added to adv_entries. So, if
	 * bdaddr was found, don't add it. */
	if (entry) {
		entry->flags = flags;
		return 0;
	}

	entry = kzalloc(sizeof(*entry), GFP_ATOMIC);
	if (!entry)
		return -ENOMEM;

	bacpy(&entry->bdaddr, &ev->bdaddr);
	entry->bdaddr_type = ev->bdaddr_type;
	entry->flags = flags;

	write_lock(&hdev->adv_entries_lock);
	list_add(&entry->list, &hdev->adv_entries);
	write_unlock(&hdev->adv_entries_lock);
=======
int hci_cancel_le_scan(struct hci_dev *hdev)
{
	BT_DBG("%s", hdev->name);

	if (!test_bit(HCI_LE_SCAN, &hdev->dev_flags))
		return -EALREADY;

	if (cancel_delayed_work(&hdev->le_scan_disable)) {
		struct hci_cp_le_set_scan_enable cp;

		/* Send HCI command to disable LE Scan */
		memset(&cp, 0, sizeof(cp));
		hci_send_cmd(hdev, HCI_OP_LE_SET_SCAN_ENABLE, sizeof(cp), &cp);
	}
>>>>>>> common/android-3.10.y

	return 0;
}

static struct crypto_blkcipher *alloc_cypher(void)
{
<<<<<<< HEAD
	if (enable_smp)
		return crypto_alloc_blkcipher("ecb(aes)", 0, CRYPTO_ALG_ASYNC);
=======
	struct hci_dev *hdev = container_of(work, struct hci_dev, le_scan);
	struct le_scan_params *param = &hdev->le_scan_params;

	BT_DBG("%s", hdev->name);

	hci_do_le_scan(hdev, param->type, param->interval, param->window,
		       param->timeout);
}

int hci_le_scan(struct hci_dev *hdev, u8 type, u16 interval, u16 window,
		int timeout)
{
	struct le_scan_params *param = &hdev->le_scan_params;

	BT_DBG("%s", hdev->name);

	if (test_bit(HCI_LE_PERIPHERAL, &hdev->dev_flags))
		return -ENOTSUPP;

	if (work_busy(&hdev->le_scan))
		return -EINPROGRESS;

	param->type = type;
	param->interval = interval;
	param->window = window;
	param->timeout = timeout;

	queue_work(system_long_wq, &hdev->le_scan);
>>>>>>> common/android-3.10.y

	return ERR_PTR(-ENOTSUPP);
}

/* Alloc HCI device */
struct hci_dev *hci_alloc_dev(void)
{
<<<<<<< HEAD
	struct list_head *head = &hci_dev_list, *p;
	int i, id;

	BT_DBG("%p name %s bus %d owner %p", hdev, hdev->name,
						hdev->bus, hdev->owner);

	if (!hdev->open || !hdev->close || !hdev->destruct)
		return -EINVAL;

	id = (hdev->dev_type == HCI_BREDR) ? 0 : 1;

	write_lock_bh(&hci_dev_list_lock);

	/* Find first available device id */
	list_for_each(p, &hci_dev_list) {
		if (list_entry(p, struct hci_dev, list)->id != id)
			break;
		head = p; id++;
	}

	snprintf(hdev->name, sizeof(hdev->name), "hci%d", id);
	hdev->id = id;
	list_add(&hdev->list, head);

	atomic_set(&hdev->refcnt, 1);
	spin_lock_init(&hdev->lock);

	hdev->flags = 0;
=======
	struct hci_dev *hdev;

	hdev = kzalloc(sizeof(struct hci_dev), GFP_KERNEL);
	if (!hdev)
		return NULL;

>>>>>>> common/android-3.10.y
	hdev->pkt_type  = (HCI_DM1 | HCI_DH1 | HCI_HV1);
	hdev->esco_type = (ESCO_HV1);
	hdev->link_mode = (HCI_LM_ACCEPT);
	hdev->io_capability = 0x03; /* No Input No Output */
	hdev->inq_tx_power = HCI_TX_POWER_INVALID;
	hdev->adv_tx_power = HCI_TX_POWER_INVALID;

	hdev->sniff_max_interval = 800;
	hdev->sniff_min_interval = 80;

<<<<<<< HEAD
	tasklet_init(&hdev->cmd_task, hci_cmd_task, (unsigned long) hdev);
	tasklet_init(&hdev->rx_task, hci_rx_task, (unsigned long) hdev);
	tasklet_init(&hdev->tx_task, hci_tx_task, (unsigned long) hdev);
=======
	mutex_init(&hdev->lock);
	mutex_init(&hdev->req_lock);

	INIT_LIST_HEAD(&hdev->mgmt_pending);
	INIT_LIST_HEAD(&hdev->blacklist);
	INIT_LIST_HEAD(&hdev->uuids);
	INIT_LIST_HEAD(&hdev->link_keys);
	INIT_LIST_HEAD(&hdev->long_term_keys);
	INIT_LIST_HEAD(&hdev->remote_oob_data);
	INIT_LIST_HEAD(&hdev->conn_hash.list);

	INIT_WORK(&hdev->rx_work, hci_rx_work);
	INIT_WORK(&hdev->cmd_work, hci_cmd_work);
	INIT_WORK(&hdev->tx_work, hci_tx_work);
	INIT_WORK(&hdev->power_on, hci_power_on);
	INIT_WORK(&hdev->le_scan, le_scan_work);

	INIT_DELAYED_WORK(&hdev->power_off, hci_power_off);
	INIT_DELAYED_WORK(&hdev->discov_off, hci_discov_off);
	INIT_DELAYED_WORK(&hdev->le_scan_disable, le_scan_disable_work);
>>>>>>> common/android-3.10.y

	skb_queue_head_init(&hdev->rx_q);
	skb_queue_head_init(&hdev->cmd_q);
	skb_queue_head_init(&hdev->raw_q);

<<<<<<< HEAD
	setup_timer(&hdev->cmd_timer, hci_cmd_timer, (unsigned long) hdev);
	setup_timer(&hdev->disco_timer, mgmt_disco_timeout,
						(unsigned long) hdev);
	setup_timer(&hdev->disco_le_timer, mgmt_disco_le_timeout,
						(unsigned long) hdev);

	for (i = 0; i < NUM_REASSEMBLY; i++)
		hdev->reassembly[i] = NULL;

=======
>>>>>>> common/android-3.10.y
	init_waitqueue_head(&hdev->req_wait_q);

<<<<<<< HEAD
	inquiry_cache_init(hdev);

	hci_conn_hash_init(hdev);
	hci_chan_list_init(hdev);

	INIT_LIST_HEAD(&hdev->blacklist);

	INIT_LIST_HEAD(&hdev->uuids);

	INIT_LIST_HEAD(&hdev->link_keys);
=======
	setup_timer(&hdev->cmd_timer, hci_cmd_timeout, (unsigned long) hdev);

	hci_init_sysfs(hdev);
	discovery_init(hdev);
>>>>>>> common/android-3.10.y

	return hdev;
}
EXPORT_SYMBOL(hci_alloc_dev);

<<<<<<< HEAD
	INIT_LIST_HEAD(&hdev->adv_entries);
	rwlock_init(&hdev->adv_entries_lock);
	setup_timer(&hdev->adv_timer, hci_adv_clear, (unsigned long) hdev);

	INIT_WORK(&hdev->power_on, hci_power_on);
	INIT_WORK(&hdev->power_off, hci_power_off);
	setup_timer(&hdev->off_timer, hci_auto_off, (unsigned long) hdev);
=======
/* Free HCI device */
void hci_free_dev(struct hci_dev *hdev)
{
	/* will free via device release */
	put_device(&hdev->dev);
}
EXPORT_SYMBOL(hci_free_dev);

/* Register HCI device */
int hci_register_dev(struct hci_dev *hdev)
{
	int id, error;

	if (!hdev->open || !hdev->close)
		return -EINVAL;
>>>>>>> common/android-3.10.y

	/* Do not allow HCI_AMP devices to register at index 0,
	 * so the index can be used as the AMP controller ID.
	 */
	switch (hdev->dev_type) {
	case HCI_BREDR:
		id = ida_simple_get(&hci_index_ida, 0, 0, GFP_KERNEL);
		break;
	case HCI_AMP:
		id = ida_simple_get(&hci_index_ida, 1, 0, GFP_KERNEL);
		break;
	default:
		return -EINVAL;
	}

	if (id < 0)
		return id;

<<<<<<< HEAD
	write_unlock_bh(&hci_dev_list_lock);

	hdev->workqueue = create_singlethread_workqueue(hdev->name);
	if (!hdev->workqueue)
		goto nomem;

	hdev->tfm = alloc_cypher();
	if (IS_ERR(hdev->tfm))
		BT_INFO("Failed to load transform for ecb(aes): %ld",
							PTR_ERR(hdev->tfm));

	hci_register_sysfs(hdev);
=======
	sprintf(hdev->name, "hci%d", id);
	hdev->id = id;

	BT_DBG("%p name %s bus %d", hdev, hdev->name, hdev->bus);

	write_lock(&hci_dev_list_lock);
	list_add(&hdev->list, &hci_dev_list);
	write_unlock(&hci_dev_list_lock);

	hdev->workqueue = alloc_workqueue(hdev->name, WQ_HIGHPRI | WQ_UNBOUND |
					  WQ_MEM_RECLAIM, 1);
	if (!hdev->workqueue) {
		error = -ENOMEM;
		goto err;
	}

	hdev->req_workqueue = alloc_workqueue(hdev->name,
					      WQ_HIGHPRI | WQ_UNBOUND |
					      WQ_MEM_RECLAIM, 1);
	if (!hdev->req_workqueue) {
		destroy_workqueue(hdev->workqueue);
		error = -ENOMEM;
		goto err;
	}

	error = hci_add_sysfs(hdev);
	if (error < 0)
		goto err_wqueue;
>>>>>>> common/android-3.10.y

	hdev->rfkill = rfkill_alloc(hdev->name, &hdev->dev,
				    RFKILL_TYPE_BLUETOOTH, &hci_rfkill_ops,
				    hdev);
	if (hdev->rfkill) {
		if (rfkill_register(hdev->rfkill) < 0) {
			rfkill_destroy(hdev->rfkill);
			hdev->rfkill = NULL;
		}
	}

<<<<<<< HEAD
	set_bit(HCI_AUTO_OFF, &hdev->flags);
	set_bit(HCI_SETUP, &hdev->flags);
	queue_work(hdev->workqueue, &hdev->power_on);
=======
	if (hdev->rfkill && rfkill_blocked(hdev->rfkill))
		set_bit(HCI_RFKILLED, &hdev->dev_flags);

	set_bit(HCI_SETUP, &hdev->dev_flags);

	if (hdev->dev_type != HCI_AMP)
		set_bit(HCI_AUTO_OFF, &hdev->dev_flags);
>>>>>>> common/android-3.10.y

	hci_notify(hdev, HCI_DEV_REG);

	queue_work(hdev->req_workqueue, &hdev->power_on);

	return id;

<<<<<<< HEAD
nomem:
	write_lock_bh(&hci_dev_list_lock);
=======
err_wqueue:
	destroy_workqueue(hdev->workqueue);
	destroy_workqueue(hdev->req_workqueue);
err:
	ida_simple_remove(&hci_index_ida, hdev->id);
	write_lock(&hci_dev_list_lock);
>>>>>>> common/android-3.10.y
	list_del(&hdev->list);
	write_unlock_bh(&hci_dev_list_lock);

	return -ENOMEM;
}
EXPORT_SYMBOL(hci_register_dev);

/* Unregister HCI device */
int hci_unregister_dev(struct hci_dev *hdev)
{
	int i, id;

	BT_DBG("%p name %s bus %d", hdev, hdev->name, hdev->bus);

<<<<<<< HEAD
	write_lock_bh(&hci_dev_list_lock);
=======
	set_bit(HCI_UNREGISTER, &hdev->dev_flags);

	id = hdev->id;

	write_lock(&hci_dev_list_lock);
>>>>>>> common/android-3.10.y
	list_del(&hdev->list);
	write_unlock_bh(&hci_dev_list_lock);

	hci_dev_do_close(hdev, hdev->bus == HCI_SMD);

	for (i = 0; i < NUM_REASSEMBLY; i++)
		kfree_skb(hdev->reassembly[i]);

	cancel_work_sync(&hdev->power_on);

	if (!test_bit(HCI_INIT, &hdev->flags) &&
<<<<<<< HEAD
				!test_bit(HCI_SETUP, &hdev->flags) &&
				hdev->dev_type == HCI_BREDR) {
		hci_dev_lock_bh(hdev);
		mgmt_index_removed(hdev->id);
		hci_dev_unlock_bh(hdev);
=======
	    !test_bit(HCI_SETUP, &hdev->dev_flags)) {
		hci_dev_lock(hdev);
		mgmt_index_removed(hdev);
		hci_dev_unlock(hdev);
>>>>>>> common/android-3.10.y
	}

	if (!IS_ERR(hdev->tfm))
		crypto_free_blkcipher(hdev->tfm);

	hci_notify(hdev, HCI_DEV_UNREG);

	if (hdev->rfkill) {
		rfkill_unregister(hdev->rfkill);
		rfkill_destroy(hdev->rfkill);
	}

	hci_unregister_sysfs(hdev);

<<<<<<< HEAD
	/* Disable all timers */
	hci_del_off_timer(hdev);
	del_timer(&hdev->adv_timer);
	del_timer(&hdev->cmd_timer);
	del_timer(&hdev->disco_timer);
	del_timer(&hdev->disco_le_timer);

=======
>>>>>>> common/android-3.10.y
	destroy_workqueue(hdev->workqueue);
	destroy_workqueue(hdev->req_workqueue);

	hci_dev_lock_bh(hdev);
	hci_blacklist_clear(hdev);
	hci_uuids_clear(hdev);
	hci_link_keys_clear(hdev);
	hci_remote_oob_data_clear(hdev);
<<<<<<< HEAD
	hci_adv_entries_clear(hdev);
	hci_dev_unlock_bh(hdev);

	__hci_dev_put(hdev);

	return 0;
=======
	hci_dev_unlock(hdev);

	hci_dev_put(hdev);

	ida_simple_remove(&hci_index_ida, id);
>>>>>>> common/android-3.10.y
}
EXPORT_SYMBOL(hci_unregister_dev);

/* Suspend HCI device */
int hci_suspend_dev(struct hci_dev *hdev)
{
	hci_notify(hdev, HCI_DEV_SUSPEND);
	return 0;
}
EXPORT_SYMBOL(hci_suspend_dev);

/* Resume HCI device */
int hci_resume_dev(struct hci_dev *hdev)
{
	hci_notify(hdev, HCI_DEV_RESUME);
	return 0;
}
EXPORT_SYMBOL(hci_resume_dev);

/* Receive frame from HCI drivers */
int hci_recv_frame(struct sk_buff *skb)
{
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;
	if (!hdev || (!test_bit(HCI_UP, &hdev->flags)
		      && !test_bit(HCI_INIT, &hdev->flags))) {
		kfree_skb(skb);
		return -ENXIO;
	}

	/* Incoming skb */
	bt_cb(skb)->incoming = 1;

	/* Time stamp */
	__net_timestamp(skb);

	/* Queue frame for rx task */
	skb_queue_tail(&hdev->rx_q, skb);
	tasklet_schedule(&hdev->rx_task);

	return 0;
}
EXPORT_SYMBOL(hci_recv_frame);

static int hci_reassembly(struct hci_dev *hdev, int type, void *data,
			  int count, __u8 index)
{
	int len = 0;
	int hlen = 0;
	int remain = count;
	struct sk_buff *skb;
	struct bt_skb_cb *scb;

	if ((type < HCI_ACLDATA_PKT || type > HCI_EVENT_PKT) ||
	    index >= NUM_REASSEMBLY)
		return -EILSEQ;

	skb = hdev->reassembly[index];

	if (!skb) {
		switch (type) {
		case HCI_ACLDATA_PKT:
			len = HCI_MAX_FRAME_SIZE;
			hlen = HCI_ACL_HDR_SIZE;
			break;
		case HCI_EVENT_PKT:
			len = HCI_MAX_EVENT_SIZE;
			hlen = HCI_EVENT_HDR_SIZE;
			break;
		case HCI_SCODATA_PKT:
			len = HCI_MAX_SCO_SIZE;
			hlen = HCI_SCO_HDR_SIZE;
			break;
		}

		skb = bt_skb_alloc(len, GFP_ATOMIC);
		if (!skb)
			return -ENOMEM;

		scb = (void *) skb->cb;
		scb->expect = hlen;
		scb->pkt_type = type;

		skb->dev = (void *) hdev;
		hdev->reassembly[index] = skb;
	}

	while (count) {
		scb = (void *) skb->cb;
		len = min(scb->expect, (__u16)count);

		memcpy(skb_put(skb, len), data, len);

		count -= len;
		data += len;
		scb->expect -= len;
		remain = count;

		switch (type) {
		case HCI_EVENT_PKT:
			if (skb->len == HCI_EVENT_HDR_SIZE) {
				struct hci_event_hdr *h = hci_event_hdr(skb);
				scb->expect = h->plen;

				if (skb_tailroom(skb) < scb->expect) {
					kfree_skb(skb);
					hdev->reassembly[index] = NULL;
					return -ENOMEM;
				}
			}
			break;

		case HCI_ACLDATA_PKT:
			if (skb->len  == HCI_ACL_HDR_SIZE) {
				struct hci_acl_hdr *h = hci_acl_hdr(skb);
				scb->expect = __le16_to_cpu(h->dlen);

				if (skb_tailroom(skb) < scb->expect) {
					kfree_skb(skb);
					hdev->reassembly[index] = NULL;
					return -ENOMEM;
				}
			}
			break;

		case HCI_SCODATA_PKT:
			if (skb->len == HCI_SCO_HDR_SIZE) {
				struct hci_sco_hdr *h = hci_sco_hdr(skb);
				scb->expect = h->dlen;

				if (skb_tailroom(skb) < scb->expect) {
					kfree_skb(skb);
					hdev->reassembly[index] = NULL;
					return -ENOMEM;
				}
			}
			break;
		}

		if (scb->expect == 0) {
			/* Complete frame */

			bt_cb(skb)->pkt_type = type;
			hci_recv_frame(skb);

			hdev->reassembly[index] = NULL;
			return remain;
		}
	}

	return remain;
}

int hci_recv_fragment(struct hci_dev *hdev, int type, void *data, int count)
{
	int rem = 0;

	if (type < HCI_ACLDATA_PKT || type > HCI_EVENT_PKT)
		return -EILSEQ;

	while (count) {
		rem = hci_reassembly(hdev, type, data, count, type - 1);
		if (rem < 0)
			return rem;

		data += (count - rem);
		count = rem;
	};

	return rem;
}
EXPORT_SYMBOL(hci_recv_fragment);

#define STREAM_REASSEMBLY 0

int hci_recv_stream_fragment(struct hci_dev *hdev, void *data, int count)
{
	int type;
	int rem = 0;

	while (count) {
		struct sk_buff *skb = hdev->reassembly[STREAM_REASSEMBLY];

		if (!skb) {
			struct { char type; } *pkt;

			/* Start of the frame */
			pkt = data;
			type = pkt->type;

			data++;
			count--;
		} else
			type = bt_cb(skb)->pkt_type;

		rem = hci_reassembly(hdev, type, data, count,
				     STREAM_REASSEMBLY);
		if (rem < 0)
			return rem;

		data += (count - rem);
		count = rem;
	};

	return rem;
}
EXPORT_SYMBOL(hci_recv_stream_fragment);

/* ---- Interface to upper protocols ---- */

/* Register/Unregister protocols.
 * hci_task_lock is used to ensure that no tasks are running. */
int hci_register_proto(struct hci_proto *hp)
{
	int err = 0;

	BT_DBG("%p name %s id %d", hp, hp->name, hp->id);

	if (hp->id >= HCI_MAX_PROTO)
		return -EINVAL;

	write_lock_bh(&hci_task_lock);

	if (!hci_proto[hp->id])
		hci_proto[hp->id] = hp;
	else
		err = -EEXIST;

	write_unlock_bh(&hci_task_lock);

	return err;
}
EXPORT_SYMBOL(hci_register_proto);

int hci_unregister_proto(struct hci_proto *hp)
{
	int err = 0;

	BT_DBG("%p name %s id %d", hp, hp->name, hp->id);

	if (hp->id >= HCI_MAX_PROTO)
		return -EINVAL;

	write_lock_bh(&hci_task_lock);

	if (hci_proto[hp->id])
		hci_proto[hp->id] = NULL;
	else
		err = -ENOENT;

	write_unlock_bh(&hci_task_lock);

	return err;
}
EXPORT_SYMBOL(hci_unregister_proto);

int hci_register_cb(struct hci_cb *cb)
{
	BT_DBG("%p name %s", cb, cb->name);

	write_lock_bh(&hci_cb_list_lock);
	list_add(&cb->list, &hci_cb_list);
	write_unlock_bh(&hci_cb_list_lock);

	return 0;
}
EXPORT_SYMBOL(hci_register_cb);

int hci_unregister_cb(struct hci_cb *cb)
{
	BT_DBG("%p name %s", cb, cb->name);

	write_lock_bh(&hci_cb_list_lock);
	list_del(&cb->list);
	write_unlock_bh(&hci_cb_list_lock);

	return 0;
}
EXPORT_SYMBOL(hci_unregister_cb);

int hci_register_amp(struct amp_mgr_cb *cb)
{
	BT_DBG("%p", cb);

	write_lock_bh(&amp_mgr_cb_list_lock);
	list_add(&cb->list, &amp_mgr_cb_list);
	write_unlock_bh(&amp_mgr_cb_list_lock);

	return 0;
}
EXPORT_SYMBOL(hci_register_amp);

int hci_unregister_amp(struct amp_mgr_cb *cb)
{
	BT_DBG("%p", cb);

	write_lock_bh(&amp_mgr_cb_list_lock);
	list_del(&cb->list);
	write_unlock_bh(&amp_mgr_cb_list_lock);

	return 0;
}
EXPORT_SYMBOL(hci_unregister_amp);

void hci_amp_cmd_complete(struct hci_dev *hdev, __u16 opcode,
			struct sk_buff *skb)
{
	struct amp_mgr_cb *cb;

	BT_DBG("opcode 0x%x", opcode);

	read_lock_bh(&amp_mgr_cb_list_lock);
	list_for_each_entry(cb, &amp_mgr_cb_list, list) {
		if (cb->amp_cmd_complete_event)
			cb->amp_cmd_complete_event(hdev, opcode, skb);
	}
	read_unlock_bh(&amp_mgr_cb_list_lock);
}

void hci_amp_cmd_status(struct hci_dev *hdev, __u16 opcode, __u8 status)
{
	struct amp_mgr_cb *cb;

	BT_DBG("opcode 0x%x, status %d", opcode, status);

	read_lock_bh(&amp_mgr_cb_list_lock);
	list_for_each_entry(cb, &amp_mgr_cb_list, list) {
		if (cb->amp_cmd_status_event)
			cb->amp_cmd_status_event(hdev, opcode, status);
	}
	read_unlock_bh(&amp_mgr_cb_list_lock);
}

void hci_amp_event_packet(struct hci_dev *hdev, __u8 ev_code,
			struct sk_buff *skb)
{
	struct amp_mgr_cb *cb;

	BT_DBG("ev_code 0x%x", ev_code);

	read_lock_bh(&amp_mgr_cb_list_lock);
	list_for_each_entry(cb, &amp_mgr_cb_list, list) {
		if (cb->amp_event)
			cb->amp_event(hdev, ev_code, skb);
	}
	read_unlock_bh(&amp_mgr_cb_list_lock);
}

static int hci_send_frame(struct sk_buff *skb)
{
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;

	if (!hdev) {
		kfree_skb(skb);
		return -ENODEV;
	}

	BT_DBG("%s type %d len %d", hdev->name, bt_cb(skb)->pkt_type, skb->len);

	if (atomic_read(&hdev->promisc)) {
		/* Time stamp */
		__net_timestamp(skb);

		hci_send_to_sock(hdev, skb, NULL);
	}

	/* Get rid of skb owner, prior to sending to the driver. */
	skb_orphan(skb);

	hci_notify(hdev, HCI_DEV_WRITE);
	return hdev->send(skb);
}

void hci_req_init(struct hci_request *req, struct hci_dev *hdev)
{
	skb_queue_head_init(&req->cmd_q);
	req->hdev = hdev;
	req->err = 0;
}

int hci_req_run(struct hci_request *req, hci_req_complete_t complete)
{
	struct hci_dev *hdev = req->hdev;
	struct sk_buff *skb;
	unsigned long flags;

	BT_DBG("length %u", skb_queue_len(&req->cmd_q));

	/* If an error occured during request building, remove all HCI
	 * commands queued on the HCI request queue.
	 */
	if (req->err) {
		skb_queue_purge(&req->cmd_q);
		return req->err;
	}

	/* Do not allow empty requests */
	if (skb_queue_empty(&req->cmd_q))
		return -ENODATA;

	skb = skb_peek_tail(&req->cmd_q);
	bt_cb(skb)->req.complete = complete;

	spin_lock_irqsave(&hdev->cmd_q.lock, flags);
	skb_queue_splice_tail(&req->cmd_q, &hdev->cmd_q);
	spin_unlock_irqrestore(&hdev->cmd_q.lock, flags);

	queue_work(hdev->workqueue, &hdev->cmd_work);

	return 0;
}

static struct sk_buff *hci_prepare_cmd(struct hci_dev *hdev, u16 opcode,
				       u32 plen, const void *param)
{
	int len = HCI_COMMAND_HDR_SIZE + plen;
	struct hci_command_hdr *hdr;
	struct sk_buff *skb;

	skb = bt_skb_alloc(len, GFP_ATOMIC);
	if (!skb)
		return NULL;

	hdr = (struct hci_command_hdr *) skb_put(skb, HCI_COMMAND_HDR_SIZE);
	hdr->opcode = cpu_to_le16(opcode);
	hdr->plen   = plen;

	if (plen)
		memcpy(skb_put(skb, plen), param, plen);

	BT_DBG("skb len %d", skb->len);

	bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
	skb->dev = (void *) hdev;

	return skb;
}

/* Send HCI command */
int hci_send_cmd(struct hci_dev *hdev, __u16 opcode, __u32 plen,
		 const void *param)
{
	struct sk_buff *skb;

	BT_DBG("%s opcode 0x%4.4x plen %d", hdev->name, opcode, plen);

	skb = hci_prepare_cmd(hdev, opcode, plen, param);
	if (!skb) {
		BT_ERR("%s no memory for command", hdev->name);
		return -ENOMEM;
	}

	/* Stand-alone HCI commands must be flaged as
	 * single-command requests.
	 */
	bt_cb(skb)->req.start = true;

	skb_queue_tail(&hdev->cmd_q, skb);
	tasklet_schedule(&hdev->cmd_task);

	return 0;
}
EXPORT_SYMBOL(hci_send_cmd);

/* Queue a command to an asynchronous HCI request */
void hci_req_add_ev(struct hci_request *req, u16 opcode, u32 plen,
		    const void *param, u8 event)
{
	struct hci_dev *hdev = req->hdev;
	struct sk_buff *skb;

	BT_DBG("%s opcode 0x%4.4x plen %d", hdev->name, opcode, plen);

	/* If an error occured during request building, there is no point in
	 * queueing the HCI command. We can simply return.
	 */
	if (req->err)
		return;

	skb = hci_prepare_cmd(hdev, opcode, plen, param);
	if (!skb) {
		BT_ERR("%s no memory for command (opcode 0x%4.4x)",
		       hdev->name, opcode);
		req->err = -ENOMEM;
		return;
	}

	if (skb_queue_empty(&req->cmd_q))
		bt_cb(skb)->req.start = true;

	bt_cb(skb)->req.event = event;

	skb_queue_tail(&req->cmd_q, skb);
}

void hci_req_add(struct hci_request *req, u16 opcode, u32 plen,
		 const void *param)
{
	hci_req_add_ev(req, opcode, plen, param, 0);
}

/* Get data from the previously sent command */
void *hci_sent_cmd_data(struct hci_dev *hdev, __u16 opcode)
{
	struct hci_command_hdr *hdr;

	if (!hdev->sent_cmd)
		return NULL;

	hdr = (void *) hdev->sent_cmd->data;

	if (hdr->opcode != cpu_to_le16(opcode))
		return NULL;

	BT_DBG("%s opcode 0x%4.4x", hdev->name, opcode);

	return hdev->sent_cmd->data + HCI_COMMAND_HDR_SIZE;
}

/* Send ACL data */
static void hci_add_acl_hdr(struct sk_buff *skb, __u16 handle, __u16 flags)
{
	struct hci_acl_hdr *hdr;
	int len = skb->len;

	skb_push(skb, HCI_ACL_HDR_SIZE);
	skb_reset_transport_header(skb);
	hdr = (struct hci_acl_hdr *)skb_transport_header(skb);
	hdr->handle = cpu_to_le16(hci_handle_pack(handle, flags));
	hdr->dlen   = cpu_to_le16(len);
}

<<<<<<< HEAD
void hci_send_acl(struct hci_conn *conn, struct hci_chan *chan,
		struct sk_buff *skb, __u16 flags)
=======
static void hci_queue_acl(struct hci_chan *chan, struct sk_buff_head *queue,
			  struct sk_buff *skb, __u16 flags)
>>>>>>> common/android-3.10.y
{
	struct hci_conn *conn = chan->conn;
	struct hci_dev *hdev = conn->hdev;
	struct sk_buff *list;

<<<<<<< HEAD
	BT_DBG("%s conn %p chan %p flags 0x%x", hdev->name, conn, chan, flags);

	skb->dev = (void *) hdev;
	bt_cb(skb)->pkt_type = HCI_ACLDATA_PKT;
	if (hdev->dev_type == HCI_BREDR)
		hci_add_acl_hdr(skb, conn->handle, flags);
	else
		hci_add_acl_hdr(skb, chan->ll_handle, flags);
=======
	skb->len = skb_headlen(skb);
	skb->data_len = 0;

	bt_cb(skb)->pkt_type = HCI_ACLDATA_PKT;

	switch (hdev->dev_type) {
	case HCI_BREDR:
		hci_add_acl_hdr(skb, conn->handle, flags);
		break;
	case HCI_AMP:
		hci_add_acl_hdr(skb, chan->handle, flags);
		break;
	default:
		BT_ERR("%s unknown dev_type %d", hdev->name, hdev->dev_type);
		return;
	}
>>>>>>> common/android-3.10.y

	list = skb_shinfo(skb)->frag_list;
	if (!list) {
		/* Non fragmented */
		BT_DBG("%s nonfrag skb %p len %d", hdev->name, skb, skb->len);

		skb_queue_tail(&conn->data_q, skb);
	} else {
		/* Fragmented */
		BT_DBG("%s frag %p len %d", hdev->name, skb, skb->len);

		skb_shinfo(skb)->frag_list = NULL;

		/* Queue all fragments atomically */
		spin_lock_bh(&conn->data_q.lock);

		__skb_queue_tail(&conn->data_q, skb);
		flags &= ~ACL_PB_MASK;
		flags |= ACL_CONT;
		do {
			skb = list; list = list->next;

			skb->dev = (void *) hdev;
			bt_cb(skb)->pkt_type = HCI_ACLDATA_PKT;
			hci_add_acl_hdr(skb, conn->handle, flags);

			BT_DBG("%s frag %p len %d", hdev->name, skb, skb->len);

			__skb_queue_tail(&conn->data_q, skb);
		} while (list);

		spin_unlock_bh(&conn->data_q.lock);
	}

<<<<<<< HEAD
	tasklet_schedule(&hdev->tx_task);
=======
void hci_send_acl(struct hci_chan *chan, struct sk_buff *skb, __u16 flags)
{
	struct hci_dev *hdev = chan->conn->hdev;

	BT_DBG("%s chan %p flags 0x%4.4x", hdev->name, chan, flags);

	skb->dev = (void *) hdev;

	hci_queue_acl(chan, &chan->data_q, skb, flags);

	queue_work(hdev->workqueue, &hdev->tx_work);
>>>>>>> common/android-3.10.y
}

/* Send SCO data */
void hci_send_sco(struct hci_conn *conn, struct sk_buff *skb)
{
	struct hci_dev *hdev = conn->hdev;
	struct hci_sco_hdr hdr;

	BT_DBG("%s len %d", hdev->name, skb->len);

	hdr.handle = cpu_to_le16(conn->handle);
	hdr.dlen   = skb->len;

	skb_push(skb, HCI_SCO_HDR_SIZE);
	skb_reset_transport_header(skb);
	memcpy(skb_transport_header(skb), &hdr, HCI_SCO_HDR_SIZE);

	skb->dev = (void *) hdev;
	bt_cb(skb)->pkt_type = HCI_SCODATA_PKT;

	skb_queue_tail(&conn->data_q, skb);
	tasklet_schedule(&hdev->tx_task);
}

/* ---- HCI TX task (outgoing data) ---- */
/* HCI ACL Connection scheduler */
static inline struct hci_conn *hci_low_sent_acl(struct hci_dev *hdev,
								int *quote)
{
	struct hci_conn_hash *h = &hdev->conn_hash;
	struct hci_conn *conn = NULL;
	int num = 0, min = ~0, conn_num = 0;
	struct list_head *p;

	/* We don't have to lock device here. Connections are always
	 * added and removed with TX task disabled. */
	list_for_each(p, &h->list) {
		struct hci_conn *c;
		c = list_entry(p, struct hci_conn, list);
		if (c->type == ACL_LINK)
			conn_num++;

		if (skb_queue_empty(&c->data_q))
			continue;

		if (c->state != BT_CONNECTED && c->state != BT_CONFIG)
			continue;

		num++;

		if (c->sent < min) {
			min  = c->sent;
			conn = c;
		}
	}

	if (conn) {
		int cnt, q;
		cnt = hdev->acl_cnt;
		q = cnt / num;
		*quote = q ? q : 1;
	} else
		*quote = 0;

	if ((*quote == hdev->acl_cnt) &&
		(conn->sent == (hdev->acl_pkts - 1)) &&
		(conn_num > 1)) {
			*quote = 0;
			conn = NULL;
	}

	BT_DBG("conn %p quote %d", conn, *quote);
	return conn;
}

/* HCI Connection scheduler */
static struct hci_conn *hci_low_sent(struct hci_dev *hdev, __u8 type,
				     int *quote)
{
	struct hci_conn_hash *h = &hdev->conn_hash;
<<<<<<< HEAD
	struct hci_conn *conn = NULL;
	int num = 0, min = ~0;
	struct list_head *p;
=======
	struct hci_conn *conn = NULL, *c;
	unsigned int num = 0, min = ~0;
>>>>>>> common/android-3.10.y

	/* We don't have to lock device here. Connections are always
	 * added and removed with TX task disabled. */
	list_for_each(p, &h->list) {
		struct hci_conn *c;
		c = list_entry(p, struct hci_conn, list);

		if (c->type != type || skb_queue_empty(&c->data_q))
			continue;

		if (c->state != BT_CONNECTED && c->state != BT_CONFIG)
			continue;

		num++;

		if (c->sent < min) {
			min  = c->sent;
			conn = c;
		}
	}

	if (conn) {
		int cnt, q;

		switch (conn->type) {
		case ACL_LINK:
			cnt = hdev->acl_cnt;
			break;
		case SCO_LINK:
		case ESCO_LINK:
			cnt = hdev->sco_cnt;
			break;
		case LE_LINK:
			cnt = hdev->le_mtu ? hdev->le_cnt : hdev->acl_cnt;
			break;
		default:
			cnt = 0;
			BT_ERR("Unknown link type");
		}

		q = cnt / num;
		*quote = q ? q : 1;
	} else
		*quote = 0;

	BT_DBG("conn %p quote %d", conn, *quote);
	return conn;
}

static void hci_link_tx_to(struct hci_dev *hdev, __u8 type)
{
	struct hci_conn_hash *h = &hdev->conn_hash;
	struct list_head *p;
	struct hci_conn  *c;

	BT_ERR("%s link tx timeout", hdev->name);

	/* Kill stalled connections */
	list_for_each(p, &h->list) {
		c = list_entry(p, struct hci_conn, list);
		if (c->type == type && c->sent) {
			BT_ERR("%s killing stalled connection %pMR",
			       hdev->name, &c->dst);
			hci_disconnect(c, HCI_ERROR_REMOTE_USER_TERM);
		}
	}
}

<<<<<<< HEAD
static inline void hci_sched_acl(struct hci_dev *hdev)
=======
static struct hci_chan *hci_chan_sent(struct hci_dev *hdev, __u8 type,
				      int *quote)
{
	struct hci_conn_hash *h = &hdev->conn_hash;
	struct hci_chan *chan = NULL;
	unsigned int num = 0, min = ~0, cur_prio = 0;
	struct hci_conn *conn;
	int cnt, q, conn_num = 0;

	BT_DBG("%s", hdev->name);

	rcu_read_lock();

	list_for_each_entry_rcu(conn, &h->list, list) {
		struct hci_chan *tmp;

		if (conn->type != type)
			continue;

		if (conn->state != BT_CONNECTED && conn->state != BT_CONFIG)
			continue;

		conn_num++;

		list_for_each_entry_rcu(tmp, &conn->chan_list, list) {
			struct sk_buff *skb;

			if (skb_queue_empty(&tmp->data_q))
				continue;

			skb = skb_peek(&tmp->data_q);
			if (skb->priority < cur_prio)
				continue;

			if (skb->priority > cur_prio) {
				num = 0;
				min = ~0;
				cur_prio = skb->priority;
			}

			num++;

			if (conn->sent < min) {
				min  = conn->sent;
				chan = tmp;
			}
		}

		if (hci_conn_num(hdev, type) == conn_num)
			break;
	}

	rcu_read_unlock();

	if (!chan)
		return NULL;

	switch (chan->conn->type) {
	case ACL_LINK:
		cnt = hdev->acl_cnt;
		break;
	case AMP_LINK:
		cnt = hdev->block_cnt;
		break;
	case SCO_LINK:
	case ESCO_LINK:
		cnt = hdev->sco_cnt;
		break;
	case LE_LINK:
		cnt = hdev->le_mtu ? hdev->le_cnt : hdev->acl_cnt;
		break;
	default:
		cnt = 0;
		BT_ERR("Unknown link type");
	}

	q = cnt / num;
	*quote = q ? q : 1;
	BT_DBG("chan %p quote %d", chan, *quote);
	return chan;
}

static void hci_prio_recalculate(struct hci_dev *hdev, __u8 type)
>>>>>>> common/android-3.10.y
{
	struct hci_conn *conn;
	struct sk_buff *skb;
	int quote;

	BT_DBG("%s", hdev->name);

<<<<<<< HEAD
	if (!test_bit(HCI_RAW, &hdev->flags)) {
		/* ACL tx timeout must be longer than maximum
		 * link supervision timeout (40.9 seconds) */
		if (hdev->acl_cnt <= 0 &&
			time_after(jiffies, hdev->acl_last_tx + HZ * 45))
=======
	rcu_read_lock();

	list_for_each_entry_rcu(conn, &h->list, list) {
		struct hci_chan *chan;

		if (conn->type != type)
			continue;

		if (conn->state != BT_CONNECTED && conn->state != BT_CONFIG)
			continue;

		num++;

		list_for_each_entry_rcu(chan, &conn->chan_list, list) {
			struct sk_buff *skb;

			if (chan->sent) {
				chan->sent = 0;
				continue;
			}

			if (skb_queue_empty(&chan->data_q))
				continue;

			skb = skb_peek(&chan->data_q);
			if (skb->priority >= HCI_PRIO_MAX - 1)
				continue;

			skb->priority = HCI_PRIO_MAX - 1;

			BT_DBG("chan %p skb %p promoted to %d", chan, skb,
			       skb->priority);
		}

		if (hci_conn_num(hdev, type) == num)
			break;
	}

	rcu_read_unlock();

}

static inline int __get_blocks(struct hci_dev *hdev, struct sk_buff *skb)
{
	/* Calculate count of blocks used by this packet */
	return DIV_ROUND_UP(skb->len - HCI_ACL_HDR_SIZE, hdev->block_len);
}

static void __check_timeout(struct hci_dev *hdev, unsigned int cnt)
{
	if (!test_bit(HCI_RAW, &hdev->flags)) {
		/* ACL tx timeout must be longer than maximum
		 * link supervision timeout (40.9 seconds) */
		if (!cnt && time_after(jiffies, hdev->acl_last_tx +
				       HCI_ACL_TX_TIMEOUT))
>>>>>>> common/android-3.10.y
			hci_link_tx_to(hdev, ACL_LINK);
	}

<<<<<<< HEAD
	while (hdev->acl_cnt > 0 &&
		((conn = hci_low_sent_acl(hdev, &quote)) != NULL)) {

		while (quote > 0 &&
			  (skb = skb_dequeue(&conn->data_q))) {
			int count = 1;

			BT_DBG("skb %p len %d", skb, skb->len);
=======
static void hci_sched_acl_pkt(struct hci_dev *hdev)
{
	unsigned int cnt = hdev->acl_cnt;
	struct hci_chan *chan;
	struct sk_buff *skb;
	int quote;

	__check_timeout(hdev, cnt);

	while (hdev->acl_cnt &&
	       (chan = hci_chan_sent(hdev, ACL_LINK, &quote))) {
		u32 priority = (skb_peek(&chan->data_q))->priority;
		while (quote-- && (skb = skb_peek(&chan->data_q))) {
			BT_DBG("chan %p skb %p len %d priority %u", chan, skb,
			       skb->len, skb->priority);

			/* Stop if priority has changed */
			if (skb->priority < priority)
				break;

			skb = skb_dequeue(&chan->data_q);

			hci_conn_enter_active_mode(chan->conn,
						   bt_cb(skb)->force_active);

			hci_send_frame(skb);
			hdev->acl_last_tx = jiffies;

			hdev->acl_cnt--;
			chan->sent++;
			chan->conn->sent++;
		}
	}

	if (cnt != hdev->acl_cnt)
		hci_prio_recalculate(hdev, ACL_LINK);
}

static void hci_sched_acl_blk(struct hci_dev *hdev)
{
	unsigned int cnt = hdev->block_cnt;
	struct hci_chan *chan;
	struct sk_buff *skb;
	int quote;
	u8 type;

	__check_timeout(hdev, cnt);

	BT_DBG("%s", hdev->name);

	if (hdev->dev_type == HCI_AMP)
		type = AMP_LINK;
	else
		type = ACL_LINK;

	while (hdev->block_cnt > 0 &&
	       (chan = hci_chan_sent(hdev, type, &quote))) {
		u32 priority = (skb_peek(&chan->data_q))->priority;
		while (quote > 0 && (skb = skb_peek(&chan->data_q))) {
			int blocks;

			BT_DBG("chan %p skb %p len %d priority %u", chan, skb,
			       skb->len, skb->priority);

			/* Stop if priority has changed */
			if (skb->priority < priority)
				break;
>>>>>>> common/android-3.10.y

			if (hdev->flow_ctl_mode ==
				HCI_BLOCK_BASED_FLOW_CTL_MODE)
				/* Calculate count of blocks used by
				 * this packet
				 */
				count = ((skb->len - HCI_ACL_HDR_SIZE - 1) /
					hdev->data_block_len) + 1;

			if (count > hdev->acl_cnt)
				return;

<<<<<<< HEAD
			hci_conn_enter_active_mode(conn, bt_cb(skb)->force_active);
=======
			hci_conn_enter_active_mode(chan->conn,
						   bt_cb(skb)->force_active);
>>>>>>> common/android-3.10.y

			hci_send_frame(skb);
			hdev->acl_last_tx = jiffies;

			hdev->acl_cnt -= count;
			quote -= count;

			conn->sent += count;
		}
	}
<<<<<<< HEAD
=======

	if (cnt != hdev->block_cnt)
		hci_prio_recalculate(hdev, type);
}

static void hci_sched_acl(struct hci_dev *hdev)
{
	BT_DBG("%s", hdev->name);

	/* No ACL link over BR/EDR controller */
	if (!hci_conn_num(hdev, ACL_LINK) && hdev->dev_type == HCI_BREDR)
		return;

	/* No AMP link over AMP controller */
	if (!hci_conn_num(hdev, AMP_LINK) && hdev->dev_type == HCI_AMP)
		return;

	switch (hdev->flow_ctl_mode) {
	case HCI_FLOW_CTL_MODE_PACKET_BASED:
		hci_sched_acl_pkt(hdev);
		break;

	case HCI_FLOW_CTL_MODE_BLOCK_BASED:
		hci_sched_acl_blk(hdev);
		break;
	}
>>>>>>> common/android-3.10.y
}

/* Schedule SCO */
static void hci_sched_sco(struct hci_dev *hdev)
{
	struct hci_conn *conn;
	struct sk_buff *skb;
	int quote;

	BT_DBG("%s", hdev->name);

	while (hdev->sco_cnt && (conn = hci_low_sent(hdev, SCO_LINK, &quote))) {
		while (quote-- && (skb = skb_dequeue(&conn->data_q))) {
			BT_DBG("skb %p len %d", skb, skb->len);
			hci_send_frame(skb);

			conn->sent++;
			if (conn->sent == ~0)
				conn->sent = 0;
		}
	}
}

static void hci_sched_esco(struct hci_dev *hdev)
{
	struct hci_conn *conn;
	struct sk_buff *skb;
	int quote;

	BT_DBG("%s", hdev->name);

<<<<<<< HEAD
	while (hdev->sco_cnt && (conn = hci_low_sent(hdev, ESCO_LINK, &quote))) {
=======
	if (!hci_conn_num(hdev, ESCO_LINK))
		return;

	while (hdev->sco_cnt && (conn = hci_low_sent(hdev, ESCO_LINK,
						     &quote))) {
>>>>>>> common/android-3.10.y
		while (quote-- && (skb = skb_dequeue(&conn->data_q))) {
			BT_DBG("skb %p len %d", skb, skb->len);
			hci_send_frame(skb);

			conn->sent++;
			if (conn->sent == ~0)
				conn->sent = 0;
		}
	}
}

static void hci_sched_le(struct hci_dev *hdev)
{
	struct hci_conn *conn;
	struct sk_buff *skb;
	int quote, cnt;

	BT_DBG("%s", hdev->name);

	if (!test_bit(HCI_RAW, &hdev->flags)) {
		/* LE tx timeout must be longer than maximum
		 * link supervision timeout (40.9 seconds) */
		if (!hdev->le_cnt && hdev->le_pkts &&
		    time_after(jiffies, hdev->le_last_tx + HZ * 45))
			hci_link_tx_to(hdev, LE_LINK);
	}

	cnt = hdev->le_pkts ? hdev->le_cnt : hdev->acl_cnt;
<<<<<<< HEAD
	while (cnt && (conn = hci_low_sent(hdev, LE_LINK, &quote))) {
		while (quote-- && (skb = skb_dequeue(&conn->data_q))) {
			BT_DBG("skb %p len %d", skb, skb->len);
=======
	tmp = cnt;
	while (cnt && (chan = hci_chan_sent(hdev, LE_LINK, &quote))) {
		u32 priority = (skb_peek(&chan->data_q))->priority;
		while (quote-- && (skb = skb_peek(&chan->data_q))) {
			BT_DBG("chan %p skb %p len %d priority %u", chan, skb,
			       skb->len, skb->priority);

			/* Stop if priority has changed */
			if (skb->priority < priority)
				break;

			skb = skb_dequeue(&chan->data_q);
>>>>>>> common/android-3.10.y

			hci_send_frame(skb);
			hdev->le_last_tx = jiffies;

			cnt--;
			conn->sent++;
		}
	}
	if (hdev->le_pkts)
		hdev->le_cnt = cnt;
	else
		hdev->acl_cnt = cnt;
}

static void hci_tx_task(unsigned long arg)
{
	struct hci_dev *hdev = (struct hci_dev *) arg;
	struct sk_buff *skb;

	read_lock(&hci_task_lock);

	BT_DBG("%s acl %d sco %d le %d", hdev->name, hdev->acl_cnt,
	       hdev->sco_cnt, hdev->le_cnt);

	/* Schedule queues and send stuff to HCI driver */

	hci_sched_acl(hdev);

	hci_sched_sco(hdev);

	hci_sched_esco(hdev);

	hci_sched_le(hdev);

	/* Send next queued raw (unknown type) packet */
	while ((skb = skb_dequeue(&hdev->raw_q)))
		hci_send_frame(skb);

	read_unlock(&hci_task_lock);
}

/* ----- HCI RX task (incoming data proccessing) ----- */

/* ACL data packet */
static void hci_acldata_packet(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct hci_acl_hdr *hdr = (void *) skb->data;
	struct hci_conn *conn;
	__u16 handle, flags;

	skb_pull(skb, HCI_ACL_HDR_SIZE);

	handle = __le16_to_cpu(hdr->handle);
	flags  = hci_flags(handle);
	handle = hci_handle(handle);

	BT_DBG("%s len %d handle 0x%4.4x flags 0x%4.4x", hdev->name, skb->len,
	       handle, flags);

	hdev->stat.acl_rx++;

	hci_dev_lock(hdev);
	conn = hci_conn_hash_lookup_handle(hdev, handle);
	hci_dev_unlock(hdev);

	if (conn) {
		register struct hci_proto *hp;

<<<<<<< HEAD
		hci_conn_enter_active_mode(conn, bt_cb(skb)->force_active);

=======
>>>>>>> common/android-3.10.y
		/* Send to upper protocol */
		hp = hci_proto[HCI_PROTO_L2CAP];
		if (hp && hp->recv_acldata) {
			hp->recv_acldata(conn, skb, flags);
			return;
		}
	} else {
		BT_ERR("%s ACL packet for unknown connection handle %d",
		       hdev->name, handle);
	}

	kfree_skb(skb);
}

/* SCO data packet */
static void hci_scodata_packet(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct hci_sco_hdr *hdr = (void *) skb->data;
	struct hci_conn *conn;
	__u16 handle;

	skb_pull(skb, HCI_SCO_HDR_SIZE);

	handle = __le16_to_cpu(hdr->handle);

	BT_DBG("%s len %d handle 0x%4.4x", hdev->name, skb->len, handle);

	hdev->stat.sco_rx++;

	hci_dev_lock(hdev);
	conn = hci_conn_hash_lookup_handle(hdev, handle);
	hci_dev_unlock(hdev);

	if (conn) {
		register struct hci_proto *hp;

		/* Send to upper protocol */
		hp = hci_proto[HCI_PROTO_SCO];
		if (hp && hp->recv_scodata) {
			hp->recv_scodata(conn, skb);
			return;
		}
	} else {
		BT_ERR("%s SCO packet for unknown connection handle %d",
		       hdev->name, handle);
	}

	kfree_skb(skb);
}

<<<<<<< HEAD
static void hci_rx_task(unsigned long arg)
=======
static bool hci_req_is_complete(struct hci_dev *hdev)
{
	struct sk_buff *skb;

	skb = skb_peek(&hdev->cmd_q);
	if (!skb)
		return true;

	return bt_cb(skb)->req.start;
}

static void hci_resend_last(struct hci_dev *hdev)
{
	struct hci_command_hdr *sent;
	struct sk_buff *skb;
	u16 opcode;

	if (!hdev->sent_cmd)
		return;

	sent = (void *) hdev->sent_cmd->data;
	opcode = __le16_to_cpu(sent->opcode);
	if (opcode == HCI_OP_RESET)
		return;

	skb = skb_clone(hdev->sent_cmd, GFP_KERNEL);
	if (!skb)
		return;

	skb_queue_head(&hdev->cmd_q, skb);
	queue_work(hdev->workqueue, &hdev->cmd_work);
}

void hci_req_cmd_complete(struct hci_dev *hdev, u16 opcode, u8 status)
{
	hci_req_complete_t req_complete = NULL;
	struct sk_buff *skb;
	unsigned long flags;

	BT_DBG("opcode 0x%04x status 0x%02x", opcode, status);

	/* If the completed command doesn't match the last one that was
	 * sent we need to do special handling of it.
	 */
	if (!hci_sent_cmd_data(hdev, opcode)) {
		/* Some CSR based controllers generate a spontaneous
		 * reset complete event during init and any pending
		 * command will never be completed. In such a case we
		 * need to resend whatever was the last sent
		 * command.
		 */
		if (test_bit(HCI_INIT, &hdev->flags) && opcode == HCI_OP_RESET)
			hci_resend_last(hdev);

		return;
	}

	/* If the command succeeded and there's still more commands in
	 * this request the request is not yet complete.
	 */
	if (!status && !hci_req_is_complete(hdev))
		return;

	/* If this was the last command in a request the complete
	 * callback would be found in hdev->sent_cmd instead of the
	 * command queue (hdev->cmd_q).
	 */
	if (hdev->sent_cmd) {
		req_complete = bt_cb(hdev->sent_cmd)->req.complete;
		if (req_complete)
			goto call_complete;
	}

	/* Remove all pending commands belonging to this request */
	spin_lock_irqsave(&hdev->cmd_q.lock, flags);
	while ((skb = __skb_dequeue(&hdev->cmd_q))) {
		if (bt_cb(skb)->req.start) {
			__skb_queue_head(&hdev->cmd_q, skb);
			break;
		}

		req_complete = bt_cb(skb)->req.complete;
		kfree_skb(skb);
	}
	spin_unlock_irqrestore(&hdev->cmd_q.lock, flags);

call_complete:
	if (req_complete)
		req_complete(hdev, status);
}

static void hci_rx_work(struct work_struct *work)
>>>>>>> common/android-3.10.y
{
	struct hci_dev *hdev = (struct hci_dev *) arg;
	struct sk_buff *skb;

	BT_DBG("%s", hdev->name);

	read_lock(&hci_task_lock);

	while ((skb = skb_dequeue(&hdev->rx_q))) {
		if (atomic_read(&hdev->promisc)) {
			/* Send copy to the sockets */
			hci_send_to_sock(hdev, skb, NULL);
		}

		if (test_bit(HCI_RAW, &hdev->flags)) {
			kfree_skb(skb);
			continue;
		}

		if (test_bit(HCI_INIT, &hdev->flags)) {
			/* Don't process data packets in this states. */
			switch (bt_cb(skb)->pkt_type) {
			case HCI_ACLDATA_PKT:
			case HCI_SCODATA_PKT:
				kfree_skb(skb);
				continue;
			}
		}

		/* Process frame */
		switch (bt_cb(skb)->pkt_type) {
		case HCI_EVENT_PKT:
			hci_event_packet(hdev, skb);
			break;

		case HCI_ACLDATA_PKT:
			BT_DBG("%s ACL data packet", hdev->name);
			hci_acldata_packet(hdev, skb);
			break;

		case HCI_SCODATA_PKT:
			BT_DBG("%s SCO data packet", hdev->name);
			hci_scodata_packet(hdev, skb);
			break;

		default:
			kfree_skb(skb);
			break;
		}
	}

	read_unlock(&hci_task_lock);
}

static void hci_cmd_task(unsigned long arg)
{
	struct hci_dev *hdev = (struct hci_dev *) arg;
	struct sk_buff *skb;

	BT_DBG("%s cmd_cnt %d cmd queued %d", hdev->name,
	       atomic_read(&hdev->cmd_cnt), skb_queue_len(&hdev->cmd_q));

	/* Send queued commands */
	if (atomic_read(&hdev->cmd_cnt)) {
		skb = skb_dequeue(&hdev->cmd_q);
		if (!skb)
			return;

		kfree_skb(hdev->sent_cmd);

		hdev->sent_cmd = skb_clone(skb, GFP_ATOMIC);
		if (hdev->sent_cmd) {
			atomic_dec(&hdev->cmd_cnt);
			hci_send_frame(skb);
<<<<<<< HEAD
			mod_timer(&hdev->cmd_timer,
				  jiffies + msecs_to_jiffies(HCI_CMD_TIMEOUT));
=======
			if (test_bit(HCI_RESET, &hdev->flags))
				del_timer(&hdev->cmd_timer);
			else
				mod_timer(&hdev->cmd_timer,
					  jiffies + HCI_CMD_TIMEOUT);
>>>>>>> common/android-3.10.y
		} else {
			skb_queue_head(&hdev->cmd_q, skb);
			tasklet_schedule(&hdev->cmd_task);
		}
	}
}

<<<<<<< HEAD
module_param(enable_smp, bool, 0644);
MODULE_PARM_DESC(enable_smp, "Enable SMP support (LE only)");
=======
int hci_do_inquiry(struct hci_dev *hdev, u8 length)
{
	/* General inquiry access code (GIAC) */
	u8 lap[3] = { 0x33, 0x8b, 0x9e };
	struct hci_cp_inquiry cp;

	BT_DBG("%s", hdev->name);

	if (test_bit(HCI_INQUIRY, &hdev->flags))
		return -EINPROGRESS;

	inquiry_cache_flush(hdev);

	memset(&cp, 0, sizeof(cp));
	memcpy(&cp.lap, lap, sizeof(cp.lap));
	cp.length  = length;

	return hci_send_cmd(hdev, HCI_OP_INQUIRY, sizeof(cp), &cp);
}

int hci_cancel_inquiry(struct hci_dev *hdev)
{
	BT_DBG("%s", hdev->name);

	if (!test_bit(HCI_INQUIRY, &hdev->flags))
		return -EALREADY;

	return hci_send_cmd(hdev, HCI_OP_INQUIRY_CANCEL, 0, NULL);
}

u8 bdaddr_to_le(u8 bdaddr_type)
{
	switch (bdaddr_type) {
	case BDADDR_LE_PUBLIC:
		return ADDR_LE_DEV_PUBLIC;

	default:
		/* Fallback to LE Random address type */
		return ADDR_LE_DEV_RANDOM;
	}
}
>>>>>>> common/android-3.10.y
