/****************************************************************
 * 
 *        Copyright 2013, Big Switch Networks, Inc. 
 * 
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 * 
 *        http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 * 
 ***************************************************************/

#include <AIM/aim.h>
#include <Forwarding/forwarding_config.h>
#include <Forwarding/forwarding.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define LOXI_SUCCESS(x)  ((x) == OF_ERROR_NONE)
#define LOXI_FAILURE(x)  (!LOXI_SUCCESS(x))

#define OK(x) TEST_ASSERT((x) == INDIGO_ERROR_NONE)

void
assert_fail(unsigned line, char *expr)
{
    printf("Assertion failed, line=%u, expr=\"%s\"\n", line, expr);
}

#define TEST_ASSERT(expr)  do { if (!(expr)) { assert_fail(__LINE__, # expr); } } while (0)

#define TRUE   1
#define FALSE  0

ind_fwd_config_t ind_fwd_config[1] = {{
        OF_VERSION_1_0,             /* of_version */
        10                          /* max_flows */
    }};

struct callback_info {
    unsigned        calledf;
    unsigned        called_cnt;
    indigo_error_t  result;
    indigo_cookie_t callback_cookie;
};

void
callback_arm(struct callback_info *callback_info)
{
    callback_info->calledf = FALSE;
    callback_info->result = -1;
    callback_info->callback_cookie = INDIGO_COOKIE_NULL;
}

void
callback_record(struct callback_info *callback_info,
                indigo_error_t       result,
                indigo_cookie_t      callback_cookie)
{
    callback_info->calledf         = TRUE;
    callback_info->result          = result;
    callback_info->callback_cookie = callback_cookie;
}

void
callback_chk(struct callback_info *callback_info,
             indigo_cookie_t      callback_cookie)
{
    TEST_ASSERT(callback_info->calledf);
    TEST_ASSERT(callback_info->result == INDIGO_ERROR_NONE);
    TEST_ASSERT(callback_info->callback_cookie == callback_cookie);
}


/* Fake state manager callback functions */

struct callback_info indigo_state_manager_flow_create_callback_info[1];

static indigo_cookie_t created_flow_id;

void
indigo_core_flow_create_callback(indigo_error_t result,
                                 indigo_cookie_t flow_id,
                                 uint8_t table_id,
                                 indigo_cookie_t callback_cookie)
{
    callback_record(indigo_state_manager_flow_create_callback_info,
                    result, callback_cookie);
    created_flow_id = flow_id;
}

struct callback_info indigo_state_manager_flow_modify_callback_info[1];


void
indigo_core_flow_modify_callback(indigo_error_t result,
                                 indigo_fi_flow_stats_t *flow_stats,
                                 indigo_cookie_t callback_cookie)
{
    callback_record(indigo_state_manager_flow_modify_callback_info, result,
                    callback_cookie);
}

struct callback_info indigo_state_manager_flow_delete_callback_info[1];

static int last_packets;
static int last_bytes;


void
indigo_core_flow_delete_callback(indigo_error_t result,
                                 indigo_fi_flow_stats_t *flow_stats,
                                 indigo_cookie_t callback_cookie)
{
    callback_record(indigo_state_manager_flow_delete_callback_info,
                    result, callback_cookie);
    last_packets = flow_stats->packets;
    last_bytes = flow_stats->bytes;
}

struct callback_info indigo_core_flow_stats_get_callback_info[1];

void indigo_core_flow_stats_get_callback(
    indigo_error_t result,
    indigo_fi_flow_stats_t *flow_stats,
    indigo_cookie_t callback_cookie)
{
    callback_record(indigo_core_flow_stats_get_callback_info, result,
                    callback_cookie);
    last_packets = flow_stats->packets;
    last_bytes = flow_stats->bytes;
}

struct callback_info indigo_core_table_stats_get_callback_info[1];

static int last_active_count;
static int last_lookup_count;
static int last_matched_count;

