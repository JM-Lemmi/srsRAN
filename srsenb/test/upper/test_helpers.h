/**
 * Copyright 2013-2021 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef SRSENB_TEST_HELPERS_H
#define SRSENB_TEST_HELPERS_H

#include "srsenb/src/enb_cfg_parser.h"
#include "srsenb/test/common/dummy_classes.h"
#include "srslte/adt/span.h"
#include "srslte/common/log_filter.h"

using namespace srsenb;
using namespace asn1::rrc;

namespace argparse {

extern std::string            repository_dir;
extern srslte::LOG_LEVEL_ENUM log_level;

inline void usage(char* prog)
{
  printf("Usage: %s [v] -i repository_dir\n", prog);
  printf("\t-v [set srslte_verbose to debug, default none]\n");
}

inline void parse_args(int argc, char** argv)
{
  int opt;

  while ((opt = getopt(argc, argv, "i")) != -1) {
    switch (opt) {
      case 'i':
        repository_dir = argv[optind];
        break;
      case 'v':
        log_level = srslte::LOG_LEVEL_DEBUG;
        break;
      default:
        usage(argv[0]);
        exit(-1);
    }
  }
  if (repository_dir.empty()) {
    usage(argv[0]);
    exit(-1);
  }
}

} // namespace argparse

namespace test_dummies {

class s1ap_mobility_dummy : public s1ap_dummy
{
public:
  struct ho_req_data {
    uint16_t                     rnti;
    uint32_t                     target_eci;
    srslte::plmn_id_t            target_plmn;
    srslte::unique_byte_buffer_t rrc_container;
  } last_ho_required = {};
  struct enb_status_transfer_info {
    bool                            status_present;
    uint16_t                        rnti;
    std::vector<bearer_status_info> bearer_list;
  } last_enb_status = {};
  std::vector<uint8_t> added_erab_ids;
  struct ho_req_ack {
    uint16_t                                     rnti;
    srslte::unique_byte_buffer_t                 ho_cmd_pdu;
    std::vector<asn1::fixed_octstring<4, true> > admitted_bearers;
  } last_ho_req_ack;

  bool send_ho_required(uint16_t                     rnti,
                        uint32_t                     target_eci,
                        srslte::plmn_id_t            target_plmn,
                        srslte::unique_byte_buffer_t rrc_container) final
  {
    last_ho_required = ho_req_data{rnti, target_eci, target_plmn, std::move(rrc_container)};
    return true;
  }
  bool send_enb_status_transfer_proc(uint16_t rnti, std::vector<bearer_status_info>& bearer_status_list) override
  {
    last_enb_status = {true, rnti, bearer_status_list};
    return true;
  }
  bool send_ho_req_ack(const asn1::s1ap::ho_request_s&               msg,
                       uint16_t                                      rnti,
                       srslte::unique_byte_buffer_t                  ho_cmd,
                       srslte::span<asn1::fixed_octstring<4, true> > admitted_bearers) override
  {
    last_ho_req_ack.rnti       = rnti;
    last_ho_req_ack.ho_cmd_pdu = std::move(ho_cmd);
    last_ho_req_ack.admitted_bearers.assign(admitted_bearers.begin(), admitted_bearers.end());
    return true;
  }
  void ue_erab_setup_complete(uint16_t rnti, const asn1::s1ap::erab_setup_resp_s& res) override
  {
    if (res.protocol_ies.erab_setup_list_bearer_su_res_present) {
      for (const auto& item : res.protocol_ies.erab_setup_list_bearer_su_res.value) {
        added_erab_ids.push_back(item.value.erab_setup_item_bearer_su_res().erab_id);
      }
    }
  }
  void user_mod(uint16_t old_rnti, uint16_t new_rnti) override {}
};

class pdcp_mobility_dummy : public pdcp_dummy
{
public:
  struct last_sdu_t {
    uint16_t                     rnti;
    uint32_t                     lcid;
    srslte::unique_byte_buffer_t sdu;
  } last_sdu;
  struct lcid_cfg_t {
    bool                         enable_integrity  = false;
    bool                         enable_encryption = false;
    srslte::pdcp_lte_state_t     state{};
    srslte::as_security_config_t sec_cfg{};
  };
  std::map<uint16_t, std::map<uint32_t, lcid_cfg_t> > bearers;

  void write_sdu(uint16_t rnti, uint32_t lcid, srslte::unique_byte_buffer_t sdu) override
  {
    last_sdu.rnti = rnti;
    last_sdu.lcid = lcid;
    last_sdu.sdu  = std::move(sdu);
  }
  bool set_bearer_state(uint16_t rnti, uint32_t lcid, const srslte::pdcp_lte_state_t& state) override
  {
    bearers[rnti][lcid].state = state;
    return true;
  }
  void enable_integrity(uint16_t rnti, uint32_t lcid) override { bearers[rnti][lcid].enable_integrity = true; }
  void enable_encryption(uint16_t rnti, uint32_t lcid) override { bearers[rnti][lcid].enable_encryption = true; }
  void config_security(uint16_t rnti, uint32_t lcid, srslte::as_security_config_t sec_cfg_) override
  {
    bearers[rnti][lcid].sec_cfg = sec_cfg_;
  }
};

class rlc_mobility_dummy : public rlc_dummy
{
public:
  struct ue_ctxt {
    int                          nof_pdcp_sdus = 0, reest_sdu_counter = 0;
    uint32_t                     last_lcid = 0;
    srslte::unique_byte_buffer_t last_sdu;
  };
  std::map<uint16_t, ue_ctxt> ue_db;

  void test_reset_all()
  {
    for (auto& u : ue_db) {
      u.second = {};
    }
  }

  void write_sdu(uint16_t rnti, uint32_t lcid, srslte::unique_byte_buffer_t sdu) override
  {
    ue_db[rnti].nof_pdcp_sdus++;
    ue_db[rnti].reest_sdu_counter++;
    ue_db[rnti].last_lcid = lcid;
    ue_db[rnti].last_sdu  = std::move(sdu);
  }
  void reestablish(uint16_t rnti) final { ue_db[rnti].reest_sdu_counter = 0; }
};

class mac_mobility_dummy : public mac_dummy
{
public:
  int ue_cfg(uint16_t rnti, sched_interface::ue_cfg_t* cfg) override
  {
    ue_db[rnti] = *cfg;
    return 0;
  }
  int ue_set_crnti(uint16_t temp_crnti, uint16_t crnti, sched_interface::ue_cfg_t* cfg) override
  {
    ue_db[crnti] = *cfg;
    return 0;
  }
  std::map<uint16_t, sched_interface::ue_cfg_t> ue_db;
};

class phy_mobility_dummy : public phy_dummy
{
public:
  void set_config(uint16_t rnti, const phy_rrc_cfg_list_t& dedicated_list) override
  {
    phy_cfg_set = true;
    last_cfg    = dedicated_list;
  }
  bool               phy_cfg_set = false;
  phy_rrc_cfg_list_t last_cfg;
};

} // namespace test_dummies

namespace test_helpers {

int parse_default_cfg(rrc_cfg_t* rrc_cfg, srsenb::all_args_t& args);
int parse_default_cfg_phy(rrc_cfg_t* rrc_cfg, phy_cfg_t* phy_cfg, srsenb::all_args_t& args);

template <typename ASN1Type>
bool unpack_asn1(ASN1Type& asn1obj, srslte::const_byte_span pdu)
{
  asn1::cbit_ref bref{pdu.data(), (uint32_t)pdu.size()};
  if (asn1obj.unpack(bref) != asn1::SRSASN_SUCCESS) {
    srslte::logmap::get("TEST")->error("Failed to unpack ASN1 type\n");
    return false;
  }
  return true;
}

inline void copy_msg_to_buffer(srslte::unique_byte_buffer_t& pdu, srslte::const_byte_span msg)
{
  srslte::byte_buffer_pool* pool = srslte::byte_buffer_pool::get_instance();
  pdu                            = srslte::allocate_unique_buffer(*pool, true);
  memcpy(pdu->msg, msg.data(), msg.size());
  pdu->N_bytes = msg.size();
}

int bring_rrc_to_reconf_state(srsenb::rrc& rrc, srslte::timer_handler& timers, uint16_t rnti);

} // namespace test_helpers

namespace srsenb {

meas_cell_cfg_t generate_cell1();

report_cfg_eutra_s generate_rep1();

bool is_cell_cfg_equal(const meas_cell_cfg_t& cfg, const cells_to_add_mod_s& cell);

} // namespace srsenb

#endif // SRSENB_TEST_HELPERS_H
