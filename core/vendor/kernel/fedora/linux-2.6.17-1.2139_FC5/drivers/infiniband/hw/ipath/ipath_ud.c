/*
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <rdma/ib_smi.h>

#include "ipath_verbs.h"
#include "ips_common.h"

/**
 * ipath_ud_loopback - handle send on loopback QPs
 * @sqp: the QP
 * @ss: the SGE state
 * @length: the length of the data to send
 * @wr: the work request
 * @wc: the work completion entry
 *
 * This is called from ipath_post_ud_send() to forward a WQE addressed
 * to the same HCA.
 */
static void ipath_ud_loopback(struct ipath_qp *sqp,
			      struct ipath_sge_state *ss,
			      u32 length, struct ib_send_wr *wr,
			      struct ib_wc *wc)
{
	struct ipath_ibdev *dev = to_idev(sqp->ibqp.device);
	struct ipath_qp *qp;
	struct ib_ah_attr *ah_attr;
	unsigned long flags;
	struct ipath_rq *rq;
	struct ipath_srq *srq;
	struct ipath_sge_state rsge;
	struct ipath_sge *sge;
	struct ipath_rwqe *wqe;

	qp = ipath_lookup_qpn(&dev->qp_table, wr->wr.ud.remote_qpn);
	if (!qp)
		return;

	/*
	 * Check that the qkey matches (except for QP0, see 9.6.1.4.1).
	 * Qkeys with the high order bit set mean use the
	 * qkey from the QP context instead of the WR (see 10.2.5).
	 */
	if (unlikely(qp->ibqp.qp_num &&
		     ((int) wr->wr.ud.remote_qkey < 0
		      ? qp->qkey : wr->wr.ud.remote_qkey) != qp->qkey)) {
		/* XXX OK to lose a count once in a while. */
		dev->qkey_violations++;
		dev->n_pkt_drops++;
		goto done;
	}

	/*
	 * A GRH is expected to preceed the data even if not
	 * present on the wire.
	 */
	wc->byte_len = length + sizeof(struct ib_grh);

	if (wr->opcode == IB_WR_SEND_WITH_IMM) {
		wc->wc_flags = IB_WC_WITH_IMM;
		wc->imm_data = wr->imm_data;
	} else {
		wc->wc_flags = 0;
		wc->imm_data = 0;
	}