void
indigo_core_table_stats_get_callback(indigo_error_t result,
                                     of_table_stats_reply_t *table_stats_reply,
                                     indigo_cookie_t callback_cookie)
{
    of_list_table_stats_entry_t   list;
    of_table_stats_entry_t        entry;
    unsigned                      n;
    int                           rv;
    uint64_t val64;
    uint32_t val32;

    callback_record(indigo_core_table_stats_get_callback_info, result,
                    callback_cookie);

    of_table_stats_reply_entries_bind(table_stats_reply, &list);

    n = 0;
    OF_LIST_TABLE_STATS_ENTRY_ITER(&list, &entry, rv) {
        of_table_stats_entry_max_entries_get(&entry, &val32);
        TEST_ASSERT(val32 == ind_fwd_config->max_flows);
        of_table_stats_entry_active_count_get(&entry, &val32);
        last_active_count = val32;
        of_table_stats_entry_lookup_count_get(&entry, &val64);
        last_lookup_count = val64;
        of_table_stats_entry_matched_count_get(&entry, &val64);
        last_matched_count = val64;

        ++n;
    }
    TEST_ASSERT(n == 1);

    of_table_stats_reply_delete(table_stats_reply);
}

/* Fake state manager notification functions */

struct {
    unsigned     calledf;
    unsigned     called_cnt;
    of_port_no_t in_port;
    of_octets_t  of_octets;
    uint16_t     total_len;
    uint8_t      reason;
} pkt_in_info[1];

indigo_error_t indigo_core_packet_in(of_packet_in_t *of_packet_in)
{
    pkt_in_info->calledf = TRUE;
    ++pkt_in_info->called_cnt;
    of_packet_in_in_port_get(of_packet_in, &pkt_in_info->in_port);
    of_packet_in_data_get(of_packet_in, &pkt_in_info->of_octets);
    of_packet_in_total_len_get(of_packet_in, &pkt_in_info->total_len);
    of_packet_in_reason_get(of_packet_in, &pkt_in_info->reason);

    of_packet_in_delete(of_packet_in);

    return INDIGO_ERROR_NONE;
}

void
pkt_in_arm(void)
{
    pkt_in_info->calledf = FALSE;
}

void
pkt_in_chk(of_port_no_t in_port, uint8_t *data, unsigned len, unsigned reason)
{
    TEST_ASSERT(pkt_in_info->calledf);
    TEST_ASSERT(pkt_in_info->in_port == in_port);
    TEST_ASSERT(memcmp(pkt_in_info->of_octets.data, data, len) == 0);
    TEST_ASSERT(pkt_in_info->of_octets.bytes == len);
    TEST_ASSERT(pkt_in_info->total_len == len);
    TEST_ASSERT(pkt_in_info->reason == reason);
}


/* Stubbed out port manager functions */

unsigned
ind_port_packet_in_is_enabled(of_port_no_t of_port_num)
{
    return (TRUE);
}

struct {
    unsigned     flag;
    of_port_no_t of_port_num;
    uint8_t      *data;
    unsigned     len;
} pkt_tx_info[1];

void
pkt_tx_arm(void)
{
    memset(pkt_tx_info, 0, sizeof(*pkt_tx_info));
}

void
pkt_tx_chk(of_port_no_t of_port_num, uint8_t *data, unsigned len)
{
    TEST_ASSERT(pkt_tx_info->flag);
    TEST_ASSERT(pkt_tx_info->of_port_num == of_port_num);
    TEST_ASSERT(pkt_tx_info->data        == data);
    TEST_ASSERT(pkt_tx_info->len         == len);
}

indigo_error_t
indigo_port_packet_emit(of_port_no_t of_port_num, unsigned queue_id, uint8_t *data, unsigned len)
{
    pkt_tx_info->flag        = TRUE;
    pkt_tx_info->of_port_num = of_port_num;
    pkt_tx_info->data        = data;
    pkt_tx_info->len         = len;

    return (INDIGO_ERROR_NONE);
}

indigo_error_t
indigo_port_packet_emit_group(of_port_no_t group_id,
                              of_port_no_t ingress_port_num,
                              uint8_t      *data,
                              unsigned     len
                              )
{
    pkt_tx_info->flag        = TRUE;
    pkt_tx_info->of_port_num = group_id;
    pkt_tx_info->data        = data;
    pkt_tx_info->len         = len;

    return (INDIGO_ERROR_NONE);
}

indigo_error_t
indigo_port_packet_emit_all(of_port_no_t skip_of_port_num, uint8_t *data, unsigned len)
{
    return (INDIGO_ERROR_NONE);
}


void
tbl_stats_chk(unsigned exp_active_count,
              unsigned exp_lookup_count,
              unsigned exp_matched_count)
{
    indigo_cookie_t               callback_cookie = (indigo_cookie_t) random();
    of_table_stats_request_t      *table_stats_request;

    table_stats_request =
        of_table_stats_request_new(ind_fwd_config->of_version);
    TEST_ASSERT(table_stats_request != NULL);
    
    callback_arm(indigo_core_table_stats_get_callback_info);
    
    indigo_fwd_table_stats_get(table_stats_request, callback_cookie);
    
    callback_chk(indigo_core_table_stats_get_callback_info, callback_cookie);

    of_table_stats_request_delete(table_stats_request);
    TEST_ASSERT(last_matched_count == exp_matched_count);
    TEST_ASSERT(last_active_count == exp_active_count);
    TEST_ASSERT(last_lookup_count == exp_lookup_count);
}

void
flow_stats_chk(indigo_cookie_t flow_id,
               unsigned exp_cnt_packets,
               unsigned exp_cnt_bytes)
{
    indigo_cookie_t              callback_cookie = (indigo_cookie_t) random();

    callback_arm(indigo_core_flow_stats_get_callback_info);
    indigo_fwd_flow_stats_get(flow_id, callback_cookie);

    callback_chk(indigo_core_flow_stats_get_callback_info, callback_cookie);
    
    TEST_ASSERT(last_packets == exp_cnt_packets);
    TEST_ASSERT(last_bytes == exp_cnt_bytes);
}


static void
test_features_reply(void)
{
    of_features_reply_t *of_features_reply = 0;
    int rv;

    of_features_reply = of_features_reply_new(ind_fwd_config->of_version);
    TEST_ASSERT(of_features_reply != NULL);

    rv = indigo_fwd_forwarding_features_get(of_features_reply);
    TEST_ASSERT(INDIGO_SUCCESS(rv) || "Failed features get call");

    tbl_stats_chk(0, 0, 0);     /* Check table stats */

    of_features_reply_delete(of_features_reply);
}

static void
test_packet_receive(void)
{
    uint8_t buf[100];
    int rv;

    memset(buf, 0, sizeof(buf));

    pkt_in_arm();

    rv = indigo_fwd_packet_receive(1, buf, sizeof(buf));
    TEST_ASSERT(INDIGO_SUCCESS(rv) || "Failed packet receive");

    pkt_in_chk(1, buf, sizeof(buf), OF_PACKET_IN_REASON_NO_MATCH);
    tbl_stats_chk(0, 1, 0);     /* Check table stats */
}