	/*
	 * Get the next work request entry to find where to put the data.
	 * Note that it is safe to drop the lock after changing rq->tail
	 * since ipath_post_receive() won't fill the empty slot.
	 */
	if (qp->ibqp.srq) {
		srq = to_isrq(qp->ibqp.srq);
		rq = &srq->rq;
	} else {
		srq = NULL;
		rq = &qp->r_rq;
	}
	spin_lock_irqsave(&rq->lock, flags);
	if (rq->tail == rq->head) {
		spin_unlock_irqrestore(&rq->lock, flags);
		dev->n_pkt_drops++;
		goto done;
	}
	/* Silently drop packets which are too big. */
	wqe = get_rwqe_ptr(rq, rq->tail);
	if (wc->byte_len > wqe->length) {
		spin_unlock_irqrestore(&rq->lock, flags);
		dev->n_pkt_drops++;
		goto done;
	}
	wc->wr_id = wqe->wr_id;
	rsge.sge = wqe->sg_list[0];
	rsge.sg_list = wqe->sg_list + 1;
	rsge.num_sge = wqe->num_sge;
	if (++rq->tail >= rq->size)
		rq->tail = 0;
	if (srq && srq->ibsrq.event_handler) {
		u32 n;

		if (rq->head < rq->tail)
			n = rq->size + rq->head - rq->tail;
		else
			n = rq->head - rq->tail;
		if (n < srq->limit) {
			struct ib_event ev;

			srq->limit = 0;
			spin_unlock_irqrestore(&rq->lock, flags);
			ev.device = qp->ibqp.device;
			ev.element.srq = qp->ibqp.srq;
			ev.event = IB_EVENT_SRQ_LIMIT_REACHED;
			srq->ibsrq.event_handler(&ev,
						 srq->ibsrq.srq_context);
		} else
			spin_unlock_irqrestore(&rq->lock, flags);
	} else
		spin_unlock_irqrestore(&rq->lock, flags);
	ah_attr = &to_iah(wr->wr.ud.ah)->attr;
	if (ah_attr->ah_flags & IB_AH_GRH) {
		ipath_copy_sge(&rsge, &ah_attr->grh, sizeof(struct ib_grh));
		wc->wc_flags |= IB_WC_GRH;
	} else
		ipath_skip_sge(&rsge, sizeof(struct ib_grh));
	sge = &ss->sge;
	while (length) {
		u32 len = sge->length;

		if (len > length)
			len = length;
		BUG_ON(len == 0);
		ipath_copy_sge(&rsge, sge->vaddr, len);
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr != NULL) {
			if (++sge->n >= IPATH_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		length -= len;
	}
	wc->status = IB_WC_SUCCESS;
	wc->opcode = IB_WC_RECV;
	wc->vendor_err = 0;
	wc->qp_num = qp->ibqp.qp_num;
	wc->src_qp = sqp->ibqp.qp_num;
	/* XXX do we know which pkey matched? Only needed for GSI. */
	wc->pkey_index = 0;
	wc->slid = ipath_layer_get_lid(dev->dd) |
		(ah_attr->src_path_bits &
		 ((1 << (dev->mkeyprot_resv_lmc & 7)) - 1));
	wc->sl = ah_attr->sl;
	wc->dlid_path_bits =
		ah_attr->dlid & ((1 << (dev->mkeyprot_resv_lmc & 7)) - 1);
	/* Signal completion event if the solicited bit is set. */
	ipath_cq_enter(to_icq(qp->ibqp.recv_cq), wc,
		       wr->send_flags & IB_SEND_SOLICITED);

done:
	if (atomic_dec_and_test(&qp->refcount))
		wake_up(&qp->wait);
}

/**
 * ipath_post_ud_send - post a UD send on QP
 * @qp: the QP
 * @wr: the work request
 *
 * Note that we actually send the data as it is posted instead of putting
 * the request into a ring buffer.  If we wanted to use a ring buffer,
 * we would need to save a reference to the destination address in the SWQE.
 */
int ipath_post_ud_send(struct ipath_qp *qp, struct ib_send_wr *wr)
{
	struct ipath_ibdev *dev = to_idev(qp->ibqp.device);
	struct ipath_other_headers *ohdr;
	struct ib_ah_attr *ah_attr;
	struct ipath_sge_state ss;
	struct ipath_sge *sg_list;
	struct ib_wc wc;
	u32 hwords;
	u32 nwords;
	u32 len;
	u32 extra_bytes;
	u32 bth0;
	u16 lrh0;
	u16 lid;
	int i;
	int ret;

	if (!(ib_ipath_state_ops[qp->state] & IPATH_PROCESS_SEND_OK)) {
		ret = 0;
		goto bail;
	}

	/* IB spec says that num_sge == 0 is OK. */
	if (wr->num_sge > qp->s_max_sge) {
		ret = -EINVAL;
		goto bail;
	}

	if (wr->num_sge > 1) {
		sg_list = kmalloc((qp->s_max_sge - 1) * sizeof(*sg_list),
				  GFP_ATOMIC);
		if (!sg_list) {
			ret = -ENOMEM;
			goto bail;
		}
	} else
		sg_list = NULL;

	/* Check the buffer to send. */
	ss.sg_list = sg_list;
	ss.sge.mr = NULL;
	ss.sge.vaddr = NULL;
	ss.sge.length = 0;
	ss.sge.sge_length = 0;
	ss.num_sge = 0;
	len = 0;
	for (i = 0; i < wr->num_sge; i++) {
		/* Check LKEY */
		if (to_ipd(qp->ibqp.pd)->user && wr->sg_list[i].lkey == 0) {
			ret = -EINVAL;
			goto bail;
		}

		if (wr->sg_list[i].length == 0)
			continue;
		if (!ipath_lkey_ok(&dev->lk_table, ss.num_sge ?
				   sg_list + ss.num_sge - 1 : &ss.sge,
				   &wr->sg_list[i], 0)) {
			ret = -EINVAL;
			goto bail;
		}
		len += wr->sg_list[i].length;
		ss.num_sge++;
	}
	extra_bytes = (4 - len) & 3;
	nwords = (len + extra_bytes) >> 2;

	/* Construct the header. */
	ah_attr = &to_iah(wr->wr.ud.ah)->attr;
	if (ah_attr->dlid == 0) {
		ret = -EINVAL;
		goto bail;
	}
	if (ah_attr->dlid >= IPS_MULTICAST_LID_BASE) {
		if (ah_attr->dlid != IPS_PERMISSIVE_LID)
			dev->n_multicast_xmit++;
		else
			dev->n_unicast_xmit++;
	} else {
		dev->n_unicast_xmit++;
		lid = ah_attr->dlid &
			~((1 << (dev->mkeyprot_resv_lmc & 7)) - 1);
		if (unlikely(lid == ipath_layer_get_lid(dev->dd))) {
			/*
			 * Pass in an uninitialized ib_wc to save stack
			 * space.
			 */
			ipath_ud_loopback(qp, &ss, len, wr, &wc);
			goto done;
		}
	}
	if (ah_attr->ah_flags & IB_AH_GRH) {
		/* Header size in 32-bit words. */
		hwords = 17;
		lrh0 = IPS_LRH_GRH;
		ohdr = &qp->s_hdr.u.l.oth;
		qp->s_hdr.u.l.grh.version_tclass_flow =
			cpu_to_be32((6 << 28) |
				    (ah_attr->grh.traffic_class << 20) |
				    ah_attr->grh.flow_label);
		qp->s_hdr.u.l.grh.paylen =
			cpu_to_be16(((wr->opcode ==
				      IB_WR_SEND_WITH_IMM ? 6 : 5) +
				     nwords + SIZE_OF_CRC) << 2);
		/* next_hdr is defined by C8-7 in ch. 8.4.1 */
		qp->s_hdr.u.l.grh.next_hdr = 0x1B;
		qp->s_hdr.u.l.grh.hop_limit = ah_attr->grh.hop_limit;
		/* The SGID is 32-bit aligned. */
		qp->s_hdr.u.l.grh.sgid.global.subnet_prefix =
			dev->gid_prefix;
		qp->s_hdr.u.l.grh.sgid.global.interface_id =
			ipath_layer_get_guid(dev->dd);
		qp->s_hdr.u.l.grh.dgid = ah_attr->grh.dgid;
		/*
		 * Don't worry about sending to locally attached multicast
		 * QPs.  It is unspecified by the spec. what happens.
		 */
	} else {
		/* Header size in 32-bit words. */
		hwords = 7;
		lrh0 = IPS_LRH_BTH;
		ohdr = &qp->s_hdr.u.oth;
	}
	if (wr->opcode == IB_WR_SEND_WITH_IMM) {
		ohdr->u.ud.imm_data = wr->imm_data;
		wc.imm_data = wr->imm_data;
		hwords += 1;
		bth0 = IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE << 24;
	} else if (wr->opcode == IB_WR_SEND) {
		wc.imm_data = 0;
		bth0 = IB_OPCODE_UD_SEND_ONLY << 24;
	} else {
		ret = -EINVAL;
		goto bail;
	}
	lrh0 |= ah_attr->sl << 4;
	if (qp->ibqp.qp_type == IB_QPT_SMI)
		lrh0 |= 0xF000;	/* Set VL (see ch. 13.5.3.1) */
	qp->s_hdr.lrh[0] = cpu_to_be16(lrh0);
	qp->s_hdr.lrh[1] = cpu_to_be16(ah_attr->dlid);	/* DEST LID */
	qp->s_hdr.lrh[2] = cpu_to_be16(hwords + nwords + SIZE_OF_CRC);
	lid = ipath_layer_get_lid(dev->dd);
	if (lid) {
		lid |= ah_attr->src_path_bits &
			((1 << (dev->mkeyprot_resv_lmc & 7)) - 1);
		qp->s_hdr.lrh[3] = cpu_to_be16(lid);
	} else
		qp->s_hdr.lrh[3] = IB_LID_PERMISSIVE;
	if (wr->send_flags & IB_SEND_SOLICITED)
		bth0 |= 1 << 23;
	bth0 |= extra_bytes << 20;
	bth0 |= qp->ibqp.qp_type == IB_QPT_SMI ? IPS_DEFAULT_P_KEY :
		ipath_layer_get_pkey(dev->dd, qp->s_pkey_index);
	ohdr->bth[0] = cpu_to_be32(bth0);
	/*
	 * Use the multicast QP if the destination LID is a multicast LID.
	 */
	ohdr->bth[1] = ah_attr->dlid >= IPS_MULTICAST_LID_BASE &&
		ah_attr->dlid != IPS_PERMISSIVE_LID ?
		__constant_cpu_to_be32(IPS_MULTICAST_QPN) :
		cpu_to_be32(wr->wr.ud.remote_qpn);
	/* XXX Could lose a PSN count but not worth locking */
	ohdr->bth[2] = cpu_to_be32(qp->s_next_psn++ & IPS_PSN_MASK);
	/*
	 * Qkeys with the high order bit set mean use the
	 * qkey from the QP context instead of the WR (see 10.2.5).
	 */
	ohdr->u.ud.deth[0] = cpu_to_be32((int)wr->wr.ud.remote_qkey < 0 ?
					 qp->qkey : wr->wr.ud.remote_qkey);
	ohdr->u.ud.deth[1] = cpu_to_be32(qp->ibqp.qp_num);
	if (ipath_verbs_send(dev->dd, hwords, (u32 *) &qp->s_hdr,
			     len, &ss))
		dev->n_no_piobuf++;

done:
	/* Queue the completion status entry. */
	if (!test_bit(IPATH_S_SIGNAL_REQ_WR, &qp->s_flags) ||
	    (wr->send_flags & IB_SEND_SIGNALED)) {
		wc.wr_id = wr->wr_id;
		wc.status = IB_WC_SUCCESS;
		wc.vendor_err = 0;
		wc.opcode = IB_WC_SEND;
		wc.byte_len = len;
		wc.qp_num = qp->ibqp.qp_num;
		wc.src_qp = 0;
		wc.wc_flags = 0;
		/* XXX initialize other fields? */
		ipath_cq_enter(to_icq(qp->ibqp.send_cq), &wc, 0);
	}
	kfree(sg_list);

	ret = 0;

bail:
	return ret;
}

/**
 * ipath_ud_rcv - receive an incoming UD packet
 * @dev: the device the packet came in on
 * @hdr: the packet header
 * @has_grh: true if the packet has a GRH
 * @data: the packet data
 * @tlen: the packet length
 * @qp: the QP the packet came on
 *
 * This is called from ipath_qp_rcv() to process an incoming UD packet
 * for the given QP.
 * Called at interrupt level.
 */
void ipath_ud_rcv(struct ipath_ibdev *dev, struct ipath_ib_header *hdr,
		  int has_grh, void *data, u32 tlen, struct ipath_qp *qp)
{
	struct ipath_other_headers *ohdr;
	int opcode;
	u32 hdrsize;
	u32 pad;
	unsigned long flags;
	struct ib_wc wc;
	u32 qkey;
	u32 src_qp;
	struct ipath_rq *rq;
	struct ipath_srq *srq;
	struct ipath_rwqe *wqe;
	u16 dlid;
	int header_in_data;

	/* Check for GRH */
	if (!has_grh) {
		ohdr = &hdr->u.oth;
		hdrsize = 8 + 12 + 8;	/* LRH + BTH + DETH */
		qkey = be32_to_cpu(ohdr->u.ud.deth[0]);
		src_qp = be32_to_cpu(ohdr->u.ud.deth[1]);
		header_in_data = 0;
	} else {
		ohdr = &hdr->u.l.oth;
		hdrsize = 8 + 40 + 12 + 8; /* LRH + GRH + BTH + DETH */
		/*
		 * The header with GRH is 68 bytes and the core driver sets
		 * the eager header buffer size to 56 bytes so the last 12
		 * bytes of the IB header is in the data buffer.
		 */
		header_in_data =
			ipath_layer_get_rcvhdrentsize(dev->dd) == 16;
		if (header_in_data) {
			qkey = be32_to_cpu(((__be32 *) data)[1]);
			src_qp = be32_to_cpu(((__be32 *) data)[2]);
			data += 12;
		} else {
			qkey = be32_to_cpu(ohdr->u.ud.deth[0]);
			src_qp = be32_to_cpu(ohdr->u.ud.deth[1]);
		}
	}
	src_qp &= IPS_QPN_MASK;

	/*
	 * Check that the permissive LID is only used on QP0
	 * and the QKEY matches (see 9.6.1.4.1 and 9.6.1.5.1).
	 */
	if (qp->ibqp.qp_num) {
		if (unlikely(hdr->lrh[1] == IB_LID_PERMISSIVE ||
			     hdr->lrh[3] == IB_LID_PERMISSIVE)) {
			dev->n_pkt_drops++;
			goto bail;
		}
		if (unlikely(qkey != qp->qkey)) {
			/* XXX OK to lose a count once in a while. */
			dev->qkey_violations++;
			dev->n_pkt_drops++;
			goto bail;
		}
	} else if (hdr->lrh[1] == IB_LID_PERMISSIVE ||
		   hdr->lrh[3] == IB_LID_PERMISSIVE) {
		struct ib_smp *smp = (struct ib_smp *) data;

		if (smp->mgmt_class != IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
			dev->n_pkt_drops++;
			goto bail;
		}
	}

	/* Get the number of bytes the message was padded by. */
	pad = (be32_to_cpu(ohdr->bth[0]) >> 20) & 3;
	if (unlikely(tlen < (hdrsize + pad + 4))) {
		/* Drop incomplete packets. */
		dev->n_pkt_drops++;
		goto bail;
	}
	tlen -= hdrsize + pad + 4;

	/* Drop invalid MAD packets (see 13.5.3.1). */
	if (unlikely((qp->ibqp.qp_num == 0 &&
		      (tlen != 256 ||
		       (be16_to_cpu(hdr->lrh[0]) >> 12) != 15)) ||
		     (qp->ibqp.qp_num == 1 &&
		      (tlen != 256 ||
		       (be16_to_cpu(hdr->lrh[0]) >> 12) == 15)))) {
		dev->n_pkt_drops++;
		goto bail;
	}

	/*
	 * A GRH is expected to preceed the data even if not
	 * present on the wire.
	 */
	wc.byte_len = tlen + sizeof(struct ib_grh);

	/*
	 * The opcode is in the low byte when its in network order
	 * (top byte when in host order).
	 */
	opcode = be32_to_cpu(ohdr->bth[0]) >> 24;
	if (qp->ibqp.qp_num > 1 &&
	    opcode == IB_OPCODE_UD_SEND_ONLY_WITH_IMMEDIATE) {
		if (header_in_data) {
			wc.imm_data = *(__be32 *) data;
			data += sizeof(__be32);
		} else
			wc.imm_data = ohdr->u.ud.imm_data;
		wc.wc_flags = IB_WC_WITH_IMM;
		hdrsize += sizeof(u32);
	} else if (opcode == IB_OPCODE_UD_SEND_ONLY) {
		wc.imm_data = 0;
		wc.wc_flags = 0;
	} else {
		dev->n_pkt_drops++;
		goto bail;
	}

	/*
	 * Get the next work request entry to find where to put the data.
	 * Note that it is safe to drop the lock after changing rq->tail
	 * since ipath_post_receive() won't fill the empty slot.
	 */
	if (qp->ibqp.srq) {
		srq = to_isrq(qp->ibqp.srq);
		rq = &srq->rq;
	} else {
		srq = NULL;
		rq = &qp->r_rq;
	}
	spin_lock_irqsave(&rq->lock, flags);
	if (rq->tail == rq->head) {
		spin_unlock_irqrestore(&rq->lock, flags);
		dev->n_pkt_drops++;
		goto bail;
	}
	/* Silently drop packets which are too big. */
	wqe = get_rwqe_ptr(rq, rq->tail);
	if (wc.byte_len > wqe->length) {
		spin_unlock_irqrestore(&rq->lock, flags);
		dev->n_pkt_drops++;
		goto bail;
	}
	wc.wr_id = wqe->wr_id;
	qp->r_sge.sge = wqe->sg_list[0];
	qp->r_sge.sg_list = wqe->sg_list + 1;
	qp->r_sge.num_sge = wqe->num_sge;
	if (++rq->tail >= rq->size)
		rq->tail = 0;
	if (srq && srq->ibsrq.event_handler) {
		u32 n;

		if (rq->head < rq->tail)
			n = rq->size + rq->head - rq->tail;
		else
			n = rq->head - rq->tail;
		if (n < srq->limit) {
			struct ib_event ev;

			srq->limit = 0;
			spin_unlock_irqrestore(&rq->lock, flags);
			ev.device = qp->ibqp.device;
			ev.element.srq = qp->ibqp.srq;
			ev.event = IB_EVENT_SRQ_LIMIT_REACHED;
			srq->ibsrq.event_handler(&ev,
						 srq->ibsrq.srq_context);
		} else
			spin_unlock_irqrestore(&rq->lock, flags);
	} else
		spin_unlock_irqrestore(&rq->lock, flags);
	if (has_grh) {
		ipath_copy_sge(&qp->r_sge, &hdr->u.l.grh,
			       sizeof(struct ib_grh));
		wc.wc_flags |= IB_WC_GRH;
	} else
		ipath_skip_sge(&qp->r_sge, sizeof(struct ib_grh));
	ipath_copy_sge(&qp->r_sge, data,
		       wc.byte_len - sizeof(struct ib_grh));
	wc.status = IB_WC_SUCCESS;
	wc.opcode = IB_WC_RECV;
	wc.vendor_err = 0;
	wc.qp_num = qp->ibqp.qp_num;
	wc.src_qp = src_qp;
	/* XXX do we know which pkey matched? Only needed for GSI. */
	wc.pkey_index = 0;
	wc.slid = be16_to_cpu(hdr->lrh[3]);
	wc.sl = (be16_to_cpu(hdr->lrh[0]) >> 4) & 0xF;
	dlid = be16_to_cpu(hdr->lrh[1]);
	/*
	 * Save the LMC lower bits if the destination LID is a unicast LID.
	 */
	wc.dlid_path_bits = dlid >= IPS_MULTICAST_LID_BASE ? 0 :
		dlid & ((1 << (dev->mkeyprot_resv_lmc & 7)) - 1);
	/* Signal completion event if the solicited bit is set. */
	ipath_cq_enter(to_icq(qp->ibqp.recv_cq), &wc,
		       (ohdr->bth[0] &
			__constant_cpu_to_be32(1 << 23)) != 0);

bail:;
}