int
main(int argc, char* argv[])
{
    /* Init module */
    TEST_ASSERT(INDIGO_SUCCESS(ind_fwd_init(ind_fwd_config)));
    TEST_ASSERT(INDIGO_SUCCESS(ind_fwd_enable_set(1)));

    test_features_reply();
    test_packet_receive();
    
    /* Create a flow */
    {
        const indigo_flow_id_t flow_id = 0x12345678;

        of_flow_add_t                 *of_flow_add = 0;
        of_match_t                    of_match[1];
        of_list_action_t              *of_list_action = 0;
        of_action_t                   *of_action = 0;
        indigo_cookie_t               callback_cookie = (indigo_cookie_t) random();

        TEST_ASSERT((of_flow_add = of_flow_add_new(ind_fwd_config->of_version)) != 0);

        of_flow_add_priority_set(of_flow_add, 1234);
        memset(of_match, 0, sizeof(*of_match));
        of_match->fields.in_port = 1;
        of_match->masks.in_port  = ~0;
        OK(of_flow_add_match_set(of_flow_add, of_match));
        of_flow_add_hard_timeout_set(of_flow_add, 5);
        of_action = (of_action_t *) of_action_output_new(ind_fwd_config->of_version);
        TEST_ASSERT(of_action != 0);
        of_action_output_port_set(&of_action->output, 2);
        TEST_ASSERT((of_list_action = of_list_action_new(ind_fwd_config->of_version)) != 0);
        OK(of_list_action_append(of_list_action, of_action));
        OK(of_flow_add_actions_set(of_flow_add, of_list_action));
                
        callback_arm(indigo_state_manager_flow_create_callback_info);

        indigo_fwd_flow_create(flow_id, of_flow_add, callback_cookie);

        TEST_ASSERT(created_flow_id == flow_id);

        callback_chk(indigo_state_manager_flow_create_callback_info,
                     callback_cookie);

        of_action_delete(of_action);
        of_list_action_delete(of_list_action);
        of_flow_add_delete(of_flow_add);
    }

    tbl_stats_chk(1, 1, 0);     /* Check table stats */
    flow_stats_chk(created_flow_id, 0, 0); /* Check flow stats */

    /* Process a test packet
       -- Should match defined flow
     */
    {
        uint8_t buf[100];

        memset(buf, 0, sizeof(buf));

        pkt_tx_arm();

        TEST_ASSERT(INDIGO_SUCCESS(indigo_fwd_packet_receive(1, buf, sizeof(buf))));

        pkt_tx_chk(2, buf, sizeof(buf));
    }

    tbl_stats_chk(1, 2, 1);     /* Check table stats */
    flow_stats_chk(created_flow_id, 1, 100); /* Check flow stats */

    /* Modify the flow */
    {
        of_flow_modify_strict_t       *of_flow_modify = 0;
        of_match_t                    of_match[1];
        of_list_action_t              *of_list_action = 0;
        of_action_t                   *of_action = 0;
        indigo_cookie_t               callback_cookie = (indigo_cookie_t) random();
        
        TEST_ASSERT((of_flow_modify = of_flow_modify_strict_new(ind_fwd_config->of_version)) != 0);
        of_flow_modify_strict_priority_set(of_flow_modify, 1234);
        memset(of_match, 0, sizeof(*of_match));
        of_match->fields.in_port = 1;
        of_match->masks.in_port  = ~0;
        OK(of_flow_modify_strict_match_set(of_flow_modify, of_match));
        TEST_ASSERT((of_action = (of_action_t *) of_action_output_new(ind_fwd_config->of_version)) != 0);
        of_action_output_port_set(&of_action->output, 3);
        TEST_ASSERT((of_list_action = of_list_action_new(ind_fwd_config->of_version)) != 0);
        OK(of_list_action_append(of_list_action, of_action));
        OK(of_flow_modify_strict_actions_set(of_flow_modify, of_list_action));

        callback_arm(indigo_state_manager_flow_modify_callback_info);

        indigo_fwd_flow_modify(created_flow_id, of_flow_modify, callback_cookie);

        callback_chk(indigo_state_manager_flow_modify_callback_info, 
                     callback_cookie);

        of_action_delete(of_action);
        of_list_action_delete(of_list_action);
        of_flow_modify_strict_delete(of_flow_modify);
    }

    tbl_stats_chk(1, 2, 1);     /* Check table stats */
    flow_stats_chk(created_flow_id, 1, 100); /* Check flow stats */
    
    /* Process a test packet
       -- Should match modified flow
     */
    {
        uint8_t buf[100];

        memset(buf, 0, sizeof(buf));

        pkt_tx_arm();

        TEST_ASSERT(INDIGO_SUCCESS(indigo_fwd_packet_receive(1, buf, sizeof(buf))));

        pkt_tx_chk(3, buf, sizeof(buf));
    }

    tbl_stats_chk(1, 3, 2);     /* Check table stats */
    flow_stats_chk(created_flow_id, 2, 200); /* Check flow stats */

    sleep(6);                   /* Pause for flow to expire */

    /* Process a test packet
       -- Should result in "packet in"
     */
    {
        uint8_t buf[100];

        memset(buf, 0, sizeof(buf));

        pkt_in_arm();

        TEST_ASSERT(INDIGO_SUCCESS(indigo_fwd_packet_receive(1, buf, sizeof(buf))));

        pkt_in_chk(1, buf, sizeof(buf), OF_PACKET_IN_REASON_NO_MATCH);
    }

    tbl_stats_chk(1, 4, 2);     /* Check table stats */
    flow_stats_chk(created_flow_id, 2, 200); /* Check flow stats */    

    /* Delete the flow */
    {
        indigo_cookie_t callback_cookie = (indigo_cookie_t) random();

        callback_arm(indigo_state_manager_flow_delete_callback_info);

        indigo_fwd_flow_delete(created_flow_id, callback_cookie);

        callback_chk(indigo_state_manager_flow_delete_callback_info,
                     callback_cookie);

        TEST_ASSERT(last_packets == 2);
        TEST_ASSERT(last_bytes   == 200);
    }

    tbl_stats_chk(0, 4, 2);     /* Check table stats */

    /* Shut down module */
    TEST_ASSERT(ind_fwd_finish() == INDIGO_ERROR_NONE);
  
    return (0);
}

